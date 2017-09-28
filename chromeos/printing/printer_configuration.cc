// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/printing/printer_configuration.h"

#include <string>

#include "base/guid.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "net/base/ip_endpoint.h"
#include "url/url_constants.h"

namespace chromeos {

namespace {

// Separator = ://
constexpr size_t kSeparatorLen = 3U;

// Returns the index of the first character representing the hostname in |uri|.
// Returns npos if the start of the hostname is not found.
size_t HostnameStart(base::StringPiece uri) {
  size_t scheme_separator_start = uri.find(url::kStandardSchemeSeparator);
  if (scheme_separator_start == base::StringPiece::npos) {
    return base::StringPiece::npos;
  }
  return scheme_separator_start + kSeparatorLen;
}

base::StringPiece HostAndPort(base::StringPiece uri) {
  size_t hostname_start = HostnameStart(uri);
  if (hostname_start == base::StringPiece::npos) {
    return "";
  }

  size_t hostname_end = uri.find("/", hostname_start);
  if (hostname_end == base::StringPiece::npos) {
    // No trailing slash.  Use end of string.
    hostname_end = uri.size();
  }

  DCHECK_GT(hostname_end, hostname_start);
  return uri.substr(hostname_start, hostname_end - hostname_start);
}

}  // namespace

Printer::Printer() : source_(SRC_USER_PREFS) {
  id_ = base::GenerateGUID();
}

Printer::Printer(const std::string& id) : id_(id), source_(SRC_USER_PREFS) {
  if (id_.empty())
    id_ = base::GenerateGUID();
}

Printer::Printer(const Printer& other) = default;

Printer& Printer::operator=(const Printer& other) = default;

Printer::~Printer() {}

bool Printer::IsIppEverywhere() const {
  return ppd_reference_.autoconf;
}

bool Printer::RequiresIpResolution() const {
  return base::StartsWith(id_, "zeroconf-", base::CompareCase::SENSITIVE) &&
         effective_uri_.empty();
}

net::HostPortPair Printer::GetHostAndPort() const {
  if (uri_.empty()) {
    return net::HostPortPair();
  }

  return net::HostPortPair::FromString(HostAndPort(uri_).as_string());
}

std::string Printer::ReplaceHostAndPort(const net::IPEndPoint& ip) const {
  if (uri_.empty()) {
    return "";
  }

  base::StringPiece uri_piece(uri_);
  size_t hostname_start = HostnameStart(uri_);
  if (hostname_start == base::StringPiece::npos) {
    return "";
  }
  size_t host_port_len = HostAndPort(uri_).length();
  std::string effective_uri = uri_piece.substr(0, hostname_start).as_string();
  effective_uri.append(ip.ToString());
  effective_uri.append(
      uri_piece.substr(hostname_start + host_port_len).as_string());

  return effective_uri;
}

Printer::PrinterProtocol Printer::GetProtocol() const {
  const base::StringPiece uri(uri_);

  if (uri.starts_with("usb:"))
    return PrinterProtocol::kUsb;

  if (uri.starts_with("ipp:"))
    return PrinterProtocol::kIpp;

  if (uri.starts_with("ipps:"))
    return PrinterProtocol::kIpps;

  if (uri.starts_with("http:"))
    return PrinterProtocol::kHttp;

  if (uri.starts_with("https:"))
    return PrinterProtocol::kHttps;

  if (uri.starts_with("socket:"))
    return PrinterProtocol::kSocket;

  if (uri.starts_with("lpd:"))
    return PrinterProtocol::kLpd;

  return PrinterProtocol::kUnknown;
}

bool Printer::PpdReference::operator==(
    const Printer::PpdReference& other) const {
  return user_supplied_ppd_url == other.user_supplied_ppd_url &&
         effective_make_and_model == other.effective_make_and_model &&
         autoconf == other.autoconf;
}

}  // namespace chromeos
