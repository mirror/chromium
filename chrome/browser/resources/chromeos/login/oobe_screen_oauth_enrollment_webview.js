// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

login.createScreen('OAuthEnrollmentScreen', 'oauth-enrollment', function() {
  /** @const */ var STEP_SIGNIN = 'signin';
  /** @const */ var STEP_WORKING = 'working';
  /** @const */ var STEP_ATTRIBUTE_PROMPT = 'attribute-prompt';
  /** @const */ var STEP_ERROR = 'error';
  /** @const */ var STEP_SUCCESS = 'success';

  /** @const */ var HELP_TOPIC_ENROLLMENT = 4631259;

  return {
    EXTERNAL_API: [
      'showStep',
      'showError',
      'doReload',
      'showAttributePromptStep',
    ],

    /**
     * Authenticator object that wraps GAIA webview.
     */
    authenticator_: null,

    /**
     * The current step. This is the last value passed to showStep().
     */
    currentStep_: null,

    /**
     * We block esc, back button and cancel button until gaia is loaded to
     * prevent multiple cancel events.
     */
    isCancelDisabled_: null,

    get isCancelDisabled() { return this.isCancelDisabled_ },
    set isCancelDisabled(disabled) {
      if (disabled == this.isCancelDisabled)
        return;
      this.isCancelDisabled_ = disabled;
    },

    /** @override */
    decorate: function() {
      var webview = document.createElement('webview');
      webview.id = webview.name = 'oauth-enroll-auth-view';
      webview.classList.toggle('oauth-enroll-focus-on-signin', true);
      $('oauth-enroll-auth-view-container').appendChild(webview);
      this.authenticator_ = new cr.login.Authenticator(webview);

      this.authenticator_.addEventListener('ready',
          (function() {
            if (this.currentStep_ != STEP_SIGNIN)
              return;
            this.isCancelDisabled = false;
            chrome.send('frameLoadingCompleted', [0]);
          }).bind(this));

      this.authenticator_.addEventListener('authCompleted',
          (function(e) {
            var detail = e.detail;
            if (!detail.email || !detail.authCode) {
              this.showError(
                  loadTimeData.getString('fatalEnrollmentError'),
                  false);
              return;
            }
            chrome.send('oauthEnrollCompleteLogin', [detail.email,
                                                     detail.authCode]);
          }).bind(this));

      this.authenticator_.addEventListener('authFlowChange',
          (function(e) {
            var isSAML = this.authenticator_.authFlow ==
                             cr.login.Authenticator.AuthFlow.SAML;
            if (isSAML) {
              $('oauth-saml-notice-message').textContent =
                  loadTimeData.getStringF('samlNotice',
                                          this.authenticator_.authDomain);
            }
            this.classList.toggle('saml', isSAML);
            if (Oobe.getInstance().currentScreen === this)
              Oobe.getInstance().updateScreenSize(this);
          }).bind(this));

      this.authenticator_.addEventListener('backButton',
          (function(e) {
            $('oauth-enroll-back-button').hidden = !e.detail;
          }).bind(this));

      this.authenticator_.insecureContentBlockedCallback =
          (function(url) {
            this.showError(
                loadTimeData.getStringF('insecureURLEnrollmentError', url),
                false);
          }).bind(this);

      this.authenticator_.missingGaiaInfoCallback =
          (function() {
            this.showError(
                loadTimeData.getString('fatalEnrollmentError'),
                false);
          }).bind(this);

      $('oauth-enroll-error-retry').addEventListener('click',
                                                     this.doRetry_.bind(this));
      $('oauth-enroll-cancel-button').addEventListener('click',
                                                       this.cancel.bind(this));

      $('oauth-enroll-back-button').addEventListener('click',
          (function(e) {
            $('oauth-enroll-back-button').hidden = true;
            $('oauth-enroll-auth-view').back();
            e.preventDefault();
          }).bind(this));
    },

    /**
     * Header text of the screen.
     * @type {string}
     */
    get header() {
      return loadTimeData.getString('oauthEnrollScreenTitle');
    },

    /**
     * Buttons in oobe wizard's button strip.
     * @type {array} Array of Buttons.
     */
    get buttons() {
      var buttons = [];
      var ownerDocument = this.ownerDocument;

      function makeButton(id, classes, label, handler) {
        var button = ownerDocument.createElement('button');
        button.id = id;
        button.classList.add('oauth-enroll-button');
        button.classList.add.apply(button.classList, classes);
        button.textContent = label;
        button.addEventListener('click', handler);
        buttons.push(button);
      }

      makeButton(
          'oauth-enroll-done-button',
          ['oauth-enroll-focus-on-success'],
          loadTimeData.getString('oauthEnrollDone'),
          function() {
            chrome.send('oauthEnrollClose', ['done']);
          });

      makeButton(
          'oauth-enroll-continue-button',
          ['oauth-enroll-focus-on-attribute-prompt'],
          loadTimeData.getString('oauthEnrollContinue'),
          function() {
            chrome.send('oauthEnrollAttributes',
                       [$('oauth-enroll-asset-id').value,
                        $('oauth-enroll-location').value]);
          });

      return buttons;
    },

    /**
     * Event handler that is invoked just before the frame is shown.
     * @param {Object} data Screen init payload, contains the signin frame
     * URL.
     */
    onBeforeShow: function(data) {
      var gaiaParams = {};
      gaiaParams.gaiaUrl = data.gaiaUrl;
      gaiaParams.gaiaPath = 'embedded/setup/chromeos';
      gaiaParams.isNewGaiaFlowChromeOS = true;
      gaiaParams.needPassword = false;
      if (data.management_domain) {
        gaiaParams.enterpriseDomain = data.management_domain;
        gaiaParams.emailDomain = data.management_domain;
      }
      gaiaParams.flow = 'enterprise';
      this.authenticator_.load(cr.login.Authenticator.AuthMode.DEFAULT,
                               gaiaParams);

      var modes = ['manual', 'forced', 'recovery'];
      for (var i = 0; i < modes.length; ++i) {
        this.classList.toggle('mode-' + modes[i],
                              data.enrollment_mode == modes[i]);
      }
      this.isCancelDisabled = true;
      this.showStep(STEP_SIGNIN);
    },

    /**
     * Shows attribute-prompt step with pre-filled asset ID and
     * location.
     */
    showAttributePromptStep: function(annotated_asset_id, annotated_location) {
      /**
       * innerHTML is used instead of textContent,
       * because oauthEnrollAttributes contains text formatting (bold, italic).
       */
      $('oauth-enroll-attribute-prompt-message').innerHTML =
          loadTimeData.getString('oauthEnrollAttributes');

      $('oauth-enroll-asset-id').value = annotated_asset_id;
      $('oauth-enroll-location').value = annotated_location;

      this.showStep(STEP_ATTRIBUTE_PROMPT);
    },

    /**
     * Cancels enrollment and drops the user back to the login screen.
     */
    cancel: function() {
      if (this.isCancelDisabled)
        return;
      this.isCancelDisabled = true;
      chrome.send('oauthEnrollClose', ['cancel']);
    },

    /**
     * Returns whether |step| is the device attribute update error or
     * not.
     */
    isAttributeUpdateError: function(step) {
      return this.currentStep_ == STEP_ATTRIBUTE_PROMPT && step == STEP_ERROR;
    },

    /**
     * Shows cancel or done button.
     */
    hideCancelShowDone: function(hide) {
      $('oauth-enroll-cancel-button').hidden = hide;
      $('oauth-enroll-cancel-button').disabled = hide;

      $('oauth-enroll-done-button').hidden = !hide;
      $('oauth-enroll-done-button').disabled = !hide;
    },

    /**
     * Switches between the different steps in the enrollment flow.
     * @param {string} step the steps to show, one of "signin", "working",
     * "attribute-prompt", "error", "success".
     */
    showStep: function(step) {
      var focusStep = step;

      this.classList.toggle('oauth-enroll-state-' + this.currentStep_, false);
      this.classList.toggle('oauth-enroll-state-' + step, true);

      /**
       * In case of device attribute update error done button should be shown
       * instead of cancel button and focus on success,
       * because enrollment has completed
       */
      if (this.isAttributeUpdateError(step)) {
        focusStep = STEP_SUCCESS;
        this.hideCancelShowDone(true);
      } else {
        if (step == STEP_ERROR)
          this.hideCancelShowDone(false);
      }

      var focusElements =
          this.querySelectorAll('.oauth-enroll-focus-on-' + focusStep);
      for (var i = 0; i < focusElements.length; ++i) {
        if (getComputedStyle(focusElements[i])['display'] != 'none') {
          focusElements[i].focus();
          break;
        }
      }
      this.currentStep_ = step;
    },

    /**
     * Sets an error message and switches to the error screen.
     * @param {string} message the error message.
     * @param {boolean} retry whether the retry link should be shown.
     */
    showError: function(message, retry) {
      $('oauth-enroll-error-message').textContent = message;
      $('oauth-enroll-error-retry').hidden = !retry;
      this.showStep(STEP_ERROR);
    },

    doReload: function() {
      this.authenticator_.reload();
    },

    /**
     * Retries the enrollment process after an error occurred in a previous
     * attempt. This goes to the C++ side through |chrome| first to clean up the
     * profile, so that the next attempt is performed with a clean state.
     */
    doRetry_: function() {
      chrome.send('oauthEnrollRetry');
    }
  };
});

