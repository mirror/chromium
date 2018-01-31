// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-list-item',

  properties: {
    /** @type {!print_preview.Destination} */
    destination: Object,

    /** @type {?RegExp} */
    searchQuery: Object,
  },

  listeners: {
    'dom-change': 'highlightFields_',
  },

  /**
   * @return {!Function}
   * @private
   */
  computeFilter_: function() {
    return !!this.searchQuery ? property => property.match(this.searchQuery) :
                                property => {
                                  return false;
                                };
  },

  /** @private */
  highlightFields_: function() {
    cr.search_highlight_utils.findAndRemoveHighlights(this);
    if (!this.searchQuery)
      return;

    this.shadowRoot.querySelectorAll('.searchable').forEach(element => {
      element.childNodes.forEach(node => {
        if (node.nodeType != Node.TEXT_NODE)
          return;

        const textContent = node.nodeValue.trim();
        if (textContent.length == 0)
          return;

        if (this.searchQuery.test(textContent)) {
          // Don't highlight <select> nodes, yellow rectangles can't be
          // displayed within an <option>.
          // TODO(rbpotter): solve issue below before adding advanced
          // settings so that this function can be re-used.
          // TODO(dpapad): highlight <select> controls with a search bubble
          // instead.
          if (node.parentNode.nodeName != 'OPTION') {
            cr.search_highlight_utils.highlight(
                node, textContent.split(this.searchQuery));
          }
        }
      });
    });
  },
});
