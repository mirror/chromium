// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/content_browser_test_utils.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#include "base/lazy_instance.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_objc_class_swizzler.h"
#include "content/browser/renderer_host/render_widget_host_view_mac.h"
#include "content/browser/renderer_host/text_input_client_mac.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/events/test/cocoa_test_event_utils.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/range/range.h"

// The interface class used to override the implementation of some of
// RenderWidgetHostViewCocoa methods for tests.
@interface RenderWidgetHostViewCocoaSwizzler : NSObject
- (void)didAddSubview:(NSView*)view;
@end

namespace {
// List of selector strings for methods overridden.
const char* kSelectorDidAddSubview = "didAddSubview:";
}

namespace content {
namespace {
using base::mac::ScopedObjCClassSwizzler;

// A scoped swizzler class which will replace the implementation of methods
// between two classes for a
class ScopedSwizzlerWrapper {
 public:
  ScopedSwizzlerWrapper(Class objc_original_class,
                        Class swizzle_helper_class,
                        const char* method_name)
      : method_name_(method_name),
        internal_scoped_swizzler_(new ScopedObjCClassSwizzler(
            objc_original_class,
            swizzle_helper_class,
            NSSelectorFromString(
                [NSString stringWithUTF8String:method_name]))) {}

  ~ScopedSwizzlerWrapper() {}

  const std::string& method_name() const { return method_name_; }

  // The original implementation of the method.
  IMP GetOriginalImplementation() {
    return internal_scoped_swizzler_->GetOriginalImplementation();
  }

 private:
  const std::string method_name_;
  std::unique_ptr<ScopedObjCClassSwizzler> internal_scoped_swizzler_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSwizzlerWrapper);
};

base::LazyInstance<std::set<std::unique_ptr<ScopedSwizzlerWrapper>>>::
    DestructorAtExit g_swizzlers = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<
    std::map<WebContents*, std::set<RenderWidgetHostViewCocoaObserver*>>>::
    DestructorAtExit g_observer_map = LAZY_INSTANCE_INITIALIZER;

std::set<RenderWidgetHostViewCocoaObserver*>* GetObserverSet(
    WebContents* web_contents) {
  if (!g_observer_map.Get().count(web_contents))
    return nullptr;
  return &g_observer_map.Get()[web_contents];
}

void SetUpMethodSwizzlers() {
  if (!g_swizzlers.Get().empty())
    return;

  // [RenderWidgetHostViewCocoa didAddSubview:NSView*].
  g_swizzlers.Get().insert(base::MakeUnique<ScopedSwizzlerWrapper>(
      GetRenderWidgetHostViewCocoaClassForTesting(),
      [RenderWidgetHostViewCocoaSwizzler class], kSelectorDidAddSubview));
}

content::RenderWidgetHostViewMac* GetRenderWidgetHostViewMac(NSObject* object) {
  for (auto& pair : g_observer_map.Get()) {
    RenderWidgetHostViewMac* rwhv_mac = static_cast<RenderWidgetHostViewMac*>(
        pair.first->GetRenderWidgetHostView());
    if (rwhv_mac->cocoa_view() == object)
      return rwhv_mac;
  }
  return nullptr;
}

IMP GetOriginalImplementation(const std::string& method_name) {
  for (auto& swizzler : g_swizzlers.Get()) {
    if (swizzler->method_name() == method_name)
      return swizzler->GetOriginalImplementation();
  }
  return nullptr;
}

}  // namespace

RenderWidgetHostViewCocoaObserver::RenderWidgetHostViewCocoaObserver(
    WebContents* web_contents)
    : web_contents_(web_contents) {
  if (g_swizzlers.Get().empty())
    SetUpMethodSwizzlers();

  g_observer_map.Get()[web_contents].insert(this);
}

RenderWidgetHostViewCocoaObserver::~RenderWidgetHostViewCocoaObserver() {
  g_observer_map.Get()[web_contents_].erase(this);

  if (g_observer_map.Get()[web_contents_].empty())
    g_observer_map.Get().erase(web_contents_);

  if (g_observer_map.Get().empty())
    g_swizzlers.Get().clear();
}

void SetWindowBounds(gfx::NativeWindow window, const gfx::Rect& bounds) {
  NSRect new_bounds = NSRectFromCGRect(bounds.ToCGRect());
  if ([[NSScreen screens] count] > 0) {
    new_bounds.origin.y =
        [[[NSScreen screens] firstObject] frame].size.height -
        new_bounds.origin.y - new_bounds.size.height;
  }

  [window setFrame:new_bounds display:NO];
}

void GetStringAtPointForRenderWidget(
    RenderWidgetHost* rwh,
    const gfx::Point& point,
    base::Callback<void(const std::string&, const gfx::Point&)>
        result_callback) {
  TextInputClientMac::GetInstance()->GetStringAtPoint(
      rwh, point, ^(NSAttributedString* string, NSPoint baselinePoint) {
        result_callback.Run([[string string] UTF8String],
                            gfx::Point(baselinePoint.x, baselinePoint.y));
      });
}

void GetStringFromRangeForRenderWidget(
    RenderWidgetHost* rwh,
    const gfx::Range& range,
    base::Callback<void(const std::string&, const gfx::Point&)>
        result_callback) {
  TextInputClientMac::GetInstance()->GetStringFromRange(
      rwh, range.ToNSRange(),
      ^(NSAttributedString* string, NSPoint baselinePoint) {
        result_callback.Run([[string string] UTF8String],
                            gfx::Point(baselinePoint.x, baselinePoint.y));
      });
}

}  // namespace content

@implementation RenderWidgetHostViewCocoaSwizzler
- (void)didAddSubview:(NSView*)view {
  content::GetOriginalImplementation(kSelectorDidAddSubview)(self, _cmd, view);

  content::RenderWidgetHostViewMac* rwhv_mac =
      content::GetRenderWidgetHostViewMac(self);

  std::set<content::RenderWidgetHostViewCocoaObserver*>* observers =
      content::GetObserverSet(rwhv_mac->GetWebContents());

  if (!observers || observers->empty())
    return;

  NSRect bounds_in_cocoa_view =
      [view convertRect:view.bounds toView:rwhv_mac->cocoa_view()];

  gfx::Rect rect =
      [rwhv_mac->cocoa_view() flipNSRectToRect:bounds_in_cocoa_view];

  for (auto* observer : *observers)
    observer->DidAddSubviewWillBeDismissed(rect);

  // This override is useful for testing popups. To make sure the run loops end
  // after the call it is best to dismiss the popup soon.
  NSEvent* dismissal_event =
      cocoa_test_event_utils::MouseEventWithType(NSEventTypeLeftMouseDown, 0);
  [[NSApplication sharedApplication] postEvent:dismissal_event atStart:false];
}
@end
