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

    this._filter = this.contentElement.createChild('div', 'sw-filter-input');

    this._filterInput = this._filter.createChild('input');
    this._filterInput.addEventListener('focus', () => this._filterInput.classList.add('focused'));
    this._filterInput.addEventListener('blur', () => this._filterInput.classList.remove('focused'));
    this._filterInput.addEventListener('input', () => this._onChangeCallback(), false);
    this._filterInput.setAttribute('placeholder', Common.UIString('Filter'));

    var clearButton = this._filter.createChild('div', 'sw-filter-clear-button');
    clearButton.appendChild(UI.Icon.create('mediumicon-gray-cross-hover', 'search-cancel-button'));
    clearButton.addEventListener('click', () => this._internalSetValue('', true));
    this._filterInput.addEventListener('keydown', event => this._onKeydownCallback(event));

    this._updateStyles();
  }

  /**
   * @param {!SDK.ServiceWorkerRegistration} registration
   * @return {boolean}
   */
  isRegistrationVisible(registration) {
    if (!this._checkboxElement.checked)
      return false;

    var filterString = this._filterInput.value;
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

  /**
   * @param {string} value
   * @param {boolean} notify
   */
  _internalSetValue(value, notify) {
    this._filterInput.value = value;
    if (notify)
      this._onChangeCallback();
    this._updateStyles();
  }

  /**
   * @param {!Event} event
   */
  _onKeydownCallback(event) {
    if (!isEscKey(event) || !this._filterInput.value)
      return;
    this._internalSetValue('', true);
    event.consume(true);
  }

  _onChangeCallback() {
    this._updateStyles();
    this._throttler.schedule(
        () => Promise.resolve(this.dispatchEventToListeners(
            Resources.ServiceWorkerFilter.Events.FilterChanged, this._filterInput.value)));
  }

  _updateStyles() {
    this.contentElement.classList.toggle('sw-filter-collapsed', !this._checkboxElement.checked);
    this._filter.classList.toggle('sw-filter-input-empty', !this._filterInput.value);
  }
};

/** @enum {symbol} */
Resources.ServiceWorkerFilter.Events = {
  FilterChanged: Symbol('FilterChanged')
};
