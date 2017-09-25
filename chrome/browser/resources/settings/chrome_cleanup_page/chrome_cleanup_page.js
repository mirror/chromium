// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The reason why the controller is in state kIdle.
 * Must be kept in sync with ChromeCleanerController::IdleReason.
 * @enum {string}
 */
settings.ChromeCleanupIdleReason = {
  INITIAL: 'initial',
  SCANNING_FOUND_NOTHING: 'scanning_found_nothing',
  SCANNING_FAILED: 'scanning_failed',
  CONNECTION_LOST: 'connection_lost',
  USER_DECLINED_CLEANUP: 'user_declined_cleanup',
  CLEANING_FAILED: 'cleaning_failed',
  CLEANING_SUCCEEDED: 'cleaning_succeeded',
};

/**
 * The source of the dismiss action. Used when reporting metrics about how the
 * card was dismissed. The numeric values must be kept in sync with the
 * definition of ChromeCleanerDismissSource in
 * src/chrome/browser/ui/webui/settings/chrome_cleanup_handler.cc.
 * @enum {number}
 */
settings.ChromeCleanupDismissSource = {
  OTHER: 0,
  CLEANUP_SUCCESS_DONE_BUTTON: 1,
  CLEANUP_FAILURE_DONE_BUTTON: 2,
};

settings.ChromeCleanupActionIcons = {
  // Shows a spinner to the left of the title.
  WORKING: 1,

  // Shows an icon to the left of the title.
  REMOVE: 2,
  DONE: 3,
  WARNING: 4,
};

settings.ChromeCleanupActionButtons = {
  REMOVE: 1,
  RESTART_COMPUTER: 2,
  DISMISS_CLEANUP_SUCCESS: 3,
  DISMISS_CLEANUP_FAILURE: 4,
};

settings.ChromeCleanupCardFlags = {
  NONE: 0,
  SHOW_DETAILS: 1 << 0,
  SHOW_LOGS_PERMISSIONS: 1 << 1,
  SHOW_LEARN_MORE: 1 << 2,
};

settings.CardComponents = {
  SCANNING: {
      // No card will be rendered for this state.
  },

  CLEANUP_OFFERED: {
    title_id: 'chromeCleanupTitleRemove',
    icon: settings.ChromeCleanupActionIcons.REMOVE,
    actionButton: settings.ChromeCleanupActionButtons.REMOVE,
    details: settings.ChromeCleanupCardFlags.SHOW_DETAILS |
        settings.ChromeCleanupCardFlags.SHOW_LOGS_PERMISSIONS |
        settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE,
  },

  CLEANING: {
    title_id: 'chromeCleanupTitleRemoving',
    icon: settings.ChromeCleanupActionIcons.WORKING,
    details: settings.ChromeCleanupCardFlags.SHOW_DETAILS |
        settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE,
  },

  REBOOT_REQUIRED: {
    title_id: 'chromeCleanupTitleRestart',
    icon: settings.ChromeCleanupActionIcons.DONE,
    actionButton: settings.ChromeCleanupActionButtons.RESTART_COMPUTER,
  },

  CLEANUP_SUCCEEDED: {
    title_id: 'chromeCleanupTitleRemoved',
    icon: settings.ChromeCleanupActionIcons.DONE,
    actionButton: settings.ChromeCleanupActionButtons.DISMISS_CLEANUP_SUCCESS,
  },

  CLEANING_FAILED: {
    title_id: 'chromeCleanupTitleErrorCantRemove',
    icon: settings.ChromeCleanupActionIcons.WARNING,
    actionButton: settings.ChromeCleanupActionButtons.DISMISS_CLEANUP_FAILURE,
    details: settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE,
  },
};

/**
 * @fileoverview
 * 'settings-chrome-cleanup-page' is the settings page containing Chrome
 * Cleanup settings.
 *
 * Example:
 *
 *    <iron-animated-pages>
 *      <settings-chrome-cleanup-page></settings-chrome-cleanup-page>
 *      ... other pages ...
 *    </iron-animated-pages>
 */
