// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

/**
 * A menu to show and to control the visibility of the data series in current
 * line chart.
 * @const
 */
LineChart.Menu = class {
  constructor(/** function(): undefined */ callback) {
    /**
     * Handle the menu status changed event, include clicking button, hiding or
     * showing the menu.
     * @const {function(): undefined}
     */
    this.callback_ = callback;

    /** @const {Element} - Root of the menu. */
    this.rootDiv_ = createElementWithClassName('div', 'line-chart-menu');

    /** @const {Element} - Container of buttons. */
    this.buttonOuterDiv_ =
        createElementWithClassName('div', 'line-chart-menu-button-outer');
    this.rootDiv_.appendChild(this.buttonOuterDiv_);

    /** @const {Element} - Handle to control the visibility of menu. */
    this.handleDiv_ =
        createElementWithClassName('div', 'line-chart-menu-handle');
    this.rootDiv_.appendChild(this.handleDiv_);
    this.handleDiv_.onclick = this.handleOnClick_.bind(this);

    /** @type {Array<LineChart.DataSeries>} */
    this.dataSeries_ = [];

    /** @type {Array<Element>} - Buttons of data series. */
    this.buttons_ = [];
  }

  /**
   * Handle menu showing and hiding.
   * @this {LineChart.Menu}
   */
  handleOnClick_() {
    const /** string|null */ hiddenAttr =
        this.buttonOuterDiv_.getAttribute('hidden');
    if (hiddenAttr == null) {
      this.buttonOuterDiv_.setAttribute('hidden', '');
    } else {
      this.buttonOuterDiv_.removeAttribute('hidden');
    }
    this.callback_();
  }

  /**
   * Return the root div of menu.
   * @return {Element}
   */
  getRootDiv() {
    return this.rootDiv_;
  }

  /**
   * Return the width of menu.
   * @return {number}
   */
  getWidth() {
    return this.rootDiv_.offsetWidth;
  }

  /**
   * A a data series to the menu.
   * @param {LineChart.DataSeries} dataSeries
   */
  addDataSeries(dataSeries) {
    const /** Element */ button = this.createButton_(dataSeries);
    this.buttons_.push(button);
    this.buttonOuterDiv_.appendChild(button);
    this.dataSeries_.push(dataSeries);

    /* Width may change. */
    this.callback_();
  }

  /**
   * Create a button to control the data series.
   * @param {LineChart.DataSeries} dataSeries
   * @return {Element}
   */
  createButton_(dataSeries) {
    const /** Element */ buttonInner =
        createElementWithClassName('span', 'line-chart-menu-button-inner-span');
    buttonInner.innerText = dataSeries.getTitle();
    const /** Element */ button =
        createElementWithClassName('div', 'line-chart-menu-button');
    button.appendChild(buttonInner);
    this.setupButtonOnClickHandler_(button, dataSeries);

    const /** boolean */ visible = dataSeries.isVisible();
    this.updateButtonStyle_(button, dataSeries, visible);
    return button;
  }

  /**
   * Add a onclick handler to the button.
   * @param {Element} button
   * @param {LineChart.DataSeries} dataSeries
   */
  setupButtonOnClickHandler_(button, dataSeries) {
    const /** Element */ buttonConst = button;
    const /** LineChart.DataSeries */ dataSeriesConst = dataSeries;
    const /** function(Event): undefined */ handler = function(event) {
      const /** boolean */ visible = !dataSeriesConst.isVisible();
      dataSeriesConst.setVisible(visible);
      this.updateButtonStyle_(buttonConst, dataSeriesConst, visible);
      this.callback_();
    }.bind(this);
    button.onclick = handler;
  }

  /**
   * Update the button style with the visibility of data series.
   * @param {Element} button
   * @param {LineChart.DataSeries} dataSeries
   * @param {boolean} visible
   */
  updateButtonStyle_(button, dataSeries, visible) {
    if (visible) {
      button.style.backgroundColor = dataSeries.getColor();
      const /** string */ color =
          dataSeries.isMenuTextBlack() ? '#333' : '#e9e9e9';
      button.style.color = color;
    } else {
      button.style.backgroundColor = '#e9e9e9';
      button.style.color = '#333';
    }
  }

  /**
   * Remove a data series from the menu.
   * @param {LineChart.DataSeries} dataSeries
   */
  removeDataSeries(dataSeries) {
    const /** number */ idx = this.dataSeries_.indexOf(dataSeries);
    if (idx == -1)
      return;
    this.dataSeries_.splice(idx, 1);
    const /** Element */ button = this.buttons_.splice(idx, 1)[0];
    this.buttonOuterDiv_.removeChild(button);
    /* Width may change. */
    this.callback_();
  }
};

})();
