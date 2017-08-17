// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_web_contents_manager.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/web_contents.h"

namespace chromecast {

CastWebContentsManager::CastWebContentsManager(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      task_runner_(base::SequencedTaskRunnerHandle::Get()),
      weak_factory_(this) {
  DCHECK(browser_context_);
  DCHECK(task_runner_);
}

CastWebContentsManager::~CastWebContentsManager() = default;

std::unique_ptr<CastWebView> CastWebContentsManager::CreateWebView(
    CastWebView::Delegate* delegate,
    scoped_refptr<content::SiteInstance> site_instance,
    bool transparent) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return base::MakeUnique<CastWebView>(delegate, this, browser_context_,
                                       site_instance, transparent);
}

void CastWebContentsManager::DelayWebContentsDeletion(
    std::unique_ptr<content::WebContents> web_contents,
    const base::TimeDelta& time_delta) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto* web_contents_ptr = web_contents.get();
  // Suspend the MediaSession to free up media resources for the next content
  // window.
  content::MediaSession::Get(web_contents_ptr)
      ->Suspend(content::MediaSession::SuspendType::SYSTEM);
  expiring_web_contents_.push_back(std::move(web_contents));
  LOG(INFO) << "WebContents will be deleted in " << time_delta.InMilliseconds()
            << " milliseconds.";
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CastWebContentsManager::DeleteWebContents,
                     weak_factory_.GetWeakPtr(), web_contents_ptr),
      time_delta);
}

void CastWebContentsManager::DeleteWebContents(
    content::WebContents* web_contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(web_contents);
  for (auto it = expiring_web_contents_.begin();
       it != expiring_web_contents_.end();) {
    if (it->get() == web_contents) {
      expiring_web_contents_.erase(it);
      return;
    } else {
      ++it;
    }
  }
}

}  // namespace chromecast
