#include "parsing_table.h"

#include <iomanip>
#include <sstream>

namespace cc {

std::string ActionToString(const Action& a) {
  switch (a.type) {
  case ActionType::Shift:
    return "s" + std::to_string(a.target);
  case ActionType::Reduce:
    return "r" + std::to_string(a.target);
  case ActionType::Accept:
    return "acc";
  }
  return "?";
}

bool ParsingTable::SetAction(int state, const std::string& terminal, const Action& a,
                             std::optional<Conflict>* outConflict) {
  auto key = std::make_pair(state, terminal);
  auto it = action_.find(key);
  if (it == action_.end()) {
    action_[key] = a;
    return true;
  }
  if (it->second == a) return true;

  Conflict c;
  c.state = state;
  c.terminal = terminal;
  c.existing = it->second;
  c.incoming = a;
  conflicts_.push_back(c);
  if (outConflict) *outConflict = c;
  return false;
}

void ParsingTable::SetGoto(int state, const std::string& nonterminal, int nextState) {
  goTo_[{state, nonterminal}] = nextState;
}

std::optional<Action> ParsingTable::GetAction(int state, const std::string& terminal) const {
  auto it = action_.find({state, terminal});
  if (it == action_.end()) return std::nullopt;
  return it->second;
}

std::optional<int> ParsingTable::GetGoto(int state, const std::string& nonterminal) const {
  auto it = goTo_.find({state, nonterminal});
  if (it == goTo_.end()) return std::nullopt;
  return it->second;
}

std::set<std::string> ParsingTable::ActionColumns() const {
  std::set<std::string> cols;
  for (const auto& kv : action_) cols.insert(kv.first.second);
  return cols;
}

std::set<std::string> ParsingTable::GotoColumns() const {
  std::set<std::string> cols;
  for (const auto& kv : goTo_) cols.insert(kv.first.second);
  return cols;
}

std::string ParsingTable::ToString() const {
  std::ostringstream out;
  auto aCols = ActionColumns();
  auto gCols = GotoColumns();

  // gather states
  std::set<int> states;
  for (const auto& kv : action_) states.insert(kv.first.first);
  for (const auto& kv : goTo_) states.insert(kv.first.first);

  // Materialize columns for deterministic printing.
  std::vector<std::string> aColVec(aCols.begin(), aCols.end());
  std::vector<std::string> gColVec(gCols.begin(), gCols.end());

  // Compute widths.
  const int stateW = 7; // enough for "State"
  int cellW = 5;        // minimum for "acc"/"s10"/"r10"
  for (const auto& t : aColVec) cellW = std::max<int>(cellW, static_cast<int>(t.size()));
  for (const auto& nt : gColVec) cellW = std::max<int>(cellW, static_cast<int>(nt.size()));
  for (int s : states) {
    for (const auto& t : aColVec) {
      if (auto a = GetAction(s, t)) cellW = std::max<int>(cellW, static_cast<int>(ActionToString(*a).size()));
    }
    for (const auto& nt : gColVec) {
      if (auto g = GetGoto(s, nt)) cellW = std::max<int>(cellW, static_cast<int>(std::to_string(*g).size()));
    }
  }
  cellW = std::min(cellW, 16); // keep tables readable

  auto printSep = [&]() {
    out << std::string(stateW, '-') << "-+-";
    out << std::string((cellW + 3) * static_cast<int>(aColVec.size()) + (aColVec.empty() ? 0 : 1), '-');
    out << "-+-";
    out << std::string((cellW + 3) * static_cast<int>(gColVec.size()) + (gColVec.empty() ? 0 : 1), '-');
    out << "\n";
  };

  // Two-line header: group names then actual columns.
  out << std::left << std::setw(stateW) << "State"
      << " | ";
  if (aColVec.empty()) {
    out << std::left << std::setw(cellW) << "(no ACTION)";
  } else {
    out << std::left << std::setw((cellW + 3) * static_cast<int>(aColVec.size()) - 3) << "ACTION";
  }
  out << " | ";
  if (gColVec.empty()) {
    out << std::left << std::setw(cellW) << "(no GOTO)";
  } else {
    out << std::left << std::setw((cellW + 3) * static_cast<int>(gColVec.size()) - 3) << "GOTO";
  }
  out << "\n";

  out << std::left << std::setw(stateW) << ""
      << " | ";
  for (size_t i = 0; i < aColVec.size(); ++i) {
    if (i) out << " | ";
    out << std::left << std::setw(cellW) << aColVec[i];
  }
  if (aColVec.empty()) out << std::left << std::setw(cellW) << "";
  out << " | ";
  for (size_t i = 0; i < gColVec.size(); ++i) {
    if (i) out << " | ";
    out << std::left << std::setw(cellW) << gColVec[i];
  }
  if (gColVec.empty()) out << std::left << std::setw(cellW) << "";
  out << "\n";
  printSep();

  for (int s : states) {
    out << std::left << std::setw(stateW) << ("I" + std::to_string(s)) << " | ";

    for (size_t i = 0; i < aColVec.size(); ++i) {
      if (i) out << " | ";
      auto a = GetAction(s, aColVec[i]);
      out << std::left << std::setw(cellW) << (a ? ActionToString(*a) : "");
    }
    if (aColVec.empty()) out << std::left << std::setw(cellW) << "";

    out << " | ";

    for (size_t i = 0; i < gColVec.size(); ++i) {
      if (i) out << " | ";
      auto g = GetGoto(s, gColVec[i]);
      out << std::left << std::setw(cellW) << (g ? std::to_string(*g) : "");
    }
    if (gColVec.empty()) out << std::left << std::setw(cellW) << "";
    out << "\n";
  }

  if (!conflicts_.empty()) {
    out << "\nCONFLICTS:\n";
    for (const auto& c : conflicts_) {
      out << "State I" << c.state << ", on terminal '" << c.terminal << "': "
          << ActionToString(c.existing) << " vs " << ActionToString(c.incoming) << "\n";
    }
  }
  return out.str();
}

} // namespace cc

