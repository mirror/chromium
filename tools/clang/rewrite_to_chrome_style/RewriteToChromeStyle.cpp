// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Changes Blink-style names to Chrome-style names. Currently transforms:
//   fields:
//     int m_operationCount => int operation_count_
//   variables (including parameters):
//     int mySuperVariable => int my_super_variable
//   constants:
//     const int maxThings => const int kMaxThings
//   free functions and methods:
//     void doThisThenThat() => void DoThisAndThat()

#include <assert.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"

#include "EditTracker.h"

using namespace clang::ast_matchers;
using clang::tooling::CommonOptionsParser;
using clang::tooling::Replacement;
using llvm::StringRef;

namespace {

class MyRewriter : public MatchFinder::MatchCallback {
 public:
  explicit MyRewriter(std::set<Replacement>* replacements)
      : replacements_(replacements) {}

  void run(const MatchFinder::MatchResult& result) override {
    const clang::MemberExpr* expr =
        result.Nodes.getNodeAs<clang::MemberExpr>("expr");
    clang::SourceLocation loc = expr->getMemberLoc();

    std::string expected_old_text = "GetRenderProcessHost";
    const clang::SourceManager& source_manager = *result.SourceManager;
    clang::SourceLocation spell = source_manager.getSpellingLoc(loc);
    clang::CharSourceRange range = clang::CharSourceRange::getCharRange(
        spell, spell.getLocWithOffset(expected_old_text.size()));
    replacements_->insert(
        Replacement(source_manager, range, "GetMainFrame()->GetProcess"));
  }

 private:
  std::set<Replacement>* const replacements_;
};

}  // namespace

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

int main(int argc, const char* argv[]) {
  // TODO(dcheng): Clang tooling should do this itself.
  // http://llvm.org/bugs/show_bug.cgi?id=21627
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::cl::OptionCategory category("TODO.");
  CommonOptionsParser options(argc, argv, category);
  clang::tooling::ClangTool tool(options.getCompilations(),
                                 options.getSourcePathList());

  MatchFinder match_finder;
  std::set<Replacement> replacements;

  auto content_namespace_decl =
      namespaceDecl(hasName("content"), hasParent(translationUnitDecl()));
  auto decl_under_content_namespace = decl(hasAncestor(content_namespace_decl));
  auto method_decl_matcher = id(
      "decl", cxxMethodDecl(
                  decl_under_content_namespace, hasName("GetRenderProcessHost"),
                  hasAncestor(cxxRecordDecl(hasName("WebContents")))));
  auto method_member_matcher =
      id("expr", memberExpr(member(method_decl_matcher)));

  MyRewriter my_rewriter(&replacements);
  match_finder.addMatcher(method_member_matcher, &my_rewriter);

  // Prepare and run the tool.
  std::unique_ptr<clang::tooling::FrontendActionFactory> factory =
      clang::tooling::newFrontendActionFactory(&match_finder);
  int result = tool.run(factory.get());
  if (result != 0)
    return result;

  // Serialization format is documented in tools/clang/scripts/run_tool.py
  llvm::outs() << "==== BEGIN EDITS ====\n";
  for (const auto& r : replacements) {
    std::string replacement_text = r.getReplacementText().str();
    std::replace(replacement_text.begin(), replacement_text.end(), '\n', '\0');
    llvm::outs() << "r:::" << r.getFilePath() << ":::" << r.getOffset()
                 << ":::" << r.getLength() << ":::" << replacement_text << "\n";
  }
  llvm::outs() << "==== END EDITS ====\n";

  return 0;
}
