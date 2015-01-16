// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_PLUGIN_INSTANCE_THROTTLER_H_
#define CONTENT_PUBLIC_RENDERER_PLUGIN_INSTANCE_THROTTLER_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/content_export.h"

class GURL;

namespace content {

class RenderFrame;

// Plugin instances are "peripheral" if they are heuristically determined to be
// not "essential" to the web page content. See comments on
// RenderFrame::ShouldThrottleContent for details on these heuristics.
enum PluginPowerSaverMode {
  // Plugin content is main content, and therefore never throttled.
  POWER_SAVER_MODE_ESSENTIAL = 0,
  // Plugin content is peripheral, but throttling is disabled.
  POWER_SAVER_MODE_PERIPHERAL_UNTHROTTLED = 1,
  // Plugin content is peripheral, and throttling is enabled.
  POWER_SAVER_MODE_PERIPHERAL_THROTTLED = 2
};

// This class manages the metric collection, throttling, and unthrottling of a
// single peripheral plugin instance. If the Power Saver feature is disabled,
// the plugin instance will never actually be throttled, but still collects
// user interaction metrics.
//
// The process for throttling a plugin is as follows:
// 1) Attempt to find a representative keyframe to display as a placeholder for
//    the plugin.
// 2) a) If a representative keyframe is found, throttle the plugin at that
//       keyframe.
//    b) If a representative keyframe is not found, throttle the plugin after a
//       certain period of time.
//
// The plugin will then be unthrottled by receiving a mouse click from the user.
//
// To choose a representative keyframe, we first wait for a certain number of
// "interesting" frames to be displayed by the plugin. A frame is called
// interesting if it meets some heuristic. After we have seen a certain number
// of interesting frames, we throttle the plugin and use that frame as the
// representative keyframe.
class CONTENT_EXPORT PluginInstanceThrottler {
 public:
  // How the throttled power saver is unthrottled, if ever.
  // These numeric values are used in UMA logs; do not change them.
  enum PowerSaverUnthrottleMethod {
    UNTHROTTLE_METHOD_NEVER = 0,
    UNTHROTTLE_METHOD_BY_CLICK = 1,
    UNTHROTTLE_METHOD_BY_WHITELIST = 2,
    UNTHROTTLE_METHOD_BY_AUDIO = 3,
    UNTHROTTLE_METHOD_NUM_ITEMS
  };

  class Observer {
   public:
    virtual void OnThrottleStateChange() = 0;
  };

  // Returns a nullptr if no throttler needed based on |power_saver_mode|.
  static scoped_ptr<PluginInstanceThrottler> Get(
      RenderFrame* frame,
      const GURL& plugin_url,
      PluginPowerSaverMode power_saver_mode);

  virtual ~PluginInstanceThrottler() {}

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  virtual bool IsThrottled() const = 0;

  // Marks the plugin as essential. Unthrottles the plugin if already throttled.
  virtual void MarkPluginEssential(PowerSaverUnthrottleMethod method) = 0;

 protected:
  PluginInstanceThrottler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(PluginInstanceThrottler);
};
}

#endif  // CONTENT_PUBLIC_RENDERER_PLUGIN_INSTANCE_THROTTLER_H_
