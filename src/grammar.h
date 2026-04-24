#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace cc {

struct Production {
  std::string lhs;
  std::vector<std::string> rhs; // epsilon production is represented as empty rhs
};

class Grammar {
public:
  static constexpr const char* kEpsilon = "@";
  static constexpr const char* kEndMarker = "$";

  // Parse grammar from file:
  // One production per line: NonTerminal -> prod1 | prod2 | ...
  // Symbols inside productions must be separated by whitespace.
  static Grammar FromFile(const std::string& path);

  // Parse grammar from raw lines (useful for tests / demos).
  static Grammar FromLines(const std::vector<std::string>& lines);

  // Augment grammar with new start symbol S' -> S.
  void Augment();

  const std::string& StartSymbol() const { return start_symbol_; }
  const std::string& AugmentedStartSymbol() const { return augmented_start_symbol_; }

  const std::vector<Production>& Productions() const { return productions_; }
  const std::map<std::string, std::vector<int>>& ProdsByLHS() const { return prods_by_lhs_; }

  const std::set<std::string>& NonTerminals() const { return nonterminals_; }
  const std::set<std::string>& Terminals() const { return terminals_; }

  bool IsNonTerminal(const std::string& sym) const;
  bool IsTerminal(const std::string& sym) const;

  // FIRST / FOLLOW
  void ComputeFirst();
  void ComputeFollow();

  // FIRST of a sequence of symbols (may include kEndMarker as a terminal).
  // Returns a set that may include "@" if the entire sequence can derive epsilon.
  std::set<std::string> FirstOfSequence(const std::vector<std::string>& seq) const;

  const std::map<std::string, std::set<std::string>>& FirstSets() const { return first_; }
  const std::map<std::string, std::set<std::string>>& FollowSets() const { return follow_; }

  std::string ToString() const;

private:
  static Grammar FromText(const std::string& text);
  static std::string Trim(const std::string& s);
  static std::vector<std::string> Split(const std::string& s, char delim);
  static std::vector<std::string> SplitWS(const std::string& s);
  static bool StartsWithUpper(const std::string& s);

  void Reindex();
  void InferSymbols();

  std::string start_symbol_;
  std::string augmented_start_symbol_; // empty if not augmented yet

  std::vector<Production> productions_;
  std::map<std::string, std::vector<int>> prods_by_lhs_;

  std::set<std::string> nonterminals_;
  std::set<std::string> terminals_;

  std::map<std::string, std::set<std::string>> first_;
  std::map<std::string, std::set<std::string>> follow_;
};

} // namespace cc

