// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');
/**
 * @typedef {{
 *   is_default: (boolean | undefined),
 *   custom_display_name: (string | undefined),
 *   custom_display_name_localized: (Array<!{locale: string, value:string}> |
 *                                   undefined),
 *   name: (string | undefined),
 * }}
 */
print_preview_new.SelectOption;

Polymer({
  is: 'print-preview-settings-select',

  properties: {
    /** @type {{ option: Array<!print_preview_new.SelectOption> }} */
    capability: Object,

    selectedValue: {
      type: String,
      notify: true,
      value: '',
    },
  },

  observers: ['onUpdateCapability_(capability)'],

  /**
   * Updates the drop down with the options that are available.
   * @private
   */
  onUpdateCapability_: function() {
    const select = this.$$('select');
    let indexToSelect = select.selectedIndex;
    let selected = false;
    if (!this.capability || !this.capability.option)
      return;  // this section will be hidden anyway.
    this.capability.option.forEach((option, index) => {
      const selectOption = document.createElement('option');
      selectOption.text = this.getDisplayName_(option);
      selectOption.value = JSON.stringify(option);
      select.appendChild(selectOption);
      if (!selected && selectOption.value == this.selectedValue) {
        selected = true;
        indexToSelect = index;
      }
      if (!selected && option.is_default)
        indexToSelect = index;
    });
    select.selectedIndex = indexToSelect;
    this.selectedValue = select.options[indexToSelect].value;
  },

  /**
   * @param {!print_preview_new.SelectOption} option Option to get the display
   *    name for.
   * @return {string} Display name for the option.
   * @private
   */
  getDisplayName_: function(option) {
    let displayName = option.custom_display_name;
    if (!displayName && option.custom_display_name_localized) {
      displayName = getStringForCurrentLocale(
          assert(option.custom_display_name_localized));
    }
    return displayName || option.name || '';
  },
});
