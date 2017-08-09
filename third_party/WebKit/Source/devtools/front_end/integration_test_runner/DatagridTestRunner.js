// Copyright 2017 The Chromium Authors. All
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview using private properties isn't a Closure violation in tests.
 * @suppress {accessControls}
 */

TestRunner.validateDataGrid = function(root) {
  var children = root.children;

  for (var i = 0; i < children.length; ++i) {
    var child = children[i];

    if (child.parent !== root)
      throw 'Wrong parent for child ' + child.data.id + ' of ' + root.data.id;

    if (child.nextSibling !== ((i + 1 === children.length ? null : children[i + 1])))
      throw 'Wrong child.nextSibling for ' + child.data.id + ' (' + i + ' of ' + children.length + ') ';

    if (child.previousSibling !== ((i ? children[i - 1] : null)))
      throw 'Wrong child.previousSibling for ' + child.data.id + ' (' + i + ' of ' + children.length + ') ';

    if (child.parent && !child.parent._isRoot && child.depth !== root.depth + 1)
      throw 'Wrong depth for ' + child.data.id + ' expected ' + (root.depth + 1) + ' but got ' + child.depth;

    TestRunner.validateDataGrid(child);
  }

  var selectedNode = root.dataGrid.selectedNode;

  if (!root.parent && selectedNode) {
    if (!selectedNode.selectable)
      throw 'Selected node is not selectable';

    for (var node = selectedNode; node && node !== root; node = node.parent) {
    }

    if (!node)
      throw 'Selected node (' + selectedNode.data.id + ') is not within the DataGrid';
  }
};

TestRunner.dumpAndValidateDataGrid = function(root) {
  DataGridTestRunner.dumpDataGrid(root);
  TestRunner.validateDataGrid(root);
};
