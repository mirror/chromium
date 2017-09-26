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

/**
 * Boolean properties for a cleanup card state.
 * @enum {number}
 */
settings.ChromeCleanupCardFlags = {
  NONE: 0,
  SHOW_DETAILS: 1 << 0,
  SHOW_LOGS_PERMISSIONS: 1 << 1,
  SHOW_LEARN_MORE: 1 << 2,
  IS_REMOVING: 1 << 3,
};

/**
 * @typedef {{
 *   statusIcon: !string,
 *   statusIconClassName: !string,
 * }}
 */
settings.ChromeCleanupCardIcon;

/**
 * @typedef {{
 *   label: !string,
 *   doAction: !function(),
 * }}
 */
settings.ChromeCleanupCardActionButton;

/**
 * @typedef {{
 *   title: (undefined|?string),
 *   icon: (undefined|?settings.ChromeCleanupCardIcon),
 *   actionButton: (undefined|?settings.ChromeCleanupCardActionButton),
 *   flags: (undefined|?settings.ChromeCleanupCardFlags),
 * }}
 */
settings.ChromeCleanupCardComponents;

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
    filesToRemoveListExpanded_: {
      type: Boolean,
      value: false,
      observer: 'filesToRemoveListExpandedChanged_',
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
    this.card_components_ = this.getCardComponents_();

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
  filesToRemoveListExpandedChanged_: function() {
    if (this.browserProxy_)
      this.browserProxy_.notifyShowDetails(this.filesToRemoveListExpanded_);
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
    return this.filesToRemoveListExpanded_ && this.isPartnerPowered_;
  },

  /**
   * Listener of event 'chrome-cleanup-on-idle'.
   * @param {number} idleReason
   * @private
   */
  onIdle_: function(idleReason) {
    if (idleReason == settings.ChromeCleanupIdleReason.CLEANING_SUCCEEDED) {
      this.renderCleanupCard_(this.card_components_.CLEANUP_SUCCEEDED, []);
    } else if (idleReason == settings.ChromeCleanupIdleReason.INITIAL) {
      this.dismiss_(settings.ChromeCleanupDismissSource.OTHER);
    } else {
      // Scanning-related idle reasons are unexpected. Show an error message for
      // all reasons other than |CLEANING_SUCCEEDED| and |INITIAL|.
      this.renderCleanupCard_(this.card_components_.CLEANING_FAILED, []);
    }
  },

  /**
   * Listener of event 'chrome-cleanup-on-scanning'.
   * No UI will be shown in the Settings page on that state, so we simply hide
   * the card and cleanup this element's fields.
   * @private
   */
  onScanning_: function() {
    this.renderCleanupCard_(this.card_components_.HIDDEN_CARD, []);
  },

  /**
   * Listener of event 'chrome-cleanup-on-infected'.
   * Offers a cleanup to the user and enables presenting files to be removed.
   * @param {!Array<!string>} files The list of files to present to the user.
   * @private
   */
  onInfected_: function(files) {
    this.renderCleanupCard_(this.card_components_.CLEANUP_OFFERED, files);
  },

  /**
   * Listener of event 'chrome-cleanup-on-cleaning'.
   * Shows a spinner indicating that an on-going action and enables presenting
   * files to be removed.
   * @param {!Array<!string>} files The list of files to present to the user.
   * @private
   */
  onCleaning_: function(files) {
    this.renderCleanupCard_(this.card_components_.CLEANING, files);
  },

  /**
   * Listener of event 'chrome-cleanup-on-reboot-required'.
   * No UI will be shown in the Settings page on that state, so we simply hide
   * the card and cleanup this element's fields.
   * @private
   */
  onRebootRequired_: function() {
    this.renderCleanupCard_(this.card_components_.REBOOT_REQUIRED, []);
  },

  /**
   * Renders the cleanup card given components and list of files.
   * @param{!settings.ChromeCleanupCardComponents} components The card
   *     components to be rendered.
   * @param{!Array<!string>} files The list of files to present to the user.
   * @private
   */
  renderCleanupCard_: function(components, files) {
    this.filesToRemove_ = files;
    this.title_ = components.title ? components.title : '';
    this.updateIcon_(components.icon);
    this.updateActionButton_(components.actionButton);
    this.updateCardFlags_(
        components.flags ? components.flags :
                           settings.ChromeCleanupCardFlags.NONE);
  },

  /**
   * Updates the icon on the cleanup card to show the current state.
   * @param{?settings.ChromeCleanupCardIcon} icon The icon to render.
   * @private
   */
  updateIcon_: function(icon) {
    if (!icon) {
      this.statusIcon_ = '';
      this.statusIconClassName_ = '';
    } else {
      this.statusIcon_ = icon.statusIcon;
      this.statusIconClassName_ = icon.statusIconClassName;
    }
  },

  /**
   * Updates the icon on the cleanup card to show the current state.
   * @param{?settings.ChromeCleanupCardActionButton} actionButton The button to
   *     add to the card.
   * @private
   */
  updateActionButton_: function(actionButton) {
    if (!actionButton) {
      this.showActionButton_ = false;
      this.actionButtonLabel_ = '';
      this.doAction_ = null;
    } else {
      this.showActionButton_ = true;
      this.actionButtonLabel_ = actionButton.label;
      this.doAction_ = actionButton.doAction;
    }
  },

  /**
   * Updates boolean flags corresponding to optional components to be rendered
   * on the card.
   * @param{!settings.ChromeCleanupCardFlags} flags Flags indicating optional
   *     components to be rendered.
   * @private
   */
  updateCardFlags_: function(flags) {
    this.showDetails_ =
        (flags & settings.ChromeCleanupCardFlags.SHOW_DETAILS) !=
        settings.ChromeCleanupCardFlags.NONE;
    this.showLogsPermission_ =
        (flags & settings.ChromeCleanupCardFlags.SHOW_LOGS_PERMISSIONS) !=
        settings.ChromeCleanupCardFlags.NONE;
    this.showLearnMore_ =
        (flags & settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE) !=
        settings.ChromeCleanupCardFlags.NONE;
    this.isRemoving_ = (flags & settings.ChromeCleanupCardFlags.IS_REMOVING) !=
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
    this.renderCleanupCard_({/* clear card contents */}, []);
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
   * Returns the map of card states to components to be rendered.
   * @return {Object<string, settings.ChromeCleanupCardComponents>}
   * @private
   */
  getCardComponents_: function() {
    /**
     * The icons to show on the card.
     * @enum {settings.ChromeCleanupCardIcon}
     */
    var icons = {
      // Card's icon indicates a cleanup offer.
      REMOVE: {
        statusIcon: 'settings:security',
        statusIconClassName: 'status-icon-remove',
      },

      // Card's icon indicates a warning (in case of failure).
      WARNING: {
        statusIcon: 'settings:error',
        statusIconClassName: 'status-icon-warning',
      },

      // Card's icon indicates completion or reboot required.
      DONE: {
        statusIcon: 'settings:check-circle',
        statusIconClassName: 'status-icon-done',
      },
    };

    /**
     * The action buttons to show on the card.
     * @enum {settings.ChromeCleanupCardActionButton}
     */
    var actionButtons = {
      REMOVE: {
        label: this.i18n('chromeCleanupRemoveButtonLabel'),
        doAction: this.startCleanup_.bind(this),
      },

      RESTART_COMPUTER: {
        label: this.i18n('chromeCleanupRestartButtonLabel'),
        doAction: this.restartComputer_.bind(this),
      },

      DISMISS_CLEANUP_SUCCESS: {
        label: this.i18n('chromeCleanupDoneButtonLabel'),
        doAction: this.dismiss_.bind(
            this,
            settings.ChromeCleanupDismissSource.CLEANUP_SUCCESS_DONE_BUTTON),
      },

      DISMISS_CLEANUP_FAILURE: {
        label: this.i18n('chromeCleanupDoneButtonLabel'),
        doAction: this.dismiss_.bind(
            this,
            settings.ChromeCleanupDismissSource.CLEANUP_FAILURE_DONE_BUTTON),
      },
    };

    return {
      HIDDEN_CARD: {
          // No card rendered for this state.
      },

      CLEANUP_OFFERED: {
        title: this.i18n('chromeCleanupTitleRemove'),
        icon: icons.REMOVE,
        actionButton: actionButtons.REMOVE,
        flags: settings.ChromeCleanupCardFlags.SHOW_DETAILS |
            settings.ChromeCleanupCardFlags.SHOW_LOGS_PERMISSIONS |
            settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE,
      },

      CLEANING: {
        title: this.i18n('chromeCleanupTitleRemoving'),
        flags: settings.ChromeCleanupCardFlags.SHOW_DETAILS |
            settings.ChromeCleanupCardFlags.IS_REMOVING |
            settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE,
      },

      REBOOT_REQUIRED: {
        title: this.i18n('chromeCleanupTitleRestart'),
        icon: icons.DONE,
        actionButton: actionButtons.RESTART_COMPUTER,
      },

      CLEANUP_SUCCEEDED: {
        title: this.i18n('chromeCleanupTitleRemoved'),
        icon: icons.DONE,
        actionButton: actionButtons.DISMISS_CLEANUP_SUCCESS,
      },

      CLEANING_FAILED: {
        title: this.i18n('chromeCleanupTitleErrorCantRemove'),
        icon: icons.WARNING,
        actionButton: actionButtons.DISMISS_CLEANUP_FAILURE,
        flags: settings.ChromeCleanupCardFlags.SHOW_LEARN_MORE,
      },
    };
  },
});
