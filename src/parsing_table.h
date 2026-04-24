#pragma once

#include "grammar.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cc {

enum class ActionType { Shift, Reduce, Accept };

struct Action {
  ActionType type;
  int target = -1; // for Shift: next state; for Reduce: production index; for Accept unused

  bool operator==(const Action& o) const { return type == o.type && target == o.target; }
};

struct Conflict {
  int state = -1;
  std::string terminal;
  Action existing;
  Action incoming;
};

class ParsingTable {
public:
  bool SetAction(int state, const std::string& terminal, const Action& a, std::optional<Conflict>* outConflict);
  void SetGoto(int state, const std::string& nonterminal, int nextState);

  std::optional<Action> GetAction(int state, const std::string& terminal) const;
  std::optional<int> GetGoto(int state, const std::string& nonterminal) const;

  const std::vector<Conflict>& Conflicts() const { return conflicts_; }

  std::set<std::string> ActionColumns() const;
  std::set<std::string> GotoColumns() const;

  // Stats for comparison/performance report.
  size_t ActionEntryCount() const { return action_.size(); }
  size_t GotoEntryCount() const { return goTo_.size(); }
  size_t TotalEntryCount() const { return action_.size() + goTo_.size(); }

  // Rough estimate (not exact allocator usage).
  size_t ApproxBytes() const { return action_.size() * sizeof(Action) + goTo_.size() * sizeof(int); }

  std::string ToString() const;

private:
  std::map<std::pair<int, std::string>, Action> action_;
  std::map<std::pair<int, std::string>, int> goTo_;
  std::vector<Conflict> conflicts_;
};

std::string ActionToString(const Action& a);

} // namespace cc

