// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Handles the rewriting of base::Value::GetType() to base::Value::type().

#ifndef TOOLS_CLANG_VALUE_CLEANUP_VALUE_REWRITER_H_
#define TOOLS_CLANG_VALUE_CLEANUP_VALUE_REWRITER_H_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"

class ValueRewriter {
 public:
  explicit ValueRewriter(std::set<clang::tooling::Replacement>* replacements);

  void RegisterMatchers(clang::ast_matchers::MatchFinder* match_finder);

 private:
  class ListValueCallback
      : public clang::ast_matchers::MatchFinder::MatchCallback {
   public:
    explicit ListValueCallback(
        std::set<clang::tooling::Replacement>* replacements);

    void run(
        const clang::ast_matchers::MatchFinder::MatchResult& result) override;

    const std::string kPrefix = "::base::ListValue::";
    const std::vector<std::pair<std::string, std::string>> kMethodReplacements =
        {
            {"Clear", "GetList().clear"},
            {"GetSize", "GetList().size"},
            {"empty", "GetList().empty"},
            {"Reserve", "GetList().reserve"},
            {"AppendBoolean", "GetList().emplace_back"},
            {"AppendInteger", "GetList().emplace_back"},
            {"AppendDouble", "GetList().emplace_back"},
            {"AppendString", "GetList().emplace_back"},
        };

   private:
    std::set<clang::tooling::Replacement>* const replacements_;
  };

  ListValueCallback list_value_callback_;
};

#endif  // TOOLS_CLANG_VALUE_CLEANUP_VALUE_REWRITER_H_
