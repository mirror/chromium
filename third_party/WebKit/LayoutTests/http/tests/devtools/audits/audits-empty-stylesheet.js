// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests running audits with a single empty stylesheet.\n`);
  await TestRunner.loadModule('audits_test_runner');
  await TestRunner.showPanel('audits');
  await TestRunner.loadHTML(`
      <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
      <html>
      <head>
      <style>
      /* Empty stylesheet */
      </style>
      </head>
      </html>
    `);

  TestRunner.reloadPage(step1);

  function step1() {
    Audits.AuditRuleResult.resourceDomain = function() {
      return '[domain]';
    };

    AuditsTestRunner.launchAllAudits(false, step2);
  }

  function step2() {
    AuditsTestRunner.collectAuditResults(TestRunner.completeTest);
  }
})();
