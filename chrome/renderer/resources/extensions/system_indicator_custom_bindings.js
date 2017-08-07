// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the systemIndicator API.
// TODO(dewittj) Refactor custom binding to reduce redundancy between the
// extension action APIs.

var binding = apiBridge || require('binding').Binding.create('systemIndicator');

var setIcon = require('setIcon').setIcon;
var sendRequest = bindingUtil ?
    bindingUtil.sendRequest.bind(bindingUtil) : require('sendRequest').sendRequest;

binding.registerCustomHook(function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;

  apiFunctions.setHandleRequest('setIcon', function(details, callback) {
    setIcon(details, function(args) {
      sendRequest('systemIndicator.setIcon', [args, callback],
                  apiBridge ? undefined : this.definition.parameters, null);
    }.bind(this));
  });
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
