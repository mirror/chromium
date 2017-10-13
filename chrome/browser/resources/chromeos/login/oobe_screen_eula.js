// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Oobe eula screen implementation.
 */

login.createScreen('EulaScreen', 'eula', function() {
  var CONTEXT_KEY_USAGE_STATS_ENABLED = 'usageStatsEnabled';

  /**
   * Load text/html contents from the given url into the given webview. Loaded
   * data is set to webview via data url so that it is properly sandboxed.
   *
   * @param {!WebView} webview Webview element to host the terms.
   * @param {!string} url URL to load terms contents.
   */
  function loadUrlToWebview(webview, url) {
    assert(webview.tagName === 'WEBVIEW');

    var onError = function() {
      webview.src = 'about:blank';
    };

    var setContents = function(contents) {
      webview.src =
          'data:text/html;charset=utf-8,' + encodeURIComponent(contents);
    };

    var xhr = new XMLHttpRequest();
    xhr.open('GET', url);
    xhr.setRequestHeader('Accept', 'text/html');
    xhr.onreadystatechange = function() {
      if (xhr.readyState != 4)
        return;
      if (xhr.status != 200) {
        onError();
        return;
      }

      var contentType = xhr.getResponseHeader('Content-Type');
      if (contentType && !/text\/html/.test(contentType)) {
        onError();
        return;
      }

      setContents(xhr.response);
    };
    xhr.ontimeout = onError;

    try {
      xhr.send();
    } catch (e) {
      onError();
    }
  }

  return {
    /**
     * Tracks OEM Eula url so that it could be properly reloaded.
     * @type {?string}
     */
    oemEulaUrl_: null,

    /** @override */
    decorate: function() {
      $('eula-chrome-credits-link').hidden = true;
      $('eula-chromeos-credits-link').hidden = true;
      $('stats-help-link').addEventListener('click', function(event) {
        chrome.send('eulaOnLearnMore');
      });
      $('installation-settings-link')
          .addEventListener('click', function(event) {
            chrome.send('eulaOnInstallationSettingsPopupOpened');
            $('popup-overlay').hidden = false;
            $('installation-settings-ok-button').focus();
          });
      $('installation-settings-ok-button')
          .addEventListener('click', function(event) {
            $('popup-overlay').hidden = true;
          });

      $('cros-eula-frame')
          .addEventListener('contentload', this.onFrameLoad.bind(this));

      var self = this;
      $('usage-stats').addEventListener('click', function(event) {
        self.onUsageStatsClicked_($('usage-stats').checked);
        event.stopPropagation();
      });
      $('oobe-eula-md').screen = this;
    },

    /**
     * Event handler for $('usage-stats') click event.
     * @param {boolean} value $('usage-stats').checked value.
     */
    onUsageStatsClicked_: function(value) {
      this.context.set(CONTEXT_KEY_USAGE_STATS_ENABLED, value);
      this.commitContextChanges();
    },

    /**
     * Event handler that is invoked when 'chrome://terms' is loaded.
     */
    onFrameLoad: function() {
      $('accept-button').disabled = false;
      $('eula').classList.remove('eula-loading');
      // Initially, the back button is focused and the accept button is
      // disabled.
      // Move the focus to the accept button now but only if the user has not
      // moved the focus anywhere in the meantime.
      if (!$('back-button').blurred)
        $('accept-button').focus();
    },

    /**
     * Event handler that is invoked just before the screen is shown.
     * @param {object} data Screen init payload.
     */
    onBeforeShow: function() {
      $('eula').classList.add('eula-loading');
      $('accept-button').disabled = true;
      this.updateLocalizedContent();
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return loadTimeData.getString('eulaScreenTitle');
    },

    /**
     * Buttons in oobe wizard's button strip.
     * @type {Array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];

      var backButton = this.declareButton('back-button');
      backButton.textContent = loadTimeData.getString('back');
      buttons.push(backButton);

      var acceptButton = this.declareButton('accept-button');
      acceptButton.disabled = true;
      acceptButton.classList.add('preserve-disabled-state');
      acceptButton.textContent = loadTimeData.getString('acceptAgreement');
      acceptButton.addEventListener('click', function(e) {
        $('eula').classList.add('loading');  // Mark EULA screen busy.
        Oobe.clearErrors();
        e.stopPropagation();
      });
      buttons.push(acceptButton);

      return buttons;
    },

    /**
     * Returns a control which should receive an initial focus.
     */
    get defaultControl() {
      if (loadTimeData.getString('newOobeUI') == 'on')
        return $('oobe-eula-md');

      return $('accept-button').disabled ? $('back-button') :
                                           $('accept-button');
    },

    enableKeyboardFlow: function() {
      $('eula-chrome-credits-link').hidden = false;
      $('eula-chromeos-credits-link').hidden = false;
      $('eula-chrome-credits-link').addEventListener('click', function(event) {
        chrome.send('eulaOnChromeCredits');
      });
      $('eula-chromeos-credits-link')
          .addEventListener('click', function(event) {
            chrome.send('eulaOnChromeOSCredits');
          });
    },

    /**
     * This method takes care of switching to material-design OOBE.
     * @private
     */
    setMDMode_: function() {
      var useMDOobe = (loadTimeData.getString('newOobeUI') == 'on');
      $('oobe-eula-md').hidden = !useMDOobe;
      $('oobe-eula').hidden = useMDOobe;
    },

    /**
     * Updates localized content of the screen that is not updated via template.
     */
    updateLocalizedContent: function() {
      this.setMDMode_();

      // Reload the terms contents.
      if (!$('oobe-eula-md').hidden)
        $('oobe-eula-md').updateLocalizedContent();

      if (!$('oobe-eula').hidden) {
        this.loadEulaToWebview_($('cros-eula-frame'));
        if (this.oemEulaUrl_)
          loadUrlToWebview($('oem-eula-frame'), this.oemEulaUrl_);
      }
    },

    /**
     * Sets url for OEM Eula. Oem Eula UI is hidden if the url is null or empty.
     * @param {?string} oemEulaUrl The URL for OEM Eula.
     */
    setOemEulaUrl: function(oemEulaUrl) {
      this.oemEulaUrl_ = oemEulaUrl;

      if (this.oemEulaUrl_) {
        loadUrlToWebview($('oem-eula-frame'), this.oemEulaUrl_);
        $('eulas').classList.remove('one-column');
      } else {
        $('eulas').classList.add('one-column');
      }
    },

    /**
     * Load terms contents into the given webview. The contents is fetched via
     * XHR then put into a data url to pass to the webview. Webview is used as
     * a sandbox for both online and local contents and data url is used so that
     * webview never needs to have the privileged webui bindings.
     *
     * @param {!WebView} webview Webview element to host the terms.
     */
    loadEulaToWebview_: function(webview) {
      assert(webview.tagName === 'WEBVIEW');

      /** @type {string} */ var TERMS_URL = 'chrome://terms';
      loadUrlToWebview(webview, TERMS_URL);
    },
  };
});
