// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TARGET_REGISTRY_H_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TARGET_REGISTRY_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/threading/thread_checker.h"
#include "base/unguessable_token.h"

namespace content {

class WebContents;

class DevToolsTargetRegistry {
 public:
  struct TargetInfo {
    int child_id;
    int routing_id;
    int frame_tree_node_id;
    base::UnguessableToken devtools_token;
    base::UnguessableToken devtools_host_id;
  };

  // UI thread method
  static void AddWebContents(WebContents* web_contents);

  // IO thread methods
  static const TargetInfo* GetInfoByFrameTreeNodeId(int frame_node_id);
  static const TargetInfo* GetInfoByRenderFramePair(int child_id,
                                                    int routing_id);

 private:
  friend struct base::DefaultSingletonTraits<DevToolsTargetRegistry>;
  friend class base::Singleton<DevToolsTargetRegistry>;
  class ContentsObserver;

  static DevToolsTargetRegistry* GetInstance();
  static void AddAll(std::vector<std::unique_ptr<const TargetInfo>> infos);
  static void Update(std::unique_ptr<const TargetInfo> old_info,
                     std::unique_ptr<const TargetInfo> new_info);

  DevToolsTargetRegistry();
  ~DevToolsTargetRegistry();

  void Add(std::unique_ptr<const TargetInfo> info);
  void Remove(const TargetInfo& info);

  std::map<std::pair<int, int>, std::unique_ptr<const TargetInfo>>
      target_info_by_render_frame_pair_;
  std::map<int, const TargetInfo*> target_info_by_ftn_id_;
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(DevToolsTargetRegistry);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_TARGET_REGISTRY_H_
