// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_PRESENTER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_PRESENTER_H_

#import <UIKit/UIKit.h>

#import "ios/public/provider/chrome/browser/voice/logo_animation_controller.h"

class ViewInfo;

// Protocol used by UIViewControllers that present VoiceSearch.
@protocol VoiceSearchPresenter<LogoAnimationControllerOwnerOwner, NSObject>

// The button that was tapped in order to trigger VoiceSearch.
// TODO(crbug.com/783037): Remove as third portion of 3-sided patch and use
// ViewInfo instead.
@property(nonatomic, readonly) UIView* voiceSearchButton;

// The ViewInfo for the button that was tapped to trigger VoiceSearch
@property(nonatomic, readonly) const ViewInfo* voiceSearchOriginViewInfo;

@end

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_VOICE_VOICE_SEARCH_PRESENTER_H_
