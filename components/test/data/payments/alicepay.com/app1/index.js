/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Prints output.
 * @param {String} src - Where the message is coming from.
 * @param {String} txt - The text to print.
 */
function output(src, txt) {
  // Handle DOMException:
  if (txt.message) {
    txt = txt.message;
  }
  txt = src + ': ' + txt;
  if (window.domAutomationController) {
    domAutomationController.send(txt);
  }
  document.getElementById('output').innerHTML += txt + '<br>';
}

/**
 * Installs a payment app.
 * @param {String} method - The payment method name that this app supports.
 */
function install(method) {  // eslint-disable-line no-unused-vars
  if (!navigator.serviceWorker) {
    output('install()', 'ServiceWorker API not found.');
    return;
  }

  navigator.serviceWorker.getRegistration('app.js')
      .then((registration) => {
        if (registration) {
          output(
              'serviceWorker.getRegistration()',
              'The ServiceWorker is already installed.');
          return;
        }
        navigator.serviceWorker.register('app.js')
            .then(() => {
              return navigator.serviceWorker.ready;
            })
            .then((registration) => {
              if (!registration.paymentManager) {
                output(
                    'serviceWorker.register()',
                    'PaymentManager API not found.');
                return;
              }

              registration.paymentManager.instruments
                  .set('123456', {name: 'Alice Pay', enabledMethods: [method]})
                  .then(() => {
                    output(
                        'instruments.set()',
                        'Payment app for "' + method + '" method installed.');
                  })
                  .catch((error) => {
                    output('instruments.set()', error);
                  });
            })
            .catch((error) => {
              output('serviceWorker.register()', error);
            });
      })
      .catch((error) => {
        output('serviceWorker.getRegistration()', error);
      });
}

/**
 * Checks whether a payment can be made.
 * @param {String} method - The payment method name to check.
 */
function canMakePayment(method) {  // eslint-disable-line no-unused-vars
  let paymentRequest;
  try {
    paymentRequest = new PaymentRequest(
        [{supportedMethods: method}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '1.00'}}});
  } catch (error) {
    output('new PaymentRequest()', error);
    return;
  }

  paymentRequest.canMakePayment()
      .then((result) => {
        if (result) {
          output(
              'paymentRequest.canMakePayment()',
              'Can make payments with "' + method + '".');
        } else {
          output(
              'paymentRequest.canMakePayment()',
              'Cannot make payments with "' + method + '".');
        }
      })
      .catch((error) => {
        output('paymentRequest.canMakePayment()', error);
      });
}
