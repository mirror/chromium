// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/accessibility/inspect/ax_tree_server.h"

#include <string>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace content {

const std::string& kAllowOptEmptyStr = "@ALLOW-EMPTY:";
const std::string& kAllowOptStr = "@ALLOW:";
const std::string& kDenyOptStr = "@DENY:";

AXTreeServer::AXTreeServer(base::ProcessId pid, base::string16& opts_path) {
  std::unique_ptr<AccessibilityTreeFormatter> formatter(
      AccessibilityTreeFormatter::Create());

  // Get accessibility tree as nested dictionary.
  base::string16 accessibility_contents_utf16;
  std::unique_ptr<base::DictionaryValue> dict =
      formatter->BuildAccessibilityTreeForProcess(pid);

  if (!dict) {
    printf("Failed to get accessibility tree");
    return;
  }

  Format(*formatter, *dict, opts_path);
}

AXTreeServer::AXTreeServer(gfx::AcceleratedWidget widget,
                           base::string16& opts_path) {
  std::unique_ptr<AccessibilityTreeFormatter> formatter(
      AccessibilityTreeFormatter::Create());

  // Get accessibility tree as nested dictionary.
  std::unique_ptr<base::DictionaryValue> dict =
      formatter->BuildAccessibilityTreeForWindow(widget);

  if (!dict) {
    printf("Failed to get accessibility tree");
    return;
  }

  Format(*formatter, *dict, opts_path);
}

std::vector<AccessibilityTreeFormatter::Filter> GetFilters(
    const base::string16& opts_path) {
  std::vector<AccessibilityTreeFormatter::Filter> filters;
  if (!opts_path.empty()) {
    std::string opts;
    base::ScopedAllowBlockingForTesting allow_io_for_test_setup;
    if (base::ReadFileToString(base::FilePath(opts_path), &opts)) {
      for (const std::string& line : base::SplitString(
               opts, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
        if (base::StartsWith(line, kAllowOptEmptyStr,
                             base::CompareCase::SENSITIVE)) {
          filters.push_back(AccessibilityTreeFormatter::Filter(
              base::UTF8ToUTF16(line.substr(kAllowOptEmptyStr.size())),
              AccessibilityTreeFormatter::Filter::ALLOW_EMPTY));
        } else if (base::StartsWith(line, kAllowOptStr,
                                    base::CompareCase::SENSITIVE)) {
          filters.push_back(AccessibilityTreeFormatter::Filter(
              base::UTF8ToUTF16(line.substr(kAllowOptStr.size())),
              AccessibilityTreeFormatter::Filter::ALLOW));
        } else if (base::StartsWith(line, kDenyOptStr,
                                    base::CompareCase::SENSITIVE)) {
          filters.push_back(AccessibilityTreeFormatter::Filter(
              base::UTF8ToUTF16(line.substr(kDenyOptStr.size())),
              AccessibilityTreeFormatter::Filter::DENY));
        }
      }
    }
  }
  if (filters.empty()) {
    filters.push_back(AccessibilityTreeFormatter::Filter(
        base::ASCIIToUTF16("*"), AccessibilityTreeFormatter::Filter::ALLOW));
  }

  return filters;
}

void AXTreeServer::Format(AccessibilityTreeFormatter& formatter,
                          base::DictionaryValue& dict,
                          base::string16& opts_path) {
  std::vector<AccessibilityTreeFormatter::Filter> filters =
      GetFilters(opts_path);

  // Set filters.
  formatter.SetFilters(filters);

  // Format accessibility tree as text.
  base::string16 accessibility_contents_utf16;
  formatter.FormatAccessibilityTree(dict, &accessibility_contents_utf16);

  // Write to console.
  std::string accessibility_contents_utf8 =
      base::UTF16ToUTF8(accessibility_contents_utf16);
  printf("%s\n", accessibility_contents_utf8.c_str());
}

AXTreeServer::~AXTreeServer() {}

}  // namespace content
