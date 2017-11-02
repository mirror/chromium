// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console can filter messages by source.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.addScriptTag('resources/log-source.js');
  await TestRunner.evaluateInPagePromise(`
    console.info("sample info");
    console.log("sample log");
    console.warn("sample warning");
    console.debug("sample debug");
    console.error("sample error");

    console.info("abc info");
    console.info("def info");

    console.warn("abc warn");
    console.warn("def warn");
  `);

  function dumpVisibleMessages() {
    var menuText = Console.ConsoleView.instance()._filter._levelMenuButton._text;
    TestRunner.addResult('Level menu: ' + menuText);

    var messages = Console.ConsoleView.instance()._visibleViewMessages;
    for (var i = 0; i < messages.length; i++)
      TestRunner.addResult('>' + messages[i].toMessageElement().deepTextContent());
  }

  var testSuite = [
    function dumpLevels(next) {
      TestRunner.addResult('All levels');
      TestRunner.addObject(Console.ConsoleFilter.allLevelsFilterValue());
      TestRunner.addResult('Default levels');
      TestRunner.addObject(Console.ConsoleFilter.defaultLevelsFilterValue());
      next();
    },

    function beforeFilter(next) {
      TestRunner.addResult(arguments.callee.name);
      dumpVisibleMessages();
      next();
    },

    function allLevels(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleViewFilter.levelFilterSetting().set(Console.ConsoleFilter.allLevelsFilterValue());
    },

    function defaultLevels(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleViewFilter.levelFilterSetting().set(Console.ConsoleFilter.defaultLevelsFilterValue());
    },

    function verbose(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleViewFilter.levelFilterSetting().set({ verbose: true });
    },

    function info(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleViewFilter.levelFilterSetting().set({ info: true });
    },

    function warningsAndErrors(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleViewFilter.levelFilterSetting().set({ warning: true, error: true });
    },

    function abcMessagePlain(next) {
      Console.ConsoleViewFilter.levelFilterSetting().set({ verbose: true });
      Console.ConsoleView.instance()._filter._textFilterUI.setValue('abc');
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleView.instance()._filter._onFilterChanged();
    },

    function abcMessageRegex(next) {
      Console.ConsoleView.instance()._filter._textFilterUI.setValue('/ab[a-z]/');
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleView.instance()._filter._onFilterChanged();
    },

    function abcMessageRegexWarning(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_onFilterChangeCompleted', () => {
        dumpVisibleMessages();
        next();
      });
      Console.ConsoleViewFilter.levelFilterSetting().set({ warning: true });
    }
  ];

  ConsoleTestRunner.evaluateInConsole(
    "'Should be always visible'",
    TestRunner.runTestSuite.bind(TestRunner, testSuite)
  );
})();
