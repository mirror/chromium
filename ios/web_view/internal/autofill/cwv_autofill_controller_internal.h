// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H

#import "ios/web_view/public/cwv_autofill_controller.h"

namespace web {
class WebState;
}

@interface CWVAutofillController ()

// The |webState| which this autofill controller should observe.
@property(nonatomic, assign) web::WebState* webState;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H
