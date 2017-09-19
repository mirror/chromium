// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test to ensure that labels and descriptions that come from elsewhere in the
// tree are updated when the related content changes.

// TODO(aleventhal) why are neither of these working?
//function findById(id) {
  //return rootNode.domQuerySelector('#' + id);
  //return rootNode.find({ htmlAttributes: { id }});
//}

var allTests = [
  function testUpdateRelatedNamesAndDescriptions() {
    var cats = rootNode.find({ attributes: { name: 'cats' }});
    var apples = rootNode.find({ attributes: { name: 'apples' }});
    var butter = rootNode.find({ attributes: { name: 'butter' }});
    // TODO(aleventhal) why are we getting the wrong objects for these?
    // var cats = findById('input').name;
    // var apples = findById('main').name;
    // var butter = findById('nav').name;

    assertEq('cats', cats.name);
    assertEq('apples', apples.name);
    assertEq('butter', butter.name);

    chrome.automation.addTreeChangeObserver("allTreeChanges", function(change) {
      if (change.type == "textChanged") {
        var dogs = rootNode.find({ attributes: { name: 'dogs' }});
        var oranges = rootNode.find({ attributes: { name: 'oranges' }});
        var margarine = rootNode.find({ attributes: { name: 'margarine' }});
        assertEq('dogs', dogs.name);
        assertEq('oranges', oranges.name);
       assertEq('margarine', margarine.name);
       chrome.test.succeed();
      }
    });

    var button = rootNode.find({ attributes: { name: 'Change' }});
    button.doDefault();
  },
];

setUpAndRunTests(allTests, 'tree_change_indirect.html');
