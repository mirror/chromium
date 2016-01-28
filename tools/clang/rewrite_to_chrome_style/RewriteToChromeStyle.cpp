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
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/file.h>
#include <unistd.h>
#endif

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchersMacros.h"
#include "clang/Basic/CharInfo.h"
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
using clang::tooling::Replacement;
using clang::tooling::Replacements;
using llvm::StringRef;

namespace {

AST_MATCHER(clang::FunctionDecl, isOverloadedOperator) {
  return Node.isOverloadedOperator();
}

constexpr char kBlinkFieldPrefix[] = "m_";
constexpr char kBlinkStaticMemberPrefix[] = "s_";

bool GetNameForDecl(const clang::FunctionDecl& decl,
                    const clang::ASTContext& context,
                    std::string& name) {
  name = decl.getNameAsString();
  name[0] = clang::toUppercase(name[0]);
  return true;
}

// Helper to convert from a camelCaseName to camel_case_name. It uses some
// heuristics to try to handle acronyms in camel case names correctly.
std::string CamelCaseToUnderscoreCase(StringRef input) {
  std::string output;
  bool needs_underscore = false;
  bool was_lowercase = false;
  bool was_uppercase = false;
  // Iterate in reverse to minimize the amount of backtracking.
  for (const unsigned char* i = input.bytes_end() - 1; i >= input.bytes_begin();
       --i) {
    char c = *i;
    bool is_lowercase = clang::isLowercase(c);
    bool is_uppercase = clang::isUppercase(c);
    c = clang::toLowercase(c);
    // Transitioning from upper to lower case requires an underscore. This is
    // needed to handle names with acronyms, e.g. handledHTTPRequest needs a '_'
    // in 'dH'. This is a complement to the non-acronym case further down.
    if (needs_underscore || (was_uppercase && is_lowercase)) {
      output += '_';
      needs_underscore = false;
    }
    output += c;
    // Handles the non-acronym case: transitioning from lower to upper case
    // requires an underscore when emitting the next character, e.g. didLoad
    // needs a '_' in 'dL'.
    if (i != input.bytes_end() - 1 && was_lowercase && is_uppercase)
      needs_underscore = true;
    was_lowercase = is_lowercase;
    was_uppercase = is_uppercase;
  }
  std::reverse(output.begin(), output.end());
  return output;
}

bool GetNameForDecl(const clang::FieldDecl& decl,
                    const clang::ASTContext& context,
                    std::string& name) {
  StringRef original_name = decl.getName();
  // Blink style field names are prefixed with `m_`. If this prefix isn't
  // present, assume it's already been converted to Google style.
  if (original_name.size() < strlen(kBlinkFieldPrefix) ||
      !original_name.startswith(kBlinkFieldPrefix))
    return false;
  name = CamelCaseToUnderscoreCase(
      original_name.substr(strlen(kBlinkFieldPrefix)));
  // The few examples I could find used struct-style naming with no `_` suffix
  // for unions.
  bool c = decl.getParent()->isClass();
  // There appears to be a GCC bug that makes this branch incorrectly if we
  // don't use a temp variable!! Clang works right. crbug.com/580745
  if (c)
    name += '_';
  return true;
}

bool IsProbablyConst(const clang::VarDecl& decl,
                     const clang::ASTContext& context) {
  clang::QualType type = decl.getType();
  if (!type.isConstQualified())
    return false;

  if (type.isVolatileQualified())
    return false;

  // http://google.github.io/styleguide/cppguide.html#Constant_Names
  // Static variables that are const-qualified should use kConstantStyle naming.
  if (decl.getStorageDuration() == clang::SD_Static)
    return true;

  const clang::Expr* initializer = decl.getInit();
  if (!initializer)
    return false;

  // If the expression can be evaluated at compile time, then it should have a
  // kFoo style name. Otherwise, not.
  return initializer->isEvaluatable(context);
}

bool GetNameForDecl(const clang::VarDecl& decl,
                    const clang::ASTContext& context,
                    std::string& name) {
  StringRef original_name = decl.getName();

  // Nothing to do for unnamed parameters.
  if (clang::isa<clang::ParmVarDecl>(decl) && original_name.empty())
    return false;

  // static class members match against VarDecls. Blink style dictates that
  // these should be prefixed with `s_`, so strip that off. Also check for `m_`
  // and strip that off too, for code that accidentally uses the wrong prefix.
  if (original_name.startswith(kBlinkStaticMemberPrefix))
    original_name = original_name.substr(strlen(kBlinkStaticMemberPrefix));
  else if (original_name.startswith(kBlinkFieldPrefix))
    original_name = original_name.substr(strlen(kBlinkFieldPrefix));

  bool is_const = IsProbablyConst(decl, context);
  if (is_const) {
    // Don't try to rename constants that already conform to Chrome style.
    if (original_name.size() >= 2 && original_name[0] == 'k' &&
        clang::isUppercase(original_name[1]))
      return false;
    name = 'k';
    name.append(original_name.data(), original_name.size());
    name[1] = clang::toUppercase(name[1]);
  } else {
    name = CamelCaseToUnderscoreCase(original_name);
  }

  // Static members end with _ just like other members, but constants should
  // not.
  if (!is_const && decl.isStaticDataMember()) {
    name += '_';
  }

  return true;
}

template <typename Type>
struct TargetNodeTraits;

template <>
struct TargetNodeTraits<clang::NamedDecl> {
  static constexpr char kName[] = "decl";
  static clang::CharSourceRange GetRange(const clang::NamedDecl& decl) {
    return clang::CharSourceRange::getTokenRange(decl.getLocation());
  }
};
constexpr char TargetNodeTraits<clang::NamedDecl>::kName[];

template <>
struct TargetNodeTraits<clang::MemberExpr> {
  static constexpr char kName[] = "expr";
  static clang::CharSourceRange GetRange(const clang::MemberExpr& expr) {
    return clang::CharSourceRange::getTokenRange(expr.getMemberLoc());
  }
};
constexpr char TargetNodeTraits<clang::MemberExpr>::kName[];

template <>
struct TargetNodeTraits<clang::DeclRefExpr> {
  static constexpr char kName[] = "expr";
  static clang::CharSourceRange GetRange(const clang::DeclRefExpr& expr) {
    return clang::CharSourceRange::getTokenRange(expr.getLocation());
  }
};
constexpr char TargetNodeTraits<clang::DeclRefExpr>::kName[];

template <>
struct TargetNodeTraits<clang::CXXCtorInitializer> {
  static constexpr char kName[] = "initializer";
  static clang::CharSourceRange GetRange(
      const clang::CXXCtorInitializer& init) {
    return clang::CharSourceRange::getTokenRange(init.getSourceLocation());
  }
};
constexpr char TargetNodeTraits<clang::CXXCtorInitializer>::kName[];

template <typename DeclNode, typename TargetNode>
class RewriterBase : public MatchFinder::MatchCallback {
 public:
  explicit RewriterBase(Replacements* replacements)
      : replacements_(replacements) {}

