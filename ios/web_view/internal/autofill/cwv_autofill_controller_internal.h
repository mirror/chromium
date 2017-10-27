// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_

#import "ios/web_view/public/cwv_autofill_controller.h"

@class AutofillAgent;
@class JsAutofillManager;

namespace web {
class WebState;
class WebStateObserverBridge;
}  // namespace web

namespace autofill {
class AutofillClientIOS;
class AutofillManager;
}  // namespace autofill

@interface CWVAutofillController ()

// The |webState| which this autofill controller should observe.
@property(nonatomic, assign) web::WebState* webState;

// Bridge to observe the |webState|.
@property(nonatomic, readonly)
    web::WebStateObserverBridge* webStateObserverBridge;

// Autofill client created from |webState|.
@property(nonatomic, readonly) autofill::AutofillClientIOS* autofillClient;

// Autofill agent created from |webState|.
@property(nonatomic, readonly) AutofillAgent* autofillAgent;

// Autofill manager created from |webState|.
@property(nonatomic, readonly) autofill::AutofillManager* autofillManager;

// Exposes injected javascript.
@property(nonatomic, readonly) JsAutofillManager* JSAutofillManager;

@end

#endif  // IOS_WEB_VIEW_INTERNAL_AUTOFILL_CWV_AUTOFILL_CONTROLLER_INTERNAL_H_
