#pragma once

#include "grammar.h"
#include "parsing_table.h"
#include "tree.h"

#include <string>
#include <vector>

namespace cc {

struct TraceRow {
  int step = 0;
  std::string stack;
  std::string input;
  std::string action;
};

struct ParseResult {
  bool accepted = false;
  std::vector<TraceRow> trace;
  ParseTree tree;
  std::string error;
};

// Shift-reduce engine; uses ACTION/GOTO tables built by SLR or LR(1).
class ShiftReduceParser {
public:
  ShiftReduceParser(const Grammar& g, const ParsingTable& table) : g_(g), table_(table) {}

  // tokens should NOT include "$"; it will be appended automatically.
  ParseResult Parse(const std::vector<std::string>& tokens) const;

private:
  const Grammar& g_;
  const ParsingTable& table_;
};

} // namespace cc

