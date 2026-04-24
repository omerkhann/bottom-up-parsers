#include "grammar.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace cc {

static bool IsEpsilonSym(const std::string& s) { return s == Grammar::kEpsilon || s == "epsilon"; }

Grammar Grammar::FromFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("Failed to open grammar file: " + path);
  std::ostringstream ss;
  ss << in.rdbuf();
  return FromText(ss.str());
}

Grammar Grammar::FromLines(const std::vector<std::string>& lines) {
  std::ostringstream ss;
  for (const auto& l : lines) ss << l << "\n";
  return FromText(ss.str());
}

Grammar Grammar::FromText(const std::string& text) {
  Grammar g;
  std::istringstream in(text);
  std::string line;
  bool start_set = false;

  while (std::getline(in, line)) {
    line = Trim(line);
    if (line.empty()) continue;
    if (line.size() >= 2 && line[0] == '/' && line[1] == '/') continue;

    auto parts = Split(line, '-'); // crude; we'll search for "->" properly below
    (void)parts;

    const auto arrowPos = line.find("->");
    if (arrowPos == std::string::npos) {
      throw std::runtime_error("Invalid production line (missing ->): " + line);
    }
    std::string lhs = Trim(line.substr(0, arrowPos));
    std::string rhsAll = Trim(line.substr(arrowPos + 2));
    if (lhs.empty() || rhsAll.empty()) {
      throw std::runtime_error("Invalid production line: " + line);
    }
    if (!StartsWithUpper(lhs)) {
      throw std::runtime_error("NonTerminal must start with uppercase: " + lhs);
    }

    if (!start_set) {
      g.start_symbol_ = lhs;
      start_set = true;
    }

    auto alts = Split(rhsAll, '|');
    for (auto& alt : alts) {
      alt = Trim(alt);
      Production p;
      p.lhs = lhs;
      if (alt.empty() || alt == "epsilon" || alt == kEpsilon) {
        p.rhs = {};
      } else {
        auto symbols = SplitWS(alt);
        for (auto& s : symbols) {
          if (IsEpsilonSym(s)) {
            // If epsilon appears explicitly inside a RHS, treat it as empty RHS.
            // (Assignment format uses epsilon/@ as the whole alternative.)
            symbols.clear();
            break;
          }
        }
        p.rhs = std::move(symbols);
      }
      g.productions_.push_back(std::move(p));
    }
  }

  if (g.productions_.empty()) throw std::runtime_error("No productions found in grammar.");
  g.Reindex();
  g.InferSymbols();
  return g;
}

void Grammar::Augment() {
  if (!augmented_start_symbol_.empty()) return;
  if (start_symbol_.empty()) throw std::runtime_error("Cannot augment grammar without a start symbol.");

  augmented_start_symbol_ = start_symbol_ + "Prime";
  while (nonterminals_.count(augmented_start_symbol_) != 0) {
    augmented_start_symbol_ += "Prime";
  }

  Production p;
  p.lhs = augmented_start_symbol_;
  p.rhs = {start_symbol_};
  productions_.insert(productions_.begin(), std::move(p));

  Reindex();
  InferSymbols();
}

bool Grammar::StartsWithUpper(const std::string& s) {
  return !s.empty() && std::isupper(static_cast<unsigned char>(s[0])) != 0;
}

bool Grammar::IsNonTerminal(const std::string& sym) const {
  return nonterminals_.count(sym) != 0;
}

bool Grammar::IsTerminal(const std::string& sym) const {
  if (sym == kEpsilon) return false;
  return terminals_.count(sym) != 0;
}

void Grammar::InferSymbols() {
  nonterminals_.clear();
  terminals_.clear();

  for (const auto& p : productions_) nonterminals_.insert(p.lhs);

  for (const auto& p : productions_) {
    for (const auto& sym : p.rhs) {
      if (StartsWithUpper(sym)) {
        nonterminals_.insert(sym);
      } else {
        terminals_.insert(sym);
      }
    }
  }

  // End marker behaves like a terminal.
  terminals_.insert(kEndMarker);
}

