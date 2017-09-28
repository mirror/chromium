// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
Resources.ServiceWorkerFilter = class extends UI.Widget {
  constructor() {
    super(true);
    this._throttler = new Common.Throttler(150);
    this.contentElement.classList.add('sw-filter');
    this.registerRequiredCSS('resources/serviceWorkerFilter.css');
    this._checkboxElement = this.contentElement.createChild('input', 'sw-filter-show-all-checkbox');
    this._checkboxElement.type = 'checkbox';
    this._checkboxElement.setAttribute('id', 'expand-all');
    this._textElement = this.contentElement.createChild('label', 'sw-filter-label link');
    this._textElement.setAttribute('for', 'expand-all');
    this._checkboxElement.addEventListener('change', () => this._onChangeCallback());

    var toolbar = new UI.Toolbar('sw-filter-toolbar', this.contentElement);
    this._filter = new UI.ToolbarInput('Filter', 1);
    this._filter.addEventListener(UI.ToolbarInput.Event.TextChanged, () => this._onChangeCallback());
    toolbar.appendToolbarItem(this._filter);

    this._updateCollapsedStyle();
  }

  /**
   * @param {!SDK.ServiceWorkerRegistration} registration
   * @return {boolean}
   */
  isRegistrationVisible(registration) {
    if (!this._checkboxElement.checked)
      return false;

    var filterString = this._filter.value();
    if (!filterString || !registration.scopeURL)
      return true;

    var regex = String.filterRegex(filterString);
    return registration.scopeURL.match(regex);
  }

  /**
   * @param {string} label
   */
  setLabel(label) {
    this._textElement.textContent = label;
  }

  /**
   * @return {boolean}
   */
  showingAll() {
    return this._checkboxElement.checked;
  }

  _onChangeCallback() {
    this._updateCollapsedStyle();
    this._throttler.schedule(
        () => Promise.resolve(
            this.dispatchEventToListeners(Resources.ServiceWorkerFilter.Events.FilterChanged, this._filter.value())));
  }

  _updateCollapsedStyle() {
    this.contentElement.classList.toggle('sw-filter-collapsed', !this._checkboxElement.checked);
  }
};

/** @enum {symbol} */
Resources.ServiceWorkerFilter.Events = {
  FilterChanged: Symbol('FilterChanged')
};
