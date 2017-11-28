// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  'use strict';

  const OptionsDialog = Polymer({
    is: 'extensions-options-dialog',

    behaviors: [extensions.ItemBehavior],

    properties: {
      /** @private {Object} */
      extensionOptions_: Object,

      /** @private {chrome.developerPrivate.ExtensionInfo} */
      data_: Object,
    },

    get open() {
      return this.$$('dialog').open;
    },

    /** @param {chrome.developerPrivate.ExtensionInfo} data */
    show: function(data) {
      console.log('-----show 0');
      this.data_ = data;
      if (!this.extensionOptions_)
        this.extensionOptions_ = document.createElement('ExtensionOptions');
      console.log('-----show 1');
      this.extensionOptions_.extension = this.data_.id;
      this.extensionOptions_.onclose = this.close.bind(this);

      console.log('-----show 2');
      const onSizeChanged = e => {
        console.log('-----show 6');
        this.extensionOptions_.style.height = e.height + 'px';
        this.extensionOptions_.style.width = e.width + 'px';

        console.log('-----show 7');
        if (!this.$$('dialog').open) {
          console.log('-----show 8');
          this.$$('dialog').showModal();
          console.log('-----show 9');
        }
        console.log('-----show 10');
      };

      console.log('-----show 3');
      this.extensionOptions_.onpreferredsizechanged = onSizeChanged;
      console.log('-----show 4');
      this.$.body.appendChild(this.extensionOptions_);
      console.log('-----show 5');
    },

    close: function() {
      this.$$('dialog').close();
      this.extensionOptions_.onpreferredsizechanged = null;
    },

    /** @private */
    onClose_: function() {
      const currentPage = extensions.navigation.getCurrentPage();
      // We update the page when the options dialog closes, but only if we're
      // still on the details page. We could be on a different page if the
      // user hit back while the options dialog was visible; in that case, the
      // new page is already correct.
      if (currentPage && currentPage.page == Page.DETAILS) {
        // This will update the currentPage_ and the NavigationHelper; since
        // the active page is already the details page, no main page
        // transition occurs.
        extensions.navigation.navigateTo(
            {page: Page.DETAILS, extensionId: currentPage.extensionId});
      }
    },
  });

  return {OptionsDialog: OptionsDialog};
});
