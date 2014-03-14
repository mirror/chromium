// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var assertEq = chrome.test.assertEq;
var assertTrue = chrome.test.assertTrue;

// Do not test orientation attributes (similar to exclusions on native
// accessibility), since they can be inconsistent depending on the environment.
var RemoveUntestedStates = function(state) {
  delete state['vertical'];
  delete state['horizontal'];
};

var allTests = [
  function testSimplePage() {
    function makeAssertions(tree) {
      var title = tree.root.attributes['ax_attr_doc_title'];
      assertEq('Automation Tests', title);
      RemoveUntestedStates(tree.root.state);
      assertEq(
          {enabled: true, focusable: true, read_only: true},
          tree.root.state);
      var children = tree.root.children();
      assertEq(1, children.length);

      var body = children[0];
      assertEq('body', body.attributes['ax_attr_html_tag']);

      RemoveUntestedStates(body.state);
      assertEq({enabled: true, read_only: true},
               body.state);

      var contentChildren = body.children();
      assertEq(3, contentChildren.length);
      var okButton = contentChildren[0];
      assertEq('Ok', okButton.attributes['ax_attr_name']);
      RemoveUntestedStates(okButton.state);
      assertEq({enabled: true, focusable: true, read_only: true},
               okButton.state);
      var userNameInput = contentChildren[1];
      assertEq('Username',
               userNameInput.attributes['ax_attr_description']);
      RemoveUntestedStates(userNameInput.state);
      assertEq({enabled: true, focusable: true},
               userNameInput.state);
      var cancelButton = contentChildren[2];
      assertEq('Cancel',
               cancelButton.attributes['ax_attr_name']);
      RemoveUntestedStates(cancelButton.state);
      assertEq({enabled: true, focusable: true, read_only: true},
               cancelButton.state);

      // Traversal.
      assertEq(undefined, tree.root.parent());
      assertEq(tree.root, body.parent());

      assertEq(body, tree.root.firstChild());
      assertEq(body, tree.root.lastChild());

      assertEq(okButton, body.firstChild());
      assertEq(cancelButton, body.lastChild());

      assertEq(body, okButton.parent());
      assertEq(body, userNameInput.parent());
      assertEq(body, cancelButton.parent());

      assertEq(undefined, okButton.previousSibling());
      assertEq(undefined, okButton.firstChild());
      assertEq(userNameInput, okButton.nextSibling());
      assertEq(undefined, okButton.lastChild());

      assertEq(okButton, userNameInput.previousSibling());
      assertEq(cancelButton, userNameInput.nextSibling());

      assertEq(userNameInput, cancelButton.previousSibling());
      assertEq(undefined, cancelButton.nextSibling());

      chrome.test.succeed();
    };

    chrome.tabs.create({url: 'test.html'}, function(newTab) {
      chrome.automation.getTree(makeAssertions);
    });
  }
];

chrome.test.runTests(allTests);
