#pragma once

#include "grammar.h"
#include "items.h"
#include "parsing_table.h"
#include "slr_parser.h"

namespace cc {

class LR1ParserBuilder {
public:
  explicit LR1ParserBuilder(const Grammar& g);
  ParserBuildResult Build() const;

private:
  const Grammar& g_;
};

} // namespace cc

