// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BUBBLE_TEXT_FIELD_H_
#define CHROME_BROWSER_UI_COCOA_BUBBLE_TEXT_FIELD_H_

#import <Cocoa/Cocoa.h>

// This class is used to customize the field editors for textfields in
// dialogs. It intercepts touch bar methods to prevent the textfield's
// touch Bar from overriding the dialog's.
//
// TODO(spqchan): Add the textfield's candidate list Touch Bar item to the
// touch bar. I've tried combining the touch bar created from the parent and
// dialog, but the result is buggy.
@interface DialogTextFieldEditor : NSTextView

@end

#endif  // CHROME_BROWSER_UI_COCOA_BASE_BUBBLE_CONTROLLER_H_