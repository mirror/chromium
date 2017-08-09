// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview JavaScript implementation of the credential management API
 * defined at http://w3c.github.io/webappsec-credential-management
 * This is a minimal implementation that sends data to the app side to
 * integrate with the password manager. When loaded, installs the API onto
 * the window.navigator.object.
 */

// TODO(crbug.com/435046) tgarbus: This only contains method signatures.
// Implement the JavaScript API and types according to the spec:
// https://w3c.github.io/webappsec-credential-management


// Namespace for credential management. __gCrWeb must have already
// been defined.
__gCrWeb['credentialManager'] = {

  /**
   * Used to apply unique requestId fields to messages sent to host.
   * Those IDs can be later used to call a corresponding resolver/rejecter.
   * @private {number}
   */
  nextId_: 0,

  /**
   * Stores the functions for resolving Promises returned by
   * navigator.credentials method calls. A resolver for a call with
   * requestId: |id| is stored at resolvers_[id].
   * @type {!Object<number, function(?Credential)|function()>}
   * @private
   */
  resolvers_: {},

  /**
   * Stores the functions for rejecting Promises returned by
   * navigator.credentials method calls. A rejecter for a call with
   * requestId: |id| is stored at rejecters_[id].
   * @type {!Object<number, function(?Error)>}
   * @private
   */
  rejecters_: {}
};

/**
 * Creates and returns a Promise with given |requestId|. The Promise's executor
 * function stores resolver and rejecter functions in
 * __gCrWeb['credentialManager'] under the key |requestId| so they can be called
 * from the host after executing app side code.
 * @param {number} requestId The number assigned to newly created Promise.
 * @return {!Promise<?Credential>|!Promise<!Credential>|!Promise<void>}
 *     The created Promise.
 * @private
 */
__gCrWeb['credentialManager'].createPromise_ = function(requestId) {
  return new Promise(function(resolve, reject) {
    __gCrWeb['credentialManager'].resolvers_[requestId] = resolve;
    __gCrWeb['credentialManager'].rejecters_[requestId] = reject;
  });
};

/**
 * Sends a message to the app side, invoking |command| with the given |options|.
 * @param {string} command The name of the invoked command.
 * @param {Object} options A dictionary of additional properties to forward to
 *     the app.
 * @return {!Promise<?Credential>|!Promise<!Credential>|!Promise<void>}
 *     A promise to be returned by the calling method.
 * @private
 */
__gCrWeb['credentialManager'].invokeOnHost_ = function(command, options) {
  var requestId = __gCrWeb['credentialManager'].nextId_++;
  var message = {
    'command': command,
    'requestId': requestId
  };
  if (options) {
    Object.keys(options).forEach(function(key) {
      message[key] = options[key];
    });
  }
  __gCrWeb.message.invokeOnHost(message);
  return __gCrWeb['credentialManager'].createPromise_(requestId);
};

/**
 * The Credential interface, for more information see
 * https://w3c.github.io/webappsec-credential-management/#the-credential-interface
 * @constructor
 */
function Credential() {
  /** @type {string} */
  this.id;
  /** @type {string} */
  this.type;
}

/**
 * PasswordCredential interace, for more information see
 * https://w3c.github.io/webappsec-credential-management/#passwordcredential-interface
 * @extends {Credential}
 * @constructor
 */
// TODO(crbug.com/435046) tgarbus: Implement two constructors: from
// HTMLFormElement and from PasswordCredentialData
function PasswordCredential() {
  /** @type {string} */
  this.name;
  /** @type {string} */
  this.iconURL;
  /** @type {string} */
  this.password;
}

/**
 * FederatedCredential interface, for more information see
 * https://w3c.github.io/webappsec-credential-management/#federatedcredential-interface
 * @extends {Credential}
 * @constructor
 */
// TODO(crbug.com/435046) tgarbus: Implement constructor taking
// FederatedCredentialInit
function FederatedCredential() {
  /** @type {string} */
  this.name;
  /** @type {string} */
  this.iconURL;
  /** @type {string} */
  this.provider;
  /** @type {?string} */
  this.protocol;
}

