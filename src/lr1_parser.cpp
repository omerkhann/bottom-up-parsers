#include "lr1_parser.h"

#include <optional>

namespace cc {

LR1ParserBuilder::LR1ParserBuilder(const Grammar& g) : g_(g) {}

ParserBuildResult LR1ParserBuilder::Build() const {
  ParserBuildResult out;

  // FIRST is required for LR(1) closure.
  const_cast<Grammar&>(g_).ComputeFirst();

  ItemsBuilder b(g_);
  out.collection = b.BuildCanonicalLR1();

  for (int i = 0; i < static_cast<int>(out.collection.states.size()); ++i) {
    const auto& I = out.collection.states[i];

    for (const auto& tr : out.collection.transitions[i]) {
      const auto& X = tr.first;
      const int j = tr.second;
      if (g_.IsTerminal(X) && X != Grammar::kEndMarker) {
        std::optional<Conflict> c;
        out.table.SetAction(i, X, Action{ActionType::Shift, j}, &c);
      } else if (g_.IsNonTerminal(X)) {
        out.table.SetGoto(i, X, j);
      }
    }

    for (const auto& it : I.Items()) {
      const auto& p = g_.Productions().at(it.prod);

      const bool atEnd = (it.dot >= static_cast<int>(p.rhs.size()));
      if (!atEnd) continue;

      // Accept: [S' -> S ·, $]
      if (!g_.AugmentedStartSymbol().empty() && p.lhs == g_.AugmentedStartSymbol() &&
          p.rhs.size() == 1 && p.rhs[0] == g_.StartSymbol() && it.lookahead == Grammar::kEndMarker) {
        std::optional<Conflict> c;
        out.table.SetAction(i, Grammar::kEndMarker, Action{ActionType::Accept, -1}, &c);
        continue;
      }

      // Reduce only on the item's lookahead (NOT FOLLOW)
      if (!it.lookahead.empty()) {
        std::optional<Conflict> c;
        out.table.SetAction(i, it.lookahead, Action{ActionType::Reduce, it.prod}, &c);
      }
    }
  }

  return out;
}

} // namespace cc

