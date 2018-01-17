'use strict';

// Mocks the CredentialManager interface defined in credential_manager.mojom.
class MockCredentialManager {
  constructor() {
    this.reset();

    this.binding_ = new mojo.Binding(passwordManager.mojom.CredentialManager, this);
    this.interceptor_ = new MojoInterfaceInterceptor(
        passwordManager.mojom.CredentialManager.name);
    this.interceptor_.oninterfacerequest = e => {
      this.binding_.bind(e.handle);
    };
    this.interceptor_.start();
  }

  constructCredentialInfo_(type, id, password, name, icon) {
    return new passwordManager.mojom.CredentialInfo({
      type: type,
      id: id,
      name: name,
      icon: new url.mojom.Url({url: icon}),
      password: password,
      federation: new url.mojom.Origin(
          {scheme: '', host: '', port: 0, suborigin: '', unique: true})
    });
  }

  // Mock functions:

  async get(mediation, includePasswords, federations) {
    console.log(passwordManager.mojom.CredentialType.PASSWORD);
    if (this.error_ == passwordManager.mojom.CredentialManagerError.SUCCESS) {
      return {error: this.error_, credential: this.credentialInfo_};
    } else {
      return {error: this.error_, credential: null};
    }
  }

  async store(credential) {
    return {};
  }

  async preventSilentAccess() {
    return {};
  }

  // Resets state of mock CredentialManager.
  reset() {
    this.error_ = passwordManager.mojom.CredentialManagerError.SUCCESS;
    this.credentialInfo_ = this.constructCredentialInfo_(
        passwordManager.mojom.CredentialType.EMPTY, '', '', '', '');
  }

  setResponse(id, password, name, icon) {
    this.credentialInfo_ = this.constructCredentialInfo_(
        passwordManager.mojom.CredentialType.PASSWORD, id, password, name,
        icon);
  }

  setError(error) {
    this.error_ = error;
  }
}

// Class that mocks Authenticator interface defined in authenticator.mojom.
class MockAuthenticator {
  constructor() {
    this.reset();

    this.binding_ = new mojo.Binding(webauth.mojom.Authenticator, this);
    this.interceptor_ = new MojoInterfaceInterceptor(
        webauth.mojom.Authenticator.name);
    this.interceptor_.oninterfacerequest = e => {
      this.binding_.bind(e.handle);
    };
    this.interceptor_.start();
  }

  // Returns a MakeCredentialResponse to the client.
  async makeCredential(options) {
    var response = null;
    if (this.status_ == webauth.mojom.AuthenticatorStatus.SUCCESS) {
      let info = new webauth.mojom.CommonCredentialInfo(
          { id: this.id_,
            rawId: this.rawId_,
            clientDataJson: this.clientDataJson_,
          });
      response = new webauth.mojom.MakeCredentialAuthenticatorResponse(
          { info: info,
            attestationObject: this.attestationObject_
          });
    }
    let status = this.status_;
    this.reset();
    return {status, credential: response};
  }

  async getAssertion(options) {
    var response = null;
    if (this.status_ == webauth.mojom.AuthenticatorStatus.SUCCESS) {
      let info = new webauth.mojom.CommonCredentialInfo(
          { id: this.id_,
            rawId: this.rawId_,
            clientDataJson: this.clientDataJson_,
          });
      response = new webauth.mojom.GetAssertionAuthenticatorResponse(
          { info: info,
            authenticatorData: this.authenticatorData_,
            signature: this.signature_,
            userHandle: this.userHandle_,
          });
    }
    let status = this.status_;
    this.reset();
    return {status, credential: response};
  }

