// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/** @const */ var LineChart = {};

/**
 * TODO
 * @const {number}
 */
LineChart.CHART_MARGIN = 10;

/**
 * TODO
 * @const {number}
 */
LineChart.MIN_SCALE = 10;

/**
 * TODO
 * @const {number}
 */
LineChart.MAX_SCALE = 1000 * 60 * 3;

/**
 * TODO
 * @const {number}
 */
LineChart.DEFAULT_SCALE = 100;

/**
 * TODO
 * @const {number}
 */
LineChart.MAX_VERTICAL_LABEL_NUM = 6;

/**
 * TODO
 * @const {number}
 */
LineChart.MIN_LABEL_VERTICAL_SPACING = 4;

/**
 * TODO
 * @const {number}
 */
LineChart.MIN_LABEL_HORIZONTAL_SPACING = 3;

/**
 * TODO
 * @const {number}
 */
LineChart.MIN_TIME_LABEL_HORIZONTAL_SPACING = 25;

/**
 * TODO
 * @const {number}
 */
LineChart.Y_AXIS_TICK_LENGTH = 20;

/**
 * TODO
 * @const {number}
 */
LineChart.MOUSE_WHEEL_UNITS = 120;

/**
 * TODO
 * @const {number}
 */
LineChart.TOUCH_ZOOM_UNITS = 60;

/**
 * TODO
 * @const {number}
 */
LineChart.ZOOM_RATE = 1.25;

/**
 * TODO
 * @const {number}
 */
LineChart.MOUSE_WHEEL_SCROLL_RATE = 120;

/**
 * TODO
 * @const {number}
 */
LineChart.DRAG_RATE = 3;

/**
 * TODO
 * @const {Array<number>}
 */
LineChart.TIME_STEP_UNITS = [
  1000,  // 1 second
  1000 * 5,
  1000 * 30,
  1000 * 60,  // 1 minute
  1000 * 60 * 5,
  1000 * 60 * 30,
  1000 * 60 * 60,  // 1 hour
  1000 * 60 * 60 * 5,
  1000 * 60 * 60 * 10,
];

/**
 * TODO
 * @const {number}
 */
LineChart.SAMPLE_RATE = 15;

/**
 * TODO
 * @const {string}
 */
LineChart.TEXT_COLOR = '#000';

/**
 * TODO
 * @const {string}
 */
LineChart.GRID_COLOR = '#888';

/**
 * TODO
 * @const {string}
 */
LineChart.BACKGROUND_COLOR = '#e3e3e3';

/**
 * TODO
 * @enum {number}
 */
LineChart.LabelAlign = {
  LEFT: 0,
  RIGHT: 1
};

/*
cr.define('LineChart', function() {
  'use strict';

  return {
    CHART_MARGIN: 10,

    // 1 px = 10 ms
    MIN_SCALE: 10,
    // 1 px = 3 minute
    MAX_SCALE: 1000 * 60 * 3,
    // 1 px = 100 ms
    DEFAULT_SCALE: 100,

    MAX_VERTICAL_LABEL_NUM: 6,
    MIN_LABEL_VERTICAL_SPACING: 4,
    MIN_LABEL_HORIZONTAL_SPACING: 3,
    MIN_TIME_LABEL_HORIZONTAL_SPACING: 25,

    Y_AXIS_TICK_LENGTH: 20,

    MOUSE_WHEEL_UNITS: 120,
    TOUCH_ZOOM_UNITS: 60,
    // Amount we zoom for one vertical tick of the mouse wheel, as a ratio.
    ZOOM_RATE: 1.25,
    // Amount we scroll for one horizontal tick of the mouse wheel, in pixels.
    MOUSE_WHEEL_SCROLL_RATE: 120,
    // Number of pixels to scroll per pixel the mouse is dragged.
    DRAG_RATE: 3,



    SAMPLE_RATE: 15,

    TEXT_COLOR: '#000',
    GRID_COLOR: '#888',
    BACKGROUND_COLOR: '#e3e3e3',

    LabelAlign: {LEFT: 0, RIGHT: 1}
  };
});*/