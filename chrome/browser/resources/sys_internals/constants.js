// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Constants use by chrome://sys-internals.
 */

/**
 * The page update period, in milliseconds.
 * @const {number}
 */
const UPDATE_PERIOD = 1000;

/** @const {Array<string>} */
const UNITS_NUMBER_PER_SECOND = ['/s', 'K/s', 'M/s'];

/** @const {number} */
const UNITBASE_NUMBER_PER_SECOND = 1000;

/** @const {Array<string>} */
const UNITS_MEMORY = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];

/** @const {number} */
const UNITBASE_MEMORY = 1024;

/** @const {number} - The precision of the number on the info page. */
const INFO_PAGE_PRECISION = 2;

/** @const {Array<string>} */
const CPU_COLOR_SET = [
  '#2fa2ff', '#ff93e2', '#a170d0', '#fe6c6c', '#2561a4', '#15b979', '#fda941',
  '#79dbcd'
];

/** @const {Array<string>} */
const MEMORY_COLOR_SET = ['#fa4e30', '#8d6668', '#73418c', '#41205e'];

/** @const {Array<string>} - Note: 4th and 5th colors use black menu text. */
const ZRAM_COLOR_SET = ['#9cabd4', '#4a4392', '#dcfaff', '#fff9c9', '#ffa3ab'];

/** @enum {string} */
const PAGE_HASH = {
  INFO: '',
  CPU: '#CPU',
  MEMORY: '#Memory',
  ZRAM: '#Zram',
};
