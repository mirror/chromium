'use strict';

function assertRejectsWithError(promise, name) {
  return promise.then(() => {
    assert_unreached('expected promise to reject with ' + name);
  }, error => {
    assert_equals(error.name, name);
  });
}

// Class that mocks Authenticator interface defined in authenticator.mojom.
class MockAuthenticator {
  constructor() {
    this.status_ = webauth.mojom.AuthenticatorStatus.NOT_IMPLEMENTED;
    this.id_ = null;
    this.rawId_ =  new Uint8Array(0);
    this.clientDataJson_ =  new Uint8Array(0);
    this.attestationObject_ =  new Uint8Array(0);
    this.authenticatorData_ =  new Uint8Array(0);
    this.signature_ =  new Uint8Array(0);

    this.binding_ = new mojo.Binding(webauth.mojom.Authenticator, this);
    this.interceptor_ = new MojoInterfaceInterceptor(
        webauth.mojom.Authenticator.name);
    this.interceptor_.oninterfacerequest = e => {
      this.bindToPipe(e.handle);
    };
    this.interceptor_.start();
  }

  // Binds object to mojo message pipe
  bindToPipe(pipe) {
    this.binding_.bind(pipe);
    this.binding_.setConnectionErrorHandler(() => {
      this.reset();
    });
  }

  // Returns a PublicKeyCredentialInfo to the client.
  async makeCredential(options) {
    var info = null;
    if (this.status_ == webauth.mojom.AuthenticatorStatus.SUCCESS) {
      let response = new webauth.mojom.AuthenticatorResponse(
          { attestationObject: this.attestationObject_,
            authenticatorData: this.authenticatorData_,
            signature: this.signature_
          });
      info = new webauth.mojom.PublicKeyCredentialInfo(
          { id: this.id_,
            rawId: this.rawId_,
            clientDataJson: this.clientDataJson_,
            response: response
          });
    }
    return Promise.resolve({status: this.status_, credential: info});
  }

  // Mock functions

  // Resets state of mock Authenticatorbetween test runs.
  reset() {
    this.status_ = webauth.mojom.AuthenticatorStatus.NOT_IMPLEMENTED;
    this.id_ = null;
    this.rawId_ = null;
    this.clientDataJson_ = null;
    this.attestationObject_ = null;
    this.authenticatorData_ = null;
    this.signature_ = null;
  }

  setAuthenticatorStatus(status) {
    this.status_ = status;
  }

  setId(id) {
    this.id_ = id;
  }

  setRawId(rawId) {
    this.rawId_ = rawId;
  }

  setClientDataJson(clientDataJson) {
    this.clientDataJson_ = clientDataJson;
  }

  setAttestationObject(attestationObject) {
    this.attestationObject_ = attestationObject;
  }

  setAuthenticatorData(authenticatorData) {
    this.authenticatorData_ = authenticatorData;
  }

  setSignature(signature) {
    this.signature_ = signature;
  }
}

let mockAuthenticator = new MockAuthenticator();

function public_key_test(func, name, properties) {
  promise_test(async function() {
    try {
      await func();
    } finally {
      mockAuthenticator.reset();
    }
  }, name, properties);
}