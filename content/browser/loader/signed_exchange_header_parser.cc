// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_header_parser.h"

#include <map>
#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"

namespace content {

namespace {

struct ParameterisedLabel {
  std::string label;
  std::map<std::string, std::string> params;
};

// Parser for (a subset of) Structured Headers defined in [SH].
// [SH] https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02
class StructuredHeaderParser {
 public:
  using iterator = std::string::const_iterator;

  explicit StructuredHeaderParser(iterator begin, iterator end)
      : current_(begin), end_(end), failed_(false) {}

  bool ParsedSuccessfully() const { return !failed_ && current_ == end_; }

  // Parses a List ([SH] 4.8) of Strings.
  void ParseStringList(std::vector<std::string>* values) {
    values->push_back(ReadString());
    while (!failed_) {
      SkipWhitespaces();
      if (!ConsumeChar(','))
        break;
      SkipWhitespaces();
      values->push_back(ReadString());
    }
  }

  // Parses a List ([SH] 4.8) of Parameterised Labels.
  void ParseParameterisedLabelList(std::vector<ParameterisedLabel>* values) {
    values->push_back(ParameterisedLabel());
    ParseParameterisedLabel(&values->back());
    while (!failed_) {
      SkipWhitespaces();
      if (!ConsumeChar(','))
        break;
      SkipWhitespaces();
      values->push_back(ParameterisedLabel());
      ParseParameterisedLabel(&values->back());
    }
  }

  // Parses a Parameterised Label ([SH] 4.4).
  void ParseParameterisedLabel(ParameterisedLabel* out) {
    std::string label = ReadToken();
    if (label.empty()) {
      failed_ = true;
      return;
    }
    out->label = label;

    while (!failed_) {
      SkipWhitespaces();
      if (!ConsumeChar(';'))
        break;
      SkipWhitespaces();

      std::string name = ReadToken();
      if (name.empty()) {
        failed_ = true;
        return;
      }
      // TODO: reject duplicated params
      std::string& value = out->params[name];
      if (ConsumeChar('='))
        value = ReadItem();
    }
  }

 private:
  bool IsWhitespace(char c) const { return (c == ' ' || c == '\t'); }

  void SkipWhitespaces() {
    while (current_ != end_ && IsWhitespace(*current_))
      current_++;
  }

  // This covers the characters allowed in Numbers, Labels, and Binary Content.
  bool IsTokenChar(char c) const {
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
           ('0' <= c && c <= '9') || c == '_' || c == '+' || c == '-' ||
           c == '*' || c == '/';
  }

  std::string ReadToken() {
    iterator token_start = current_;
    while (current_ != end_ && IsTokenChar(*current_))
      current_++;
    return std::string(token_start, current_);
  }

  bool ConsumeChar(char expected) {
    if (current_ != end_ && *current_ == expected) {
      current_++;
      return true;
    }
    return false;
  }

  // [SH] 4.2. Strings
  std::string ReadString() {
    std::string s;
    if (!ConsumeChar('"')) {
      failed_ = true;
      return s;
    }
    while (*current_ != '"') {
      iterator start = current_;
      while (current_ != end_ && *current_ != '"' && *current_ != '\\')
        current_++;
      s.append(start, current_);
      if (current_ == end_) {  // Missing closing '"'
        failed_ = true;
        return s;
      }
      if (*current_ == '\\') {
        current_++;
        if (current_ == end_) {  // Backslash at the end
          failed_ = true;
          return s;
        }
        s.push_back(*current_++);
      }
    }
    current_++;  // Consume '"'
    return s;
  }

  // [SH] 4.5. Binary Content
  std::string ReadBinary() {
    if (!ConsumeChar('*')) {
      failed_ = true;
      return std::string();
    }
    std::string base64 = ReadToken();
    // Binary Content does not have padding, so we have to add it.
    base64.resize((base64.size() + 3) / 4 * 4, '=');
    std::string binary;
    if (!base::Base64Decode(base64, &binary))
      failed_ = true;
    return binary;
  }

  // [SH] 4.6. Items
  std::string ReadItem() {
    if (current_ == end_) {
      failed_ = true;
      return std::string();
    }
    switch (*current_) {
      case '"':
        return ReadString();
      case '*':
        return ReadBinary();
      default:  // label or number
        return ReadToken();
    }
  }

  iterator current_;
  iterator end_;

  bool failed_;
};

}  // namespace

bool SignedExchangeHeaderParser::ParseSignedHeaders(
    const std::string& signed_headers_str,
    std::vector<std::string>* headers) {
  StructuredHeaderParser parser(signed_headers_str.begin(),
                                signed_headers_str.end());
  parser.ParseStringList(headers);
  return parser.ParsedSuccessfully();
}

bool SignedExchangeHeaderParser::ParseSignature(
    const std::string& signature_str,
    std::vector<Signature>* signatures) {
  StructuredHeaderParser parser(signature_str.begin(), signature_str.end());
  std::vector<ParameterisedLabel> values;
  parser.ParseParameterisedLabelList(&values);
  if (!parser.ParsedSuccessfully())
    return false;

  signatures->clear();
  signatures->reserve(values.size());
  for (auto& value : values) {
    uint64_t date, expires;
    if (!base::StringToUint64(value.params["date"], &date) ||
        !base::StringToUint64(value.params["expires"], &expires)) {
      signatures->clear();
      return false;
    }

    signatures->push_back(Signature());
    Signature& sig = signatures->back();
    sig.label = value.label;
    sig.sig = value.params["sig"];
    sig.integrity = value.params["integrity"];
    sig.certUrl = value.params["certUrl"];
    sig.certSha256 = value.params["certSha256"];
    sig.ed25519Key = value.params["ed25519Key"];
    sig.validityUrl = value.params["validityUrl"];
    sig.date = date;
    sig.expires = expires;
  }
  return true;
}

SignedExchangeHeaderParser::Signature::Signature() = default;
SignedExchangeHeaderParser::Signature::Signature(const Signature& other) =
    default;
SignedExchangeHeaderParser::Signature::~Signature() = default;

}  // namespace content
