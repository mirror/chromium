// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

let AutomationNode = chrome.automation.AutomationNode;

let debuggingEnabled = true;
/**
 * @constructor
 */
let SwitchAccess = function() {
  console.log('Switch access is enabled');

  // Currently selected node.
  /** @private {AutomationNode} */
  this.node_ = null;

  // Root node (i.e., the desktop).
  /** @private {AutomationNode} */
  this.root_ = null;

  // List of nodes to push to / pop from in case this.node_ is lost.
  /** @private {!Array<!AutomationNode>} */
  this.ancestorList_ = [];

  chrome.automation.getDesktop(function(desktop) {
    this.node_ = desktop;
    this.root_ = desktop;
    console.log('AutomationNode for desktop is loaded');
    this.printDetails_();

    document.addEventListener('keyup', function(event) {
      switch (event.key) {
        case '1':
          console.log('1 = go to previous element');
          this.moveToPrevious_();
          break;
        case '2':
          console.log('2 = go to next element');
          this.moveToNext_();
          break;
        case '3':
          console.log('3 = do default on element');
          this.doDefault_();
          break;
      }
      if (debuggingEnabled) {
        switch (event.key) {
          case '6':
            console.log('6 = go to previous element (debug mode)');
            this.debugMoveToPrevious_();
            break;
          case '7':
            console.log('7 = go to next element (debug mode)');
            this.debugMoveToNext_();
            break;
          case '8':
            console.log('8 = go to child element (debug mode)');
            this.debugMoveToFirstChild_();
            break;
          case '9':
            console.log('9 = go to parent element (debug mode)');
            this.debugMoveToParent_();
            break;
        }
      }
      if (this.node_)
        chrome.accessibilityPrivate.setFocusRing([this.node_.location]);
    }.bind(this));
  }.bind(this));
};

