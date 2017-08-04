// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Accessibility Audit API wrapping the aXe-core library. */

/**
 * @typedef {{
 *   rules: {
 *     'color_contrast': ({ enabled: boolean} | undefined),
 *     'aria_valid_attr': ({ enabled: boolean} | undefined),
 *   }
 * }}
 * @see https://github.com/dequelabs/axe-core/blob/develop/doc/API.md#options-parameter
 */
function AccessibilityAuditConfig() {}

// Namespace for the accessibility audit.
function AccessibilityTest() {}

/**
 * @param {string} auditRuleId aXe-core audit rule ID of the rule to except
 * @param {!function(axe.NodeResult)} selector Function that returns false when
 *    the node result should be excluded from violations for the audit rule.
 * @constructor
 */
AccessibilityTest.Exception = function(auditRuleId, excluder) {
  return {
    auditRuleId: auditRuleId, excluder: excluder
  }
};

/**
 * Run aXe-core accessibility audit, print console-friendly representation
 * of violations to console, and fail the test.
 * @param {AccessibilityAuditConfig} options Dictionary disabling specific
 *    audit rules.
 * @param {Array<AccessibilityTest.Exception>} exceptions List of exceptions to
 *     exclude from audit results.
 * @return {Promise} A promise that will be resolved with the accessibility
 *    audit is complete.
 */
AccessibilityTest.runAudit = function(options, exceptions) {
  // Ignore iron-iconset-svg elements that have duplicate ids and result in
  // false postives from the audit.
  var context = {exclude: ['iron-iconset-svg']};
  options = options || {};
  // Run the audit with element references included in the results.
  options.elementRef = true;
  exceptions = exceptions || [];

  return new Promise(function(resolve, reject) {
    axe.run(context, options, function(err, results) {
      if (err)
        reject(err);

      var filteredViolations =
          AccessibilityTest.pruneExceptions_(results.violations, exceptions);

      var violationCount = filteredViolations.length;
      if (violationCount) {
        AccessibilityTest.printViolations(filteredViolations);
        reject('Found ' + violationCount + ' accessibility violations.');
      } else {
        resolve();
      }
    });
  });
};

/**
 * Get list of audit violations that excludes given exceptions.
 * @param {!Array<axe.Result>} violations List of accessibility violations.
 * @param {!Array<AccessibilityTest.Exception>} exceptions List of exceptions
 *    to prune from the results.
 * @return {!Array<axe.Result>} List of violations that do not
 *    match those described by exceptions.
 */
AccessibilityTest.pruneExceptions_ = function(violations, exceptions) {
  // Create a dictionary to map violation types to their objects
  var violationMap = {};
  for (i = 0; i < violations.length; i++) {
    violationMap[violations[i].id] = violations[i];
  }

  for (let exception of exceptions) {
    if (exception.auditRuleId in violationMap) {
      var violation = violationMap[exception.auditRuleId];
      var filteredNodes = violation.nodes.filter(exception.excluder);
      if (filteredNodes.length > 0) {
        violation.nodes = filteredNodes;
        violationMap[exception.auditRuleId].nodes = filteredNodes;
      } else
        delete violationMap[exception.auditRuleId];
    }
  }
  return Object.values(violationMap);
};

AccessibilityTest.printViolations = function(violations) {
  // Elements have circular references and must be removed before printing.
  for (let violation of violations) {
    for (let node of violation.nodes) {
      delete node['element'];
    }
  }

  console.log(JSON.stringify(violations, null, 4));
};
