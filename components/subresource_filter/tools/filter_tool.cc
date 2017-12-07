// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/subresource_filter/core/common/activation_level.h"
#include "components/subresource_filter/core/common/activation_state.h"
#include "components/subresource_filter/core/common/document_subresource_filter.h"
#include "components/subresource_filter/core/common/load_policy.h"
#include "components/subresource_filter/core/common/memory_mapped_ruleset.h"
#include "components/url_pattern_index/proto/rules.pb.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

void PrintHelp() {
  std::cout << std::endl;
  std::cout << "filtertool <indexed_ruleset_path> cmd" << std::endl;
  std::cout << "commands:" << std::endl;
  std::cout << "  match <document-origin> <request-url> <optional request-type>"
            << std::endl;
  std::cout << std::endl;
  std::cout << "  batch <csv-file-path>" << std::endl;
  std::cout << "    The csv file format should be:" << std::endl;
  std::cout
      << "      <id>;<document-origin>;<request-url>;<optional request-type>"
      << std::endl;
  std::cout << "      The id is your choice, it's printed in the output:"
            << std::endl;
  std::cout << "        <id> BLOCKED/NOT BLOCKED" << std::endl;
  std::cout << std::endl;
}

url::Origin ParseOrigin(base::StringPiece arg) {
  GURL origin_url(arg);
  if (!origin_url.is_valid()) {
    std::cerr << "Origin is not valid: " << arg << std::endl;
    PrintHelp();
    exit(1);
  }
  return url::Origin::Create(origin_url);
}

GURL ParseRequestUrl(base::StringPiece arg) {
  GURL request_url(arg);
  if (!request_url.is_valid()) {
    std::cerr << "Request URL is not valid: " << arg << std::endl;
    PrintHelp();
    exit(1);
  }
  return request_url;
}

url_pattern_index::proto::ElementType ParseType(base::StringPiece type) {
  url_pattern_index::proto::ElementType element_type =
      url_pattern_index::proto::ELEMENT_TYPE_OTHER;

  // If the user provided a resource type, use it. Else unspecified.
  if (type == "other")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_OTHER;
  else if (type == "script")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_SCRIPT;
  else if (type == "image")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_IMAGE;
  else if (type == "stylesheet")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_STYLESHEET;
  else if (type == "object")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_OBJECT;
  else if (type == "xmlhttprequest")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_XMLHTTPREQUEST;
  else if (type == "object_subrequest")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_OBJECT_SUBREQUEST;
  else if (type == "subdocument")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_SUBDOCUMENT;
  else if (type == "ping")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_PING;
  else if (type == "media")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_MEDIA;
  else if (type == "font")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_FONT;
  else if (type == "popup")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_POPUP;
  else if (type == "websocket")
    element_type = url_pattern_index::proto::ELEMENT_TYPE_WEBSOCKET;

  return element_type;
}

bool ShouldBlockRequest(subresource_filter::MemoryMappedRuleset* ruleset,
                        const url::Origin& origin,
                        const GURL& request_url,
                        url_pattern_index::proto::ElementType type) {
  subresource_filter::DocumentSubresourceFilter filter(
      origin,
      subresource_filter::ActivationState(
          subresource_filter::ActivationLevel::ENABLED),
      ruleset);

  subresource_filter::LoadPolicy policy =
      filter.GetLoadPolicy(request_url, type);

  return policy == subresource_filter::LoadPolicy::DISALLOW;
}

int ParseId(base::StringPiece arg) {
  int out;
  if (!base::StringToInt(arg, &out)) {
    std::cerr << "Could not convert id to integer: " << arg << std::endl;
    PrintHelp();
    exit(1);
  }
  return out;
}

std::string BlockedAsString(bool is_blocked) {
  return is_blocked ? "BLOCKED" : "ALLOWED";
}

}  // namespace

// Runs on the main thread
int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  base::CommandLine::StringVector args = command_line.GetArgs();

  if (args.size() < 2U) {
    PrintHelp();
    return 1;
  }

  base::File rules_file(base::FilePath(args[0]),
                        base::File::FLAG_OPEN | base::File::FLAG_READ);

  if (!rules_file.IsValid()) {
    std::cerr << "Could not open file: " << args[0] << std::endl;
    PrintHelp();
    return 1;
  }

  scoped_refptr<subresource_filter::MemoryMappedRuleset> ruleset(
      new subresource_filter::MemoryMappedRuleset(std::move(rules_file)));

  if (ruleset->length() == 0u) {
    std::cerr << "Empty ruleset for file: " << args[0] << std::endl;
    PrintHelp();
    return 1;
  }

  std::string cmd = args[1];

  if (cmd == "match") {
    bool is_blocked =
        ShouldBlockRequest(ruleset.get(), ParseOrigin(args[2]),
                           ParseRequestUrl(args[3]), ParseType(args[4]));
    std::cout << BlockedAsString(is_blocked) << std::endl;
    return 0;
  }

  if (cmd != "batch") {
    std::cerr << "Not a recognized command " << cmd << std::endl;
    PrintHelp();
    return 1;
  }

  std::ifstream stream(args[2]);
  std::string line;
  while (std::getline(stream, line)) {
    std::vector<base::StringPiece> strings = base::SplitStringPiece(
        line, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    if (line == "")
      continue;

    if (strings.size() != 4u) {
      std::cerr << "Expected 4 columns, but got " << strings.size() << ": "
                << line << std::endl;
      PrintHelp();
      exit(1);
    }

    int id = ParseId(strings[0]);
    bool is_blocked =
        ShouldBlockRequest(ruleset.get(), ParseOrigin(strings[1]),
                           ParseRequestUrl(strings[2]), ParseType(strings[3]));

    std::cout << id << " " << BlockedAsString(is_blocked) << std::endl;
  }
  return 0;
}
