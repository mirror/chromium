/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/** Invokes PaymentRequest with a very long request identifier. */
function buy() {  // eslint-disable-line no-unused-vars
  let foo = Object.freeze({supportedMethods: ['basic-card']});
  let defaultMethods = Object.freeze([foo]);
  let defaultDetails = Object.freeze({
    total: {
      label: 'Label',
      amount: {
        currency: 'USD',
        value: '1.00',
      },
    },
  });
  let newDetails = Object.assign({}, defaultDetails, {
    id: ''.padStart(100000000, '\n very long id \t \n '),
  });
  try {
    // eslint-disable-next-line no-unused-vars
    let request = new PaymentRequest(defaultMethods, newDetails);
  } catch (error) {
    print(error);
  }
}
