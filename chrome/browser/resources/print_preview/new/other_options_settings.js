// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');
/**
 * @typedef {{
 *   isHidden: boolean,
 *   displayName: string,
 *   id: string,
 *   settingName: string,
 *   checked: boolean,
 * }}
 */
print_preview_new.OptionCheckbox;

Polymer({
  is: 'print-preview-other-options-settings',

  behaviors: [SettingsBehavior, I18nBehavior],

  properties: {
    /** @private {!Array<!print_preview_new.OptionCheckbox>} */
    options_: {
      type: Array,
      computed: 'computeOptions_(' +
          'settings.headerFooter.available, settings.duplex.available, ' +
          'settings.cssBackground.available, settings.rasterize.available, ' +
          'settings.selectionOnly.available)',
    },
  },

  observers: [
    'onSettingChange_(settings.headerFooter.value, ' +
        'settings.duplex.value, ' +
        'settings.cssBackground.value, ' +
        'settings.rasterize.value, ' +
        'settings.selectionOnly.value)',
  ],

  /**
   * @private {!Array<string>}
   * @const
   */
  settingNames_:
      ['headerFooter', 'duplex', 'cssBackground', 'rasterize', 'selectionOnly'],

  /**
   * @private {!Array<string>}
   * @const
   */
  ids_: [
    'header-footer', 'duplex', 'css-background', 'rasterize', 'selection-only'
  ],

  /**
   * @return {!Array<!print_preview_new.OptionCheckbox>}
   * @private
   */
  computeOptions_: function() {
    const options = Array(this.settingNames_.length);
    const displayNames = [
      'optionHeaderFooter', 'optionTwoSided', 'optionBackgroundColorsAndImages',
      'optionRasterize', 'optionSelectionOnly'
    ];
    this.settingNames_.forEach((name, index) => {
      options[index] = {
        isHidden: !this.getSetting(name).available,
        displayName: this.i18n(displayNames[index]),
        id: this.ids_[index],
        settingName: name,
        checked: this.getSetting(name).value,
      };
    });
    return options;
  },

  /** @private */
  onSettingChange_: function() {
    this.settingNames_.forEach((name, index) => {
      const checkbox = this.$$('#' + this.ids_[index]);
      if (checkbox)  // May not be defined yet if this is the first update.
        checkbox.checked = this.getSetting(name).value;
    });
  },

  /**
   * @param {!{model : {item: print_preview_new.OptionCheckbox} }} e
   *     Event containing the item that changed to trigger the event.
   * @private
   */
  onCheckboxChange_: function(e) {
    const item = e.model.item;
    this.setSetting(item.settingName, this.$$('#' + item.id).checked);
  },
});
