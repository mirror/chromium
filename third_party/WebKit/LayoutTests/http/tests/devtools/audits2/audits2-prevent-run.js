// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  // about:blank never fires a load event so just wait until we see the URL change
  async function navigateToAboutBlankAndWait() {
    TestRunner.navigate('about:blank');

    var url;
    while (url !== 'about:blank') {
      await new Promise(resolve => setTimeout(resolve, 50));
      url = SDK.targetManager.mainTarget().inspectedURL();
    }
  }

  TestRunner.addResult('Tests that audits panel prevents run of unauditable pages.\n');

  await TestRunner.loadModule('audits2_test_runner');
  await TestRunner.showPanel('audits2');

  var auditsPanel = UI.panels.audits2;
  auditsPanel._setIsUnderTest(true);

  TestRunner.addResult('**Prevents audit with no categories**');
  Audits2TestRunner.openDialog();
  var dialogElement = Audits2TestRunner.getDialogElement();
  var checkboxes = dialogElement.querySelectorAll('.checkbox');
  checkboxes.forEach(checkbox => checkbox.checkboxElement.click());
  Audits2TestRunner.addDialogStateToExpectations();

  TestRunner.addResult('**Allows audit with a single category**');
  checkboxes[0].checkboxElement.click();
  Audits2TestRunner.addDialogStateToExpectations();

  TestRunner.addResult('**Prevents audit on undockable page**');
  auditsPanel._setIsUnderTest(false);
  Audits2TestRunner.addDialogStateToExpectations();
  auditsPanel._setIsUnderTest(true);

  TestRunner.addResult('**Prevents audit on internal page**');
  // FIXME(crbug.com/762906): logic is not triggered on navigation, so close the dialog and re-open
  Audits2TestRunner.getCancelButton().click();
  await navigateToAboutBlankAndWait();
  TestRunner.addResult(`URL: ${SDK.targetManager.mainTarget().inspectedURL()}`);
  Audits2TestRunner.openDialog();
  Audits2TestRunner.addDialogStateToExpectations();

  TestRunner.completeTest();
})();
