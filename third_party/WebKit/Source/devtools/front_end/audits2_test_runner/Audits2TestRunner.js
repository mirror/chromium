// Copyright 2017 The Chromium Authors. All
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview using private properties isn't a Closure violation in tests.
 * @suppress {accessControls}
 */

Audits2TestRunner.getResultsElement = function() {
  return UI.panels.audits2._getAuditResultsElementForTest();
};

Audits2TestRunner.getDialogElement = function() {
  return UI.panels.audits2._getDialogForTest().contentElement.shadowRoot.querySelector('.audits2-view');
};

Audits2TestRunner.getRunButton = function() {
  return Audits2TestRunner.getDialogElement().querySelectorAll('button')[0];
};

Audits2TestRunner.getCancelButton = function() {
  return Audits2TestRunner.getDialogElement().querySelectorAll('button')[1];
};

Audits2TestRunner.openDialog = function() {
  var resultsElement = Audits2TestRunner.getResultsElement();
  resultsElement.querySelector('button').click();
};

Audits2TestRunner.addStatusListener = function(onMsg) {
  TestRunner.addSniffer(Audits2.Audits2Panel.prototype, '_updateStatus', onMsg, true);
};

Audits2TestRunner.waitForResults = function() {
  return new Promise(resolve => {
    TestRunner.addSniffer(Audits2.Audits2Panel.prototype, '_buildReportUI', resolve);
  });
};

Audits2TestRunner._checkboxState = function(checkboxContainer) {
  var label = checkboxContainer.textElement.textContent;
  var checkedLabel = checkboxContainer.checkboxElement.checked ? 'x' : ' ';
  return `[${checkedLabel}] ${label}`;
};

Audits2TestRunner._buttonState = function(btn) {
  var enabledLabel = btn.disabled ? 'disabled' : 'enabled';
  var hiddenLabel = btn.classList.contains('hidden') ? 'hidden' : 'visible';
  return `${btn.textContent}: ${enabledLabel} ${hiddenLabel}`;
};

Audits2TestRunner.addDialogStateToExpectations = function() {
  TestRunner.addResult('\n========== Audits2 Dialog State ==========');
  var dialog = UI.panels.audits2._getDialogForTest();
  TestRunner.addResult(dialog ? 'Dialog is visible\n' : 'No dialog');
  if (!dialog)
    return;

  var dialogElement = Audits2TestRunner.getDialogElement();
  dialogElement.querySelectorAll('.checkbox').forEach(element => {
    TestRunner.addResult(Audits2TestRunner._checkboxState(element));
  });

  var helpText = dialogElement.querySelector('.audits2-dialog-help-text');
  if (!helpText.classList.contains('hidden'))
    TestRunner.addResult(`Help text: ${helpText.textContent}`);

  TestRunner.addResult(Audits2TestRunner._buttonState(Audits2TestRunner.getRunButton()));
  TestRunner.addResult(Audits2TestRunner._buttonState(Audits2TestRunner.getCancelButton()) + '\n');
};