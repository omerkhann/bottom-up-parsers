#include "tree.h"

namespace cc {

std::unique_ptr<ParseTreeNode> ParseTree::MakeLeaf(const std::string& sym) {
  auto n = std::make_unique<ParseTreeNode>();
  n->symbol = sym;
  return n;
}

std::unique_ptr<ParseTreeNode> ParseTree::MakeNode(const std::string& sym,
                                                   std::vector<std::unique_ptr<ParseTreeNode>> kids) {
  auto n = std::make_unique<ParseTreeNode>();
  n->symbol = sym;
  n->children = std::move(kids);
  return n;
}

void ParseTree::ToStringRec(const ParseTreeNode* n, std::string& out, const std::string& prefix,
                            bool isLast) {
  if (!n) return;
  out += prefix;
  out += (isLast ? "└── " : "├── ");
  out += n->symbol;
  out += "\n";

  for (size_t i = 0; i < n->children.size(); ++i) {
    const bool last = (i + 1 == n->children.size());
    ToStringRec(n->children[i].get(), out, prefix + (isLast ? "    " : "│   "), last);
  }
}

std::string ParseTree::ToString() const {
  std::string out;
  if (!root_) return out;
  // root gets printed without the leading branches
  out += root_->symbol + "\n";
  for (size_t i = 0; i < root_->children.size(); ++i) {
    const bool last = (i + 1 == root_->children.size());
    ToStringRec(root_->children[i].get(), out, "", last);
  }
  return out;
}

} // namespace cc

