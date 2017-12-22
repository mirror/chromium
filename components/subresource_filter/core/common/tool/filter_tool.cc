// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "components/subresource_filter/core/common/activation_level.h"
#include "components/subresource_filter/core/common/activation_state.h"
#include "components/subresource_filter/core/common/document_subresource_filter.h"
#include "components/subresource_filter/core/common/load_policy.h"
#include "components/subresource_filter/core/common/memory_mapped_ruleset.h"
#include "components/url_pattern_index/proto/rules.pb.h"
#include "components/url_pattern_index/url_rule_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

// If you change any of the switch strings, update the kHelpMsg accordingly.
const char kSwitchRuleset[] = "ruleset";
const char kSwitchOrigin[] = "origin";
const char kSwitchUrl[] = "url";
const char kSwitchType[] = "type";
const char kSwitchInputFile[] = "input_file";
const char kSwitchMinMatches[] = "min_matches";

const char kHelpMsg[] = R"(
  filtertool --ruleset=<indexed_ruleset_path> command

  Filtertool prints an easylist-formatted ruleset with the rules
  from <indexed_ruleset> that match the given request. For a given URL, if a
  whitelist rule matches as well as a blacklist rule, the whitelist rule is
  recorded but not the blacklist rule. If only a whitelist rule matches, it's
  not recorded.

  commands:
    * match --origin=<origin> --url=<request_url> --type=<request_type>
    * batch --input_file=<csv_file_path> --min_matches=<optional min_matches>
        The csv file is space delimited like so:
          <origin> <request_url> <type>
        |min_matches| is the minimum number of times the rule has to be
        matched to be included in the output. If left empty, the default is 1.
)";

void PrintHelp() {
  std::cout << kHelpMsg << std::endl << std::endl;
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

  // If the user provided a resource type, use it. Else if it's the empty string
  // it will default to ELEMENT_TYPE_OTHER.
  if (type == "other")
    return url_pattern_index::proto::ELEMENT_TYPE_OTHER;
  if (type == "script")
    return url_pattern_index::proto::ELEMENT_TYPE_SCRIPT;
  if (type == "image")
    return url_pattern_index::proto::ELEMENT_TYPE_IMAGE;
  if (type == "stylesheet")
    return url_pattern_index::proto::ELEMENT_TYPE_STYLESHEET;
  if (type == "object")
    return url_pattern_index::proto::ELEMENT_TYPE_OBJECT;
  if (type == "xmlhttprequest")
    return url_pattern_index::proto::ELEMENT_TYPE_XMLHTTPREQUEST;
  if (type == "object_subrequest")
    return url_pattern_index::proto::ELEMENT_TYPE_OBJECT_SUBREQUEST;
  if (type == "subdocument")
    return url_pattern_index::proto::ELEMENT_TYPE_SUBDOCUMENT;
  if (type == "ping")
    return url_pattern_index::proto::ELEMENT_TYPE_PING;
  if (type == "media")
    return url_pattern_index::proto::ELEMENT_TYPE_MEDIA;
  if (type == "font")
    return url_pattern_index::proto::ELEMENT_TYPE_FONT;
  if (type == "popup")
    return url_pattern_index::proto::ELEMENT_TYPE_POPUP;
  if (type == "websocket")
    return url_pattern_index::proto::ELEMENT_TYPE_WEBSOCKET;

  return element_type;
}

const url_pattern_index::flat::UrlRule* FindMatchingUrlRule(
    subresource_filter::MemoryMappedRuleset* ruleset,
    const url::Origin& origin,
    const GURL& request_url,
    url_pattern_index::proto::ElementType type) {
  subresource_filter::DocumentSubresourceFilter filter(
      origin,
      subresource_filter::ActivationState(
          subresource_filter::ActivationLevel::ENABLED),
      ruleset);

  return filter.FindMatchingUrlRule(request_url, type);
}

}  // namespace

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();

  base::CommandLine::StringVector args = command_line.GetArgs();

  if (args.size() != 1U) {
    PrintHelp();
    return 1;
  }

  if (!command_line.HasSwitch(kSwitchRuleset)) {
    PrintHelp();
    return 1;
  }

  base::File rules_file(command_line.GetSwitchValuePath(kSwitchRuleset),
                        base::File::FLAG_OPEN | base::File::FLAG_READ);

  if (!rules_file.IsValid()) {
    std::cerr << "Could not open file: "
              << command_line.GetSwitchValueASCII(kSwitchRuleset) << std::endl;
    PrintHelp();
    return 1;
  }

  auto ruleset = base::MakeRefCounted<subresource_filter::MemoryMappedRuleset>(
      std::move(rules_file));

  if (ruleset->length() == 0u) {
    std::cerr << "Empty ruleset for file: " << args[0] << std::endl;
    PrintHelp();
    return 1;
  }

  std::string cmd = args[0];

  if (cmd == "match") {
    if (!command_line.HasSwitch(kSwitchOrigin) ||
        !command_line.HasSwitch(kSwitchUrl) ||
        !command_line.HasSwitch(kSwitchType)) {
      std::cerr << "Missing argument for match command:" << std::endl;
      PrintHelp();
      return 1;
    }

    const url_pattern_index::flat::UrlRule* rule = FindMatchingUrlRule(
        ruleset.get(),
        ParseOrigin(command_line.GetSwitchValueASCII(kSwitchOrigin)),
        ParseRequestUrl(command_line.GetSwitchValueASCII(kSwitchUrl)),
        ParseType(command_line.GetSwitchValueASCII(kSwitchType)));

    if (rule) {
      std::cout << url_pattern_index::FlatUrlRuleToString(rule).c_str()
                << std::endl;
    }

    return 0;
  }

  if (cmd != "batch") {
    std::cerr << "Not a recognized command " << cmd << std::endl;
    PrintHelp();
    return 1;
  }

  int min_match_count = 0;
  if (command_line.HasSwitch(kSwitchMinMatches) &&
      !base::StringToInt(command_line.GetSwitchValueASCII(kSwitchMinMatches),
                         &min_match_count)) {
    std::cerr << "Could not convert min matches to integer: "
              << command_line.GetSwitchValueASCII(kSwitchMinMatches)
              << std::endl;
    PrintHelp();
    return 1;
  }

  if (!command_line.HasSwitch(kSwitchInputFile)) {
    PrintHelp();
    return 1;
  }

  std::unordered_map<const url_pattern_index::flat::UrlRule*, int>
      matched_rules;

  std::ifstream stream(command_line.GetSwitchValueASCII(kSwitchInputFile));
  std::string line;
  while (std::getline(stream, line)) {
    if (line.empty())
      continue;

    std::vector<base::StringPiece> strings = base::SplitStringPiece(
        line, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (strings.size() != 3u) {
      std::cerr << "Expected 3 columns, but got " << strings.size() << ": "
                << line << std::endl;
      PrintHelp();
      return 1;
    }

    const url_pattern_index::flat::UrlRule* rule =
        FindMatchingUrlRule(ruleset.get(), ParseOrigin(strings[0]),
                            ParseRequestUrl(strings[1]), ParseType(strings[2]));
    if (rule)
      matched_rules[rule] += 1;
  }

  for (auto rule_and_count : matched_rules) {
    if (rule_and_count.second >= min_match_count)
      std::cout << url_pattern_index::FlatUrlRuleToString(rule_and_count.first)
                       .c_str()
                << std::endl;
  }

  return 0;
}
