#include "stack.h"

#include <sstream>
#include <stdexcept>

namespace cc {

static std::string Join(const std::vector<std::string>& v, size_t start) {
  std::ostringstream out;
  for (size_t i = start; i < v.size(); ++i) {
    if (i != start) out << " ";
    out << v[i];
  }
  return out.str();
}

ParseResult ShiftReduceParser::Parse(const std::vector<std::string>& tokens) const {
  ParseResult res;

  std::vector<std::string> input = tokens;
  input.push_back(Grammar::kEndMarker);

  std::vector<int> stateStack;
  std::vector<std::string> symStack;
  std::vector<std::unique_ptr<ParseTreeNode>> nodeStack;

  stateStack.push_back(0);

  size_t ip = 0;
  int step = 1;
  while (true) {
    int s = stateStack.back();
    const std::string a = (ip < input.size() ? input[ip] : Grammar::kEndMarker);

    TraceRow row;
    row.step = step++;
    {
      std::ostringstream st;
      st << stateStack[0];
      for (size_t i = 0; i < symStack.size(); ++i) {
        st << " " << symStack[i] << " " << stateStack[i + 1];
      }
      row.stack = st.str();
    }
    row.input = Join(input, ip);

    auto actOpt = table_.GetAction(s, a);
    if (!actOpt) {
      row.action = "error";
      res.trace.push_back(std::move(row));
      res.accepted = false;
      res.error = "No ACTION for state " + std::to_string(s) + " on terminal '" + a + "'";
      return res;
    }

    const Action act = *actOpt;
    if (act.type == ActionType::Shift) {
      row.action = "shift " + std::to_string(act.target);
      symStack.push_back(a);
      stateStack.push_back(act.target);
      nodeStack.push_back(ParseTree::MakeLeaf(a));
      ip++;
      res.trace.push_back(std::move(row));
      continue;
    }

    if (act.type == ActionType::Reduce) {
      const int prodIdx = act.target;
      const auto& p = g_.Productions().at(prodIdx);
      row.action = "reduce " + p.lhs + " ->";
      if (p.rhs.empty()) row.action += " " + std::string(Grammar::kEpsilon);
      else for (const auto& s2 : p.rhs) row.action += " " + s2;

      int popCount = 0;
      popCount = static_cast<int>(p.rhs.size());

      std::vector<std::unique_ptr<ParseTreeNode>> kids;
      kids.reserve(popCount);
      for (int i = 0; i < popCount; ++i) {
        if (stateStack.size() <= 1 || symStack.empty() || nodeStack.empty()) {
          throw std::runtime_error("Parser stack underflow during reduction.");
        }
        stateStack.pop_back();
        symStack.pop_back();
        kids.push_back(std::move(nodeStack.back()));
        nodeStack.pop_back();
      }
      // reverse kids to preserve left-to-right order
      for (size_t i = 0; i < kids.size() / 2; ++i) std::swap(kids[i], kids[kids.size() - 1 - i]);

      auto parent = ParseTree::MakeNode(p.lhs, std::move(kids));

      int t = stateStack.back();
      auto go = table_.GetGoto(t, p.lhs);
      if (!go) {
        row.action += " (error: missing GOTO)";
        res.trace.push_back(std::move(row));
        res.accepted = false;
        res.error = "No GOTO for state " + std::to_string(t) + " on nonterminal '" + p.lhs + "'";
        return res;
      }

      symStack.push_back(p.lhs);
      stateStack.push_back(*go);
      nodeStack.push_back(std::move(parent));

      res.trace.push_back(std::move(row));
      continue;
    }

    if (act.type == ActionType::Accept) {
      row.action = "accept";
      res.trace.push_back(std::move(row));
      res.accepted = true;
      if (!nodeStack.empty()) {
        res.tree = ParseTree(std::move(nodeStack.back()));
      }
      return res;
    }
  }
}

} // namespace cc

