// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/test_palette_delegate.h"

namespace ash {

TestPaletteDelegate::TestPaletteDelegate() {}

TestPaletteDelegate::~TestPaletteDelegate() {}

void TestPaletteDelegate::set_fake_has_seen_stylus_pref(bool has_seen_stylus) {
  fake_has_seen_stylus_pref_ = has_seen_stylus;
  seen_stylus_callback_list_.Notify(fake_has_seen_stylus_pref_);
}

std::unique_ptr<PaletteDelegate::EnableListenerSubscription>
TestPaletteDelegate::AddPaletteEnableListener(
    const EnableListener& on_state_changed) {
  return nullptr;
}

std::unique_ptr<PaletteDelegate::SeenStylusListenerSubscription>
TestPaletteDelegate::AddSeenStylusListener(
    const SeenStylusListener& on_state_changed) {
  auto subscription = seen_stylus_callback_list_.Add(on_state_changed);
  seen_stylus_callback_list_.Notify(fake_has_seen_stylus_pref_);
  return subscription;
}

void TestPaletteDelegate::CreateNote() {
  ++create_note_count_;
}

bool TestPaletteDelegate::HasNoteApp() {
  ++has_note_app_count_;
  return has_note_app_;
}

bool TestPaletteDelegate::ShouldAutoOpenPalette() {
  return should_auto_open_palette_;
}

bool TestPaletteDelegate::ShouldShowPalette() {
  return should_show_palette_;
}

void TestPaletteDelegate::TakeScreenshot() {
  ++take_screenshot_count_;
}

void TestPaletteDelegate::TakePartialScreenshot(const base::Closure& done) {
  ++take_partial_screenshot_count_;
  partial_screenshot_done_ = done;
}

void TestPaletteDelegate::CancelPartialScreenshot() {}

bool TestPaletteDelegate::IsMetalayerSupported() {
  return is_metalayer_supported_;
}

void TestPaletteDelegate::ShowMetalayer(const base::Closure& closed) {
  ++show_metalayer_count_;
  metalayer_closed_ = closed;
}

void TestPaletteDelegate::HideMetalayer() {
  ++hide_metalayer_count_;
}

void TestPaletteDelegate::SetHasSeenStylusEvent() {
  set_fake_has_seen_stylus_pref(true);
}

}  // namespace ash
