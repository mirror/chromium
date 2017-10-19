// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_target_registry.h"

#include "base/task_runner.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

namespace {

std::unique_ptr<const DevToolsTargetRegistry::TargetInfo> BuildTargetInfo(
    RenderFrameHost* rfh) {
  std::unique_ptr<DevToolsTargetRegistry::TargetInfo> info(
      new DevToolsTargetRegistry::TargetInfo());
  info->child_id = rfh->GetProcess()->GetID();
  info->routing_id = rfh->GetRoutingID();

  const FrameTreeNode* ftn =
      static_cast<RenderFrameHostImpl*>(rfh)->frame_tree_node();
  info->devtools_token = ftn->devtools_frame_token();
  info->frame_tree_node_id = ftn->frame_tree_node_id();

  // Traverse up to the nearest local root. Watch out for current (leaf)
  // frame host being null when it's being destroyed.
  while (!ftn->IsMainFrame() && !rfh->IsCrossProcessSubframe()) {
    ftn = ftn->parent();
    rfh = ftn->current_frame_host();
  }
  info->devtools_host_id = ftn->devtools_frame_token();

  return std::move(info);
}

}  // namespace

class DevToolsTargetRegistry::ContentsObserver : public WebContentsObserver {
 public:
  ContentsObserver(WebContents* web_contents,
                   scoped_refptr<base::TaskRunner> task_runner)
      : WebContentsObserver(web_contents), task_runner_(task_runner) {}

 private:
  void RenderFrameHostChanged(RenderFrameHost* old_host,
                              RenderFrameHost* new_host) override {
    std::unique_ptr<const TargetInfo> old_target;
    if (old_host)
      old_target = BuildTargetInfo(old_host);
    std::unique_ptr<const TargetInfo> new_target = BuildTargetInfo(new_host);
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(DevToolsTargetRegistry::Update, std::move(old_target),
                       std::move(new_target)));
  }

  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(DevToolsTargetRegistry::Update, nullptr,
                                  BuildTargetInfo(render_frame_host)));
  }

  void WebContentsDestroyed() override {
    Observe(nullptr);
    delete this;
  }

  scoped_refptr<base::TaskRunner> task_runner_;
};

// static
void DevToolsTargetRegistry::AddWebContents(WebContents* web_contents) {
  scoped_refptr<base::TaskRunner> task_runner =
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
  // The observer will die along with the web contents.
  new DevToolsTargetRegistry::ContentsObserver(web_contents, task_runner);
  std::vector<std::unique_ptr<const TargetInfo>> infos;
  for (RenderFrameHost* render_frame_host : web_contents->GetAllFrames())
    infos.push_back(BuildTargetInfo(render_frame_host));

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&DevToolsTargetRegistry::AddAll, std::move(infos)));
}

// static
const DevToolsTargetRegistry::TargetInfo*
DevToolsTargetRegistry::GetInfoByFrameTreeNodeId(int frame_node_id) {
  DevToolsTargetRegistry* self = GetInstance();
  auto it = self->target_info_by_ftn_id_.find(frame_node_id);
  return it != self->target_info_by_ftn_id_.end() ? it->second : nullptr;
}

// static
const DevToolsTargetRegistry::TargetInfo*
DevToolsTargetRegistry::GetInfoByRenderFramePair(int child_id, int routing_id) {
  DevToolsTargetRegistry* self = GetInstance();
  auto it = self->target_info_by_render_frame_pair_.find(
      std::make_pair(child_id, routing_id));
  return it != self->target_info_by_render_frame_pair_.end() ? it->second.get()
                                                             : nullptr;
}

// static
void DevToolsTargetRegistry::Update(
    std::unique_ptr<const TargetInfo> old_info,
    std::unique_ptr<const TargetInfo> new_info) {
  if (old_info)
    GetInstance()->Remove(*old_info);
  GetInstance()->Add(std::move(new_info));
}

// static
void DevToolsTargetRegistry::AddAll(
    std::vector<std::unique_ptr<const TargetInfo>> infos) {
  DevToolsTargetRegistry* self = GetInstance();
  for (auto& info : infos)
    self->Add(std::move(info));
}

void DevToolsTargetRegistry::Add(std::unique_ptr<const TargetInfo> info) {
  if (info->frame_tree_node_id != -1) {
    target_info_by_ftn_id_.insert(
        std::make_pair(info->frame_tree_node_id, info.get()));
  }
  auto key = std::make_pair(info->child_id, info->routing_id);
  target_info_by_render_frame_pair_.insert(
      std::make_pair(key, std::move(info)));
}

void DevToolsTargetRegistry::Remove(const TargetInfo& info) {
  if (info.frame_tree_node_id != -1)
    target_info_by_ftn_id_.erase(info.frame_tree_node_id);
  target_info_by_render_frame_pair_.erase(
      std::make_pair(info.child_id, info.routing_id));
}

DevToolsTargetRegistry* DevToolsTargetRegistry::GetInstance() {
  DevToolsTargetRegistry* instance =
      base::Singleton<DevToolsTargetRegistry>::get();
  DCHECK_CALLED_ON_VALID_THREAD(instance->thread_checker_);
  return instance;
}

DevToolsTargetRegistry::DevToolsTargetRegistry() = default;
DevToolsTargetRegistry::~DevToolsTargetRegistry() = default;

}  // namespace content
