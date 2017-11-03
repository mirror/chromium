// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_

#import "ios/web_view/public/cwv_autofill_controller.h"

@class AutofillAgent;

namespace web {
class WebState;
}  // namespace web

@interface CWVAutofillController ()

- (void)setUpWithWebState:(web::WebState*)webState
            autofillAgent:(AutofillAgent*)autofillAgent;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
