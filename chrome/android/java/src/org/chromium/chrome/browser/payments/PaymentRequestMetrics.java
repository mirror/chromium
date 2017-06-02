// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import org.chromium.base.metrics.RecordHistogram;

/**
 * A class used to record metrics for the Payment Request feature.
 */
public final class PaymentRequestMetrics {
    // There should be no instance of PaymentRequestMetrics created.
    private PaymentRequestMetrics() {}

    /*
     * Records the metric that keeps track of what payment method was used to complete a Payment
     * Request transaction.
     *
     * @param paymentMethod The payment method that was used to complete the current transaction.
     */
    public static void recordSelectedPaymentMethodHistogram(int paymentMethod) {
        assert paymentMethod < SelectedPaymentMethod.MAX;
        RecordHistogram.recordEnumeratedHistogram(
                "PaymentRequest.SelectedPaymentMethod", paymentMethod, SelectedPaymentMethod.MAX);
    }
}
