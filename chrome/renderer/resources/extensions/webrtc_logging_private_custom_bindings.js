// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the webrtcLogginPrivate API.

var binding = apiBridge ||
              require('binding').Binding.create('webrtcLoggingPrivate');
var sendRequest = bindingUtil ?
    $Function.bind(bindingUtil.sendRequest, bindingUtil) :
    require('sendRequest').sendRequest;
var fileSystemHelpers = requireNative('file_system_natives');
var GetIsolatedFileSystem = fileSystemHelpers.GetIsolatedFileSystem;
var entryIdManager = require('entryIdManager');
//var entryIdManager = fileBindings.entryIdManager;
var safeCallbackApply = require('uncaught_exception_handler').safeCallbackApply;
// var getFileBindingsForApi =
//     require('fileEntryBindingUtil').getFileBindingsForApi;
// var fileBindings = getFileBindingsForApi('fileSystem');
// var bindFileEntryCallback = fileBindings.bindFileEntryCallback;
// var fileSystemNatives = requireNative('file_system_natives');

binding.registerCustomHook(function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;
//  var fileSystem = bindingsAPI.compiledApi;

 //  function bindFileEntryFunction(functionName) {
 //    apiFunctions.setUpdateArgumentsPostValidate(
 //        functionName, function(fileEntry, callback) {
 //      var fileSystemName = fileEntry.filesystem.name;
 //      var relativePath = $String.slice(fileEntry.fullPath, 1);
 //      return [fileSystemName, relativePath, callback];
 //    });
 //  }
 // $Array.forEach(['startAudioDebugRecordings'],
 //                 function(functionName) {
 //   bindFileEntryCallback(functionName, apiFunctions);
 // });

 apiFunctions.setCustomCallback('startAudioDebugRecordings',
    function(name, request, callback, response) {
    var entries = [];
    var numEntries = response.fileEntries.length;

    var entryLoaded = function(err, entry) {
      if (err) {
        console.error('Error getting fileEntry, code: ' + err.code);
      } else {
        $Array.push(entries, entry);
      }
      if (--numEntries === 0) {
        response.entries = entries;
        safeCallbackApply(name, request, callback, [response]);
      }
    };

    $Array.forEach(response.fileEntries, function(entry) {
      var fs = GetIsolatedFileSystem(entry.fileSystemId);
      if (entry.isDirectory) {
        fs.root.getDirectory(entry.baseName, {}, function(dirEntry) {
          entryIdManager.registerEntry(entry.id, dirEntry);
          entryLoaded(null, {entry: dirEntry});
        }, function(fileError) {
          entryLoaded(fileError);
        });
      } else {
        fs.root.getFile(entry.baseName, {}, function(fileEntry) {
          entryIdManager.registerEntry(entry.id, fileEntry);
          entryLoaded(null, {entry: fileEntry});
        }, function(fileError) {
          entryLoaded(fileError);
        });
      }
    });
  });
});

if (!apiBridge)
  exports.$set('binding', binding.generate());
