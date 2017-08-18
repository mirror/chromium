function ExposeInterfaceToWindow(interfaceObj) {
  Object.defineProperty(window, interfaceObj.name, {
    configurable: true,
    enumerable: false,
    writable: true,
    value: interfaceObj
  });
}

function ThrowIfNotInstanceOf(instance, classObj) {
  if (!(instance instanceof classObj)) {
    throw new TypeError('Error');
  }
}

function MarkPropertyEnumerableInPrototype(classObj, name) {
  Object.defineProperty(classObj.prototype, name, { enumerable: true });
}

/**
 * The Credential interface, for more information see
 * https://w3c.github.io/webappsec-credential-management/#the-credential-interface
 * @constructor
 */
class Credential {
  constructor() {
    this.id;
    this.type;
  }

  get [Symbol.toStringTag]() {
    return 'Credential';
  }
};
ExposeInterfaceToWindow(Credential);

class PasswordCredential extends Credential {
  constructor(dataOrForm) {
    super();
    this.id = dataOrForm.id;
    this._password = dataOrForm.password;
  }

  get [Symbol.toStringTag]() {
    return 'PasswordCredential';
  }

  get password() {
    // [13] required so that accessing the property directly on the prototype
    // throws TypeError.
    ThrowIfNotInstanceOf(this, PasswordCredential);
    return this._password;
  }
};
ExposeInterfaceToWindow(PasswordCredential);
MarkPropertyEnumerableInPrototype(PasswordCredential, 'password');

/**
 * FederatedCredential interface, for more information see
 * https://w3c.github.io/webappsec-credential-management/#federatedcredential-interface
 * @param {FederatedCredentialInit} init Dictionary to create
 *     FederatedCredential from.
 * @extends {Credential}
 * @constructor
 */
class FederatedCredential extends Credential {
  constructor(data) {
    super();
    this._provider = data.provider;
    this._protocol = data.protocol;
  }

  get [Symbol.toStringTag]() {
    return 'FederatedCredential';
  }

  get provider() {
    ThrowIfNotInstanceOf(this, FederatedCredential);
    return this._provider;
  }

  get protocol() {
    ThrowIfNotInstanceOf(this, FederatedCredential);
    return this._protocol;
  }
};
ExposeInterfaceToWindow(FederatedCredential);
MarkPropertyEnumerableInPrototype(FederatedCredential, 'provider');
MarkPropertyEnumerableInPrototype(FederatedCredential, 'protocol');

/**
 * CredentialData dictionary
 * https://w3c.github.io/webappsec-credential-management/#dictdef-credentialdata
 * @dict
 * @typedef {{id: string}}
 */
var CredentialData;

/**
 * PasswordCredentialData used for constructing PasswordCredential objects
 * https://w3c.github.io/webappsec-credential-management/#dictdef-passwordcredentialdata
 * @dict
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
 * @dict
 * @typedef {{
 *     id: string,
 *     name: string,
 *     iconURL: string,
 *     provider: string,
 *     protocol: string
 * }}
 */
var FederatedCredentialInit;

/**
 * The CredentialRequestOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#credentialrequestoptions-dictionary
 * @dict
 * @typedef {{mediation: string}}
 */
var CredentialRequestOptions;

/**
 * The FederatedCredentialRequestOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#dictdef-federatedcredentialrequestoptions
 * @dict
 * @typedef {{
 *     providers: !Array<string>,
 *     protocols: !Array<string>
 * }}
 */
var FederatedCredentialRequestOptions;

/**
 * The CredentialCreationOptions dictionary, for more information see
 * https://w3c.github.io/webappsec-credential-management/#credentialcreationoptions-dictionary
 * @dict
 * @typedef {Object}
 */
var CredentialCreationOptions;

/**
 * Implements the public Credential Management API. For more information, see
 * https://w3c.github.io/webappsec-credential-management/#credentialscontainer
 */
// [5] This can't be a function because function properties are
// {configurable: false, enumerable: true, writable: true}, and we need
// {configurable: true, enumerable: false, writable: true}.
// [6] class doesn't work because it makes window.credentials type be function
// instead of object. But window.credentials can't be an instance of
// CredentialsContainer, because this interface needs to be non initializable.
// [7] It has to be a class because interface object is expected to be functions.
class CredentialsContainer {
  constructor(internal=false) {
    // [9] Allow construction of an object as navigator.credentials, but don't
    // allow anyone else to use it.
    if (!internal) {
      // [8] needed because this interface doesn't have a constructor
      throw new TypeError('This is an interface without constructor');
    }
  }

  // [12] This is how to coerce Object.prototype.toString.call(obj) to return
  // our own class name. See
  // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Symbol/toStringTag
  get [Symbol.toStringTag]() {
    return 'CredentialsContainer';
  }

  // undefined is critical to mark this as an optional argument. Otherwise,
  // DWCrednetialsContainer.prototype.get.length returns 1.
  get(options=undefined) {
    return new Promise(function(resolve, reject) {});
  }

  store(credential) {
    return new Promise(function(resolve, reject) {});
  }

  preventSilentAccess() {
    return new Promise(function(resolve, reject) {});
  }

  create(options=undefined) {
    return new Promise(function(resolve, reject) {});
  }
};
ExposeInterfaceToWindow(CredentialsContainer);

// [1] This doesn't work because it makes CredentialsContainer an unconfigurable object.
//var CredentialsContainer = {
//  get(options) {},
//
//  store(credential) {},
//
//  preventSilentAccess() {},
//
//  create(options) {}
//};

// [3] This doesn't work because get() needs to be defined on the prototype.
// See https://github.com/w3c/web-platform-tests/blob/fed14eca94f6b0ee96a5efe9ef77c73cd05cba19/resources/idlharness.js#L1741
//Object.defineProperty(CredentialsContainer, 'get', { enumerable: true});
//CredentialsContainer.get = function(options) {}

// [4] This doesn't work because Class.prototype is a readonly property.
//CredentialsContainer.prototype = {
//  get: function(options) {},
//  store: function(credential) {},
//  preventSilentAccess: function() {},
//  create: function(options) {}
//};

// [2] These are needed because ES6 methods are not enumerable.
MarkPropertyEnumerableInPrototype(CredentialsContainer, 'get');
MarkPropertyEnumerableInPrototype(CredentialsContainer, 'store');
MarkPropertyEnumerableInPrototype(CredentialsContainer, 'preventSilentAccess');
MarkPropertyEnumerableInPrototype(CredentialsContainer, 'create');

/**
 * Install the public interface.
 * @type {!CredentialsContainer}
 */
window.navigator.dwcredentials = new CredentialsContainer(true);
