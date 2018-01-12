// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_PRINTING_URI_COMPONENTS_H_
#define CHROMEOS_PRINTING_URI_COMPONENTS_H_

#include "url/third_party/mozilla/url_parse.h"

class CHROMEOS_EXPORT UriComponents {
 public:
  UriComponents() : encrypted_(false), port_(url::SpecialPort::PORT_INVALID) {}

  bool encrypted() const { return encrypted_; }
  void set_encrypted(bool encrypted) { encrypted_ = encrypted; }

  std::string scheme() const { return scheme_; }
  void set_scheme(const std::string& scheme) { scheme_ = scheme; }

  std::string host() const { return host_; }
  void set_host(const std::string& host) { host_ = host; }

  int port() const { return port_; }
  void set_port(int port) { port_ = port; }

  std::string path() const { return path_; }
  void set_path(const std::string& path) { path_ = path; }

 private:
  bool encrypted_;
  std::string scheme_;
  std::string host_;
  int port_;
  std::string path_;
};

#endif  // CHROMEOS_PRINTING_URI_COMPONENTS_H_
