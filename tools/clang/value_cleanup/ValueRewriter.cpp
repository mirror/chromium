// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ValueRewriter.h"

#include <utility>

#include "clang/Lex/Preprocessor.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"

using namespace clang::ast_matchers;

ValueRewriter::ListValueCallback::ListValueCallback(
    std::string method,
    std::string replacement,
    std::set<clang::tooling::Replacement>* replacements)
    : method_(std::move(method)),
      replacement_(std::move(replacement)),
      replacements_(replacements) {}

void ValueRewriter::ListValueCallback::run(
    const MatchFinder::MatchResult& result) {
  auto* callExpr = result.Nodes.getNodeAs<clang::CXXMemberCallExpr>(method());

  clang::CharSourceRange call_range =
      clang::CharSourceRange::getTokenRange(callExpr->getExprLoc());
  replacements_->emplace(*result.SourceManager, call_range, replacement());
}

ValueRewriter::DictValueCallback::DictValueCallback(
    std::string method,
    std::set<clang::tooling::Replacement>* replacements)
    : method_(std::move(method)), replacements_(replacements) {}

void ValueRewriter::DictValueCallback::run(
    const MatchFinder::MatchResult& result) {
  auto* call_expr = result.Nodes.getNodeAs<clang::CallExpr>(method());
  auto* key_literal =
      result.Nodes.getNodeAs<clang::StringLiteral>("keyLiteral");

  // Split the key literal by '.', expect a single token most of the time.
  llvm::SmallVector<llvm::StringRef, 1> tokens;
  key_literal->getString().split(tokens, '.');

  auto call_range =
      clang::CharSourceRange::getTokenRange(call_expr->getExprLoc());

  // Rewrite the method to either SetKey() or SetPath(), depending on the number
  // of tokens. Also construct the appropriate initializer list, if necessary.
  if (tokens.size() == 1) {
    replacements_->emplace(*result.SourceManager, call_range, "SetKey");
  } else {
    replacements_->emplace(*result.SourceManager, call_range, "SetPath");
    std::string ilist =
        "{\"" + llvm::join(tokens.begin(), tokens.end(), "\", \"") + "\"}";
    replacements_->emplace(*result.SourceManager, call_expr->getArg(0), ilist);
  }

  auto val_expr = call_expr->getArg(1);
  auto val_begin = val_expr->getExprLoc();
  auto val_end = clang::Lexer::getLocForEndOfToken(
      val_begin, 0, *result.SourceManager, result.Context->getLangOpts());

  // Wrap |val_expr| in `base::Value(...)`.
  replacements_->emplace(*result.SourceManager, val_begin, 0, "base::Value(");
  replacements_->emplace(*result.SourceManager, val_end, 0, ")");
}

ValueRewriter::ValueRewriter(
    std::set<clang::tooling::Replacement>* replacements)
    : list_value_callbacks_({
          {"::base::ListValue::Clear", "GetList().clear", replacements},
          {"::base::ListValue::GetSize", "GetList().size", replacements},
          {"::base::ListValue::empty", "GetList().empty", replacements},
          {"::base::ListValue::Reserve", "GetList().reserve", replacements},
          {"::base::ListValue::AppendBoolean", "GetList().emplace_back",
           replacements},
          {"::base::ListValue::AppendInteger", "GetList().emplace_back",
           replacements},
          {"::base::ListValue::AppendDouble", "GetList().emplace_back",
           replacements},
          {"::base::ListValue::AppendString", "GetList().emplace_back",
           replacements},
      }),
      dict_value_callbacks_({
          {"::base::DictionaryValue::SetBoolean", replacements},
          {"::base::DictionaryValue::SetInteger", replacements},
          {"::base::DictionaryValue::SetDouble", replacements},
          {"::base::DictionaryValue::SetString", replacements},
      }) {}

void ValueRewriter::RegisterMatchers(MatchFinder* match_finder) {
  for (auto& callback : list_value_callbacks_) {
    match_finder->addMatcher(
        callExpr(callee(functionDecl(hasName(callback.method()))))
            .bind(callback.method()),
        &callback);
  }

  for (auto& callback : dict_value_callbacks_) {
    match_finder->addMatcher(
        callExpr(
            callee(functionDecl(hasName(callback.method()))),
            argumentCountIs(2),
            hasArgument(0, hasDescendant(stringLiteral().bind("keyLiteral"))))
            .bind(callback.method()),
        &callback);
  }
}
