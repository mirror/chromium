// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
cr.define('extensions', function() {
  const Sidebar = Polymer({
    is: 'extensions-sidebar',

    hostAttributes: {
      role: 'navigation',
    },

    /** @override */
    attached: function() {
      this.$.sectionMenu.select(
          extensions.navigation.getCurrentPage().page == Page.SHORTCUTS ? 1 :
                                                                          0);
    },

    /**
     * @param {!Event} e
     * @private
     */
    onLinkTap_: function(e) {
      e.preventDefault();
      extensions.navigation.navigateTo({page: e.target.dataset.path});
      // Using 'cancel' instead of 'close' so it would animate.
      this.fire('cancel');
    }
  });

  return {Sidebar: Sidebar};
});