void Grammar::Reindex() {
  prods_by_lhs_.clear();
  for (int i = 0; i < static_cast<int>(productions_.size()); ++i) {
    prods_by_lhs_[productions_[i].lhs].push_back(i);
  }
}

void Grammar::ComputeFirst() {
  first_.clear();
  // Terminals: FIRST(t) = {t}
  for (const auto& t : terminals_) first_[t].insert(t);
  first_[kEpsilon].insert(kEpsilon);
  for (const auto& nt : nonterminals_) first_[nt]; // ensure present

  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto& p : productions_) {
      auto& firstA = first_[p.lhs];
      const auto seqFirst = FirstOfSequence(p.rhs);
      for (const auto& s : seqFirst) {
        if (firstA.insert(s).second) changed = true;
      }
    }
  }
}

std::set<std::string> Grammar::FirstOfSequence(const std::vector<std::string>& seq) const {
  std::set<std::string> result;
  if (seq.empty()) {
    result.insert(kEpsilon);
    return result;
  }

  bool allCanEpsilon = true;
  for (size_t i = 0; i < seq.size(); ++i) {
    const std::string& sym = seq[i];
    auto it = first_.find(sym);
    if (it == first_.end()) {
      // Unknown symbol: treat as terminal.
      result.insert(sym);
      allCanEpsilon = false;
      break;
    }

    bool symCanEpsilon = false;
    for (const auto& x : it->second) {
      if (x == kEpsilon) symCanEpsilon = true;
      else result.insert(x);
    }
    if (!symCanEpsilon) {
      allCanEpsilon = false;
      break;
    }
  }

  if (allCanEpsilon) result.insert(kEpsilon);
  return result;
}

void Grammar::ComputeFollow() {
  if (first_.empty()) ComputeFirst();
  follow_.clear();
  for (const auto& nt : nonterminals_) follow_[nt];

  // FOLLOW(start) contains $
  follow_[start_symbol_].insert(kEndMarker);

  bool changed = true;
  while (changed) {
    changed = false;
    for (const auto& p : productions_) {
      const auto& A = p.lhs;
      for (size_t i = 0; i < p.rhs.size(); ++i) {
        const auto& B = p.rhs[i];
        if (!IsNonTerminal(B)) continue;

        std::vector<std::string> beta;
        for (size_t j = i + 1; j < p.rhs.size(); ++j) beta.push_back(p.rhs[j]);
        auto firstBeta = FirstOfSequence(beta);

        auto& followB = follow_[B];
        for (const auto& x : firstBeta) {
          if (x == kEpsilon) continue;
          if (followB.insert(x).second) changed = true;
        }
        if (firstBeta.count(kEpsilon) != 0 || beta.empty()) {
          for (const auto& x : follow_[A]) {
            if (followB.insert(x).second) changed = true;
          }
        }
      }
    }
  }
}

std::string Grammar::Trim(const std::string& s) {
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) ++i;
  size_t j = s.size();
  while (j > i && std::isspace(static_cast<unsigned char>(s[j - 1])) != 0) --j;
  return s.substr(i, j - i);
}

std::vector<std::string> Grammar::Split(const std::string& s, char delim) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == delim) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

std::vector<std::string> Grammar::SplitWS(const std::string& s) {
  std::vector<std::string> out;
  std::istringstream in(s);
  std::string tok;
  while (in >> tok) out.push_back(tok);
  return out;
}

std::string Grammar::ToString() const {
  std::ostringstream out;
  out << "Start: " << start_symbol_ << "\n";
  if (!augmented_start_symbol_.empty()) out << "AugmentedStart: " << augmented_start_symbol_ << "\n";
  for (size_t i = 0; i < productions_.size(); ++i) {
    out << i << ": " << productions_[i].lhs << " ->";
    if (productions_[i].rhs.empty()) {
      out << " " << kEpsilon;
    } else {
      for (const auto& s : productions_[i].rhs) out << " " << s;
    }
    out << "\n";
  }
  return out.str();
}

} // namespace cc

