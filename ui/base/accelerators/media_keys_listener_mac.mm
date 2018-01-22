// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/accelerators/media_keys_listener.h"

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hidsystem/ev_keymap.h>

#include "ui/base/accelerators/accelerator.h"
#include "ui/base/ui_base_export.h"

namespace ui {

namespace {

const int kSystemDefinedEventMediaKeysSubtype = 8;

class MediaKeysListenerImpl : public MediaKeysListener {
 public:
  MediaKeysListenerImpl(MediaKeysListener::MediaKeysAcceleratorReceiver
                            media_keys_accelerator_receiver,
                        Scope scope);

  ~MediaKeysListenerImpl() override;

  // Implement MediaKeysListener
  void StartWatchingMediaKeys() override;
  void StopWatchingMediaKeys() override;
  bool IsWatchingMediaKeys() const override;

 private:
  ui::KeyboardCode MediaKeyCodeToKeyboardCode(int key_code);

  bool OnMediaKeyEvent(int media_key_code);

  // The callback for when an event tap happens.
  static CGEventRef EventTapCallback(CGEventTapProxy proxy,
                                     CGEventType type,
                                     CGEventRef event,
                                     void* refcon);

  MediaKeysAcceleratorReceiver const media_keys_accelerator_receiver_;
  const Scope scope_;
  // Event tap for intercepting mac media keys.
  CFMachPortRef event_tap_ = NULL;
  CFRunLoopSourceRef event_tap_source_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(MediaKeysListenerImpl);
};

MediaKeysListenerImpl::MediaKeysListenerImpl(
    MediaKeysListener::MediaKeysAcceleratorReceiver
        media_keys_accelerator_receiver,
    Scope scope)
    : media_keys_accelerator_receiver_(media_keys_accelerator_receiver),
      scope_(scope) {}

MediaKeysListenerImpl::~MediaKeysListenerImpl() {
  if (event_tap_) {
    StopWatchingMediaKeys();
  }
}

void MediaKeysListenerImpl::StartWatchingMediaKeys() {
  // Make sure there's no existing event tap.
  DCHECK(event_tap_ == NULL);
  DCHECK(event_tap_source_ == NULL);

  // Add an event tap to intercept the system defined media key events.
  event_tap_ = CGEventTapCreate(
      kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault,
      CGEventMaskBit(NX_SYSDEFINED), EventTapCallback, this);
  if (event_tap_ == NULL) {
    LOG(ERROR) << "Error: failed to create event tap.";
    return;
  }

  event_tap_source_ =
      CFMachPortCreateRunLoopSource(kCFAllocatorSystemDefault, event_tap_, 0);
  if (event_tap_source_ == NULL) {
    LOG(ERROR) << "Error: failed to create new run loop source.";
    return;
  }

  CFRunLoopAddSource(CFRunLoopGetCurrent(), event_tap_source_,
                     kCFRunLoopCommonModes);
}

void MediaKeysListenerImpl::StopWatchingMediaKeys() {
  CFRunLoopRemoveSource(CFRunLoopGetCurrent(), event_tap_source_,
                        kCFRunLoopCommonModes);
  // Ensure both event tap and source are initialized.
  DCHECK(event_tap_ != NULL);
  DCHECK(event_tap_source_ != NULL);

  // Invalidate the event tap.
  CFMachPortInvalidate(event_tap_);
  CFRelease(event_tap_);
  event_tap_ = NULL;

  // Release the event tap source.
  CFRelease(event_tap_source_);
  event_tap_source_ = NULL;
}

bool MediaKeysListenerImpl::IsWatchingMediaKeys() const {
  return event_tap_ != NULL;
}

ui::KeyboardCode MediaKeysListenerImpl::MediaKeyCodeToKeyboardCode(
    int key_code) {
  switch (key_code) {
    case NX_KEYTYPE_PLAY:
      return ui::VKEY_MEDIA_PLAY_PAUSE;
    case NX_KEYTYPE_PREVIOUS:
    case NX_KEYTYPE_REWIND:
      return ui::VKEY_MEDIA_PREV_TRACK;
    case NX_KEYTYPE_NEXT:
    case NX_KEYTYPE_FAST:
      return ui::VKEY_MEDIA_NEXT_TRACK;
  }
  return ui::VKEY_UNKNOWN;
}

bool MediaKeysListenerImpl::OnMediaKeyEvent(int media_key_code) {
  ui::KeyboardCode key_code = MediaKeyCodeToKeyboardCode(media_key_code);
  // Create an accelerator corresponding to the keyCode.
  ui::Accelerator accelerator(key_code, 0);
  bool was_handled = false;
  media_keys_accelerator_receiver_.Run(accelerator, &was_handled);
  return was_handled;
}

// Processed events should propagate if they aren't handled by any listeners.
// For events that don't matter, this handler should return as quickly as
// possible.
// Returning event causes the event to propagate to other applications.
// Returning NULL prevents the event from propagating.
// static
CGEventRef MediaKeysListenerImpl::EventTapCallback(CGEventTapProxy proxy,
                                                   CGEventType type,
                                                   CGEventRef event,
                                                   void* refcon) {
  MediaKeysListenerImpl* shortcut_listener =
      static_cast<MediaKeysListenerImpl*>(refcon);

  const bool is_active = [NSApp isActive];

  if (shortcut_listener->scope_ == Scope::kChrome && !is_active) {
    return event;
  }

  // Handle the timeout case by re-enabling the tap.
  if (type == kCGEventTapDisabledByTimeout) {
    CGEventTapEnable(shortcut_listener->event_tap_, TRUE);
    return event;
  }

  // Convert the CGEvent to an NSEvent for access to the data1 field.
  NSEvent* ns_event = [NSEvent eventWithCGEvent:event];
  if (ns_event == nil) {
    return event;
  }

  // Ignore events that are not system defined media keys.
  if (type != NX_SYSDEFINED || [ns_event type] != NSSystemDefined ||
      [ns_event subtype] != kSystemDefinedEventMediaKeysSubtype) {
    return event;
  }

  NSInteger data1 = [ns_event data1];
  // Ignore media keys that aren't previous, next and play/pause.
  // Magical constants are from http://weblog.rogueamoeba.com/2007/09/29/
  int key_code = (data1 & 0xFFFF0000) >> 16;
  if (key_code != NX_KEYTYPE_PLAY && key_code != NX_KEYTYPE_NEXT &&
      key_code != NX_KEYTYPE_PREVIOUS && key_code != NX_KEYTYPE_FAST &&
      key_code != NX_KEYTYPE_REWIND) {
    return event;
  }

  int key_flags = data1 & 0x0000FFFF;
  bool is_key_pressed = ((key_flags & 0xFF00) >> 8) == 0xA;

  // If the key wasn't pressed (eg. was released), ignore this event.
  if (!is_key_pressed)
    return event;

  // Now we have a media key that we care about. Send it to the caller.
  bool was_handled = shortcut_listener->OnMediaKeyEvent(key_code);

  // Prevent event from proagating to other apps if handled by Chrome.
  if (was_handled) {
    return NULL;
  }

  // By default, pass the event through.
  return event;
}

}  // namespace

std::unique_ptr<MediaKeysListener> CreateMediaKeysListener(
    MediaKeysListener::MediaKeysAcceleratorReceiver
        media_keys_accelerator_receiver,
    MediaKeysListener::Scope scope) {
  return std::make_unique<MediaKeysListenerImpl>(
      media_keys_accelerator_receiver, scope);
}

}  // namespace ui
