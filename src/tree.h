#pragma once

#include <memory>
#include <string>
#include <vector>

namespace cc {

struct ParseTreeNode {
  std::string symbol;
  std::vector<std::unique_ptr<ParseTreeNode>> children;
};

class ParseTree {
public:
  ParseTree() = default;
  explicit ParseTree(std::unique_ptr<ParseTreeNode> root) : root_(std::move(root)) {}

  const ParseTreeNode* Root() const { return root_.get(); }
  std::string ToString() const;

  static std::unique_ptr<ParseTreeNode> MakeLeaf(const std::string& sym);
  static std::unique_ptr<ParseTreeNode> MakeNode(const std::string& sym,
                                                 std::vector<std::unique_ptr<ParseTreeNode>> kids);

private:
  static void ToStringRec(const ParseTreeNode* n, std::string& out, const std::string& prefix,
                          bool isLast);

  std::unique_ptr<ParseTreeNode> root_;
};

} // namespace cc

