// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/views/event_monitor_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/events/event_handler.h"
#include "ui/events/event_utils.h"

// A ref-counted object to enclose a weak pointer to the event monitor. This is
// necessary because AppKit posts to all event monitors that were active when an
// event is received. This means that if *another* event monitor invokes the
// EventMonitorMac destructor, the monitor will still receive the event, even
// though the monitor was removed.
@interface EventMonitorMacHandle : NSObject {
 @public
  views::EventMonitorMac* weak_owner;
}
@end
@implementation EventMonitorMacHandle
@end

namespace views {

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateApplicationMonitor(
    ui::EventHandler* event_handler) {
  return base::WrapUnique(new EventMonitorMac(event_handler, nullptr));
}

// static
std::unique_ptr<EventMonitor> EventMonitor::CreateWindowMonitor(
    ui::EventHandler* event_handler,
    gfx::NativeWindow target_window) {
  return base::WrapUnique(new EventMonitorMac(event_handler, target_window));
}

// static
gfx::Point EventMonitor::GetLastMouseLocation() {
  return display::Screen::GetScreen()->GetCursorScreenPoint();
}

EventMonitorMac::EventMonitorMac(ui::EventHandler* event_handler,
                                 gfx::NativeWindow target_window) {
  DCHECK(event_handler);
  handle_.reset([[EventMonitorMacHandle alloc] init]);
  handle_.get()->weak_owner = this;
  EventMonitorMacHandle* handle = handle_;  // Ensure the object gets captured.

  monitor_ = [NSEvent
      addLocalMonitorForEventsMatchingMask:NSAnyEventMask
                                   handler:^NSEvent*(NSEvent* event) {
                                     if (!handle->weak_owner)
                                       return event;

                                     if (!target_window ||
                                         [event window] == target_window) {
                                       std::unique_ptr<ui::Event> ui_event =
                                           ui::EventFromNative(event);
                                       if (ui_event)
                                         event_handler->OnEvent(ui_event.get());
                                     }
                                     return event;
                                   }];
}

EventMonitorMac::~EventMonitorMac() {
  handle_.get()->weak_owner = nullptr;
  [NSEvent removeMonitor:monitor_];
}

}  // namespace views
