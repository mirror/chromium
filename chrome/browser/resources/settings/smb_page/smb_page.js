// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Settings page for managing smb features.
 */

Polymer({
  is: 'settings-smb-page',

  properties: {
    /** Preferences state. */
    mountUrl: String,
  },

  mountSmbFileSystem_: function() {
    settings.SmbBrowserProxyImpl.getInstance().smbMount(this.mountUrl);
  },

});