// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  const AppOrExtensionBehavior = {
    /**
     * @param {chrome.developerPrivate.ExtensionType} type
     * @param {string} appLabel
     * @param {string} extensionLabel
     * @return {string} The app or extension label depending on |type|.
     */
    appOrExtension: function(type, appLabel, extensionLabel) {
      return (type == chrome.developerPrivate.ExtensionType.EXTENSION) ?
          extensionLabel :
          appLabel;
    },
  };

  return {AppOrExtensionBehavior: AppOrExtensionBehavior};
});