  void run(const MatchFinder::MatchResult& result) override {
    std::string name;
    const DeclNode* decl = result.Nodes.getNodeAs<DeclNode>("decl");
    clang::ASTContext* context = result.Context;
    if (!GetNameForDecl(*decl, *context, name))
      return;
    auto r = replacements_->emplace(
        *result.SourceManager, TargetNodeTraits<TargetNode>::GetRange(
                                   *result.Nodes.getNodeAs<TargetNode>(
                                       TargetNodeTraits<TargetNode>::kName)),
        name);
    auto from = decl->getNameAsString();
    auto to = r.first->getReplacementText().str();
    if (from != to)
      replacement_names_.emplace(std::move(from), std::move(to));
  }

  const std::unordered_map<std::string, std::string>& replacement_names()
      const {
    return replacement_names_;
  }

 private:
  Replacements* const replacements_;
  std::unordered_map<std::string, std::string> replacement_names_;
};

using FieldDeclRewriter = RewriterBase<clang::FieldDecl, clang::NamedDecl>;
using VarDeclRewriter = RewriterBase<clang::VarDecl, clang::NamedDecl>;
using MemberRewriter = RewriterBase<clang::FieldDecl, clang::MemberExpr>;
using DeclRefRewriter = RewriterBase<clang::VarDecl, clang::DeclRefExpr>;
using FunctionDeclRewriter =
    RewriterBase<clang::FunctionDecl, clang::NamedDecl>;
using FunctionRefRewriter =
    RewriterBase<clang::FunctionDecl, clang::DeclRefExpr>;
using ConstructorInitializerRewriter =
    RewriterBase<clang::FieldDecl, clang::CXXCtorInitializer>;

// Helpers for rewriting methods. The tool needs to detect overrides of Blink
// methods, and uses two matchers to help accomplish this goal:
// - The first matcher matches all method declarations in Blink. When the
//   callback rewrites the declaration, it also stores a pointer to the
//   canonical declaration, to record it as a Blink method.
// - The second matcher matches all method declarations that are overrides. When
//   the callback processes the match, it checks if its overriding a method that
//   was marked as a Blink method. If so, it rewrites the declaration.
// - Because an override is determined based on inclusion in the set of Blink
//   methods, the overridden methods matcher does not need to filter out special
//   member functions: they get filtered out by virtue of the first matcher.
//
// This works because per the documentation on MatchFinder:
//   The order of matches is guaranteed to be equivalent to doing a pre-order
//   traversal on the AST, and applying the matchers in the order in which they
//   were added to the MatchFinder.
//
// Since classes cannot forward declare their base classes, it is guaranteed
// that the base class methods will be seen before processing the overridden
// methods.
class MethodDeclRewriter
    : public RewriterBase<clang::CXXMethodDecl, clang::NamedDecl> {
 public:
  explicit MethodDeclRewriter(Replacements* replacements)
      : RewriterBase(replacements) {}

  void run(const MatchFinder::MatchResult& result) override {
    const clang::CXXMethodDecl* method_decl =
        result.Nodes.getNodeAs<clang::CXXMethodDecl>("decl");
    // TODO(dcheng): Does this need to check for the override attribute, or is
    // this good enough?
    if (method_decl->size_overridden_methods() > 0) {
      if (!IsBlinkOverride(method_decl))
        return;
    } else {
      blink_methods_.emplace(method_decl->getCanonicalDecl());
    }

    RewriterBase::run(result);
  }

  bool IsBlinkOverride(const clang::CXXMethodDecl* decl) const {
    assert(decl->size_overridden_methods() > 0);
    for (auto it = decl->begin_overridden_methods();
         it != decl->end_overridden_methods(); ++it) {
      if (blink_methods_.find((*it)->getCanonicalDecl()) !=
          blink_methods_.end())
        return true;
    }
    return false;
  }

 private:
  std::unordered_set<const clang::CXXMethodDecl*> blink_methods_;
};

template <typename Base>
class FilteringMethodRewriter : public Base {
 public:
  FilteringMethodRewriter(const MethodDeclRewriter& decl_rewriter,
                          Replacements* replacements)
      : Base(replacements), decl_rewriter_(decl_rewriter) {}

