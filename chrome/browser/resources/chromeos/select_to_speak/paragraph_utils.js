// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var AutomationNode = chrome.automation.AutomationNode;
var RoleType = chrome.automation.RoleType;

/**
 * Gets the parent paragraph of a node, or null if it is not in a paragraph.
 * @return {?AutomationNode} the parent paragraph, or null if it doesn't exist.
 */
function getParagraphParent(node) {
  let parent = node.parent;
  while (parent != null) {
    if (parent.role == RoleType.PARAGRAPH) {
      return parent;
    }
    parent = parent.parent;
  }
  return null;
}

/**
 * Determines whether a node is inside of a paragraph.
 * @return {boolean} whether the node is in a paragraph
 */
function isInParagraph(node) {
  return getParagraphParent(node) != null;
}

/**
 * Determines whether two nodes are in the same paragraph.
 * @return {boolean} whether two nodes are in the same paragraph.
 */
function inSameParagraph(first, second) {
  if (first === undefined || second === undefined) {
    return false;
  }
  // TODO: This isn't working, because sometimes inline elements are
  // labeled as block. See elements between bold and linked nodes in
  // wikipedia for an example.
  /*
  if (first.display == 'block' || second.display == 'block') {
    return false;
  }
  */
  let firstParent = getParagraphParent(first);
  let secondParent = getParagraphParent(second);
  return firstParent != undefined && firstParent == secondParent;
}

/**
 * Builds information about nodes in a paragraph until it reaches the end
 * of text nodes.
 * @return { Paragraph } info about the paragraph
 */
function buildParagraph(nodes, index) {
  let node = nodes[index];
  let next = nodes[index + 1];
  let result = new Paragraph(node, index);
  // TODO: Don't skip nodes. Instead, go through every node in
  // this paragraph from the first to the last in the nodes list.
  // This will catch nodes at the edges of the user's selection like
  // short links at the beginning or ends of sentences.
  //
  // While next node is in the same paragraph as this node AND is
  // a text type node, continue building the paragraph.
  while (inSameParagraph(node, next)) {
    index += 1;
    node = next;
    next = nodes[index + 1];
    if (node.name === undefined || node.name == '') {
      // Don't bother with unnamed or empty nodes.
      continue;
    }
    // Update the resulting paragraph with the next node's information.
    result.nodes.push(new ParagraphNode(node, result.text.length + 1));
    let name = node.name;
    // Remove extra whitespace
    if (name[0] == ' ') {
      name = name.substr(1, name.length);
    }
    if (name[name.length - 1] == ' ') {
      name = name.substr(0, name.length - 1);
    }
    result.text += ' ' + name;
  }
  result.endIndex = index;
  return result;
}

/**
 * Class representing a paragraph object.
 *
 * @constructor
 */
function Paragraph(start, startIndex) {
  /**
   * Full text of this paragraph.
   * @type { string }
   */
  this.text = start.name;

  /**
   * List of nodes in this paragraph in order.
   * @type { Array<ParagraphNode> }
   */
  this.nodes = [new ParagraphNode(start, 0)];

  /**
   * The index of the first node in this paragraph from the list of
   * nodes originally selected by the user.
   * @type { number }
   */
  this.startIndex = startIndex;

  /**
   * The index of the last node in this paragraph from the list of
   * nodes originally selected by the user.
   * @type { number }
   */
  this.endIndex = -1;
}

/**
 * Class representing an automation node within a paragraph.
 * Each node in a paragraph has a start index within the total paragraph
 * text.
 *
 * @constructor
 */
function ParagraphNode(start, startChar) {
  /**
   * @type { AutomationNode }
   */
  this.node = start;

  /**
   * The index into the Paragraph's text string that is the first character
   * of the text of this automation node.
   * @type { number }
   */
  this.startChar = startChar;
}
