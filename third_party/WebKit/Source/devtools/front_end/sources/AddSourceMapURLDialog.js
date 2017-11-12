// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @unrestricted
 */
Sources.AddSourceMapURLDialog = class extends UI.Widget {
  /**
   * @param {function(string)} callback
   */
  constructor(callback) {
    super(true);
    this._input = UI.createInput();
    this.contentElement.appendChild(this._input);
    this._input.setAttribute('type', 'text');
    this._input.addEventListener('keydown', this._onKeyDown.bind(this), false);
    this._input.placeholder = Common.UIString('Source map URL');
    this._input.style.margin = '10px 5px 10px 10px';

    var addButton = UI.createTextButton(Common.UIString('Add'), this._apply.bind(this), '', true);
    addButton.style.marginRight = '5px';
    this.contentElement.appendChild(addButton);

    this.setDefaultFocusedElement(this._input);
    this._callback = callback;
  }

  /**
   * @param {function(string)} callback
   */
  static show(callback) {
    var dialog = new UI.Dialog();
    var addSourceMapURLDialog = new Sources.AddSourceMapURLDialog(done);
    addSourceMapURLDialog.show(dialog.contentElement);
    dialog.setSizeBehavior(UI.GlassPane.SizeBehavior.MeasureContent);
    dialog.show();
    addSourceMapURLDialog.focus();

    /**
     * @param {string} value
     */
    function done(value) {
      dialog.hide();
      callback(value);
    }
  }

  _apply() {
    this._callback(this._input.value);
  }

  /**
   * @param {!Event} event
   */
  _onKeyDown(event) {
    if (event.keyCode === UI.KeyboardShortcut.Keys.Enter.code) {
      event.preventDefault();
      this._apply();
    }
  }
};
