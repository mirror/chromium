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
    },

    behaviors: [I18nBehavior],

    listeners: {'sidebar-opened': 'setSelectedBasedOnCurrentPage_'},

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
     * @param {{detail:{pageState: PageState}}} e
     * @private
     */
    setSelectedBasedOnCurrentPage_: function(e) {
      // If 'selected' is already set by something, no need to recalculate.
      if (this.selected_ !== -1)
        return;

      let selected;
      const currentPage = e.detail.pageState;

      switch (currentPage.page) {
        case Page.LIST:
          if (currentPage.type == extensions.ShowingType.APPS)
            selected = 1;
          else
            selected = 0;
          break;
        case Page.SHORTCUTS:
          selected = 2;
          break;
        default:
          selected = -1;
          break;
      }

      this.set('selected_', selected);
    },
  });

  return {Sidebar: Sidebar};
});
