// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/web_textfield_touch_bar_controller.h"

#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_popup_view_cocoa.h"
#import "chrome/browser/ui/cocoa/tab_contents/tab_contents_controller.h"
#include "content/public/browser/focused_node_details.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#import "ui/base/cocoa/touch_bar_util.h"

namespace {

// Touch bar identifier.
NSString* const kWebTextfieldTouchBarId = @"web-textfield";

// Touch bar items identifier.
NSString* const kTextSuggestionTouchId = @"TEXT-SUGGESTIONS";
}

class WebTextfieldTouchBarBridgeObserver : public content::WebContentsObserver {
 public:
  WebTextfieldTouchBarBridgeObserver(WebTextfieldTouchBarController* owner,
                                     content::WebContents* contents)
      : content::WebContentsObserver(contents),
        owner_(owner),
        contents_(contents),
        is_editable_node_focused_(false) {}

  void set_contents(content::WebContents* contents) { contents_ = contents; }

  void OnTextSelectionChanged() override {
    LOG(INFO) << "OnTextSelectionChanged";
    bool is_node_focused = contents_->IsFocusedElementEditable();

    if (contents_->IsFocusedElementEditable()) {
      LOG(INFO) << "Element is editable!";
      LOG(INFO) << contents_->GetTextForSuggestions();
      [owner_ updateTextSuggestionCandidateListWithText:
                  base::SysUTF8ToNSString(contents_->GetTextForSuggestions())];
    } else if (is_editable_node_focused_ != is_node_focused) {
      [owner_ updateTextSuggestionCandidateListWithText:nil];
    }
  }

 private:
  WebTextfieldTouchBarController* owner_;
  content::WebContents* contents_;
  bool is_editable_node_focused_;

  std::string text_for_suggestions_;
};

@implementation WebTextfieldTouchBarController

- (instancetype)initWithTabContentsController:(TabContentsController*)owner
                                  webContents:(content::WebContents*)contents {
  if ((self = [self init])) {
    owner_ = owner;
    contents_ = contents;
    observer_.reset(new WebTextfieldTouchBarBridgeObserver(self, contents_));
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)changeWebContents:(content::WebContents*)contents {
  contents_ = contents;
  observer_->set_contents(contents_);
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

- (void)updateTextSuggestionCandidateListWithText:(NSString*)text {
  if ([textForSuggestions_ isEqualToString:text])
    return;

  textForSuggestions_.reset([text copy]);

  if (@available(macOS 10.12.2, *)) {
    NSSpellChecker* spellChecker = [NSSpellChecker sharedSpellChecker];
    [spellChecker
        requestCandidatesForSelectedRange:NSMakeRange(0, text.length)
                                 inString:text
                                    types:NSTextCheckingAllTypes
                                  options:nil
                   inSpellDocumentWithTag:0
                        completionHandler:^(
                            NSInteger sequenceNumber,
                            NSArray<NSTextCheckingResult*>* candidates) {
                          candidates_ =
                              [[NSMutableArray alloc] initWithArray:candidates];
                          if ([owner_
                                  respondsToSelector:@selector(setTouchBar:)])
                            [owner_ performSelector:@selector(setTouchBar:)
                                         withObject:nil];
                        }];
  }
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

  // TODO: Maybe move the text suggestions stuff in another class.
  if (textForSuggestions_) {
    base::scoped_nsobject<NSTouchBar> touchBar([[ui::NSTouchBar() alloc] init]);
    [touchBar setDefaultItemIdentifiers:@[
      ui::GetTouchBarItemId(kWebTextfieldTouchBarId, kTextSuggestionTouchId)
    ]];

    [touchBar setDelegate:self];

    return touchBar.autorelease();
  }

  return nil;
}

- (NSTouchBarItem*)touchBar:(NSTouchBar*)touchBar
      makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macos(10.12.2)) {
  if (!touchBar)
    return nil;

  if (!candidateListItem_) {
    candidateListItem_ = [[[NSCandidateListTouchBarItem alloc]
        initWithIdentifier:identifier] autorelease];
  }

  [candidateListItem_ setCandidates:candidates_
                   forSelectedRange:NSMakeRange(0, [textForSuggestions_ length])
                           inString:textForSuggestions_.get()];

  return candidateListItem_;
}

@end
