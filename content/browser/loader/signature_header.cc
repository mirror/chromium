// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signature_header.h"

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

// Parser for (a subset of) Structured Headers
// (https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02).
class StructuredHeaderParser {
 public:
  explicit StructuredHeaderParser(const std::string& str)
      : current_(str.begin()), end_(str.end()), valid_(true) {}

  bool ParsedSuccessfully() const { return valid_ && current_ == end_; }

  // Parses a List of Strings
  // https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02#section-4.8
  void ParseStringList(std::vector<std::string>* values) {
    values->push_back(ReadString());
    while (valid_) {
      SkipWhitespaces();
      if (!ConsumeChar(','))
        break;
      SkipWhitespaces();
      values->push_back(ReadString());
    }
  }

  // Parses a List of Parameterised Labels
  // https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02#section-4.8
  void ParseParameterisedLabelList(std::vector<ParameterisedLabel>* values) {
    values->push_back(ParameterisedLabel());
    ParseParameterisedLabel(&values->back());
    while (valid_) {
      SkipWhitespaces();
      if (!ConsumeChar(','))
        break;
      SkipWhitespaces();
      values->push_back(ParameterisedLabel());
      ParseParameterisedLabel(&values->back());
    }
  }

  // Parses a Parameterised Label
  // https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02#section-4.4
  void ParseParameterisedLabel(ParameterisedLabel* out) {
    std::string label = ReadToken();
    if (label.empty()) {
      valid_ = false;
      return;
    }
    out->label = label;

    while (valid_) {
      SkipWhitespaces();
      if (!ConsumeChar(';'))
        break;
      SkipWhitespaces();

      std::string name = ReadToken();
      if (name.empty()) {
        valid_ = false;
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

  bool IsDelim(char c) const { return (c == ';' || c == ',' || c == '='); }

  // Consumes any OWS from the input.
  void SkipWhitespaces() {
    while (current_ != end_ && IsWhitespace(*current_))
      current_++;
  }

  // Reads until whitespace or delimiter.
  std::string ReadToken() {
    std::string::const_iterator token_start = current_;
    while (current_ != end_ && !IsWhitespace(*current_) && !IsDelim(*current_))
      current_++;
    return std::string(token_start, current_);
  }

  // String
  // https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02#section-4.2
  std::string ReadString() {
    std::string s;
    if (!ConsumeChar('"')) {
      valid_ = false;
      return s;
    }
    while (*current_ != '"') {
      std::string::const_iterator start = current_;
      while (current_ != end_ && *current_ != '"' && *current_ != '\\')
        current_++;
      s.append(start, current_);
      if (current_ == end_) {  // Missing closing '"'
        valid_ = false;
        return s;
      }
      if (*current_ == '\\') {
        current_++;
        if (current_ == end_) {  // Backslash at the end
          valid_ = false;
          return s;
        }
        s.push_back(*current_++);
      }
    }
    current_++;  // Consume '"'
    return s;
  }

  // Binary Content
  // https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02#section-4.5
  std::string ReadBinary() {
    if (!ConsumeChar('*')) {
      valid_ = false;
      return std::string();
    }
    std::string base64 = ReadToken();
    // Binary Content does not have padding, so we have to add it.
    base64.resize((base64.size() + 3) / 4 * 4, '=');
    std::string binary;
    if (!base::Base64Decode(base64, &binary))
      valid_ = false;
    return binary;
  }

  // Item
  // https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-02#section-4.6
  std::string ReadItem() {
    if (current_ == end_) {
      valid_ = false;
      return std::string();
    }
    switch (*current_) {
      case '"':
        return ReadString();
      case '*':
        return ReadBinary();
      default:
        return ReadToken();
    }
  }

  bool ConsumeChar(char expected) {
    if (current_ != end_ && *current_ == expected) {
      current_++;
      return true;
    }
    return false;
  }

  std::string::const_iterator current_;
  std::string::const_iterator end_;

  bool valid_;
};

}  // namespace

// https://wicg.github.io/webpackage/draft-yasskin-http-origin-signed-responses.html#rfc.section.3.1
bool ParseSignedHeaders(const std::string& signed_headers_str,
                        std::vector<std::string>* headers) {
  StructuredHeaderParser parser(signed_headers_str);
  parser.ParseStringList(headers);
  return parser.ParsedSuccessfully();
}

SignatureHeader::SignatureHeader(const std::string& signature_str) {
  StructuredHeaderParser parser(signature_str);
  std::vector<ParameterisedLabel> values;
  parser.ParseParameterisedLabelList(&values);
  if (!parser.ParsedSuccessfully())
    return;

  signatures_.reserve(values.size());
  for (auto& value : values) {
    uint64_t date, expires;
    if (!base::StringToUint64(value.params["date"], &date) ||
        !base::StringToUint64(value.params["expires"], &expires)) {
      signatures_.clear();
      return;
    }

    signatures_.push_back(Signature());
    Signature& sig = signatures_.back();
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
}

SignatureHeader::~SignatureHeader() = default;
SignatureHeader::Signature::Signature() = default;
SignatureHeader::Signature::Signature(const Signature& other) = default;
SignatureHeader::Signature::~Signature() = default;

}  // namespace content
