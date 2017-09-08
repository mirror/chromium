// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Externs for objects use by chrome://sys-internals.
 * @externs
 */

/**
 * |getSysInfo| cpu result.
 * @typedef {{
 *   idle: number,
 *   kernel: number,
 *   total: number,
 *   user: number,
 * }}
 */
var SysInfoApiCpuResult;

/**
 * |getSysInfo| memory result.
 * @typedef {{
 *   available: number,
 *   pswpin: number,
 *   pswpout: number,
 *   swapFree: number,
 *   swapTotal: number,
 *   total: number,
 * }}
 */
var SysInfoApiMemoryResult;

/**
 * |getSysInfo| zram result.
 * @typedef {{
 *   comprDataSize: number,
 *   memUsedTotal: number,
 *   numReads: number,
 *   numWrites: number,
 *   origDataSize: number,
 * }}
 */
var SysInfoApiZramResult;

/**
 * |getSysInfo| api result.
 * @typedef {{
 *   const: {counterMax: number},
 *   cpus: Array<SysInfoApiCpuResult>,
 *   memory: SysInfoApiMemoryResult,
 *   zram: SysInfoApiZramResult,
 * }}
 */
var SysInfoApiResult;

/**
 * For info page.
 * @typedef {{
 *   core: string,
 *   idle: string,
 *   kernel: string,
 *   usage: string,
 *   user: string,
 * }}
 */
var GeneralCpuType;

/**
 * For info page.
 * @typedef {{
 *   swapTotal: string,
 *   swapUsed: string,
 *   total: string,
 *   used: string,
 * }}
 */
var GeneralMemoryType;

/**
 * For info page.
 * @typedef {{
 *   compr: string,
 *   comprRatio: string,
 *   orig: string,
 *   total: string,
 * }}
 */
var GeneralZramType;
