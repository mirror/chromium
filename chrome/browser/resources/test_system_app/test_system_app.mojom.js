// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


'use strict';

(function() {
  var mojomId = 'chrome/browser/chromeos/test_system_app/test_system_app.mojom';
  if (mojo.internal.isMojomLoaded(mojomId)) {
    console.warn('The following mojom is loaded multiple times: ' + mojomId);
    return;
  }
  mojo.internal.markMojomLoaded(mojomId);

  // TODO(yzshen): Define these aliases to minimize the differences between the
  // old/new modes. Remove them when the old mode goes away.
  var bindings = mojo;
  var associatedBindings = mojo;
  var codec = mojo.internal;
  var validator = mojo.internal;

  var exports = mojo.internal.exposeNamespace('mojom');



  function TestSystemApp_GetNumber_Params(values) {
    this.initDefaults_();
    this.initFields_(values);
  }


  TestSystemApp_GetNumber_Params.prototype.initDefaults_ = function() {
  };
  TestSystemApp_GetNumber_Params.prototype.initFields_ = function(fields) {
    for(var field in fields) {
        if (this.hasOwnProperty(field))
          this[field] = fields[field];
    }
  };

  TestSystemApp_GetNumber_Params.validate = function(messageValidator, offset) {
    var err;
    err = messageValidator.validateStructHeader(offset, codec.kStructHeaderSize);
    if (err !== validator.validationError.NONE)
        return err;

    var kVersionSizes = [
      {version: 0, numBytes: 8}
    ];
    err = messageValidator.validateStructVersion(offset, kVersionSizes);
    if (err !== validator.validationError.NONE)
        return err;

    return validator.validationError.NONE;
  };

  TestSystemApp_GetNumber_Params.encodedSize = codec.kStructHeaderSize + 0;

  TestSystemApp_GetNumber_Params.decode = function(decoder) {
    var packed;
    var val = new TestSystemApp_GetNumber_Params();
    var numberOfBytes = decoder.readUint32();
    var version = decoder.readUint32();
    return val;
  };

  TestSystemApp_GetNumber_Params.encode = function(encoder, val) {
    var packed;
    encoder.writeUint32(TestSystemApp_GetNumber_Params.encodedSize);
    encoder.writeUint32(0);
  };
  function TestSystemApp_GetNumber_ResponseParams(values) {
    this.initDefaults_();
    this.initFields_(values);
  }


  TestSystemApp_GetNumber_ResponseParams.prototype.initDefaults_ = function() {
    this.number = 0;
  };
  TestSystemApp_GetNumber_ResponseParams.prototype.initFields_ = function(fields) {
    for(var field in fields) {
        if (this.hasOwnProperty(field))
          this[field] = fields[field];
    }
  };

  TestSystemApp_GetNumber_ResponseParams.validate = function(messageValidator, offset) {
    var err;
    err = messageValidator.validateStructHeader(offset, codec.kStructHeaderSize);
    if (err !== validator.validationError.NONE)
        return err;

    var kVersionSizes = [
      {version: 0, numBytes: 16}
    ];
    err = messageValidator.validateStructVersion(offset, kVersionSizes);
    if (err !== validator.validationError.NONE)
        return err;


    return validator.validationError.NONE;
  };

  TestSystemApp_GetNumber_ResponseParams.encodedSize = codec.kStructHeaderSize + 8;

  TestSystemApp_GetNumber_ResponseParams.decode = function(decoder) {
    var packed;
    var val = new TestSystemApp_GetNumber_ResponseParams();
    var numberOfBytes = decoder.readUint32();
    var version = decoder.readUint32();
    val.number = decoder.decodeStruct(codec.Int32);
    decoder.skip(1);
    decoder.skip(1);
    decoder.skip(1);
    decoder.skip(1);
    return val;
  };

  TestSystemApp_GetNumber_ResponseParams.encode = function(encoder, val) {
    var packed;
    encoder.writeUint32(TestSystemApp_GetNumber_ResponseParams.encodedSize);
    encoder.writeUint32(0);
    encoder.encodeStruct(codec.Int32, val.number);
    encoder.skip(1);
    encoder.skip(1);
    encoder.skip(1);
    encoder.skip(1);
  };
  var kTestSystemApp_GetNumber_Name = 0;

  function TestSystemAppPtr(handleOrPtrInfo) {
    this.ptr = new bindings.InterfacePtrController(TestSystemApp,
                                                   handleOrPtrInfo);
  }

  function TestSystemAppAssociatedPtr(associatedInterfacePtrInfo) {
    this.ptr = new associatedBindings.AssociatedInterfacePtrController(
        TestSystemApp, associatedInterfacePtrInfo);
  }

  TestSystemAppAssociatedPtr.prototype =
      Object.create(TestSystemAppPtr.prototype);
  TestSystemAppAssociatedPtr.prototype.constructor =
      TestSystemAppAssociatedPtr;

  function TestSystemAppProxy(receiver) {
    this.receiver_ = receiver;
  }
  TestSystemAppPtr.prototype.getNumber = function() {
    return TestSystemAppProxy.prototype.getNumber
        .apply(this.ptr.getProxy(), arguments);
  };

  TestSystemAppProxy.prototype.getNumber = function() {
    var params = new TestSystemApp_GetNumber_Params();
    return new Promise(function(resolve, reject) {
      var builder = new codec.MessageV1Builder(
          kTestSystemApp_GetNumber_Name,
          codec.align(TestSystemApp_GetNumber_Params.encodedSize),
          codec.kMessageExpectsResponse, 0);
      builder.encodeStruct(TestSystemApp_GetNumber_Params, params);
      var message = builder.finish();
      this.receiver_.acceptAndExpectResponse(message).then(function(message) {
        var reader = new codec.MessageReader(message);
        var responseParams =
            reader.decodeStruct(TestSystemApp_GetNumber_ResponseParams);
        resolve(responseParams);
      }).catch(function(result) {
        reject(Error("Connection error: " + result));
      });
    }.bind(this));
  };

  function TestSystemAppStub(delegate) {
    this.delegate_ = delegate;
  }
  TestSystemAppStub.prototype.getNumber = function() {
    return this.delegate_ && this.delegate_.getNumber && this.delegate_.getNumber();
  }

  TestSystemAppStub.prototype.accept = function(message) {
    var reader = new codec.MessageReader(message);
    switch (reader.messageName) {
    default:
      return false;
    }
  };

  TestSystemAppStub.prototype.acceptWithResponder =
      function(message, responder) {
    var reader = new codec.MessageReader(message);
    switch (reader.messageName) {
    case kTestSystemApp_GetNumber_Name:
      var params = reader.decodeStruct(TestSystemApp_GetNumber_Params);
      this.getNumber().then(function(response) {
        var responseParams =
            new TestSystemApp_GetNumber_ResponseParams();
        responseParams.number = response.number;
        var builder = new codec.MessageV1Builder(
            kTestSystemApp_GetNumber_Name,
            codec.align(TestSystemApp_GetNumber_ResponseParams.encodedSize),
            codec.kMessageIsResponse, reader.requestID);
        builder.encodeStruct(TestSystemApp_GetNumber_ResponseParams,
                             responseParams);
        var message = builder.finish();
        responder.accept(message);
      });
      return true;
    default:
      return false;
    }
  };

  function validateTestSystemAppRequest(messageValidator) {
    var message = messageValidator.message;
    var paramsClass = null;
    switch (message.getName()) {
      case kTestSystemApp_GetNumber_Name:
        if (message.expectsResponse())
          paramsClass = TestSystemApp_GetNumber_Params;
      break;
    }
    if (paramsClass === null)
      return validator.validationError.NONE;
    return paramsClass.validate(messageValidator, messageValidator.message.getHeaderNumBytes());
  }

  function validateTestSystemAppResponse(messageValidator) {
   var message = messageValidator.message;
   var paramsClass = null;
   switch (message.getName()) {
      case kTestSystemApp_GetNumber_Name:
        if (message.isResponse())
          paramsClass = TestSystemApp_GetNumber_ResponseParams;
        break;
    }
    if (paramsClass === null)
      return validator.validationError.NONE;
    return paramsClass.validate(messageValidator, messageValidator.message.getHeaderNumBytes());
  }

  var TestSystemApp = {
    name: 'mojom::TestSystemApp',
    kVersion: 0,
    ptrClass: TestSystemAppPtr,
    proxyClass: TestSystemAppProxy,
    stubClass: TestSystemAppStub,
    validateRequest: validateTestSystemAppRequest,
    validateResponse: validateTestSystemAppResponse,
  };
  TestSystemAppStub.prototype.validator = validateTestSystemAppRequest;
  TestSystemAppProxy.prototype.validator = validateTestSystemAppResponse;
  exports.TestSystemApp = TestSystemApp;
  exports.TestSystemAppPtr = TestSystemAppPtr;
  exports.TestSystemAppAssociatedPtr = TestSystemAppAssociatedPtr;
})();