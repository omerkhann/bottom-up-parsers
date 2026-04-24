#include "items.h"

#include <queue>
#include <sstream>
#include <stdexcept>

namespace cc {

static std::vector<std::string> AllSymbolsForGoto(const Grammar& g) {
  std::vector<std::string> syms;
  for (const auto& t : g.Terminals()) {
    if (t == Grammar::kEndMarker) continue;
    syms.push_back(t);
  }
  for (const auto& nt : g.NonTerminals()) syms.push_back(nt);
  return syms;
}

std::string ItemToString(const Grammar& g, const Item& it) {
  const auto& p = g.Productions().at(it.prod);
  std::ostringstream out;
  out << p.lhs << " ->";
  if (p.rhs.empty()) {
    out << " \u00b7 " << Grammar::kEpsilon;
  } else {
    for (int i = 0; i <= static_cast<int>(p.rhs.size()); ++i) {
      if (i == it.dot) out << " \u00b7";
      if (i < static_cast<int>(p.rhs.size())) out << " " << p.rhs[i];
    }
  }
  if (!it.lookahead.empty()) out << " , " << it.lookahead;
  return out.str();
}

std::string ItemSet::Signature() const {
  std::ostringstream out;
  out << (lr1_ ? "LR1|" : "LR0|");
  for (const auto& it : items_) {
    out << it.prod << ":" << it.dot << ":" << it.lookahead << ";";
  }
  return out.str();
}

ItemsBuilder::ItemsBuilder(const Grammar& g) : g_(g) {
  if (g_.Productions().empty()) throw std::runtime_error("ItemsBuilder requires a non-empty grammar.");
}

ItemSet ItemsBuilder::ClosureLR0(const ItemSet& I) const {
  ItemSet C(false);
  std::queue<Item> q;
  for (const auto& it : I.Items()) {
    if (C.Insert(it)) q.push(it);
  }

  while (!q.empty()) {
    Item cur = q.front();
    q.pop();
    const auto& p = g_.Productions().at(cur.prod);
    if (cur.dot >= static_cast<int>(p.rhs.size())) continue;
    const auto& B = p.rhs[cur.dot];
    if (!g_.IsNonTerminal(B)) continue;

    auto pit = g_.ProdsByLHS().find(B);
    if (pit == g_.ProdsByLHS().end()) continue;
    for (int prodIdx : pit->second) {
      Item next{prodIdx, 0, ""};
      if (C.Insert(next)) q.push(next);
    }
  }
  return C;
}

ItemSet ItemsBuilder::GotoLR0(const ItemSet& I, const std::string& X) const {
  ItemSet J(false);
  for (const auto& it : I.Items()) {
    const auto& p = g_.Productions().at(it.prod);
    if (it.dot >= static_cast<int>(p.rhs.size())) continue;
    if (p.rhs[it.dot] != X) continue;
    Item moved = it;
    moved.dot++;
    J.Insert(moved);
  }
  if (J.Items().empty()) return J;
  return ClosureLR0(J);
}

// LR(1) CLOSURE
// For each [A -> α · B β, a], for each B -> γ and each b in FIRST(β a):
// add [B -> · γ, b]
ItemSet ItemsBuilder::ClosureLR1(const ItemSet& I) const {
  ItemSet C(true);
  std::queue<Item> q;
  for (const auto& it : I.Items()) {
    if (C.Insert(it)) q.push(it);
  }

  while (!q.empty()) {
    Item cur = q.front();
    q.pop();
    const auto& p = g_.Productions().at(cur.prod);
    if (cur.dot >= static_cast<int>(p.rhs.size())) continue;

    const std::string& B = p.rhs[cur.dot];
    if (!g_.IsNonTerminal(B)) continue;

    // β a sequence = (symbols after B in rhs) followed by lookahead terminal a
    std::vector<std::string> betaA;
    for (int i = cur.dot + 1; i < static_cast<int>(p.rhs.size()); ++i) betaA.push_back(p.rhs[i]);
    betaA.push_back(cur.lookahead);

    const auto first = g_.FirstOfSequence(betaA);
    auto pit = g_.ProdsByLHS().find(B);
    if (pit == g_.ProdsByLHS().end()) continue;

    for (int prodIdx : pit->second) {
      for (const auto& b : first) {
        if (b == Grammar::kEpsilon) continue;
        Item next{prodIdx, 0, b};
        if (C.Insert(next)) q.push(next);
      }
    }
  }
  return C;
}

ItemSet ItemsBuilder::GotoLR1(const ItemSet& I, const std::string& X) const {
  ItemSet J(true);
  for (const auto& it : I.Items()) {
    const auto& p = g_.Productions().at(it.prod);
    if (it.dot >= static_cast<int>(p.rhs.size())) continue;
    if (p.rhs[it.dot] != X) continue;
    Item moved = it;
    moved.dot++;
    J.Insert(moved);
  }
  if (J.Items().empty()) return J;
  return ClosureLR1(J);
}

ItemsBuilder::CanonicalCollection ItemsBuilder::BuildCanonicalLR0() const {
  CanonicalCollection cc;
  std::map<std::string, int> sigToState;

  ItemSet I0(false);
  // assumes augmented production is at index 0: S' -> S
  I0.Insert(Item{0, 0, ""});
  I0 = ClosureLR0(I0);

  cc.states.push_back(I0);
  cc.transitions.emplace_back();
  sigToState[I0.Signature()] = 0;

  std::queue<int> q;
  q.push(0);
  const auto symbols = AllSymbolsForGoto(g_);

  while (!q.empty()) {
    int i = q.front();
    q.pop();
    for (const auto& X : symbols) {
      ItemSet J = GotoLR0(cc.states[i], X);
      if (J.Items().empty()) continue;
      auto sig = J.Signature();
      auto it = sigToState.find(sig);
      int j;
      if (it == sigToState.end()) {
        j = static_cast<int>(cc.states.size());
        sigToState[sig] = j;
        cc.states.push_back(J);
        cc.transitions.emplace_back();
        q.push(j);
      } else {
        j = it->second;
      }
      cc.transitions[i][X] = j;
    }
  }
  return cc;
}

ItemsBuilder::CanonicalCollection ItemsBuilder::BuildCanonicalLR1() const {
  CanonicalCollection cc;
  std::map<std::string, int> sigToState;

  ItemSet I0(true);
  I0.Insert(Item{0, 0, Grammar::kEndMarker}); // [S' -> · S, $]
  I0 = ClosureLR1(I0);

  cc.states.push_back(I0);
  cc.transitions.emplace_back();
  sigToState[I0.Signature()] = 0;

  std::queue<int> q;
  q.push(0);
  const auto symbols = AllSymbolsForGoto(g_);

  while (!q.empty()) {
    int i = q.front();
    q.pop();
    for (const auto& X : symbols) {
      ItemSet J = GotoLR1(cc.states[i], X);
      if (J.Items().empty()) continue;
      auto sig = J.Signature();
      auto it = sigToState.find(sig);
      int j;
      if (it == sigToState.end()) {
        j = static_cast<int>(cc.states.size());
        sigToState[sig] = j;
        cc.states.push_back(J);
        cc.transitions.emplace_back();
        q.push(j);
      } else {
        j = it->second;
      }
      cc.transitions[i][X] = j;
    }
  }
  return cc;
}

} // namespace cc

