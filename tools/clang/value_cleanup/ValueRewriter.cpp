// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ValueRewriter.h"

#include <utility>

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
  auto* keyLiteral = result.Nodes.getNodeAs<clang::StringLiteral>("keyLiteral");

  std::string key = keyLiteral->getString().str();
  std::vector<std::string> tokens;

  // Split |key| by '.'.
  size_t start = 0;
  while (start != std::string::npos) {
    size_t end = key.find('.', start);
    if (end == std::string::npos) {
      tokens.emplace_back(key.substr(start));
      start = std::string::npos;
    } else {
      tokens.emplace_back(key.substr(start, end - start));
      start = end + 1;
    }
  }

  auto* callExpr = result.Nodes.getNodeAs<clang::CallExpr>(method());
  auto callRange =
      clang::CharSourceRange::getTokenRange(callExpr->getExprLoc());

  // Rewrite the method to either SetKey() or SetPath(), depending on the number
  // of tokens. Also construct the appropriate initializer list, if necessary.
  if (tokens.size() == 1) {
    replacements_->emplace(*result.SourceManager, callRange, "SetKey");
  } else {
    replacements_->emplace(*result.SourceManager, callRange, "SetPath");

    std::string path = "{";
    for (const auto& token : tokens) {
      path += '"' + token + "\",";
    }
    path.back() = '}';

    auto* keyExpr = callExpr->getArg(0);
    replacements_->emplace(
        *result.SourceManager,
        clang::CharSourceRange::getTokenRange(keyExpr->getExprLoc()), path);
  }

  auto* valExpr = callExpr->getArg(1);
  // Wrap |valExpr| in `base::Value(...)`.
  replacements_->emplace(*result.SourceManager,
                         clang::CharSourceRange::getCharRange(
                             valExpr->getLocStart(), valExpr->getLocStart()),
                         "base::Value(");

  replacements_->emplace(*result.SourceManager,
                         clang::CharSourceRange::getCharRange(
                             callExpr->getLocEnd(), callExpr->getLocEnd()),
                         ")");
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
