#include "slr_parser.h"

#include <optional>

namespace cc {

SLRParserBuilder::SLRParserBuilder(const Grammar& g) : g_(g) {}

ParserBuildResult SLRParserBuilder::Build() const {
  ParserBuildResult out;
  ItemsBuilder b(g_);
  out.collection = b.BuildCanonicalLR0();

  // Need FOLLOW for reductions
  // (Grammar already includes $ as terminal)
  // FOLLOW computed on un-augmented start symbol; but augmented doesn't matter except accept.
  // We assume caller already augmented before building.
  // Ensure follow exists:
  // const_cast acceptable here; we keep Grammar methods non-const by design.
  const_cast<Grammar&>(g_).ComputeFirst();
  const_cast<Grammar&>(g_).ComputeFollow();

  for (int i = 0; i < static_cast<int>(out.collection.states.size()); ++i) {
    const auto& I = out.collection.states[i];

    // Shifts based on transitions on terminals
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

      // Accept: S' -> S ·
      if (!g_.AugmentedStartSymbol().empty() && p.lhs == g_.AugmentedStartSymbol() &&
          p.rhs.size() == 1 && p.rhs[0] == g_.StartSymbol()) {
        std::optional<Conflict> c;
        out.table.SetAction(i, Grammar::kEndMarker, Action{ActionType::Accept, -1}, &c);
        continue;
      }

      // Reduce: A -> α · on terminals in FOLLOW(A)
      auto folIt = g_.FollowSets().find(p.lhs);
      if (folIt == g_.FollowSets().end()) continue;
      for (const auto& a : folIt->second) {
        std::optional<Conflict> c;
        out.table.SetAction(i, a, Action{ActionType::Reduce, it.prod}, &c);
      }
    }
  }

  return out;
}

} // namespace cc