  void run(const MatchFinder::MatchResult& result) override {
    const clang::CXXMethodDecl* method_decl =
        result.Nodes.getNodeAs<clang::CXXMethodDecl>("decl");
    if (method_decl->size_overridden_methods() > 0 &&
        !decl_rewriter_.IsBlinkOverride(method_decl))
      return;
    Base::run(result);
  }

 private:
  const MethodDeclRewriter& decl_rewriter_;
};

using MethodRefRewriter = FilteringMethodRewriter<
    RewriterBase<clang::CXXMethodDecl, clang::DeclRefExpr>>;
using MethodMemberRewriter = FilteringMethodRewriter<
    RewriterBase<clang::CXXMethodDecl, clang::MemberExpr>>;

}  // namespace

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

int main(int argc, const char* argv[]) {
  // TODO(dcheng): Clang tooling should do this itself.
  // http://llvm.org/bugs/show_bug.cgi?id=21627
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::cl::OptionCategory category(
      "rewrite_to_chrome_style: convert Blink style to Chrome style.");
  CommonOptionsParser options(argc, argv, category);
  clang::tooling::ClangTool tool(options.getCompilations(),
                                 options.getSourcePathList());

  MatchFinder match_finder;
  Replacements replacements;

  auto in_blink_namespace =
      decl(hasAncestor(namespaceDecl(anyOf(hasName("blink"), hasName("WTF")),
                                     hasParent(translationUnitDecl()))));
  // The ^gen/ rule is used for production code, but the /gen/ one exists here
  // too for making testing easier.
  auto not_generated = decl(unless(isExpansionInFileMatching("^gen/|/gen/")));

  // Field and variable declarations ========
  // Given
  //   int x;
  //   struct S {
  //     int y;
  //   };
  // matches |x| and |y|.
  auto field_decl_matcher =
      id("decl", fieldDecl(in_blink_namespace, not_generated));
  auto var_decl_matcher =
      id("decl", varDecl(in_blink_namespace, not_generated));

  FieldDeclRewriter field_decl_rewriter(&replacements);
  match_finder.addMatcher(field_decl_matcher, &field_decl_rewriter);

  VarDeclRewriter var_decl_rewriter(&replacements);
  match_finder.addMatcher(var_decl_matcher, &var_decl_rewriter);

  // Field and variable references ========
  // Given
  //   bool x = true;
  //   if (x) {
  //     ...
  //   }
  // matches |x| in if (x).
  auto member_matcher = id("expr", memberExpr(member(field_decl_matcher)));
  auto decl_ref_matcher = id("expr", declRefExpr(to(var_decl_matcher)));

  MemberRewriter member_rewriter(&replacements);
  match_finder.addMatcher(member_matcher, &member_rewriter);

  DeclRefRewriter decl_ref_rewriter(&replacements);
  match_finder.addMatcher(decl_ref_matcher, &decl_ref_rewriter);

  // Non-method function declarations ========
  // Given
  //   void f();
  //   struct S {
  //     void g();
  //   };
  // matches |f| but not |g|.
  auto function_decl_matcher = id(
      "decl",
      functionDecl(
          unless(anyOf(
              // Methods are covered by the method matchers.
              cxxMethodDecl(),
              // Out-of-line overloaded operators have special names and should
              // never be renamed.
              isOverloadedOperator())),
          in_blink_namespace, not_generated));
  FunctionDeclRewriter function_decl_rewriter(&replacements);
  match_finder.addMatcher(function_decl_matcher, &function_decl_rewriter);

  // Non-method function references ========
  // Given
  //   f();
  //   void (*p)() = &f;
  // matches |f()| and |&f|.
  auto function_ref_matcher =
      id("expr", declRefExpr(to(function_decl_matcher)));
  FunctionRefRewriter function_ref_rewriter(&replacements);
  match_finder.addMatcher(function_ref_matcher, &function_ref_rewriter);

  // Method declarations ========
  // Given
  //   struct S {
  //     void g();
  //   };
  // matches |g|.
  //
  // Note: the AST matchers don't provide a good way to match against an
  // override from a given base class. Instead, the rewriter uses two matchers:
  // one that matches all method declarations in the Blink namespace, and
  // another which matches all overridden methods not in the Blink namespace.
  // The second list is filtered against the first list to determine which
  // methods are inherited from Blink classes and need to be rewritten.
  auto blink_method_decl_matcher =
      id("decl", cxxMethodDecl(unless(anyOf(
                                   // Overloaded operators have special names
                                   // and should never be renamed.
                                   isOverloadedOperator(),
                                   // Similarly, constructors, destructors, and
                                   // conversion functions should not be
                                   // considered for renaming.
                                   cxxConstructorDecl(), cxxDestructorDecl(),
                                   cxxConversionDecl())),
                               in_blink_namespace, not_generated));
  // Note that the matcher for overridden methods doesn't need to filter for
  // special member functions: see implementation of FunctionDeclRewriter for
  // the full explanation.
  auto non_blink_overridden_method_decl_matcher = id(
      "decl",
      cxxMethodDecl(isOverride(), unless(in_blink_namespace), not_generated));
  MethodDeclRewriter method_decl_rewriter(&replacements);
  match_finder.addMatcher(blink_method_decl_matcher, &method_decl_rewriter);
  match_finder.addMatcher(non_blink_overridden_method_decl_matcher,
                          &method_decl_rewriter);

  // Method references in a non-member context ========
  // Given
  //   S s;
  //   s.g();
  //   void (S::*p)() = &S::g;
  // matches |&S::g| but not |s.g()|.
  auto blink_method_ref_matcher =
      id("expr", declRefExpr(to(blink_method_decl_matcher)));
  auto non_blink_overridden_method_ref_matcher =
      id("expr", declRefExpr(to(non_blink_overridden_method_decl_matcher)));

  MethodRefRewriter method_ref_rewriter(method_decl_rewriter, &replacements);
  match_finder.addMatcher(blink_method_ref_matcher, &method_ref_rewriter);
  match_finder.addMatcher(non_blink_overridden_method_ref_matcher,
                          &method_ref_rewriter);

  // Method references in a member context ========
  // Given
  //   S s;
  //   s.g();
  //   void (S::*p)() = &S::g;
  // matches |s.g()| but not |&S::g|.
  auto blink_method_member_matcher =
      id("expr", memberExpr(member(blink_method_decl_matcher)));
  auto non_blink_overridden_method_member_matcher =
      id("expr", memberExpr(member(non_blink_overridden_method_decl_matcher)));

  MethodMemberRewriter method_member_rewriter(method_decl_rewriter,
                                              &replacements);
  match_finder.addMatcher(blink_method_member_matcher, &method_member_rewriter);
  match_finder.addMatcher(non_blink_overridden_method_member_matcher,
                          &method_member_rewriter);

  // Initializers ========
  // Given
  //   struct S {
  //     int x;
  //     S() : x(2) {}
  //   };
  // matches each initializer in the constructor for S.
  auto constructor_initializer_matcher =
      cxxConstructorDecl(forEachConstructorInitializer(
          id("initializer", cxxCtorInitializer(forField(field_decl_matcher)))));

  ConstructorInitializerRewriter constructor_initializer_rewriter(
      &replacements);
  match_finder.addMatcher(constructor_initializer_matcher,
                          &constructor_initializer_rewriter);

  std::unique_ptr<clang::tooling::FrontendActionFactory> factory =
      clang::tooling::newFrontendActionFactory(&match_finder);
  int result = tool.run(factory.get());
  if (result != 0)
    return result;

#if defined(_WIN32)
  HFILE lockfd = CreateFile("rewrite-sym.lock", GENERIC_READ, FILE_SHARE_READ,
                            NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  OVERLAPPED overlapped = {};
  LockFileEx(lockfd, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &overlapped);
#else
  int lockfd = open("rewrite-sym.lock", O_RDWR | O_CREAT, 0666);
  while (flock(lockfd, LOCK_EX)) {  // :D
  }
#endif

  std::ofstream replacement_db_file("rewrite-sym.txt",
                                    std::ios_base::out | std::ios_base::app);
  for (const auto& p : field_decl_rewriter.replacement_names())
    replacement_db_file << "var:" << p.first << ":" << p.second << "\n";
  for (const auto& p : var_decl_rewriter.replacement_names())
    replacement_db_file << "var:" << p.first << ":" << p.second << "\n";
  for (const auto& p : function_decl_rewriter.replacement_names())
    replacement_db_file << "fun:" << p.first << ":" << p.second << "\n";
  for (const auto& p : method_decl_rewriter.replacement_names())
    replacement_db_file << "fun:" << p.first << ":" << p.second << "\n";
  replacement_db_file.close();

#if defined(_WIN32)
  UnlockFileEx(lockfd, 0, 1, 0, &overlapped);
  CloseHandle(lockfd);
#else
  flock(lockfd, LOCK_UN);
  close(lockfd);
#endif

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
