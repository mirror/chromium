// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'site-data' handles showing the local storage summary list for all sites.
 */

/**
 * @typedef {{
 *   site: string,
 *   id: string,
 *   localData: string,
 * }}
 */
var CookieDataSummaryItem;

/**
 * @typedef {{
 *   id: string,
 *   start: number,
 *   count: number,
 * }}
 */
var CookieRemovePacket;

/**
 * TODO(dbeam): upstream to polymer externs?
 * @constructor
 * @extends {Event}
 */
function DomRepeatEvent() {}

/** @type {?} */
DomRepeatEvent.prototype.model;

Polymer({
  is: 'site-data',

  behaviors: [
    I18nBehavior,
    settings.RouteObserverBehavior,
    WebUIListenerBehavior,
  ],

  properties: {
    /**
     * The current filter applied to the cookie data list.
     */
    filter: {
      observer: 'updateSiteList_',
      notify: true,
      type: String,
      value: '',
    },

    /** @type {!Map<string, string>} */
    focusConfig: {
      type: Object,
      observer: 'focusConfigChanged_',
    },
  },

  /** @override */
  ready: function() {
    this.browserProxy_ = settings.LocalDataBrowserProxyImpl.getInstance();
    this.addWebUIListener('onTreeItemRemoved', this.updateSiteList_.bind(this));
  },

  /**
   * Reload cookies when the site data page is visited.
   *
   * settings.RouteObserverBehavior
   * @param {!settings.Route} currentRoute
   * @protected
   */
  currentRouteChanged: function(currentRoute) {
    if (currentRoute == settings.routes.SITE_SETTINGS_SITE_DATA) {
      this.browserProxy_.reloadCookies().then(this.updateSiteList_.bind(this));
    }
  },

  /**
   * Returns the icon to use for a given site.
   * @param {string} url The url of the site to fetch the icon for.
   * @return {string} Value for background-image style.
   * @private
   */
  favicon_: function(url) {
    return cr.icon.getFavicon(url);
  },

  /**
   * @param {!Map<string, string>} newConfig
   * @param {?Map<string, string>} oldConfig
   * @private
   */
  focusConfigChanged_: function(newConfig, oldConfig) {
    // focusConfig is set only once on the parent, so this observer should only
    // fire once.
    assert(!oldConfig);

    // Populate the |focusConfig| map of the parent <settings-animated-pages>
    // element, with additional entries that correspond to subpage trigger
    // elements residing in this element's Shadow DOM.
    if (settings.routes.SITE_SETTINGS_DATA_DETAILS) {
      this.focusConfig.set(
          settings.routes.SITE_SETTINGS_DATA_DETAILS.path,
          '* /deep/ #filter /deep/ #searchInput');
    }
  },

  /**
   * A filter function for the list.
   * @param {!CookieDataSummaryItem} item The item to possibly filter out.
   * @return {boolean} Whether to show the item.
   * @private
   */
  showItem_: function(item) {
    if (this.filter.length == 0)
      return true;
    return item.site.indexOf(this.filter) > -1;
  },

  /**
   * Add elements to the site list until all the sites are listed.
   * @private
   */
  extendSiteList_: function() {
    // A small |count| will have too much IPC overhead. A large |count| may
    // cause UI stutter on low-end machines. Suggested values: 50 to 200.
    // TODO(dschuyler): A more advanced system could perform a TCP-like 'slow
    // start' to find an optimal |count|. Though that may be unnecessary. Let's
    // get some feedback on a simple, arbitrary value.
    let count = 100;  // Arbitrary.
    this.browserProxy_.getDisplayList(this.sites.length, count)
        .then((listInfo) => {
          // The last chunk (terminator) will have no items in the list.
          if (!listInfo.items.length) {
            this.fire('site-data-list-complete');
            return;
          }
          // Don't modify the DOM during idle, do it in next animation frame.
          this.extendSiteListAnimationHandle_ =
              window.requestAnimationFrame(function(listInfo) {
                this.sites = this.sites.concat(listInfo.items);
              }.bind(this, listInfo));
          // Get the next chunk.
          this.extendSiteListHandle_ =
              window.requestIdleCallback(this.extendSiteList_.bind(this));
        });
  },

  /**
   * Kick off a chain of |extendSiteList_| calls to gather all the site data.
   * @private
   */
  updateSiteList_: function() {
    // |browserProxy_| will not be defined if 'onTreeItemRemoved' happens
    // before this.ready().
    if (!this.browserProxy_) {
      return;
    }
    this.browserProxy_.filter(this.filter).then(() => {
      // Cancel pending calls to |extendSiteList_|.
      window.cancelIdleCallback(this.extendSiteListHandle_);
      window.cancelAnimationFrame(this.extendSiteListAnimationHandle_);
      this.sites = [];
      this.extendSiteListHandle_ =
          window.requestIdleCallback(this.extendSiteList_.bind(this));
    });
  },

  /**
   * Returns the string to use for the Remove label.
   * @param {string} filter The current filter string.
   * @return {string}
   * @private
   */
  computeRemoveLabel_: function(filter) {
    if (filter.length == 0)
      return loadTimeData.getString('siteSettingsCookieRemoveAll');
    return loadTimeData.getString('siteSettingsCookieRemoveAllShown');
  },

  /** @private */
  onCloseDialog_: function() {
    this.$.confirmDeleteDialog.close();
  },

  /** @private */
  onConfirmDeleteDialogClosed_: function() {
    cr.ui.focusWithoutInk(assert(this.$.removeShowingSites));
  },

  /**
   * Shows a dialog to confirm the deletion of multiple sites.
   * @param {!Event} e
   * @private
   */
  onRemoveShowingSitesTap_: function(e) {
    e.preventDefault();
    this.$.confirmDeleteDialog.showModal();
  },

  /**
   * Called when deletion for all showing sites has been confirmed.
   * @private
   */
  onConfirmDelete_: function() {
    this.$.confirmDeleteDialog.close();
    if (this.filter.length == 0) {
      this.browserProxy_.removeAll().then(this.updateSiteList_.bind(this));
    } else {
      this.browserProxy_.removeByFilter(this.filter);
      // We just deleted all items found by the filter, let's reset the filter.
      this.fire('clear-subpage-search');
    }
  },

  /**
   * Deletes all site data for a given site.
   * @param {!DomRepeatEvent} e
   * @private
   */
  onRemoveSiteTap_: function(e) {
    e.stopPropagation();
    this.browserProxy_.removeItem(e.model.item.site);
  },

  /**
   * @param {!{model: !{item: CookieDataSummaryItem}}} event
   * @private
   */
  onSiteTap_: function(event) {
    settings.navigateTo(
        settings.routes.SITE_SETTINGS_DATA_DETAILS,
        new URLSearchParams('site=' + event.model.item.site));
  },
});
