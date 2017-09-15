// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests stepping out from custom element callbacks.\n`);

  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.evaluateInPagePromise(`
    function output(message) {
      if (!self._output)
          self._output = [];
      self._output.push('[page] ' + message);
    }

    function testFunction()
    {
        var proto = Object.create(HTMLElement.prototype);
        proto.createdCallback = function createdCallback()
        {
            debugger;
            output('Invoked createdCallback.');
        };
        proto.attachedCallback = function attachedCallback()
        {
            output('Invoked attachedCallback.');
        };
        proto.detachedCallback = function detachedCallback()
        {
            output('Invoked detachedCallback.');
        };
        proto.attributeChangedCallback = function attributeChangedCallback()
        {
            output('Invoked attributeChangedCallback.');
        };
        var FooElement = document.registerElement('x-foo', { prototype: proto });
        var foo = new FooElement();
        foo.setAttribute('a', 'b');
        document.body.appendChild(foo);
        foo.remove();
    }
  `);

  SourcesTestRunner.startDebuggerTest(step1, true);

  function step1() {
    SourcesTestRunner.runTestFunctionAndWaitUntilPaused(step2);
  }

  function step2() {
    var actions = [
      'Print', 'StepOut', 'Print', 'StepInto', 'Print', 'StepOut', 'Print', 'StepInto', 'Print', 'StepOut', 'Print',
      'StepInto', 'Print', 'StepOut', 'Print'
    ];

    SourcesTestRunner.waitUntilPausedAndPerformSteppingActions(actions, step3);
  }

  function step3() {
    SourcesTestRunner.resumeExecution(step4);
  }

  async function step4() {
    const output = await TestRunner.evaluateInPageAsync('JSON.stringify(self._output)');
    TestRunner.addResults(JSON.parse(output.value));
    TestRunner.completeTest();
  }
})();