  // Resets state of mock Authenticator.
  reset() {
    this.status_ = webauth.mojom.AuthenticatorStatus.UNKNOWN_ERROR;
    this.id_ = null;
    this.rawId_ = new Uint8Array(0);
    this.clientDataJson_ = new Uint8Array(0);
    this.attestationObject_ = new Uint8Array(0);
    this.authenticatorData_ = new Uint8Array(0);
    this.signature_ = new Uint8Array(0);
    this.userHandle_ = new Uint8Array(0);
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

  setUserHandle(userHandle) {
    this.userHandle_ = userHandle;
  }
}

var mockAuthenticator = new MockAuthenticator();
var mockCredentialManager = new MockCredentialManager();

// Common mock values for the mockAuthenticator.
var challenge = new TextEncoder().encode("climb a mountain");

var public_key_rp = {
    id: "subdomain.example.test",
    name: "Acme"
};

var public_key_user = {
    id: new TextEncoder().encode("1098237235409872"),
    name: "avery.a.jones@example.com",
    displayName: "Avery A. Jones",
    icon: "https://pics.acme.com/00/p/aBjjjpqPb.png"
};

var public_key_parameters =  [{
    type: "public-key",
    alg: -7,
},];

var make_credential_options = {
    challenge,
    rp: public_key_rp,
    user: public_key_user,
    pubKeyCredParams: public_key_parameters,
    excludeCredentials: [],
};

var acceptable_credential = {
    type: "public-key",
    id: new TextEncoder().encode("acceptableCredential"),
    transports: ["usb", "nfc", "ble"]
};

var get_assertion_options = {
    challenge,
    rpId: "subdomain.example.test",
    allowCredentials: [acceptable_credential]
};

var raw_id = new TextEncoder("utf-8").encode("rawId");
var id = btoa("rawId");
var client_data_json = new TextEncoder("utf-8").encode("clientDataJSON");
var attestation_object = new TextEncoder("utf-8").encode("attestationObject");
var authenticator_data = new TextEncoder("utf-8").encode("authenticatorData");
var signature = new TextEncoder("utf-8").encode("signature");

// Verifies if |r| is the valid response to credentials.create(publicKey).
function assertValidMakeCredentialResponse(r) {
assert_equals(r.id, id, 'id');
    assert_true(r.rawId instanceof ArrayBuffer);
    assert_array_equals(new Uint8Array(r.rawId),
        raw_id, "rawId returned is the same");
    assert_true(r.response instanceof AuthenticatorAttestationResponse);
    assert_true(r.response.clientDataJSON instanceof ArrayBuffer);
    assert_array_equals(new Uint8Array(r.response.clientDataJSON),
        client_data_json, "clientDataJSON returned is the same");
    assert_true(r.response.attestationObject instanceof ArrayBuffer);
    assert_array_equals(new Uint8Array(r.response.attestationObject),
        attestation_object, "attestationObject returned is the same");
    assert_not_exists(r.response, 'authenticatorData');
    assert_not_exists(r.response, 'signature');
}

// Verifies if |r| is the valid response to credentials.get(publicKey).
function assertValidGetAssertionResponse(r) {
    assert_equals(r.id, id, 'id');
    assert_true(r.rawId instanceof ArrayBuffer);
    assert_array_equals(new Uint8Array(r.rawId),
        raw_id, "rawId returned is the same");

    // The call returned an AssertionResponse, meaning it has
    //  authenticatorData and signature and does not have an attestationObject.
    assert_true(r.response instanceof AuthenticatorAssertionResponse);
    assert_true(r.response.clientDataJSON instanceof ArrayBuffer);
    assert_array_equals(new Uint8Array(r.response.clientDataJSON),
        client_data_json, "clientDataJSON returned is the same");
    assert_true(r.response.authenticatorData instanceof ArrayBuffer);
    assert_true(r.response.signature instanceof ArrayBuffer);
    assert_array_equals(new Uint8Array(r.response.authenticatorData),
        authenticator_data, "authenticator_data returned is the same");
    assert_array_equals(new Uint8Array(r.response.signature),
        signature, "signature returned is the same");
    assert_not_exists(r.response, 'attestationObject');
}
