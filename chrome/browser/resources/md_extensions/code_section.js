// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function halfOrFill(oppositeCount) {
  const VISIBLE_LOC = 1000;
  return Math.max(VISIBLE_LOC / 2, VISIBLE_LOC - oppositeCount);
}

function nthLineIndex(str, n) {
  let index = str.indexOf('\n');
  while (--n > 0 && index !== -1)
    index = str.indexOf('\n', index + 1);
  return index == -1 ? str.length : index;
}

function nthLineLastIndex(str, n) {
  let index = str.lastIndexOf('\n');
  while (--n > 0 && index !== -1)
    index = str.lastIndexOf('\n', index - 1);
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
        value: null,
      },

      /**
       * The text of the entire source file. This value does not update on
       * highlight changes; it only updates if the content of the source
       * changes.
       * @private
       */
      codeText_: String,

      /** @private */
      lineNumbers_: String,

      /** @private */
      truncatedBefore_: Number,

      /** @private */
      truncatedAfter_: Number,

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
     * @private
     */
    onCodeChanged_: function() {
      if (!this.code) {
        this.codeText_ = '';
        this.lineNumbers_ = '';
        return;
      }

      const before = this.code.beforeHighlight;
      const highlight = this.code.highlight;
      const after = this.code.afterHighlight;

      const linesBefore = lineCount(before);
      const linesAfter = lineCount(after);

      const snipIndexBefore = nthLineLastIndex(before, halfOrFill(linesAfter));
      const snipIndexAfter = nthLineIndex(after, halfOrFill(linesBefore));

      const visibleBefore = before.substring(snipIndexBefore + 1);
      const visibleAfter = after.substring(0, snipIndexAfter);

      this.codeText_ = visibleBefore + highlight + visibleAfter;

      this.truncatedBefore_ = linesBefore - lineCount(visibleBefore);
      this.truncatedAfter_ = linesAfter - lineCount(visibleAfter);

      this.setLineNumbers_(
          this.truncatedBefore_ + 1,
          this.truncatedBefore_ + lineCount(this.codeText_) + 1);
      this.createHighlight_(
          visibleBefore.length, visibleBefore.length + highlight.length);
      this.scrollToHighlight_(lineCount(visibleBefore));
    },

    /**
     * @param {number} start
     * @param {number} end
     * @private
     */
    setLineNumbers_: function(start, end) {
      let lineNumbers = '';
      for (let i = start; i <= end; ++i)
        lineNumbers += i + '\n';

      this.lineNumbers_ = lineNumbers;
    },

    /**
     * Uses the native text-selection API to highlight desired code.
     * @param {number} start
     * @param {number} end
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

    /**
     * @param {number} linesBeforeHighlight
     * @private
     */
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
