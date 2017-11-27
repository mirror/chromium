// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/seat.h"

#include "ash/shell.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/atomic_flag.h"
#include "components/exo/data_source.h"
#include "components/exo/keyboard.h"
#include "components/exo/shell_surface.h"
#include "components/exo/surface.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/client/focus_client.h"
#include "ui/base/clipboard/clipboard_monitor.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace exo {

namespace {

Surface* GetEffectiveFocus(aura::Window* window) {
  if (!window)
    return nullptr;
  Surface* const surface = Surface::AsSurface(window);
  if (surface)
    return surface;
  // Fallback to main surface.
  aura::Window* const top_level_window = window->GetToplevelWindow();
  if (!top_level_window)
    return nullptr;
  return ShellSurface::GetMainSurface(top_level_window);
}

uint64_t GetClipboardSequence() {
  return ui::Clipboard::GetForCurrentThread()->GetSequenceNumber(
      ui::CLIPBOARD_TYPE_COPY_PASTE);
}

}  // namespace

Seat::Seat() : weak_ptr_factory_(this) {
  aura::client::GetFocusClient(ash::Shell::Get()->GetPrimaryRootWindow())
      ->AddObserver(this);
  ui::ClipboardMonitor::GetInstance()->AddObserver(this);
}

Seat::~Seat() {
  aura::client::GetFocusClient(ash::Shell::Get()->GetPrimaryRootWindow())
      ->RemoveObserver(this);
  ui::ClipboardMonitor::GetInstance()->RemoveObserver(this);
}

void Seat::AddObserver(SeatObserver* observer) {
  observers_.AddObserver(observer);
}

void Seat::RemoveObserver(SeatObserver* observer) {
  observers_.RemoveObserver(observer);
}

Surface* Seat::GetFocusedSurface() {
  return GetEffectiveFocus(WMHelper::GetInstance()->GetFocusedWindow());
}

void Seat::SetSelection(DataSource* source) {
  if (selection_source_ && selection_source_->get() == source)
    return;

  source->ReadData(base::Bind(&Seat::OnSetSelectionComplete,
                              weak_ptr_factory_.GetWeakPtr(),
                              GetClipboardSequence()));
}

void Seat::OnSetSelectionComplete(uint64_t clipboard_seq,
                                  const std::vector<uint8_t>& data,
                                  DataSource* source) {
  // Clipboard has already been updated while reading data.
  if (clipboard_seq != GetClipboardSequence()) {
    if (source)
      source->Cancelled();
    return;
  }

  // Update Clipboard data
  {
    ui::ScopedClipboardWriter writer(ui::CLIPBOARD_TYPE_COPY_PASTE);
    writer.WriteText(base::UTF8ToUTF16(base::StringPiece(
        reinterpret_cast<const char*>(data.data()), data.size())));
  }

  // After updating clipboard, selection_source_ should be nullptr.
  DCHECK(!selection_source_);
  if (source)
    selection_source_ = std::make_unique<ScopedDataSource>(source, this);
}

void Seat::OnWindowFocused(aura::Window* gained_focus,
                           aura::Window* lost_focus) {
  Surface* const surface = GetEffectiveFocus(gained_focus);
  for (auto& observer : observers_) {
    observer.OnSurfaceFocusing(surface);
  }
  for (auto& observer : observers_) {
    observer.OnSurfaceFocused(surface);
  }
}

void Seat::OnClipboardDataChanged() {
  if (!selection_source_)
    return;
  selection_source_->get()->Cancelled();
  selection_source_.reset();
}

void Seat::OnDataSourceDestroying(DataSource* source) {
  if (selection_source_ && selection_source_->get() == source)
    selection_source_.reset();
}

}  // namespace exo
