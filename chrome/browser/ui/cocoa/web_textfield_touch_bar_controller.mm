// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_textfield_touch_bar_controller.h"

#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "chrome/app/vector_icons/vector_icons.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#include "chrome/common/chrome_features.h"
#include "components/autofill/content/browser/content_autofill_driver.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/grit/components_scaled_resources.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#import "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/vector_icons/vector_icons.h"

namespace {

NSString* const kAutofillTouchBarId = @"autofill";

// Touch bar items identifiers.
NSString* const kCreditCardTouchId = @"CREDIT-CARD-POPOVER";
NSString* const kCreditCardItemsTouchId = @"CREDIT-CARD-ITEMS";

}  // namespace

class WebTextfieldTouchBarBridge : content::WebContentsObserver {
 public:
  WebTextfieldTouchBarBridge(WebTextfieldTouchBarController* owner,
                             content::WebContents* contents)
      : WebContentsObserver(contents), owner_(owner) {
    DCHECK(owner_);
  }

  void ChangeWebContents(content::WebContents* contents) { Observe(contents); }

  void OnAutofillFormFocused() override {
    LOG(INFO) << "OnAutofillFormFocused";
    [owner_ updateFocusedNodeDetails:YES];
  }

  void DidEndAutofillTextFieldEditing() override {
    LOG(INFO) << "DidEndAutofillTextFieldEditing";
    [owner_ updateFocusedNodeDetails:NO];
  }

 private:
  WebTextfieldTouchBarController* owner_;  // weak.
};

@implementation WebTextfieldTouchBarController

- (instancetype)initWithTabContentsController:(TabContentsController*)owner
                                  webContents:(content::WebContents*)contents {
  if ((self = [self init])) {
    owner_ = owner;
    contents_ = contents;
    bridge_.reset(new WebTextfieldTouchBarBridge(self, contents));
  }

  return self;
}

- (void)changeWebContents:(content::WebContents*)contents {
  isEditable_ = NO;
  contents_ = contents;
  bridge_->ChangeWebContents(contents);
  LOG(INFO) << "ChangeWebContents";
}

- (void)updateFocusedNodeDetails:(BOOL)isEditable {
  LOG(INFO) << "updateFocusedNodeDetails";
  isEditable_ = isEditable;

  if ([owner_ respondsToSelector:@selector(setTouchBar:)])
    [owner_ performSelector:@selector(setTouchBar:) withObject:nil];
}

- (NSTouchBar*)makeTouchBar {
  if (!base::FeatureList::IsEnabled(features::kBrowserTouchBar))
    return nil;

  if (!isEditable_)
    return nil;

  autofill::ContentAutofillDriver* autofillDriver =
      autofill::ContentAutofillDriver::GetForRenderFrameHost(
          contents_->GetFocusedFrame());

  DCHECK(autofillDriver);
  autofill::AutofillManager* autofillManager =
      autofillDriver->autofill_manager();
  DCHECK(autofillManager);

  autofill::FormData form = autofillManager->focused_form();
  autofill::FormFieldData field = autofillManager->focused_field();

  // Autofill stuff now...
  if (!autofillManager->IsCreditCardPopup(form, field)) {
    LOG(INFO) << "Not IsCreditCardPopup";
    return nil;
  } else {
    LOG(INFO) << "IsCreditCardPopup";
  }

  base::scoped_nsobject<NSTouchBar> touchBar([[ui::NSTouchBar() alloc] init]);
  [touchBar setCustomizationIdentifier:ui::GetTouchBarId(kAutofillTouchBarId)];
  [touchBar setDelegate:self];

  [touchBar setDefaultItemIdentifiers:@[ kCreditCardTouchId ]];
  return touchBar.autorelease();
}

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
  if ([identifier hasSuffix:kCreditCardTouchId]) {
    base::scoped_nsobject<NSPopoverTouchBarItem> touchBarItem(
        [[ui::NSPopoverTouchBarItem() alloc] initWithIdentifier:identifier]);
    [touchBarItem setCollapsedRepresentationImage:(NSImageFromImageSkia(
                                                      gfx::CreateVectorIcon(
                                                          ui::kCreditCardIcon,
                                                          16, SK_ColorWHITE)))];

    base::scoped_nsobject<NSTouchBar> popoverTouchBar(
        [[ui::NSTouchBar() alloc] init]);
    [popoverTouchBar setDelegate:self];
    [popoverTouchBar setDefaultItemIdentifiers:@[ kCreditCardItemsTouchId ]];

    [touchBarItem setPopoverTouchBar:popoverTouchBar.autorelease()];

    return touchBarItem.autorelease();
  } else if ([identifier hasSuffix:kCreditCardItemsTouchId]) {
    NSImage* masterCardIcon =
        ResourceBundle::GetSharedInstance()
            .GetNativeImageNamed(IDR_AUTOFILL_CC_MASTERCARD)
            .AsNSImage();
    NSImage* visaIcon = ResourceBundle::GetSharedInstance()
                            .GetNativeImageNamed(IDR_AUTOFILL_CC_VISA)
                            .AsNSImage();

    base::scoped_nsobject<NSCustomTouchBarItem> item1(
        [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:@"mastercard"]);
    NSButton* button1 = [NSButton buttonWithTitle:@"MasterCard ----1234"
                                            image:masterCardIcon
                                           target:self
                                           action:@selector(someAction:)];
    [button1 setContentHuggingPriority:1.0
                        forOrientation:NSLayoutConstraintOrientationHorizontal];
    button1.imageHugsTitle = YES;
    [item1 setView:button1];

    base::scoped_nsobject<NSCustomTouchBarItem> item2(
        [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:@"visa"]);
    NSButton* button2 = [NSButton buttonWithTitle:@"VISA ----1234"
                                            image:visaIcon
                                           target:self
                                           action:@selector(someAction:)];
    [button2 setContentHuggingPriority:1.0
                        forOrientation:NSLayoutConstraintOrientationHorizontal];
    button2.imageHugsTitle = YES;
    [item2 setView:button2];

    base::scoped_nsobject<NSCustomTouchBarItem> item3(
        [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:@"visa2"]);
    NSButton* button3 = [NSButton buttonWithTitle:@"VISA ----5678"
                                            image:visaIcon
                                           target:self
                                           action:@selector(someAction:)];
    button3.imageHugsTitle = YES;
    [button3 setContentHuggingPriority:1.0
                        forOrientation:NSLayoutConstraintOrientationHorizontal];
    [item3 setView:button3];

    NSArray* creditCardItems =
        @[ item1.autorelease(), item2.autorelease(), item3.autorelease() ];
    return [ui::NSGroupTouchBarItem() groupItemWithIdentifier:identifier
                                                        items:creditCardItems];
  }

  return nil;
}

@end
