// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PageTest = PageTest || {};

PageTest.InfoPage = function() {
  suite('Info page integration test', function() {
    test('check cpu info', function() {
      function getTextById(id) {
        return $(id).innerText;
      }

      assertTrue(DONT_SEND_UPDATE_REQUEST);
      updateInfoPage();
      assertEquals(getTextById('sys-internals-infopage-num-of-cpu'), '0');
      assertEquals(getTextById('sys-internals-infopage-cpu-usage'), '0.00%');
      assertEquals(
          getTextById('sys-internals-infopage-memory-total'), '0.00 B');
      assertEquals(getTextById('sys-internals-infopage-memory-used'), '0.00 B');
      assertEquals(
          getTextById('sys-internals-infopage-memory-swap-used'), '0.00 B');
      assertEquals(getTextById('sys-internals-infopage-zram-orig'), '0.00 B');
      assertEquals(getTextById('sys-internals-infopage-zram-compr'), '0.00 B');
      assertEquals(
          getTextById('sys-internals-infopage-zram-compr-ratio'), '0.00%');

      const testData = {
        const : {counterMax: 2147483647},
        cpus: [
          {idle: 100, kernel: 100, total: 100, user: 100},
          {idle: 100, kernel: 100, total: 100, user: 100},
          {idle: 100, kernel: 100, total: 100, user: 100},
          {idle: 100, kernel: 100, total: 100, user: 100},
        ],
        memory: {
          available: 1024 * 1024 * 1024 * 1024 * 4,
          pswpin: 1234,
          pswpout: 1234,
          swapFree: 1024 * 1024 * 1024 * 1024 * 4,
          swapTotal: 1024 * 1024 * 1024 * 1024 * 6,
          total: 1024 * 1024 * 1024 * 1024 * 8
        },
        zram: {
          comprDataSize: 1024 * 1024 * 1024 * 100,
          memUsedTotal: 1024 * 1024 * 1024 * 300,
          numReads: 1234,
          numWrites: 1234,
          origDataSize: 1024 * 1024 * 1024 * 200,
        },
      };
      handleUpdateData(testData, 1000);
      assertEquals(getTextById('sys-internals-infopage-num-of-cpu'), '4');
      assertEquals(getTextById('sys-internals-infopage-cpu-kernel'), '0');
      assertEquals(getTextById('sys-internals-infopage-cpu-usage'), '0.00%');
      assertEquals(
          getTextById('sys-internals-infopage-memory-total'), '8.00 TB');
      assertEquals(
          getTextById('sys-internals-infopage-memory-used'), '4.00 TB');
      assertEquals(
          getTextById('sys-internals-infopage-memory-swap-used'), '2.00 TB');
      assertEquals(
          getTextById('sys-internals-infopage-zram-orig'), '200.00 GB');
      assertEquals(
          getTextById('sys-internals-infopage-zram-compr'), '100.00 GB');
      assertEquals(
          getTextById('sys-internals-infopage-zram-compr-ratio'), '50.00%');
      testData.cpus = [
        {idle: 160, kernel: 120, total: 200, user: 120},
        {idle: 180, kernel: 110, total: 200, user: 110},
        {idle: 140, kernel: 130, total: 200, user: 130},
        {idle: 160, kernel: 120, total: 200, user: 120},
      ];
      handleUpdateData(testData, 2000);
      assertEquals(getTextById('sys-internals-infopage-cpu-usage'), '40.00%');
      assertEquals(getTextById('sys-internals-infopage-cpu-kernel'), '80');
    });
  });

  mocha.run();
};
