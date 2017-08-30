// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_

#include "base/optional.h"
#include "components/password_manager/core/common/credential_manager_types.h"

namespace web {
class WebState;
}

namespace credential_manager {

// Resolves Promise identified by |promise_id| with either Credential or
// undefined.
void ResolvePromiseWithCredentialInfo(
    web::WebState* web_state,
    int promise_id,
    base::Optional<password_manager::CredentialInfo> info);

// Resolves Promise identified by |promise_id| with void.
void ResolvePromiseWithVoid(web::WebState* web_state, int promise_id);

// Rejects Promise identified by |promise_id| with TypeError. This may be a
// result of failed parsing of arguments passed to exposed API method.
void RejectPromiseWithTypeError(web::WebState* web_state,
                                int promise_id,
                                const base::StringPiece16& message);

// Rejects Promise identified by |promise_id| with InvalidStateError. This
// should happen when credential manager is disabled or there is a pending 'get'
// request.
void RejectPromiseWithInvalidStateError(web::WebState* web_state,
                                        int promise_id,
                                        const base::StringPiece16& message);

// Rejects Promise identified by |promise_id| with NotSupportedError. This
// should happen when password store is unavailable or an unknown error occurs.
void RejectPromiseWithNotSupportedError(web::WebState* web_state,
                                        int promise_id,
                                        const base::StringPiece16& message);

}  // namespace credential_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