SwitchAccess.prototype = {
  /**
   * Set this.node_ to the previous interesting node. If no interesting node
   * comes before this.node_, set this.node_ to the last interesting node.
   *
   * @private
   */
  moveToPrevious_: function() {
    if (this.node_) {
      let prev = this.getPreviousNode_(this.node_);
      while (prev && !this.isInteresting_(prev)) {
        prev = this.getPreviousNode_(prev);
      }
      if (prev) {
        this.node_ = prev;
        this.printNode_(this.node_);
        return;
      }
    }

    if (this.root_) {
      console.log('Reached the first interesting node. Restarting with last.')
      let prev = this.getYoungestDescendant_(this.root_);
      while (prev && !this.isInteresting_(prev)) {
        prev = this.getPreviousNode_(prev);
      }
      if (prev) {
        this.node_ = prev;
        this.printNode_(this.node_);
        return;
      }
    }

    console.log('Found no interesting nodes to visit.')
  },

  /**
   * Set this.node_ to the next interesting node. If no interesting node comes
   * after this.node_, set this.node_ to the first interesting node.
   *
   * @private
   */
  moveToNext_: function() {
    if (this.node_) {
      let next = this.getNextNode_(this.node_);
      while (next && !this.isInteresting_(next))
        next = this.getNextNode_(next);
      if (next) {
        this.node_ = next;
        this.printNode_(this.node_);
        return;
      }
    }

    if (this.root_) {
      console.log('Reached the last interesting node. Restarting with first.');
      let next = this.root_.firstChild;
      while (next && !this.isInteresting_(next))
        next = this.getNextNode_(next);
      if (next) {
        this.node_ = next;
        this.printNode_(this.node_);
        return;
      }
    }

    console.log('Found no interesting nodes to visit.');
  },

  /**
   * Given a flat list of nodes in pre-order, get the node that comes after
   * |node|.
   *
   * @param {!AutomationNode} node
   * @return {AutomationNode}
   * @private
   */
  getNextNode_: function(node) {
    // Check for child.
    let child = node.firstChild;
    if (child)
      return child;

    // No child. Check for right-sibling.
    let sibling = node.nextSibling;
    if (sibling)
      return sibling;

    // No right-sibling. Get right-sibling of closest ancestor.
    let ancestor = node.parent;
    while (ancestor) {
      let aunt = ancestor.nextSibling;
      if (aunt)
        return aunt;
      ancestor = ancestor.parent;
    }

    // No node found after |node|, so return null.
    return null;
  },

  /**
   * Given a flat list of nodes in pre-order, get the node that comes before
   * |node|.
   *
   * @param {!AutomationNode} node
   * @return {AutomationNode}
   * @private
   */
  getPreviousNode_: function(node) {
    // Check for left-sibling. Return its youngest descendant if it has one.
    // Otherwise, return the sibling.
    let sibling = node.previousSibling;
    if (sibling) {
      let descendant = this.getYoungestDescendant_(sibling);
      if (descendant)
        return descendant;
      return sibling;
    }

    // No left-sibling. Check for parent.
    let parent = node.parent;
    if (parent)
      return parent;

    // No node found before |node|, so return null.
    return null;
  },

  /**
   * Get the youngest descendant of |node| if it has one.
   *
   * @param {!AutomationNode} node
   * @return {AutomationNode}
   * @private
   */
  getYoungestDescendant_: function(node) {
    if (!node.lastChild)
      return null;

    while (node.lastChild)
      node = node.lastChild;

    return node;
  },

  /**
   * Returns true if |node| is interesting.
   *
   * @param {!AutomationNode} node
   * @return {boolean}
   * @private
   */
  isInteresting_: function(node) {
    return node.state && node.state.focusable;
  },

  /**
   * Move to the previous sibling of this.node_ if it has one.
   *
   * @private
   */
  debugMoveToPrevious_: function() {
    let previous = this.node_.previousSibling;
    if (previous) {
      this.node_ = previous;
      this.printDetails_();
    } else {
      console.log('Node is first of siblings');
      console.log('\n');
    }
  },

  /**
   * Move to the next sibling of this.node_ if it has one.
   *
   * @private
   */
  debugMoveToNext_: function() {
    let next = this.node_.nextSibling;
    if (next) {
      this.node_ = next;
      this.printDetails_();
    } else {
      console.log('Node is last of siblings');
      console.log('\n');
    }
  },

  /**
   * Move to the first child of this.node_ if it has one.
   *
   * @private
   */
  debugMoveToFirstChild_: function() {
    let child = this.node_.firstChild;
    if (child) {
      this.ancestorList_.push(this.node_);
      this.node_ = child;
      this.printDetails_();
    } else {
      console.log('Node has no children');
      console.log('\n');
    }
  },

  /**
   * Move to the parent of this.node_ if it has one. If it does not have a
   * parent but it is not the top level root node, then this.node_ lost track of
   * its neighbors, and we move to an ancestor node.
   *
   * @private
   */
  debugMoveToParent_: function() {
    let parent = this.node_.parent;
    if (parent) {
      this.ancestorList_.pop();
      this.node_ = parent;
      this.printDetails_();
    } else if (this.ancestorList_.length === 0) {
      console.log('Node has no parent');
      console.log('\n');
    } else {
      console.log(
          'Node could not find its parent, so moved to recent ancestor');
      let ancestor = this.ancestorList_.pop();
      this.node_ = ancestor;
      this.printDetails_();
    }
  },

  /**
   * Perform the default action on the currently selected node.
   *
   * @private
   */
  doDefault_: function() {
    let state = this.node_.state;
    if (state && state.focusable)
      console.log('Node was focusable, doing default on it')
    else if (state)
      console.log('Node was not focusable, but still doing default');
    else
      console.log('Node has no state, still doing default');
    console.log('\n');
    this.node_.doDefault();
  },

  // TODO(elichtenberg): Move print functions to a custom logger class. Only
  // log when debuggingEnabled is true.
  /**
   * Print out details about the currently selected node and the list of
   * ancestors.
   *
   * @private
   */
  printDetails_: function() {
    this.printNode_(this.node_);
    console.log(this.ancestorList_);
    console.log('\n');
  },

  /**
   * Print out details about a node.
   *
   * @param {AutomationNode} node
   * @private
   */
  printNode_: function(node) {
    if (node) {
      console.log('Name = ' + node.name);
      console.log('Role = ' + node.role);
      if (!node.parent)
        console.log('At index ' + node.indexInParent + ', has no parent');
      else {
        let numSiblings = node.parent.children.length;
        console.log(
            'At index ' + node.indexInParent + ', there are '
            + numSiblings + ' siblings');
      }
      console.log('Has ' + node.children.length + ' children');
    } else {
      console.log('Node is null');
    }
    console.log(node);
    console.log('\n');
  }
};

window.switchAccess = new SwitchAccess();
