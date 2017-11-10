// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'welcome-win10',

  properties: {
    // Determines if the combined variant should be displayed. The combined
    // variant includes instructions on how to pin Chrome to the taskbar.
    isCombined: Boolean,

    // Indicates if the accelerated flow is enabled.
    isAccelerated: Boolean,
  },

  receivePinnedState_: function(isPinnedToTaskbar) {
    // Allow overriding of the result via a query parameter.
    // TODO(pmonette): Remove these checks when they are no longer needed.
    /** @const */
    var VARIANT_KEY = 'variant';
    var VariantTypeMap = {'defaultonly': false, 'combined': true};
    var params = new URLSearchParams(location.search);
    if (params.has(VARIANT_KEY) && params.get(VARIANT_KEY) in VariantTypeMap)
      this.isCombined = VariantTypeMap[params.get(VARIANT_KEY)];
    else
      this.isCombined = !isPinnedToTaskbar;

    // Show the module.
    this.style.opacity = 1;
  },

  ready: function() {
    this.isCombined = false;

    /** @const */
    this.isAccelerated = false;
    var FLOWTYPE_KEY = 'flowtype';
    var FlowTypeMap = {'regular': false, 'accelerated': true};
    var params = new URLSearchParams(location.search);
    if (params.has(FLOWTYPE_KEY) && params.get(FLOWTYPE_KEY) in FlowTypeMap) {
      this.isAccelerated = FlowTypeMap[params.get(FLOWTYPE_KEY)];

      // Ajust the height since the accelerated flow contains fewer steps.
      this.customStyle['--expandable-section-height'] = '26.375em';
      this.updateStyles();
    }

    // Asynchronously check if Chrome is pinned to the taskbar.
    cr.sendWithPromise('getPinnedToTaskbarState')
        .then(this.receivePinnedState_.bind(this));
  },

  computeClasses: function(isCombined) {
    return isCombined ? 'section expandable expanded' : 'section';
  },

  onContinue: function() {
    chrome.send('handleContinue');
  },

  onOpenSettings: function() {
    chrome.send('handleSetDefaultBrowser');
  },

  onToggle: function() {
    if (!this.isCombined)
      return;
    var sections = this.shadowRoot.querySelectorAll('.section.expandable');
    sections.forEach(function(section) {
      var isExpanded = section.classList.toggle('expanded');
      section.querySelector('[role~="button"]')
          .setAttribute('aria-expanded', isExpanded);
    });
  }
});