/**
 * CredentialData dictionary
 * https://w3c.github.io/webappsec-credential-management/#dictdef-credentialdata
 * @typedef {{id: string}}
 */
var CredentialData;

/**
 * PasswordCredentialData used for constructing PasswordCredential objects
 * https://w3c.github.io/webappsec-credential-management/#dictdef-passwordcredentialdata
 * @typedef {{
 *     id: string,
 *     name: string,
 *     iconURL: string,
 *     password: string
 * }}
 */
var PasswordCredentialData;

/**
 * FederatedCredentialInit used for constructing FederatedCredential objects
 * https://w3c.github.io/webappsec-credential-management/#dictdef-federatedcredentialinit
 * @typedef {{
 *     id: string,
 *     name: string,
 *     iconURL: string,
 *     provider: string
 * }}
 */
var FederatedCredentialInit;

/**
 * The CredentialRequestOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#credentialrequestoptions-dictionary
 * @typedef {{mediation: string}}
 */
var CredentialRequestOptions;

/**
 * The FederatedCredentialRequestOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#dictdef-federatedcredentialrequestoptions
 * @typedef {{
 *     providers: !Array<string>,
 *     protocols: !Array<string>
 * }}
 */
var FederatedCredentialRequestOptions;

/**
 * The CredentialCreationOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#credentialcreationoptions-dictionary
 * @typedef {Object}
 */
var CredentialCreationOptions;

/**
 * Serializes a Credential object into simple dictionary to be sent to the
 * browser.
 * @return {Object}
 */
Credential.prototype.serialize = function() {
  var serialized = {
    'id': this.id,
    'type': this.type
  };
  return serialized;
};

/**
 * Serializes a PasswordCredential object into simple dictionary to be sent to
 * the browser.
 * @return {Object}
 */
PasswordCredential.prototype.serialize = function() {
  var serialized = Credential.prototype.serialize.call(this);
  serialized.name = this.name;
  serialized.iconURL = this.iconURL;
  serialized.password = this.password;
  return serialized;
};

/**
 * Serializes a FederatedCredential object into simple dictionary to be sent to
 * the browser.
 * @return {Object}
 */
FederatedCredential.prototype.serialize = function() {
  var serialized = Credential.prototype.serialize.call(this);
  serialized.name = this.name;
  serialized.iconURL = this.iconURL;
  serialized.provider = this.provider;
  serialized.protocol = this.protocol;
  return serialized;
};

/**
 * Implements the public Credential Management API. For more information, see
 * https://w3c.github.io/webappsec-credential-management/#credentialscontainer
 * @constructor
 */
function CredentialsContainer() {}

/**
 * Performs the Request A Credential action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-request-a-credential
 * @param {!CredentialRequestOptions} options An optional dictionary of
 *     parameters for the request.
 * @return {!Promise<?Credential>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.get = function(options) {
  return __gCrWeb['credentialManager'].invokeOnHost_(
      'navigator.credentials.get', options);
};

/**
 * Performs the Store A Credential action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-store-a-credential
 * @param {!Credential} credential A credential object to store.
 * @return {!Promise<void>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.store = function(credential) {
  var serialized_credential = credential.serialize();
  return __gCrWeb['credentialManager'].invokeOnHost_(
      'navigator.credentials.store', serialized_credential);
};

/**
 * Performs the Prevent Silent Access action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-prevent-silent-access
 * @return {!Promise<void>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.preventSilentAccess = function() {
  return __gCrWeb['credentialManager'].invokeOnHost_(
      'navigator.credentials.preventSilentAccess');
};

/**
 * Performs the Create A Credential action described at
 * https://w3c.github.io/webappsec-credential-management/#abstract-opdef-create-a-credential
 * @param {!CredentialCreationOptions} options An optional dictionary of
 *     of params for creating a new Credential object.
 * @return {!Promise<?Credential>} A promise for retrieving the result
 *     of the request.
 */
CredentialsContainer.prototype.create = function(options) {
  // TODO(crbug.com/435046) tgarbus: Implement |create| as JS function
};

/**
 * Install the public interface.
 * @type {!CredentialsContainer}
 */
window.navigator.credentials = new CredentialsContainer();
