// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

class Menu_ {
  constructor(callback) {
    this.callback_ = callback;
    this.rootDiv_ = createElementWithClassName('div', 'line-chart-menu');
    this.buttonOuterDiv_ =
        createElementWithClassName('div', 'line-chart-menu-button-outer');
    this.handleDiv_ =
        createElementWithClassName('div', 'line-chart-menu-handle');
    this.dataSeries_ = [];
    this.buttons_ = [];

    this.rootDiv_.appendChild(this.buttonOuterDiv_);
    this.rootDiv_.appendChild(this.handleDiv_);
    this.handleDiv_.onclick = this.handleOnClick_.bind(this);
  }

  getWidth() {
    return this.rootDiv_.offsetWidth;
  }

  handleOnClick_() {
    const hiddenAttr = this.buttonOuterDiv_.getAttribute('hidden');
    if (hiddenAttr == null) {
      this.buttonOuterDiv_.setAttribute('hidden', '');
    } else {
      this.buttonOuterDiv_.removeAttribute('hidden');
    }
    this.callback_();
  }

  updateButtonStyle_(button, dataSeries, visible) {
    if (visible) {
      button.style.backgroundColor = dataSeries.getColor();
      if (!dataSeries.isMenuTextBlack()) {
        button.style.color = '#e9e9e9';
      }
    } else {
      button.style.backgroundColor = '#e9e9e9';
      button.style.color = '#333';
    }
  }

  setupButtonOnClickHandler_(button, dataSeries) {
    const dataSeriesConst = dataSeries;
    const buttonConst = button;
    button.onclick = function(event) {
      const visible = !dataSeriesConst.isVisible();
      dataSeriesConst.setVisible(visible);
      this.updateButtonStyle_(buttonConst, dataSeriesConst, visible);
      this.callback_();
    }.bind(this);
  }

  getRootDiv() {
    return this.rootDiv_;
  }

  createButton_(dataSeries) {
    const buttonInner =
        createElementWithClassName('span', 'line-chart-menu-button-inner-span');
    buttonInner.innerText = dataSeries.getTitle();
    const button = createElementWithClassName('div', 'line-chart-menu-button');
    button.appendChild(buttonInner);
    this.updateButtonStyle_(button, dataSeries, true);
    this.setupButtonOnClickHandler_(button, dataSeries);
    return button;
  }

  addDataSeries(dataSeries) {
    const button = this.createButton_(dataSeries);
    this.buttons_.push(button);
    this.buttonOuterDiv_.appendChild(button);
    this.dataSeries_.push(dataSeries);
    this.callback_();  // size may change
  }

  removeDataSeries(dataSeries) {
    const idx = this.dataSeries_.indexOf(dataSeries);
    if (idx == -1)
      return;
    this.dataSeries_.splice(idx, 1);
    const button = this.buttons_.splice(idx, 1)[0];
    this.buttonOuterDiv_.removeChild(button);
    this.callback_();  // size may change
  }
}

/**
 * @const
 */
LineChart.Menu = Menu_;
})();