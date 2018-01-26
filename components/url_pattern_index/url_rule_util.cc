// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_pattern_index/url_rule_util.h"

#include "base/strings/string_util.h"
#include "components/url_pattern_index/flat/url_pattern_index_generated.h"

namespace url_pattern_index {

namespace {

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

// If this is the first printed option for the rule, add a $ separator,
// otherwise a comma.
std::string WithOptionSeparator(bool* printed_options,
                                const std::string& option) {
  std::string out = *printed_options ? "," : "$";
  *printed_options = true;
  return out + option;
}

std::string PartyOptionsToString(
    bool* printed_options,
    const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;
  bool third_party = flat_rule->options() &
                     url_pattern_index::flat::OptionFlag_APPLIES_TO_THIRD_PARTY;
  bool first_party = flat_rule->options() &
                     url_pattern_index::flat::OptionFlag_APPLIES_TO_FIRST_PARTY;
  if (first_party ^ third_party) {
    if (first_party)
      out += WithOptionSeparator(printed_options, "~third-party");
    else
      out += WithOptionSeparator(printed_options, "third-party");
  }
  return out;
}

std::string TypeOptionsToString(
    bool* printed_options,
    const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;

  if (flat_rule->activation_types() &
      url_pattern_index::flat::ActivationType_DOCUMENT) {
    out += WithOptionSeparator(printed_options, "document");
  }

  if (flat_rule->activation_types() &
      url_pattern_index::flat::ActivationType_GENERIC_BLOCK) {
    out += WithOptionSeparator(printed_options, "genericblock");
  }

  uint16_t types = flat_rule->element_types();

  if (types == url_pattern_index::flat::ElementType_ANY)
    return out;

  if (types & url_pattern_index::flat::ElementType_OTHER)
    out += WithOptionSeparator(printed_options, "other");
  if (types & url_pattern_index::flat::ElementType_SCRIPT)
    out += WithOptionSeparator(printed_options, "script");
  if (types & url_pattern_index::flat::ElementType_IMAGE)
    out += WithOptionSeparator(printed_options, "image");
  if (types & url_pattern_index::flat::ElementType_STYLESHEET)
    out += WithOptionSeparator(printed_options, "stylesheet");
  if (types & url_pattern_index::flat::ElementType_OBJECT)
    out += WithOptionSeparator(printed_options, "object");
  if (types & url_pattern_index::flat::ElementType_XMLHTTPREQUEST)
    out += WithOptionSeparator(printed_options, "xmlhttprequest");
  if (types & url_pattern_index::flat::ElementType_OBJECT_SUBREQUEST)
    out += WithOptionSeparator(printed_options, "object-subrequest");
  if (types & url_pattern_index::flat::ElementType_SUBDOCUMENT)
    out += WithOptionSeparator(printed_options, "subdocument");
  if (types & url_pattern_index::flat::ElementType_PING)
    out += WithOptionSeparator(printed_options, "ping");
  if (types & url_pattern_index::flat::ElementType_MEDIA)
    out += WithOptionSeparator(printed_options, "media");
  if (types & url_pattern_index::flat::ElementType_FONT)
    out += WithOptionSeparator(printed_options, "font");
  if (types & url_pattern_index::flat::ElementType_WEBSOCKET)
    out += WithOptionSeparator(printed_options, "websocket");

  return out;
}

std::string ConvertFlatString(const flatbuffers::String* string) {
  return string ? std::string(string->data(), string->size()) : "";
}

std::string DomainOptionsToString(
    bool* printed_options,
    const url_pattern_index::flat::UrlRule* flat_rule) {
  std::string out;
  if (!flat_rule->domains_included() && !flat_rule->domains_excluded())
    return "";

  out += WithOptionSeparator(printed_options, "domain=");

  bool first = true;
  if (flat_rule->domains_included()) {
    for (auto* domain : *flat_rule->domains_included()) {
      if (!first)
        out += "|";
      first = false;
      out += ConvertFlatString(domain);
    }
  }
  if (flat_rule->domains_excluded()) {
    for (auto* domain : *flat_rule->domains_excluded()) {
      if (!first)
        out += "|";
      first = false;
      out += "~" + ConvertFlatString(domain);
    }
  }
  return out;
}

}  // namespace

std::string FlatUrlRuleToString(const flat::UrlRule* flat_rule) {
  std::string out;

  if (flat_rule->options() & url_pattern_index::flat::OptionFlag_IS_WHITELIST)
    out += "@@";

  out += AnchorToString(flat_rule->anchor_left());

  std::string pattern = ConvertFlatString(flat_rule->url_pattern());

  // Add a wildcard to pattern if necessary to differentiate it from a regex.
  // E.g., /foo/ should be /foo/*.
  if (flat_rule->url_pattern_type() !=
          url_pattern_index::flat::UrlPatternType_REGEXP &&
      pattern.size() >= 2 && pattern[0] == '/' &&
      pattern[pattern.size() - 1] == '/') {
    pattern += "*";
  }
  out += pattern;

  out += AnchorToString(flat_rule->anchor_right());

  bool printed_options = false;

  out += PartyOptionsToString(&printed_options, flat_rule);

  if (flat_rule->options() & url_pattern_index::flat::OptionFlag_IS_MATCH_CASE)
    out += WithOptionSeparator(&printed_options, "match-case");

  out += TypeOptionsToString(&printed_options, flat_rule);

  out += DomainOptionsToString(&printed_options, flat_rule);

  return out;
}  // namespace url_pattern_index

}  // namespace url_pattern_index
