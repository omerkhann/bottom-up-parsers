#pragma once

#include "grammar.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace cc {

// Item for LR(0) or LR(1). If lookahead is empty => LR(0) item.
struct Item {
  int prod = -1;              // production index in Grammar::Productions()
  int dot = 0;                // dot position: 0..rhs.size()
  std::string lookahead = ""; // for LR(1) only (terminal or "$")

  bool IsLR1() const { return !lookahead.empty(); }

  bool operator<(const Item& o) const {
    if (prod != o.prod) return prod < o.prod;
    if (dot != o.dot) return dot < o.dot;
    return lookahead < o.lookahead;
  }
  bool operator==(const Item& o) const { return prod == o.prod && dot == o.dot && lookahead == o.lookahead; }
};

class ItemSet {
public:
  explicit ItemSet(bool lr1) : lr1_(lr1) {}

  bool IsLR1() const { return lr1_; }
  const std::set<Item>& Items() const { return items_; }

  bool Insert(const Item& it) { return items_.insert(it).second; }

  // For deterministic printing / hashing.
  std::string Signature() const;

  bool operator<(const ItemSet& o) const { return Signature() < o.Signature(); }

private:
  bool lr1_ = false;
  std::set<Item> items_;
};

// LR closure/goto builders.
class ItemsBuilder {
public:
  explicit ItemsBuilder(const Grammar& g);

  // LR(0)
  ItemSet ClosureLR0(const ItemSet& I) const;
  ItemSet GotoLR0(const ItemSet& I, const std::string& X) const;

  // LR(1)
  ItemSet ClosureLR1(const ItemSet& I) const;
  ItemSet GotoLR1(const ItemSet& I, const std::string& X) const;

  // Canonical collections: returns vector of states, and transitions[state][symbol] = nextState.
  struct CanonicalCollection {
    std::vector<ItemSet> states;
    std::vector<std::map<std::string, int>> transitions;
  };

  CanonicalCollection BuildCanonicalLR0() const;
  CanonicalCollection BuildCanonicalLR1() const;

private:
  const Grammar& g_;
};

std::string ItemToString(const Grammar& g, const Item& it);

} // namespace cc

