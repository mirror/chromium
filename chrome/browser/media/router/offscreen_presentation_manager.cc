// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/offscreen_presentation_manager.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace media_router {

// OffscreenPresentationManager implementation.
OffscreenPresentationManager::OffscreenPresentationManager() {}

OffscreenPresentationManager::~OffscreenPresentationManager() {}

OffscreenPresentationManager::OffscreenPresentation*
OffscreenPresentationManager::GetOrCreateOffscreenPresentation(
    const std::string& presentation_id,
    const GURL& presentation_url) {
  auto it = offscreen_presentations_.find(presentation_id);
  if (it == offscreen_presentations_.end()) {
    it = offscreen_presentations_
             .insert(std::make_pair(presentation_id,
                                    base::MakeUnique<OffscreenPresentation>(
                                        presentation_id, presentation_url)))
             .first;
  }
  return it->second.get();
}

void OffscreenPresentationManager::RegisterOffscreenPresentationController(
    const std::string& presentation_id,
    const GURL& presentation_url,
    const RenderFrameHostId& render_frame_host_id,
    content::PresentationConnectionPtr controller) {
  DVLOG(2) << __FUNCTION__ << " [presentation_id]: " << presentation_id
           << ", [render_frame_host_id]: " << render_frame_host_id.second;
  DCHECK(thread_checker_.CalledOnValidThread());

  auto* presentation =
      GetOrCreateOffscreenPresentation(presentation_id, presentation_url);
  presentation->RegisterController(render_frame_host_id, std::move(controller));
}

void OffscreenPresentationManager::UnregisterOffscreenPresentationController(
    const std::string& presentation_id,
    const RenderFrameHostId& render_frame_host_id) {
  DVLOG(2) << __FUNCTION__ << " [presentation_id]: " << presentation_id
           << ", [render_frame_host_id]: " << render_frame_host_id.second;
  DCHECK(thread_checker_.CalledOnValidThread());

  auto it = offscreen_presentations_.find(presentation_id);
  if (it == offscreen_presentations_.end())
    return;

  // Remove presentation if no controller and receiver.
  it->second->UnregisterController(render_frame_host_id);
  if (!it->second->IsValid()) {
    DLOG(WARNING) << __func__ << "no receiver callback has been registered to "
                  << "[presentation_id]: " << presentation_id;
    offscreen_presentations_.erase(presentation_id);
  }
}

void OffscreenPresentationManager::OnOffscreenPresentationReceiverCreated(
    const std::string& presentation_id,
    const GURL& presentation_url,
    const content::ReceiverConnectionAvailableCallback& receiver_callback) {
  DVLOG(2) << __FUNCTION__ << " [presentation_id]: " << presentation_id;
  DCHECK(thread_checker_.CalledOnValidThread());
  auto* presentation =
      GetOrCreateOffscreenPresentation(presentation_id, presentation_url);
  presentation->RegisterReceiver(receiver_callback);
}

void OffscreenPresentationManager::OnOffscreenPresentationReceiverTerminated(
    const std::string& presentation_id) {
  DVLOG(2) << __FUNCTION__ << " [presentation_id]: " << presentation_id;
  DCHECK(thread_checker_.CalledOnValidThread());

  offscreen_presentations_.erase(presentation_id);
}

// OffscreenPresentation implementation.
OffscreenPresentationManager::OffscreenPresentation::OffscreenPresentation(
    const std::string& presentation_id,
    const GURL& presentation_url)
    : presentation_id_(presentation_id), presentation_url_(presentation_url) {}

OffscreenPresentationManager::OffscreenPresentation::~OffscreenPresentation() {}

void OffscreenPresentationManager::OffscreenPresentation::RegisterController(
    const RenderFrameHostId& render_frame_host_id,
    content::PresentationConnectionPtr controller) {
  if (!receiver_callback_.is_null()) {
    receiver_callback_.Run(
        content::PresentationSessionInfo(presentation_url_, presentation_id_),
        std::move(controller));
  } else {
    pending_controllers_.insert(
        std::make_pair(render_frame_host_id, std::move(controller)));
  }
}

void OffscreenPresentationManager::OffscreenPresentation::UnregisterController(
    const RenderFrameHostId& render_frame_host_id) {
  pending_controllers_.erase(render_frame_host_id);
}

void OffscreenPresentationManager::OffscreenPresentation::RegisterReceiver(
    const content::ReceiverConnectionAvailableCallback& receiver_callback) {
  DCHECK(receiver_callback_.is_null());

  for (auto& controller : pending_controllers_) {
    receiver_callback.Run(
        content::PresentationSessionInfo(presentation_url_, presentation_id_),
        std::move(controller.second));
  }
  receiver_callback_ = receiver_callback;
  pending_controllers_.clear();
}

bool OffscreenPresentationManager::OffscreenPresentation::IsValid() const {
  return !(pending_controllers_.empty() && receiver_callback_.is_null());
}

}  // namespace media_router
