#pragma once

#include "grammar.h"
#include "items.h"
#include "parsing_table.h"

namespace cc {

struct ParserBuildResult {
  ItemsBuilder::CanonicalCollection collection;
  ParsingTable table;
};

class SLRParserBuilder {
public:
  explicit SLRParserBuilder(const Grammar& g);
  ParserBuildResult Build() const;

private:
  const Grammar& g_;
};

} // namespace cc

