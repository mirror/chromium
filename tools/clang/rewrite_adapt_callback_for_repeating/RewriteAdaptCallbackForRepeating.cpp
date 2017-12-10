// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <algorithm>
#include <memory>
#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/TargetSelect.h"

using namespace clang::ast_matchers;
using clang::tooling::CommonOptionsParser;
using Replacements = std::vector<clang::tooling::Replacement>;

namespace {

// Remove base::AdaptCallbackForRepeating() where resulting
// base::RepeatingCallback is implicitly converted into base::OnceCallback.
// Example:
//   // Before
//   base::PostTask(
//       FROM_HERE,
//       base::AdaptCallbackForRepeating(base::BindOnce(&Foo)));
//   base::OnceCallback<void()> cb = base::AdaptCallbackForRepeating(
//       base::OnceBind(&Foo));
//
//   // After
//   base::PostTask(FROM_HERE, base::BindOnce(&Foo));
//   base::OnceCallback<void()> cb = base::BindOnce(&Foo);
class RewriteAdaptCallbackForRepeating : public MatchFinder::MatchCallback {
 public:
  explicit RewriteAdaptCallbackForRepeating(Replacements* replacements)
      : replacements_(replacements) {}

  StatementMatcher GetMatcher() {
    auto is_once_callback = hasType(hasCanonicalType(hasDeclaration(
        classTemplateSpecializationDecl(hasName("::base::OnceCallback")))));
    auto is_repeating_callback =
        hasType(hasCanonicalType(hasDeclaration(classTemplateSpecializationDecl(
            hasName("::base::RepeatingCallback")))));

    auto bind_call =
        callExpr(callee(namedDecl(hasName("::base::AdaptCallbackForRepeating")))).bind("target");
    auto parameter_construction =
        cxxConstructExpr(is_repeating_callback, argumentCountIs(1),
                         hasArgument(0, ignoringImplicit(bind_call)));
    auto constructor_conversion = cxxConstructExpr(
        is_once_callback, argumentCountIs(1),
        hasArgument(0, ignoringImplicit(parameter_construction)));
    auto implicit_conversion = implicitCastExpr(
        is_once_callback, hasSourceExpression(constructor_conversion));
    return implicit_conversion;
  }

  void run(const MatchFinder::MatchResult& result) override {
    auto* target = result.Nodes.getNodeAs<clang::CallExpr>("target");
    auto* callee = target->getCallee();
    auto range = clang::CharSourceRange::getTokenRange(
        result.SourceManager->getSpellingLoc(callee->getLocEnd()),
        result.SourceManager->getSpellingLoc(callee->getLocEnd()));
    replacements_->emplace_back(*result.SourceManager, range, "FooBar");
  }

 private:
  Replacements* replacements_;
};

llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

}  // namespace.

int main(int argc, const char* argv[]) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::cl::OptionCategory category(
      "Remove raw pointer on the receiver of Bind() target");
  CommonOptionsParser options(argc, argv, category);
  clang::tooling::ClangTool tool(options.getCompilations(),
                                 options.getSourcePathList());

  MatchFinder match_finder;
  std::vector<clang::tooling::Replacement> replacements;

  RewriteAdaptCallbackForRepeating rewriter(&replacements);
  match_finder.addMatcher(rewriter.GetMatcher(), &rewriter);

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
