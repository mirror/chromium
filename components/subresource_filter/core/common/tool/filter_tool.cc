// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

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

std::string ConvertString(const flatbuffers::String* string) {
  return string ? std::string(string->data(), string->size()) : "";
}

std::string OptionSeparator(bool* printed_options) {
  if (!*printed_options) {
    *printed_options = true;
    return "$";
  }

  return ",";
}

std::string PartyOptions(bool* printed_options,
                         const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;
  bool third_party = flat_rule->options() &
                     url_pattern_index::flat::OptionFlag_APPLIES_TO_THIRD_PARTY;
  bool first_party = flat_rule->options() &
                     url_pattern_index::flat::OptionFlag_APPLIES_TO_FIRST_PARTY;
  if (first_party ^ third_party) {
    if (first_party)
      out += OptionSeparator(printed_options) + "~third-party";
    else
      out += OptionSeparator(printed_options) + "third-party";
  }
  return out;
}

std::string TypeOptions(bool* printed_options,
                        const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;

  if (flat_rule->activation_types() &
      url_pattern_index::flat::ActivationType_DOCUMENT) {
    out += OptionSeparator(printed_options) + "document";
  }

  if (flat_rule->activation_types() &
      url_pattern_index::flat::ActivationType_GENERIC_BLOCK) {
    out += OptionSeparator(printed_options) + "genericblock";
  }

  uint16_t types = flat_rule->element_types();

  if (types == url_pattern_index::flat::ElementType_ANY)
    return out;

  if (types & url_pattern_index::flat::ElementType_OTHER)
    out += OptionSeparator(printed_options) + "other";
  if (types & url_pattern_index::flat::ElementType_SCRIPT)
    out += OptionSeparator(printed_options) + "script";
  if (types & url_pattern_index::flat::ElementType_IMAGE)
    out += OptionSeparator(printed_options) + "image";
  if (types & url_pattern_index::flat::ElementType_STYLESHEET)
    out += OptionSeparator(printed_options) + "stylesheet";
  if (types & url_pattern_index::flat::ElementType_OBJECT)
    out += OptionSeparator(printed_options) + "object";
  if (types & url_pattern_index::flat::ElementType_XMLHTTPREQUEST)
    out += OptionSeparator(printed_options) + "xmlhttprequest";
  if (types & url_pattern_index::flat::ElementType_OBJECT_SUBREQUEST)
    out += OptionSeparator(printed_options) + "object-subrequest";
  if (types & url_pattern_index::flat::ElementType_SUBDOCUMENT)
    out += OptionSeparator(printed_options) + "subdocument";
  if (types & url_pattern_index::flat::ElementType_PING)
    out += OptionSeparator(printed_options) + "ping";
  if (types & url_pattern_index::flat::ElementType_MEDIA)
    out += OptionSeparator(printed_options) + "media";
  if (types & url_pattern_index::flat::ElementType_WEBSOCKET)
    out += OptionSeparator(printed_options) + "websocket";

  // FONT is an ElementType but not in easylist, so ignoring it.

  return out;
}

std::string DomainOptions(bool* printed_options,
                          const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;
  if (!flat_rule->domains_included() && !flat_rule->domains_excluded())
    return "";

  out += OptionSeparator(printed_options) + "domain=";

  bool first = true;
  if (flat_rule->domains_included()) {
    for (auto* domain : *flat_rule->domains_included()) {
      if (!first)
        out += "|";
      first = false;
      out += ConvertString(domain);
    }
  }
  if (flat_rule->domains_excluded()) {
    for (auto* domain : *flat_rule->domains_excluded()) {
      if (!first)
        out += "|";
      first = false;
      out += "~" + ConvertString(domain);
    }
  }
  return out;
}

std::string AnchorToString(url_pattern_index::flat::AnchorType anchor_type) {
  std::string out;
  switch (anchor_type) {
    case url_pattern_index::flat::AnchorType_NONE:
      break;
    case url_pattern_index::flat::AnchorType_SUBDOMAIN:
      out += '|';
    case url_pattern_index::flat::AnchorType_BOUNDARY:
      out += '|';
  }
  return out;
}

std::string FlatUrlRuleToEasyListRule(
    const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;

  if (flat_rule->options() & url_pattern_index::flat::OptionFlag_IS_WHITELIST)
    out += "@@";

  out += AnchorToString(flat_rule->anchor_left());
  out += ConvertString(flat_rule->url_pattern());
  out += AnchorToString(flat_rule->anchor_right());

  bool printed_options = false;

  out += PartyOptions(&printed_options, flat_rule);

  if (flat_rule->options() & url_pattern_index::flat::OptionFlag_IS_MATCH_CASE)
    out += OptionSeparator(&printed_options) + "match-case";

  out += TypeOptions(&printed_options, flat_rule);

  out += DomainOptions(&printed_options, flat_rule);

  return out;
}

const url_pattern_index::flat::UrlRule* FindMatchingRule(
    subresource_filter::MemoryMappedRuleset* ruleset,
    const url::Origin& origin,
    const GURL& request_url,
    url_pattern_index::proto::ElementType type) {
  subresource_filter::DocumentSubresourceFilter filter(
      origin,
      subresource_filter::ActivationState(
          subresource_filter::ActivationLevel::ENABLED),
      ruleset);

  return filter.FindMatchingRule(request_url, type);
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

  scoped_refptr<subresource_filter::MemoryMappedRuleset> ruleset(
      new subresource_filter::MemoryMappedRuleset(std::move(rules_file)));

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

    const url_pattern_index::flat::UrlRule* rule = FindMatchingRule(
        ruleset.get(),
        ParseOrigin(command_line.GetSwitchValueASCII(kSwitchOrigin)),
        ParseRequestUrl(command_line.GetSwitchValueASCII(kSwitchUrl)),
        ParseType(command_line.GetSwitchValueASCII(kSwitchType)));

    if (rule) {
      std::cout << FlatUrlRuleToEasyListRule(rule).c_str() << std::endl;
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
    std::vector<base::StringPiece> strings = base::SplitStringPiece(
        line, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

    if (line == "")
      continue;

    if (strings.size() != 3u) {
      std::cerr << "Expected 3 columns, but got " << strings.size() << ": "
                << line << std::endl;
      PrintHelp();
      return 1;
    }

    const url_pattern_index::flat::UrlRule* rule =
        FindMatchingRule(ruleset.get(), ParseOrigin(strings[0]),
                         ParseRequestUrl(strings[1]), ParseType(strings[2]));
    if (rule)
      matched_rules[rule] += 1;
  }

  for (auto rule_and_count : matched_rules) {
    if (rule_and_count.second >= min_match_count)
      std::cout << FlatUrlRuleToEasyListRule(rule_and_count.first).c_str()
                << std::endl;
  }

  return 0;
}
