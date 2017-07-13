// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_textfield_touch_bar_controller.h"

#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#include "chrome/common/chrome_features.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "components/grit/components_scaled_resources.h"
#import "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {

NSString* const kAutofillTouchBarId = @"autofill";

// Touch bar items identifiers.
NSString* const kCreditCardTouchId = @"CREDIT-CARD";
NSString* const kCreditCardItemsTouchId = @"CREDIT-CARD-ITEMS";

// Returns the credit card image.
NSImage* GetCreditCardImage(int iconId) {
  if (iconId == -1)
    return nil;

  // If it's a generic card image, use the vector icon instead.
  if (iconId == IDR_AUTOFILL_CC_GENERIC) {
    return NSImageFromImageSkia(
        gfx::CreateVectorIcon(kCreditCardIcon, 16, SK_ColorWHITE));
  }

  return ResourceBundle::GetSharedInstance()
      .GetNativeImageNamed(iconId)
      .AsNSImage();
}

}  // namespace

@implementation WebTextfieldTouchBarController

- (instancetype)initWithTabContentsController:(TabContentsController*)owner {
  if ((self = [self init])) {
    owner_ = owner;
  }

  return self;
}

- (void)showCreditCardAutofillForWindow:(NSWindow*)window
                             controller:(autofill::AutofillPopupController*)
                                            controller {
  if (!base::FeatureList::IsEnabled(features::kCreditCardAutofillTouchBar))
    return;

  DCHECK(window);
  DCHECK(controller);
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center addObserver:self
             selector:@selector(popupWindowWillClose:)
                 name:NSWindowWillCloseNotification
               object:window];
  popupController_ = controller;

  if ([owner_ respondsToSelector:@selector(setTouchBar:)])
    [owner_ performSelector:@selector(setTouchBar:) withObject:nil];
}

- (void)popupWindowWillClose:(NSNotification*)notif {
  popupController_ = nullptr;

  if ([owner_ respondsToSelector:@selector(setTouchBar:)])
    [owner_ performSelector:@selector(setTouchBar:) withObject:nil];
}

- (NSTouchBar*)makeTouchBar {
  if (!popupController_)
    return nil;

  if (!popupController_->GetLineCount() ||
      !popupController_->layout_model().is_credit_card_popup()) {
    return nil;
  }

  base::scoped_nsobject<NSTouchBar> touchBar([[ui::NSTouchBar() alloc] init]);
  [touchBar setCustomizationIdentifier:ui::GetTouchBarId(kAutofillTouchBarId)];
  [touchBar setDelegate:self];

  [touchBar setDefaultItemIdentifiers:@[ kCreditCardItemsTouchId ]];
  return touchBar.autorelease();
}

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
  if (![identifier hasSuffix:kCreditCardItemsTouchId])
    return nil;

  NSMutableArray* creditCardItems = [NSMutableArray array];

  DCHECK(popupController_);
  for (int i = 0; i < popupController_->GetLineCount(); i++) {
    const autofill::Suggestion& suggestion =
        popupController_->GetSuggestionAt(i);
    if (suggestion.frontend_id < autofill::POPUP_ITEM_ID_AUTOCOMPLETE_ENTRY)
      continue;

    NSString* identifier =
        [NSString stringWithFormat:@"%@-%i", kCreditCardTouchId, i];

    base::scoped_nsobject<NSCustomTouchBarItem> item(
        [[ui::NSCustomTouchBarItem() alloc] initWithIdentifier:identifier]);

    NSString* suggestionLabel =
        SysUTF16ToNSString(popupController_->GetElidedLabelAt(i));

    NSString* buttonTitle =
        [NSString stringWithFormat:@"%@\t%@",
                                   SysUTF16ToNSString(
                                       popupController_->GetElidedValueAt(i)),
                                   suggestionLabel];

    NSImage* cardIconImage = GetCreditCardImage(
        popupController_->layout_model().GetIconResourceID(suggestion.icon));
    NSButton* button = nil;
    if (cardIconImage) {
      button = [NSButton buttonWithTitle:buttonTitle
                                   image:cardIconImage
                                  target:self
                                  action:@selector(acceptCreditCard:)];
    } else {
      button = [NSButton buttonWithTitle:buttonTitle
                                  target:self
                                  action:@selector(acceptCreditCard:)];
    }

    NSColor* subtextColor = [NSColor colorWithCalibratedRed:180.0 / 255.0
                                                      green:180.0 / 255.0
                                                       blue:180.0 / 255.0
                                                      alpha:1.0];

    NSRange labelRange = NSMakeRange(
        [buttonTitle length] - suggestionLabel.length, suggestionLabel.length);
    NSFont* labelFont = [[NSFontManager sharedFontManager]
        convertFont:button.font
             toSize:button.font.pointSize - 1];

    base::scoped_nsobject<NSMutableAttributedString> attributedString(
        [[NSMutableAttributedString alloc]
            initWithAttributedString:button.attributedTitle]);
    [attributedString addAttribute:NSForegroundColorAttributeName
                             value:[NSColor whiteColor]
                             range:NSMakeRange(0, labelRange.location)];
    [attributedString addAttribute:NSForegroundColorAttributeName
                             value:subtextColor
                             range:labelRange];
    [attributedString addAttribute:NSFontAttributeName
                             value:labelFont
                             range:labelRange];
    button.attributedTitle = attributedString;

    button.imageHugsTitle = YES;
    button.imagePosition = NSImageRight;
    button.tag = i;

    [item setView:button];
    [creditCardItems addObject:item.autorelease()];
  }

  return [ui::NSGroupTouchBarItem() groupItemWithIdentifier:identifier
                                                      items:creditCardItems];
}

- (void)acceptCreditCard:(id)sender {
  DCHECK(popupController_);
  popupController_->AcceptSuggestion([sender tag]);
}

@end
