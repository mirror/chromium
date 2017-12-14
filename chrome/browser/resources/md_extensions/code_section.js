// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
function indexOfNthChar(str, char, n, reverse) {
  let count = 0;
  let index = null;

  while (count < n && index !== -1) {
    if (reverse)
      index = str.lastIndexOf(char, index !== null ? index - 1 : undefined);
    else
      index = str.indexOf(char, index + 1);
    count++;
  }

  return index;
}

function lineCount(str) {
  return str.split('\n').length - 1;
}

cr.define('extensions', function() {
  'use strict';

  const CodeSection = Polymer({
    is: 'extensions-code-section',

    properties: {
      /**
       * The code this object is displaying.
       * @type {?chrome.developerPrivate.RequestFileSourceResponse}
       */
      code: {
        type: Object,
        // We initialize to null so that Polymer sees it as defined and calls
        // isMainHidden_().
        value: null,
      },

      /**
       * The text of the entire source file. This value does not update on
       * highlight changes; it only updates if the content of the source
       * changes.
       * @private
       */
      codeText_: String,

      lineNumbers_: String,

      truncatedBefore_: Boolean,
      truncatedAfter_: Boolean,

      /**
       * The string to display if no |code| is set (e.g. because we couldn't
       * load the relevant source file).
       * @type {string}
       */
      couldNotDisplayCode: String,
    },

    observers: [
      'onCodeChanged_(code.*)',
    ],

    /**
     * @return {string}
     * @private
     */
    onCodeChanged_: function() {
      const TOTAL_SHOWN = 1000;

      if (!this.code) {
        this.codeText_ = '';
        this.lineNumbers_ = '';
        return;
      }

      const highlight = this.code.highlight;

      const before = this.code.beforeHighlight;
      const lineCountBefore = lineCount(before);

      const after = this.code.afterHighlight;
      const lineCountAfter = lineCount(after);

      const indexBefore = indexOfNthChar(
          before, '\n', Math.max(TOTAL_SHOWN / 2, TOTAL_SHOWN - lineCountAfter),
          true);
      const indexAfter = indexOfNthChar(
          after, '\n',
          Math.max(TOTAL_SHOWN / 2, TOTAL_SHOWN - lineCountBefore));

      const visibleBefore = before.substring(indexBefore + 1);
      const visibleAfter =
          after.substring(0, indexAfter !== -1 ? indexAfter : undefined);

      this.codeText_ = visibleBefore + highlight + visibleAfter;

      const hiddenLinesBefore = lineCountBefore - lineCount(visibleBefore);
      const hiddenLinesAfter = lineCountAfter - lineCount(visibleAfter);
      const shownLineCount = lineCount(this.codeText_);

      let lineNumbers = '';
      for (let i = hiddenLinesBefore + 1;
           i <= hiddenLinesBefore + shownLineCount + 1; ++i)
        lineNumbers += i + '\n';

      this.lineNumbers_ = lineNumbers;
      this.truncatedBefore_ = hiddenLinesBefore !== 0;
      this.truncatedAfter_ = hiddenLinesAfter !== 0;

      this.createHighlight_(
          visibleBefore.length, visibleBefore.length + highlight.length);
      this.scrollToHighlight_(lineCount(visibleBefore));
    },

    /**
     * Uses the native text-selection API to highlight desired code.
     * @private
     */
    createHighlight_: function(start, end) {
      const range = document.createRange();
      const node = this.$.source.querySelector('span').firstChild;
      range.setStart(node, start);
      range.setEnd(node, end);

      const selection = window.getSelection();
      selection.removeAllRanges();
      selection.addRange(range);
    },

    scrollToHighlight_: function(linesBeforeHighlight) {
      const CSS_LINE_HEIGHT = 20;
      const SCROLL_LOC_THRESHOLD = 100;

      // Count how many pixels is above the highlighted code.
      const highlightTop = linesBeforeHighlight * CSS_LINE_HEIGHT;

      // Find the position to show the highlight roughly in the middle.
      const targetTop = highlightTop - this.clientHeight * 0.5;

      this.$['scroll-container'].scrollTo({top: targetTop});
    },
  });

  return {CodeSection: CodeSection};
});
