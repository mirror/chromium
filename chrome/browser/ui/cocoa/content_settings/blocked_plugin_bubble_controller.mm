// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/content_settings/blocked_plugin_bubble_controller.h"

#import <Cocoa/Cocoa.h>
#import "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/key_equivalent_constants.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#include "chrome/browser/ui/content_settings/content_setting_bubble_model.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#import "ui/base/cocoa/controls/button_utils.h"
#import "ui/base/cocoa/controls/textfield_utils.h"
#import "ui/base/l10n/l10n_util_mac.h"

namespace {

constexpr int kVerticalSpacing = 8;
constexpr int kHorizontalMargin = 16;


void SetControlSize(NSControl* control, NSControlSize controlSize) {
  CGFloat fontSize = [NSFont systemFontSizeForControlSize:controlSize];
  NSCell* cell = [control cell];
  [cell setFont:[NSFont systemFontOfSize:fontSize]];
  [cell setControlSize:controlSize];
}

}  // namespace

@implementation BlockedPluginBubbleController

- (id)initWithModel:(ContentSettingBubbleModel*)model
        webContents:(content::WebContents*)webContents
       parentWindow:(NSWindow*)parentWindow
         decoration:(ContentSettingDecoration*)decoration
         anchoredAt:(NSPoint)anchoredAt {
  base::scoped_nsobject<InfoBubbleWindow> window([[InfoBubbleWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 314, 179)
          styleMask:NSBorderlessWindowMask
              backing:NSBackingStoreBuffered
                defer:NO]);

  [window setAllowedAnimations:info_bubble::kAnimateNone];

  return [super initWithModel:model
                    webContents:webContents
                      window:window
                  parentWindow:parentWindow
                      decoration:decoration
                      anchoredAt:anchoredAt];
}

- (void)awakeFromNib {
  [self loadView];
  [super awakeFromNib];
}

- (NSView*)makePluginsBox:(NSRect)frame {
  NSView* plugins =
      [[[NSView alloc] initWithFrame:frame] autorelease];

  const ContentSettingBubbleModel::ListItems& items =
      contentSettingBubbleModel_->bubble_content().list_items;

  CGFloat y = NSHeight([plugins frame]) - 18;
  for (const auto& item : items) {
    DCHECK(!item.has_link);

    NSString* title = base::SysUTF16ToNSString(item.title);
    NSTextField* label = [TextFieldUtils labelWithString:title];
    SetControlSize(label, NSSmallControlSize);
    [label setFrame:NSMakeRect(0, y, NSWidth(frame), 14)];
    cocoa_l10n_util::WrapOrSizeToFit(label);
    [plugins addSubview:label];
    y -= 18;
  }

  return plugins;
}

- (void)loadView {
  int fullWidth = NSWidth([self.window.contentView frame]) - 2 * kHorizontalMargin;

  NSButton* manageButton = 
      [ButtonUtils buttonWithTitle:l10n_util::GetNSString(IDS_BLOCKED_PLUGINS_LINK)
                    action:@selector(manageBlocking:)
                    target:self];
  SetControlSize(manageButton, NSSmallControlSize);
  cocoa_l10n_util::WrapOrSizeToFit(manageButton);
  [manageButton setFrameOrigin:NSMakePoint(kHorizontalMargin, kVerticalSpacing)];
  [self.window.contentView addSubview:manageButton];

  NSButton* doneButton =
      [ButtonUtils buttonWithTitle:l10n_util::GetNSString(IDS_DONE)
              action:@selector(closeBubble:)
              target:self];
  SetControlSize(doneButton, NSSmallControlSize);
  cocoa_l10n_util::WrapOrSizeToFit(doneButton);
  [doneButton setFrameOrigin:NSMakePoint(
      NSWidth([self.window.contentView frame]) - NSWidth([doneButton frame]) -
kHorizontalMargin,
      kVerticalSpacing)];
  [doneButton setKeyEquivalent:kKeyEquivalentReturn];
  [self.window.contentView addSubview:doneButton];

  base::scoped_nsobject<NSBox> bottomSeparator([[NSBox alloc]
      initWithFrame:NSMakeRect(kHorizontalMargin, NSMaxY([manageButton frame]) +
kVerticalSpacing, fullWidth, 4)]);
  [bottomSeparator setBoxType:NSBoxSeparator];
  [self.window.contentView addSubview:bottomSeparator];

  NSButton* learnMoreLink =
      [ButtonUtils linkWithTitle:l10n_util::GetNSString(IDS_LEARN_MORE)
                        action:@selector(learnMoreLinkClicked:)
                        target:self];
  [learnMoreLink setFrame:NSMakeRect(kHorizontalMargin,
      NSMaxY([bottomSeparator frame]) + kVerticalSpacing,
      fullWidth, 18)];
  SetControlSize(learnMoreLink, NSSmallControlSize);
  cocoa_l10n_util::WrapOrSizeToFit(learnMoreLink);
  [self.window.contentView addSubview:learnMoreLink];

  NSTextField* title = [TextFieldUtils
labelWithString:l10n_util::GetNSString(IDS_BLOCKED_PLUGINS_TITLE)];
  // The extra -6 here is unprincipled, but necessary: it helps the title stand
  // out from the top of the bubble.
  [title setFrame:NSMakeRect(kHorizontalMargin,
                             NSMaxY([self.window.contentView bounds]) -
kVerticalSpacing - 18 - 6, fullWidth, 18)];
  SetControlSize(title, NSSmallControlSize);
  cocoa_l10n_util::WrapOrSizeToFit(title);
  [self.window.contentView addSubview:title];

  CGFloat pluginsY = NSMaxY([learnMoreLink frame]) + kVerticalSpacing;
  NSView* pluginsBox = [self makePluginsBox:NSMakeRect(
      kHorizontalMargin,
      pluginsY,
      fullWidth,
      NSMinY([title frame]) - kVerticalSpacing - pluginsY)];
  [self.window.contentView addSubview:pluginsBox];
}

- (void)layoutView {

}

@end
