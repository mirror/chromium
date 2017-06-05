/*
 * payment-request-mock contains a mock implementation of PaymentRequest.
 */

'use strict';

let paymentRequestMock = loadMojoModules('paymentRequestMock', [
  'components/payments/mojom/payment_request.mojom',
  'mojo/public/js/bindings',
]).then(mojo => {
  let [paymentRequest, bindings] = mojo.modules;

  class PaymentRequestMock {
    constructor(interfaceProvider) {
      interfaceProvider.addInterfaceOverrideForTesting(
        paymentRequest.PaymentRequest.name,
        handle => this.bindings_.addBinding(this, handle));

      this.interfaceProvider_ = interfaceProvider;
      this.bindings_ = new bindings.BindingSet(paymentRequest.PaymentRequest);
    }

    init(client, supportedMethods, details, options) {
      this.client_ = client;
    }

    show() {}

    updateWith(details) {}

    abort() {
      this.client_.onAbort(true);
    }

    complete(success) {}

    canMakePayment() {}
  }
  return new PaymentRequestMock(mojo.frameInterfaces);
});
