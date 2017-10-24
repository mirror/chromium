/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Builds a PaymentRequest that requests a shipping address.
 * @return {PaymentRequest} - A new PaymentRequest object.
 */
function buildPaymentRequest() {
  try {
    return new PaymentRequest(
        [{supportedMethods: 'basic-card'}], {
          total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
          shippingOptions: [{
            selected: true,
            id: 'freeShipping',
            label: 'Free shipping',
            amount: {currency: 'USD', value: '0.00'},
          }],
        },
        {requestShipping: true});
  } catch (error) {
    print(error.message);
  }
}

/**
 * Shows the PaymentRequest.
 * @param {PaymentRequest} pr - The PaymentRequest object to show.
 */
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

/** Show a PaymentRequest that requests a shipping address, but has no
 * listeners. */
function buyWithoutListeners() {  // eslint-disable-line no-unused-vars
  showPaymentRequest(buildPaymentRequest());
}

/**
 * Show a PaymentRequest that requests a shipping address, but listener don't
 * call updateWith().
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
