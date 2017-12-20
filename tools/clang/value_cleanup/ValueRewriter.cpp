// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ValueRewriter.h"

using namespace clang::ast_matchers;

ValueRewriter::ListValueCallback::ListValueCallback(
    std::set<clang::tooling::Replacement>* replacements)
    : replacements_(replacements) {}

void ValueRewriter::ListValueCallback::run(
    const MatchFinder::MatchResult& result) {
  for (const auto& method_replacement : kMethodReplacements) {
    const auto& method = method_replacement.first;
    const auto& replacement = method_replacement.second;
    auto* callExpr = result.Nodes.getNodeAs<clang::CXXMemberCallExpr>(method);
    if (!callExpr)
      continue;

    clang::CharSourceRange call_range =
        clang::CharSourceRange::getTokenRange(callExpr->getExprLoc());
    replacements_->emplace(*result.SourceManager, call_range, replacement);
  }
}

ValueRewriter::ValueRewriter(
    std::set<clang::tooling::Replacement>* replacements)
    : list_value_callback_(replacements) {}

void ValueRewriter::RegisterMatchers(MatchFinder* match_finder) {
  for (const auto& replacement : list_value_callback_.kMethodReplacements) {
    const auto& method = replacement.first;
    match_finder->addMatcher(
        callExpr(callee(functionDecl(
                     hasName(list_value_callback_.kPrefix + method))))
            .bind(method),
        &list_value_callback_);
  }
}
