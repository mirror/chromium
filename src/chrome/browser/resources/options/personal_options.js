// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('options', function() {

  var OptionsPage = options.OptionsPage;

  // State variables.
  var syncEnabled = false;
  var syncSetupCompleted = false;

  /**
   * Encapsulated handling of personal options page.
   * @constructor
   */
  function PersonalOptions() {
    OptionsPage.call(this, 'personal',
                     templateData.personalPageTabTitle,
                     'personal-page');
  }

  cr.addSingletonGetter(PersonalOptions);

  PersonalOptions.prototype = {
    // Inherit PersonalOptions from OptionsPage.
    __proto__: options.OptionsPage.prototype,

    // Initialize PersonalOptions page.
    initializePage: function() {
      // Call base class implementation to start preference initialization.
      OptionsPage.prototype.initializePage.call(this);

      var self = this;
      $('sync-action-link').onclick = function(event) {
        SyncSetupOverlay.showErrorUI();
      };
      $('start-stop-sync').onclick = function(event) {
        if (self.syncSetupCompleted)
          SyncSetupOverlay.showStopSyncingUI();
        else
          SyncSetupOverlay.showSetupUI();
      };
      $('customize-sync').onclick = function(event) {
        SyncSetupOverlay.showSetupUI();
      };
      $('manage-passwords').onclick = function(event) {
        OptionsPage.navigateToPage('passwords');
        OptionsPage.showTab($('passwords-nav-tab'));
        chrome.send('coreOptionsUserMetricsAction',
            ['Options_ShowPasswordManager']);
      };
      $('autofill-settings').onclick = function(event) {
        OptionsPage.navigateToPage('autofill');
        chrome.send('coreOptionsUserMetricsAction',
            ['Options_ShowAutofillSettings']);
      };
      $('themes-reset').onclick = function(event) {
        chrome.send('themesReset');
      };

      if (!cr.isChromeOS) {
        $('import-data').onclick = function(event) {
          // Make sure that any previous import success message is hidden, and
          // we're showing the UI to import further data.
          $('import-data-configure').hidden = false;
          $('import-data-success').hidden = true;
          OptionsPage.navigateToPage('importData');
          chrome.send('coreOptionsUserMetricsAction', ['Import_ShowDlg']);
        };

        if ($('themes-GTK-button')) {
          $('themes-GTK-button').onclick = function(event) {
            chrome.send('themesSetGTK');
          };
        }
      } else {
        $('change-picture-button').onclick = function(event) {
          OptionsPage.navigateToPage('changePicture');
        };
        chrome.send('loadAccountPicture');

        if (cr.commandLine.options['--bwsi']) {
          // Disable the screen lock checkbox and change-picture-button in
          // guest mode.
          $('enable-screen-lock').disabled = true;
          $('change-picture-button').disabled = true;
        }
      }

      if (PersonalOptions.disablePasswordManagement()) {
        // Disable the Password Manager in guest mode.
        $('passwords-offersave').disabled = true;
        $('passwords-neversave').disabled = true;
        $('passwords-offersave').value = false;
        $('passwords-neversave').value = true;
        $('manage-passwords').disabled = true;
      } else {
        // Otherwise, follow the preference, which can be toggled at runtime.
        // TODO(joaodasilva): remove the radio controls toggles below, once a
        // generic fix for updateElementState_ in pref_ui.js is available.
        // The problem is that updateElementState_ disables controls when a
        // policy enforces the preference, but doesn't re-enable them when the
        // policy is changed or removed. That's because controls can also be
        // disabled for other reasons (e.g. Guest mode, in this case) and
        // removing the policy doesn't mean the control can be enabled again.
        Preferences.getInstance().addEventListener(
            'profile.password_manager_enabled',
            function(event) {
              var managed = event.value && event.value['managed'];
              var value = event.value && event.value['value'] != undefined ?
                  event.value['value'] : event.value;
              $('passwords-offersave').disabled = managed;
              $('passwords-neversave').disabled = managed;
              $('manage-passwords').disabled = managed && !value;
            });
      }

      if (PersonalOptions.disableAutofillManagement()) {
        $('autofill-settings').disabled = true;

        // Disable and turn off autofill.
        var autofillEnabled = $('autofill-enabled');
        autofillEnabled.disabled = true;
        autofillEnabled.checked = false;
        cr.dispatchSimpleEvent(autofillEnabled, 'change');
      } else {
        Preferences.getInstance().addEventListener(
            'autofill.enabled',
            function(event) {
              var managed = event.value && event.value['managed'];
              var value = event.value && event.value['value'] != undefined ?
                  event.value['value'] : event.value;
              $('autofill-settings').disabled = managed && !value;
            });
      }
    },

    setSyncEnabled_: function(enabled) {
      this.syncEnabled = enabled;
    },

    setSyncSetupCompleted_: function(completed) {
      this.syncSetupCompleted = completed;
      $('customize-sync').hidden = !completed;
    },

    setAccountPicture_: function(image) {
      $('account-picture').src = image;
    },

    setSyncStatus_: function(status) {
      var statusSet = status != '';
      $('sync-overview').hidden = statusSet;
      $('sync-status').hidden = !statusSet;
      $('sync-status-text').innerHTML = status;
    },

    setSyncStatusErrorVisible_: function(visible) {
      visible ? $('sync-status').classList.add('sync-error') :
                $('sync-status').classList.remove('sync-error');
    },

    setSyncActionLinkEnabled_: function(enabled) {
      $('sync-action-link').disabled = !enabled;
    },

    setSyncActionLinkLabel_: function(status) {
      $('sync-action-link').textContent = status;

      // link-button does is not zero-area when the contents of the button are
      // empty, so explicitly hide the element.
      $('sync-action-link').hidden = !status.length;
    },

    setProfilesSectionVisible_: function(visible) {
      $('profiles-create').hidden = !visible;
    },

    setNewProfileButtonEnabled_: function(enabled) {
      $('new-profile').disabled = !enabled;
      if (enabled)
        $('profiles-create').classList.remove('disabled');
      else
        $('profiles-create').classList.add('disabled');
    },

    setStartStopButtonVisible_: function(visible) {
      $('start-stop-sync').hidden = !visible;
    },

    setStartStopButtonEnabled_: function(enabled) {
      $('start-stop-sync').disabled = !enabled;
    },

    setStartStopButtonLabel_: function(label) {
      $('start-stop-sync').textContent = label;
    },

    setGtkThemeButtonEnabled_: function(enabled) {
      if (!cr.isChromeOS && navigator.platform.match(/linux|BSD/i)) {
        $('themes-GTK-button').disabled = !enabled;
      }
    },

    setThemesResetButtonEnabled_: function(enabled) {
      $('themes-reset').disabled = !enabled;
    },

    hideSyncSection_: function() {
      $('sync-section').hidden = true;
    },

    /**
     * Toggles the visibility of the data type checkboxes based on whether they
     * are enabled on not.
     * @param {Object} dict A mapping from data type to a boolean indicating
     *     whether it is enabled.
     * @private
     */
    setRegisteredDataTypes_: function(dict) {
      for (var type in dict) {
        if (type.match(/Registered$/) && !dict[type]) {
          node = $(type.replace(/([a-z]+)Registered$/i, '$1').toLowerCase()
                   + '-check');
          if (node)
            node.parentNode.style.display = 'none';
        }
      }
    },
  };

  /**
   * Returns whether the user should be able to manage (view and edit) their
   * stored passwords. Password management is disabled in guest mode.
   * @return {boolean} True if password management should be disabled.
   */
  PersonalOptions.disablePasswordManagement = function() {
    return cr.commandLine.options['--bwsi'];
  };

  /**
   * Returns whether the user should be able to manage autofill settings.
   * @return {boolean} True if password management should be disabled.
   */
  PersonalOptions.disableAutofillManagement = function() {
    return cr.commandLine.options['--bwsi'];
  };

  // Forward public APIs to private implementations.
  [
    'setSyncEnabled',
    'setSyncSetupCompleted',
    'setAccountPicture',
    'setSyncStatus',
    'setSyncStatusErrorVisible',
    'setSyncActionLinkEnabled',
    'setSyncActionLinkLabel',
    'setProfilesSectionVisible',
    'setNewProfileButtonEnabled',
    'setStartStopButtonVisible',
    'setStartStopButtonEnabled',
    'setStartStopButtonLabel',
    'setGtkThemeButtonEnabled',
    'setThemesResetButtonEnabled',
    'hideSyncSection',
    'setRegisteredDataTypes',
  ].forEach(function(name) {
    PersonalOptions[name] = function(value) {
      PersonalOptions.getInstance()[name + '_'](value);
    };
  });

  // Export
  return {
    PersonalOptions: PersonalOptions
  };

});
