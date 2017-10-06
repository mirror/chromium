// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
/** Amount that each level of bookmarks is indented by (px). */
var BOOKMARK_INDENT = 20;

/**
 * The |title| is the text label displayed for the bookmark. |page| indicates
 * which 0-based page that bookmark leads to. |y| is the y position in that
 * page, in pixel coordinates, where the bookmark leads to. |children| is an
 * array of the |Bookmark|s that are below this in a table of contents tree
 * structure.
 * @typedef {{
 *   title: string,
 *   page: number,
 *   y: number,
 *   children: BookmarkList
 * }}
 */
var Bookmark;

/**
 * An array of |Bookmark|s.
 * @typedef {!Array<!Bookmark>}
 */
var BookmarkList;

Polymer({
  is: 'viewer-bookmark',

  properties: {
    /** @type {!Bookmark} */
    bookmark: {type: Object, observer: 'bookmarkChanged_'},

    depth: {type: Number, observer: 'depthChanged'},

    childDepth: Number,

    childrenShown: {type: Boolean, reflectToAttribute: true, value: false},

    keyEventTarget: {
      type: Object,
      value: function() {
        return this.$.item;
      }
    }
  },

  behaviors: [Polymer.IronA11yKeysBehavior],

  keyBindings: {'enter': 'onEnter_', 'space': 'onSpace_'},

  bookmarkChanged_: function() {
    this.$.expand.style.visibility =
        this.bookmark.children.length > 0 ? 'visible' : 'hidden';
  },

  depthChanged: function() {
    this.childDepth = this.depth + 1;
    this.$.item.style.webkitPaddingStart =
        (this.depth * BOOKMARK_INDENT) + 'px';
  },

  onClick: function() {
    if (this.bookmark.hasOwnProperty('page')) {
      if (this.bookmark.hasOwnProperty('y')) {
        this.fire(
            'change-page-and-y',
            {page: this.bookmark.page, y: this.bookmark.y});
      } else {
        this.fire('change-page', {page: this.bookmark.page});
      }
    } else if (this.bookmark.hasOwnProperty('uri')) {
      this.fire('navigate', {uri: this.bookmark.uri, newtab: true});
    }
  },

  onEnter_: function(e) {
    // Don't allow events which have propagated up from the expand button to
    // trigger a click.
    if (e.detail.keyboardEvent.target != this.$.expand)
      this.onClick();
  },

  onSpace_: function(e) {
    // paper-icon-button stops propagation of space events, so there's no need
    // to check the event source here.
    this.onClick();
    // Prevent default space scroll behavior.
    e.detail.keyboardEvent.preventDefault();
  },

  toggleChildren: function(e) {
    this.childrenShown = !this.childrenShown;
    e.stopPropagation();  // Prevent the above onClick handler from firing.
  }
});
})();
