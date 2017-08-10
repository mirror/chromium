// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/payments/payment_handler_permission_context.h"

#include "components/content_settings/core/common/content_settings_types.h"
#include "third_party/WebKit/public/platform/WebFeaturePolicyFeature.h"

namespace payments {

PaymentHandlerPermissionContext::PaymentHandlerPermissionContext(
    Profile* profile)
    : PermissionContextBase(profile,
                            CONTENT_SETTINGS_TYPE_PAYMENT_HANDLER,
                            blink::WebFeaturePolicyFeature::kNotFound) {}

PaymentHandlerPermissionContext::~PaymentHandlerPermissionContext() {}

bool PaymentHandlerPermissionContext::IsRestrictedToSecureOrigins() const {
  return true;
}

}  // namespace payments
