/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/** Builds a PaymentRequest that requests shipping. */
function buildPaymentRequest() {
  try {
    return new PaymentRequest(
        [{supportedMethods: 'basic-card'}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}},
        {requestShipping: true});
  } catch (error) {
    print(error.message);
  }
}

/** Shows the PaymentRequest. */
function showPaymentRequest(pr) {
  pr.show()
      .then(function(resp) {
        resp.complete('success')
            .then(function() {
              print(JSON.stringify(resp, undefined, 2));
            })
            .catch(function(error) {
              print(error);
            });
      })
      .catch(function(error) {
        print(error);
      });
}

/** Show a PaymentRequest that requests shipping, but has no listeners. */
function buyWithoutListeners() {  // eslint-disable-line no-unused-vars
  showPaymentRequest(buildPaymentRequest());
}

/**
 * Show a PaymentRequest that requests shipping, but listener don't call
 * updateWith().
 */
function buyWithoutCallingUpdateWith() {  // eslint-disable-line no-unused-vars
  const pr = buildPaymentRequest();
  pr.addEventListener('shippingaddresschange', function(evt) {
    print('shippingaddresschange');
  });
  pr.addEventListener('shippingoptionchange', function(evt) {
    print('shippingoptionchange');
  });
  showPaymentRequest(pr);
}
