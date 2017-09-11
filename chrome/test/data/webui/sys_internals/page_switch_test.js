// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PageTest = PageTest || {};

PageTest.Switch = function() {
  suite('Page switch integration test', function() {
    suiteSetup('Wait for the page initialize.', function(done) {
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
          total: 1024 * 1024 * 1024 * 1024 * 8,
        },
        zram: {
          comprDataSize: 1024 * 1024 * 1024 * 100,
          memUsedTotal: 1024 * 1024 * 1024 * 300,
          numReads: 1234,
          numWrites: 1234,
          origDataSize: 1024 * 1024 * 1024 * 200,
        },
      };
      setTimeout(function() {
        handleUpdateData(testData, 1000);
        done();
      }, 1000);
    });

    function isInfoPage() {
      return location.hash == '' &&
          $('sys-internals-drawer-title').innerText == 'Info' &&
          !($('sys-internals-infopage-root').hasAttribute('hidden')) &&
          !(SysInternals.lineChart.shouldRender());
    }

    function isChartPage(title) {
      return location.hash == `#${title}` &&
          $('sys-internals-drawer-title').innerText == title &&
          $('sys-internals-infopage-root').hasAttribute('hidden') &&
          SysInternals.lineChart.shouldRender();
    }

    function clickDrawerBtn(btnIndex) {
      MockInteractions.tap($('sys-internals-nav-menu-btn'));
      const infoBtn = document.getElementsByClassName(
          'sys-internals-drawer-item')[btnIndex];
      MockInteractions.tap(infoBtn);
    }

    function goPage(title, btnIndex) {
      return new Promise(function(resolve, reject) {
        console.log('Goto ' + title);
        clickDrawerBtn(btnIndex);
        setTimeout(function() {
          if (title == 'Info' && !isInfoPage() ||
              title != 'Info' && !isChartPage(title)) {
            reject(new Error(`Not at ${title} page.`));
          }
          resolve();
        }, 500);
      });
    }

    test('Switch test', function(done) {
      this.timeout(5000);
      if (!isInfoPage) {
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
