// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for the SysInternals WebUI.
 */
const ROOT_PATH = '../../../../../';

GEN('#include "base/test/scoped_feature_list.h"');
GEN('#include "chrome/browser/ui/browser.h"');
GEN('#include "chrome/common/chrome_features.h"');

function SysInternalsBrowserTest() {}

SysInternalsBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  browsePreload: 'chrome://sys-internals',

  runAccessibilityChecks: false,

  isAsync: true,

  commandLineSwitches: [{
    switchName: 'enable-features',
    switchValue: 'SysInternals'
  }],

  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
  ],

  /** @override */
  setUp: function() {
    testing.Test.prototype.setUp.call(this);
  },
};

TEST_F('SysInternalsBrowserTest', 'getSysInfo', function() {
  test('test', function(done){

    const isBitmask = function(number) {
      if (!Number.isInteger(number) && number > 0)
        return false;
      while (number > 0) {
        if ((number & 1) !== 1)
          return false;
        number >>= 1;
      }
      return true;
    };

    const checkConst = function(constVal) {
      const counterMax = constVal.counterMax;
      if (!isBitmask(counterMax)) {
        throw `result.const.counterMax is invalid : ${counterMax}`;
      }
    };

    let counterMax = 0;
    const isCounter = function(number) {
      return Number.isInteger(number) &&
             number >= 0 && number < counterMax;
    };

    const checkCpu = function(cpu) {
      return isCounter(cpu.user) &&
             isCounter(cpu.kernel) &&
             isCounter(cpu.idle) &&
             isCounter(cpu.total);
    };

    const checkCpus = function(cpus) {
      if (!Array.isArray(cpus)) {
        throw 'result.cpus is not an Array.';
        return;
      }
      for (let i = 0; i < cpus.length; ++i) {
        if (!checkCpu(cpus[i])){
          throw `result.cpus[${i}] : ${JSON.stringify(cpus[i])}`;
        }
      }
    };

    const isMemoryByte = function(number) {
      return typeof number === 'number' && number >= 0;
    };

    const checkMemory = function(memory) {
      if (!memory || typeof memory !== 'object' ||
          !isMemoryByte(memory.available) ||
          !isMemoryByte(memory.total) ||
          memory.available > memory.total ||
          !isMemoryByte(memory.swapFree) ||
          !isMemoryByte(memory.swapTotal) ||
          memory.swapFree > memory.swapTotal ||
          !isCounter(memory.pswpin) ||
          !isCounter(memory.pswpout)) {
        throw `result.memory is invalid : ${JSON.stringify(memory)}`;
      }
    };

    const checkZram = function(zram) {
      if (!zram || typeof zram !== 'object' ||
          !isMemoryByte(zram.comprDataSize) ||
          !isMemoryByte(zram.origDataSize) ||
          zram.comprDataSize > zram.origDataSize ||
          !isMemoryByte(zram.memUsedTotal) ||
          !isCounter(zram.numReads) ||
          !isCounter(zram.numWrites)) {
        throw `result.zram is invalid : ${JSON.stringify(zram)}`;
      }
    };

    cr.sendWithPromise('getSysInfo').then(function(result) {
      try {
        checkConst(result.const);
        counterMax = result.const.counterMax;
        checkCpus(result.cpus);
        checkMemory(result.memory);
        checkZram(result.zram);
        done();
      } catch(err) {
        done(new Error(err));
      }
    });
  });

  mocha.run();
});