Polymer({
  is: 'settings-chrome-cleanup-page',

  behaviors: [I18nBehavior, WebUIListenerBehavior],

  properties: {
    /** @private */
    title_: {
      type: String,
      value: '',
    },

    /** @private */
    isRemoving_: {
      type: Boolean,
      value: '',
    },

    /** @private */
    showActionButton_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    actionButtonLabel_: {
      type: String,
      value: '',
    },

    /** @private */
    showDetails_: {
      type: Boolean,
      value: false,
    },

    /**
     * Learn more should only be visible for the infected, cleaning and error
     * states.
     * @private
     */
    showLearnMore_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showLogsPermission_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    showFilesToRemove_: {
      type: Boolean,
      value: false,
      observer: 'showFilesToRemoveChanged_',
    },

    /** @private */
    filesToRemove_: {
      type: Array,
      value: [],
    },

    /** @private */
    statusIcon_: {
      type: String,
      value: '',
    },

    /** @private */
    statusIconClassName_: {
      type: String,
      value: '',
    },

    /** @private {chrome.settingsPrivate.PrefObject} */
    logsUploadPref_: {
      type: Object,
      value: function() {
        return /** @type {chrome.settingsPrivate.PrefObject} */ ({});
      },
    },

    /** @private */
    isPartnerPowered_: {
      type: Boolean,
      value: function() {
        return loadTimeData.valueExists('cleanupPoweredByPartner') &&
            loadTimeData.getBoolean('cleanupPoweredByPartner');
      },
    },
  },

  /** @private {?settings.ChromeCleanupProxy} */
  browserProxy_: null,

  /** @private {?function()} */
  doAction_: null,

  /** @override */
  attached: function() {
    this.browserProxy_ = settings.ChromeCleanupProxyImpl.getInstance();

    this.addWebUIListener('chrome-cleanup-on-idle', this.onIdle_.bind(this));
    this.addWebUIListener(
        'chrome-cleanup-on-scanning', this.onScanning_.bind(this));
    this.addWebUIListener(
        'chrome-cleanup-on-infected', this.onInfected_.bind(this));
    this.addWebUIListener(
        'chrome-cleanup-on-cleaning', this.onCleaning_.bind(this));
    this.addWebUIListener(
        'chrome-cleanup-on-reboot-required', this.onRebootRequired_.bind(this));
    this.addWebUIListener(
        'chrome-cleanup-on-dismiss', this.onDismiss_.bind(this));
    this.addWebUIListener(
        'chrome-cleanup-upload-permission-change',
        this.onUploadPermissionChange_.bind(this));
    this.browserProxy_.registerChromeCleanerObserver();
  },

  /**
   * Implements the action for the only visible button in the UI, which can be
   * either to start a cleanup or to restart the computer.
   * @private
   */
  proceed_: function() {
    listenOnce(this, 'transitionend', this.doAction_.bind(this));
  },

  getTopSettingsBoxClass_: function(showDetails) {
    return showDetails ? 'top-aligned-settings-box' : 'two-line';
  },

  /**
   * Toggles the expand button within the element being listened to.
   * @param {!Event} e
   * @private
   */
  toggleExpandButton_: function(e) {
    // The expand button handles toggling itself.
    var expandButtonTag = 'CR-EXPAND-BUTTON';
    if (e.target.tagName == expandButtonTag)
      return;

    /** @type {!CrExpandButtonElement} */
    var expandButton = e.currentTarget.querySelector(expandButtonTag);
    assert(expandButton);
    expandButton.expanded = !expandButton.expanded;
  },

  /**
   * Notify Chrome that the details section was opened or closed.
   * @private
   */
  showFilesToRemoveChanged_: function() {
    if (this.browserProxy_)
      this.browserProxy_.notifyShowDetails(this.showFilesToRemove_);
  },

  /**
   * Notfies Chrome that the "learn more" link was clicked.
   * @private
   */
  learnMore_: function() {
    this.browserProxy_.notifyLearnMoreClicked();
  },

  /**
   * @return {boolean}
   * @private
   */
  showPoweredBy_: function() {
    return this.showFilesToRemove_ && this.isPartnerPowered_;
  },

  /**
   * Listener of event 'chrome-cleanup-on-idle'.
   * @param {number} idleReason
   * @private
   */
  onIdle_: function(idleReason) {
    if (idleReason == settings.ChromeCleanupIdleReason.CLEANING_SUCCEEDED) {
      this.renderCleanupCard_(settings.CardComponents.CLEANUP_SUCCEEDED, []);
    } else if (idleReason == settings.ChromeCleanupIdleReason.INITIAL) {
      this.dismiss_(settings.ChromeCleanupDismissSource.OTHER, []);

      // Not sure if these are needed!
      // this.isRemoving_ = false;
      // this.disableDetails_();
    } else {
      // Scanning-related idle reasons are unexpected. Show an error message for
      // all reasons other than |CLEANING_SUCCEEDED| and |INITIAL|.
      this.renderCleanupCard_(settings.CardComponents.CLEANING_FAILED, []);
    }
  },

  /**
   * Listener of event 'chrome-cleanup-on-scanning'.
   * No UI will be shown in the Settings page on that state, so we simply hide
   * the card and cleanup this element's fields.
   * @private
   */
  onScanning_: function() {
    this.renderCleanupCard_(settings.CardComponents.SCANNING, []);
  },

  /**
   * Listener of event 'chrome-cleanup-on-infected'.
   * Offers a cleanup to the user and enables presenting files to be removed.
   * @param {!Array<!string>} files The list of files to present to the user.
   * @private
   */
  onInfected_: function(files) {
    this.renderCleanupCard_(settings.CardComponents.CLEANUP_OFFERED, files);
  },

  /**
   * Listener of event 'chrome-cleanup-on-cleaning'.
   * Shows a spinner indicating that an on-going action and enables presenting
   * files to be removed.
   * @param {!Array<!string>} files The list of files to present to the user.
   * @private
   */
  onCleaning_: function(files) {
    this.renderCleanupCard_(settings.CardComponents.CLEANING, files);
  },

  /**
   * Listener of event 'chrome-cleanup-on-reboot-required'.
   * No UI will be shown in the Settings page on that state, so we simply hide
   * the card and cleanup this element's fields.
   * @private
   */
  onRebootRequired_: function() {
    this.renderCleanupCard_(settings.CardComponents.REBOOT_REQUIRED, []);
  },

  renderCleanupCard_: function(options, files) {
    this.filesToRemove_ = files;
    this.title_ = options.title_id ? this.i18n(options.title_id) : '';
    this.updateIcon_(options.icon);
    this.updateActionButton_(options.actionButton);
    this.updateCardFlags_(
        options.details ? options.details :
                          settings.ChromeCleanupCardFlags.NONE);
  },

  updateIcon_: function(icon) {
    if (!icon) {
      this.isRemoving_ = false;
      this.resetIcon_();
      return;
    }

    this.isRemoving_ = icon == settings.ChromeCleanupActionIcons.WORKING;

    switch (icon) {
      case settings.ChromeCleanupActionIcons.REMOVE:
        this.setIconRemove_();
        break;

      case settings.ChromeCleanupActionIcons.DONE:
        this.setIconDone_();
        break;

      case settings.ChromeCleanupActionIcons.WARNING:
        this.setIconWarning_();
        break;

      case settings.ChromeCleanupActionIcons.WORKING:
        this.resetIcon_();
        break;

      default:
        // RFC: Is there anything similar to NOTREACHED() in Javascript?
        assert(false);
    }
  },

  updateActionButton_: function(actionButton) {
    if (!actionButton) {
      this.showActionButton_ = false;
      this.actionButtonLabel_ = '';
      this.doAction_ = null;
      return;
    }

    this.showActionButton_ = true;
    switch (actionButton) {
      case settings.ChromeCleanupActionButtons.REMOVE:
        this.actionButtonLabel_ = this.i18n('chromeCleanupRemoveButtonLabel');
        this.doAction_ = this.startCleanup_.bind(this);
        break;

      case settings.ChromeCleanupActionButtons.RESTART_COMPUTER:
        this.actionButtonLabel_ = this.i18n('chromeCleanupRestartButtonLabel');
        this.doAction_ = this.restartComputer_.bind(this);
        break;

      case settings.ChromeCleanupActionButtons.DISMISS_CLEANUP_SUCCESS:
        this.actionButtonLabel_ = this.i18n('chromeCleanupDoneButtonLabel');
        this.doAction_ = this.dismiss_.bind(
            this,
            settings.ChromeCleanupDismissSource.CLEANUP_SUCCESS_DONE_BUTTON);
        break;

      case settings.ChromeCleanupActionButtons.DISMISS_CLEANUP_FAILURE:
        this.actionButtonLabel_ = this.i18n('chromeCleanupDoneButtonLabel');
        this.doAction_ = this.dismiss_.bind(
            this,
            settings.ChromeCleanupDismissSource.CLEANUP_FAILURE_DONE_BUTTON);
        break;

      default:
        // RFC: Is there anything similar to NOTREACHED() in Javascript?
        assert(false);
    }
  },

  updateCardFlags_: function(details) {
    this.showDetails_ =
        (details & settings.ChromeCleanupCardFlags.SHOW_DETAILS) !=
        settings.ChromeCleanupCardFlags.NONE;
    this.showLogsPermission_ =
        (details & settings.ChromeCleanupCardFlags.SHOW_LOGS_PERMISSIONS) !=
        settings.ChromeCleanupCardFlags.NONE;
    this.showLearnMore_ =
        (details & settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE) !=
        settings.ChromeCleanupCardFlags.NONE;
  },

  /**
   * Listener of event 'chrome-cleanup-dismiss'.
   * Hides the Cleanup card.
   * @private
   */
  onDismiss_: function() {
    this.fire('chrome-cleanup-dismissed');
  },

  /**
   * @param {boolean} enabled Whether logs upload is enabled.
   * @private
   */
  onUploadPermissionChange_: function(enabled) {
    this.logsUploadPref_ = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    };
  },

  /**
   * @param {boolean} enabled Whether to enable logs upload.
   * @private
   */
  changeLogsPermission_: function(enabled) {
    var enabled = this.$.chromeCleanupLogsUploadControl.checked;
    this.browserProxy_.setLogsUploadPermission(enabled);
  },

  /**
   * Dismiss the card.
   * @param {number} source
   * @private
   */
  dismiss_: function(source) {
    this.browserProxy_.dismissCleanupPage(source);
  },

  /**
   * Sends an action to the browser proxy to start the cleanup.
   * @private
   */
  startCleanup_: function() {
    this.browserProxy_.startCleanup(
        this.$.chromeCleanupLogsUploadControl.checked);
  },

  /**
   * Sends an action to the browser proxy to restart the machine.
   * @private
   */
  restartComputer_: function() {
    this.browserProxy_.restartComputer();
  },

  /**
   * Sets the card's icon as the cleanup offered indication.
   * @private
   */
  setIconRemove_: function() {
    this.statusIcon_ = 'settings:security';
    this.statusIconClassName_ = 'status-icon-remove';
  },

  /**
   * Sets the card's icon as a warning (in case of failure).
   * @private
   */
  setIconWarning_: function() {
    this.statusIcon_ = 'settings:error';
    this.statusIconClassName_ = 'status-icon-warning';
  },

  /**
   * Sets the card's icon as a completed or reboot required indication.
   * @private
   */
  setIconDone_: function() {
    this.statusIcon_ = 'settings:check-circle';
    this.statusIconClassName_ = 'status-icon-done';
  },

  /**
   * Resets the card's icon.
   * @private
   */
  resetIcon_: function() {
    this.statusIcon_ = '';
    this.statusIconClassName_ = '';
  },
});
