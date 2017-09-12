// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PageTest = PageTest || {};

PageTest.Switch = function() {
  suite('Page switch integration test', function() {
    suiteSetup('Wait for the page initialize.', function(done) {
      const testData = TestUtil.getTestData([
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
        {idle: 100, kernel: 100, total: 100, user: 100},
      ]);
      SysInternals.waitSysInternalsInitialized.promise.then(function() {
        handleUpdateData(testData, 1000);
        done();
      });
    });

    function checkInfoPage() {
      assertEquals(location.hash, '');
      assertEquals($('sys-internals-drawer-title').innerText, 'Info');
      assertFalse($('sys-internals-infopage-root').hasAttribute('hidden'));
      assertFalse(SysInternals.lineChart.shouldRender());
    }

    function checkChartPage(title) {
      assertEquals(location.hash, `#${title}`);
      assertEquals($('sys-internals-drawer-title').innerText, title);
      assertTrue($('sys-internals-infopage-root').hasAttribute('hidden'));
      assertTrue(SysInternals.lineChart.shouldRender());
      if (title == 'CPU') {
        assertEquals(
            SysInternals.lineChart.subCharts_[0].dataSeriesList_.length, 0);
        assertEquals(
            SysInternals.lineChart.subCharts_[1].dataSeriesList_.length, 9);
        assertEquals(SysInternals.lineChart.menu_.buttons_.length, 9);
      } else if (title == 'Memory') {
        assertEquals(
            SysInternals.lineChart.subCharts_[0].dataSeriesList_.length, 2);
        assertEquals(
            SysInternals.lineChart.subCharts_[1].dataSeriesList_.length, 2);
        assertEquals(SysInternals.lineChart.menu_.buttons_.length, 4);
      } else if (title == 'Zram') {
        assertEquals(
            SysInternals.lineChart.subCharts_[0].dataSeriesList_.length, 2);
        assertEquals(
            SysInternals.lineChart.subCharts_[1].dataSeriesList_.length, 3);
        assertEquals(SysInternals.lineChart.menu_.buttons_.length, 5);
      } else {
        assertTrue(false);
      }
    }

    function clickDrawerBtn(btnIndex) {
      MockInteractions.tap($('sys-internals-nav-menu-btn'));
      const infoBtn = document.getElementsByClassName(
          'sys-internals-drawer-item')[btnIndex];
      MockInteractions.tap(infoBtn);
    }

    function goPage(title, btnIndex) {
      SysInternals.waitOnHashChangeCompleted = new PromiseResolver();
      clickDrawerBtn(btnIndex);
      return SysInternals.waitOnHashChangeCompleted.promise.then(function() {
        return new Promise(function(resolve, reject) {
          if (title == 'Info') {
            checkInfoPage();
          } else {
            checkChartPage(title);
          }
          resolve();
        });
      });
    }

    test('Switch test', function(done) {
      if (!isInfoPage()) {
        done(new Error('Not at Info page'));
      }
      goPage('CPU', 1)
          .then(function() {
            return goPage('Zram', 3);
          })
          .then(function() {
            return goPage('Memory', 2);
          })
          .then(function() {
            return goPage('CPU', 1);
          })
          .then(function() {
            return goPage('Info', 0);
          })
          .then(function() {
            return goPage('Memory', 2);
          })
          .then(done, done);
    });
  });

  mocha.run();
};
