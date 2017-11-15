// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that console can filter console.group() messages correctly.\n`);

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
    console.group('Outer group-A');
      console.log('message-a1');
      console.log('message-a2');

      console.group('Inner group-B');
        console.log('message-b1');
        console.log('message-b2');
      console.groupEnd();

      console.group('Inner group-C');
        console.log('message-c1');
        console.log('message-c2');
      console.groupEnd();

      console.groupCollapsed('Collapsed inner group-D');
        console.log('message-d1');
        console.log('message-d2');

        console.group('Inner inner group-E');
          console.log('message-e1');
          console.log('message-e2');
        console.groupEnd();
      console.groupEnd();

    console.groupEnd();

    console.groupCollapsed('Collapsed outer group-F');
      console.log('message-f1');
      console.log('message-f2');
    console.groupEnd();

    console.groupCollapsed('Empty outer group-G');
    console.groupEnd();

    console.log('message-z1');
    console.log('message-z2');
    `);

  var testcases = [
    '/message-a1/',
    '/message-b1/',
    '/message-d1/',
    '/message-e1/',
    '/message-f1/',
    '/message-z1/',
    '/message-a1|message-b1/',
    '/message-a1|message-d1/',
    '/message-a1|message-e1/',
    '/message-a1|message-z1/',
    '/message-b1|message-c1/',
    '/message-b1|message-d1/',
    '/message-b1|message-z1/',
    '/message-d1|message-e1/',

    '/group-A/',
    '/group-B/',
    '/group-D/',
    '/group-E/',
    '/group-F/',

    '/group-A|group-B/',
    '/group-A|group-E/',
    '/group-B|group-C/',
    '/group-B|group-E/',
    '/group-D|group-E/',

    '/group-A|message-b1/',
    '/group-A|message-d1/',
    '/group-A|message-e1/',
    '/group-A|message-z1/',
    '/group-B|message-c1/',
    '/group-B|message-d1/',
    '/group-B|message-z1/',
    '/group-D|message-e1/',
    '/message-a1|group-B/',
    '/message-b1|group-C/',
    '/message-d1|group-E/',

    'queryWithoutMatches',
    ''
  ];

  var consoleView = Console.ConsoleView.instance();
  consoleView._setImmediatelyFilterMessagesForTest();
  for (var testcase of testcases)
    setFilterAndDumpMessages(testcase);
  TestRunner.completeTest();

  /**
   * @param {string} query
   */
  function setFilterAndDumpMessages(query) {
    if (query)
      TestRunner.addResult("\nFilter set to: " + query);
    else
      TestRunner.addResult("\nFilter cleared");

    consoleView._filter._textFilterUI.setValue(query);
    consoleView._filter._onFilterChanged();
    var messages = Console.ConsoleView.instance()._visibleViewMessages;
    if (messages.length < 1)
      TestRunner.addResult("No messages to show.");
    for (var i = 0; i < messages.length; ++i) {
      var viewMessage = messages[i];
      var delimiter = viewMessage.consoleMessage().isGroupStartMessage() ? ">" : "";
      var indent = "  ".repeat(viewMessage.nestingLevel());
      TestRunner.addResult(indent + delimiter + viewMessage.toMessageElement().deepTextContent());
    }
  }
})();
