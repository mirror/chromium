// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/wired_display/wired_display_presentation_receiver_factory.h"

#include <utility>

#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/media/router/providers/wired_display/presentation_receiver_window_original_profile.h"
#include "chrome/browser/media/router/providers/wired_display/presentation_receiver_window_otr_profile.h"
#include "chrome/browser/ui/media_router/presentation_receiver_window_controller.h"

namespace media_router {

namespace {

base::LazyInstance<WiredDisplayPresentationReceiverFactory>::Leaky factory =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
std::unique_ptr<WiredDisplayPresentationReceiver>
WiredDisplayPresentationReceiverFactory::CreateOTR(
    Profile* profile,
    const gfx::Rect& bounds,
    base::OnceClosure termination_callback,
    base::RepeatingCallback<void(const std::string&)> title_change_callback) {
  return std::make_unique<PresentationReceiverWindowOTRProfile>(
      profile, bounds, std::move(termination_callback),
      std::move(title_change_callback));
}
// static
std::unique_ptr<WiredDisplayPresentationReceiver>
WiredDisplayPresentationReceiverFactory::CreateWithOpener(
    Profile* profile,
    const gfx::Rect& bounds,
    const std::pair<int, int>& opener_rf_id,
    base::OnceClosure termination_callback,
    base::RepeatingCallback<void(const std::string&)> title_change_callback) {
  return std::make_unique<PresentationReceiverWindowOriginalProfile>(
      profile, bounds, opener_rf_id, std::move(termination_callback),
      std::move(title_change_callback));
}

// static
std::unique_ptr<WiredDisplayPresentationReceiver>
WiredDisplayPresentationReceiverFactory::CreateWithWebContents(
    content::WebContents* web_contents,
    const gfx::Rect& bounds,
    base::OnceClosure termination_callback,
    base::RepeatingCallback<void(const std::string&)> title_change_callback) {
  CHECK(PresentationReceiverWindowEnabled());
  return PresentationReceiverWindowController::CreateFromOriginalProfile(
      web_contents, bounds, std::move(termination_callback),
      std::move(title_change_callback));
}

// static
void WiredDisplayPresentationReceiverFactory::SetCreateReceiverCallbackForTest(
    WiredDisplayPresentationReceiverFactory::CreateReceiverCallback callback) {
  GetInstance()->create_receiver_for_testing_ = std::move(callback);
}

WiredDisplayPresentationReceiverFactory::
    WiredDisplayPresentationReceiverFactory() = default;

WiredDisplayPresentationReceiverFactory::
    ~WiredDisplayPresentationReceiverFactory() = default;

// static
WiredDisplayPresentationReceiverFactory*
WiredDisplayPresentationReceiverFactory::GetInstance() {
  return &factory.Get();
}

}  // namespace media_router
