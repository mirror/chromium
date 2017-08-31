// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
cr.define('extensions', function() {
  const Sidebar = Polymer({
    is: 'extensions-sidebar',

    properties: {
      /** @private {number} */
      selected_: {
        type: Number,
        value: -1,
      },

      /** @type {extensions.ShowingType} */
      listType: Number,
    },

    behaviors: [I18nBehavior],

    /** @private */
    onExtensionsTap_: function() {
      extensions.navigation.navigateTo(
          {page: Page.LIST, type: extensions.ShowingType.EXTENSIONS});
    },

    /** @private */
    onAppsTap_: function() {
      extensions.navigation.navigateTo(
          {page: Page.LIST, type: extensions.ShowingType.APPS});
    },

    /** @private */
    onKeyboardShortcutsTap_: function() {
      extensions.navigation.navigateTo({page: Page.SHORTCUTS});
    },

    /**
     * @private
     */
    updateSelected_: function() {
      let selected;
      const currentPage = extensions.navigation.getCurrentPage();
      if (currentPage.page == Page.SHORTCUTS) {
        selected = 2;
      } else {
        if (this.listType == extensions.ShowingType.APPS)
          selected = 1;
        else
          selected = 0;
      }

      this.set('selected_', selected);
    },

    close: function() {
      this.$.drawer.closeDrawer();
    },

    toggle: function() {
      this.$.drawer.toggle();
      // If 'selected' is already set by something, no need to recalculate.S
      if (this.$.drawer.open && this.selected_ == -1)
        this.updateSelected_();
    }
  });

  return {Sidebar: Sidebar};
});
