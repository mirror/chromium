// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_LISTENER_H_
#define COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_LISTENER_H_

#include "base/values.h"

namespace ntp_snippets {

class BreakingNewsListener {
 public:
  using OnNewContentCallback =
      base::Callback<void(std::unique_ptr<base::Value> content)>;

  // Subscribe to the GCM service if necessary and start listening for pushed
  // content suggestions. Must not be called if already listening.
  virtual void StartListening(OnNewContentCallback on_new_content_callback) = 0;

  // Remove the handler, and stop listening for incoming GCM messages. Any
  // further pushed content suggestions will be ignored. Must be called while
  // listening.
  virtual void StopListening() = 0;
};
}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_BREAKING_NEWS_BREAKING_NEWS_LISTENER_H_
