// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview_new');

/** @enum {number} */
print_preview_new.State = {
  NOT_READY: 0,
  READY: 1,
  PREVIEW_LOADED: 2,
  HIDDEN: 3,
  PRINTING: 4,
  INVALID_TICKET: 5,
  INVALID_PRINTER: 6,
  FATAL_ERROR: 7,
  CANCELLED: 8,
};

/** @polymerBehavior */
const StateBehavior = {
  properties: {
    /** @type {print_preview_new.State} */
    state: {
      type: Number,
      notify: true,
      observer: 'onStateChanged',
      value: print_preview_new.State.NOT_READY,
    },
  },

  onStateChanged: function() {},

  transitTo: function(newState) {
    switch (newState) {
      case (print_preview_new.State.NOT_READY):
        assert(
            this.state == print_preview_new.State.READY ||
            this.state == print_preview_new.State.PREVIEW_LOADED ||
            this.state == print_preview_new.State.INVALID_PRINTER ||
            !this.state);
        break;
      case (print_preview_new.State.READY):
        assert(
            this.state == print_preview_new.State.PREVIEW_LOADED ||
            this.state == print_preview_new.State.INVALID_TICKET ||
            this.state == print_preview_new.State.NOT_READY);
        break;
      case (print_preview_new.State.PREVIEW_LOADED):
        assert(
            this.state == print_preview_new.State.READY ||
            this.state == print_preview_new.State.PRINTING ||
            this.state == print_preview_new.State.INVALID_TICKET);
        break;
      case (print_preview_new.State.HIDDEN):
        assert(this.state == print_preview_new.State.READY);
        break;
      case (print_preview_new.State.PRINTING):
        assert(
            this.state == print_preview_new.State.PREVIEW_LOADED ||
            this.state == print_preview_new.State.HIDDEN);
        break;
      case (print_preview_new.State.INVALID_TICKET):
        assert(
            this.state == print_preview_new.State.PREVIEW_LOADED ||
            this.state == print_preview_new.State.READY ||
            this.state == print_preview_new.State.NOT_READY);
        break;
      case (print_preview_new.State.INVALID_PRINTER):
        assert(
            this.state == print_preview_new.State.NOT_READY ||
            this.state == print_preview_new.State.READY ||
            this.state == print_preview_new.State.PREVIEW_LOADED);
        break;
      case (print_preview_new.State.CANCELLED):
        assert(
            this.state != print_preview_new.State.PRINTING &&
            this.state != print_preview_new.State.HIDDEN);
        break;
    }
    this.state = newState;
  },

  /**
   * @return {boolean} Whether the controls should be disabled. Used by
   *     settings sections.
   */
  getDisabled: function() {
    return this.state == print_preview_new.State.INVALID_PRINTER ||
        this.state == print_preview_new.State.INVALID_TICKET ||
        this.state == print_preview_new.State.PRINTING ||
        this.state == print_preview_new.State.FATAL_ERROR;
  },
};
