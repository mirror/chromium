// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_CONSOLE_MESSAGER_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_CONSOLE_MESSAGER_H_

class GURL;

namespace content {
class RenderFrameHost;
}  // namespace content

namespace subresource_filter {

// This class can be used to send console messages at specific events in the
// browser process related to subresource filtering.
class ConsoleMessager {
 public:
  virtual ~ConsoleMessager() {}

  // Called unconditionally on commit.
  virtual void LogOnCommit(bool successfully_activated,
                           content::RenderFrameHost* rfh) {}

  // Called when a subframe navigation is disallowed.
  virtual void LogSubframeDisallowed(const GURL& subframe_url,
                                     content::RenderFrameHost* rfh) {}

  // Whether or not disallowed subresources should have associated logging.
  // Note, currently this is only supported for better ads style logging. If you
  // want to log non better ads messages, ActivationState needs to be augmented
  // with an ActivationList enum, so messages can log conditionally based on
  // list.
  virtual bool ShouldLogSubresources();
};

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CONTENT_BROWSER_CONSOLE_MESSAGER_H_
