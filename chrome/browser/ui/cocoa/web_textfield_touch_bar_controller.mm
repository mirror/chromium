// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_textfield_touch_bar_controller.h"

#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_popup_view_cocoa.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#include "content/browser/renderer_host/render_widget_host_view_mac.h"
#include "content/public/browser/focused_node_details.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#import "ui/base/cocoa/touch_bar_util.h"

class WebTextfieldTouchBarBridgeObserver
    : public content::NotificationObserver {
 public:
  WebTextfieldTouchBarBridgeObserver(WebTextfieldTouchBarController* owner,
                                     content::WebContents* contents) {
    owner_ = owner;
    contents_ = contents;
    registrar_.Add(this, content::NOTIFICATION_FOCUS_CHANGED_IN_PAGE,
                   content::NotificationService::AllSources());
  }

  void set_contents(content::WebContents* contents) { contents_ = contents; }

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    DCHECK_EQ(type, content::NOTIFICATION_FOCUS_CHANGED_IN_PAGE);
    content::FocusedNodeDetails* node_details =
        content::Details<content::FocusedNodeDetails>(details).ptr();
    if (node_details->is_editable_node) {
    }
  }

 private:
  WebTextfieldTouchBarController* owner_;
  content::WebContents* contents_;
  content::NotificationRegistrar registrar_;
};

@implementation WebTextfieldTouchBarController

- (instancetype)initWithTabContentsController:(TabContentsController*)owner {
  if ((self = [self init])) {
    owner_ = owner;
    observer_.reset(new WebTextfieldTouchBarBridgeObserver(self, nullptr));
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)changeWebContents:(content::WebContents*)contents {
  contents_ = contents;

  observer_->set_contents(contents);
}

- (void)showCreditCardAutofillForPopupView:(AutofillPopupViewCocoa*)popupView {
  DCHECK(popupView);
  DCHECK([popupView window]);

  window_ = [popupView window];

  // Remove any existing notifications before registering for new ones.
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center removeObserver:self name:NSWindowWillCloseNotification object:nil];

  [center addObserver:self
             selector:@selector(popupWindowWillClose:)
                 name:NSWindowWillCloseNotification
               object:window_];
  popupView_ = popupView;

  if ([owner_ respondsToSelector:@selector(setTouchBar:)])
    [owner_ performSelector:@selector(setTouchBar:) withObject:nil];
}

- (void)popupWindowWillClose:(NSNotification*)notif {
  popupView_ = nil;

  if ([owner_ respondsToSelector:@selector(setTouchBar:)])
    [owner_ performSelector:@selector(setTouchBar:) withObject:nil];

  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:NSWindowWillCloseNotification
              object:window_];
}

- (NSTouchBar*)makeTouchBar {
  if (popupView_)
    return [popupView_ makeTouchBar];

  if (!contents_) {
    return nil;
  }

  return nil;
}

@end
