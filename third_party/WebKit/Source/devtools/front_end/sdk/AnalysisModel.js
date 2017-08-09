/*
 * Copyright (C) 2017 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @unrestricted
 */
SDK.AnalysisModel = class extends SDK.SDKModel {
  /**
   * @param {!SDK.Target} target
   */
  constructor(target) {
    super(target);
    this._reset();
  }

  /**
   * Formats the content of the flag label and appends it to the node.
   * @param {!SDK.DOMModel} domModel
   * @param {!SDK.AnalysisModel.Flag} flag
   * @param {!Element} node
   * @param {boolean} detailed
   */
  static appendFlagContentToNode(domModel, flag, node, detailed) {
    var icon = UI.Icon.create(`smallicon-${flag.level}`, 'element-icon force-color');
    node.appendChild(icon);

    var content = flag.label;
    if (!detailed)
      content = content.replace(/\[(.*?)\]/g, detailed ? '$1' : '');

    var details = false;
    for (var segments of content.split(/\[|\]/)) {
      var section = node.createChild('span');
      if (details)
        section.classList.add('detail');
      var code = false;
      for (var segment of segments.split('`')) {
        var links = (segment.match(/##\d+/g) || []).map(match => domModel.nodeForId(match.substr(2)));
        for (var subsegment of segment.split(/##\d+/)) {
          var container = !code ? section : section.createChild('code');
          container.createTextChild(subsegment);
          var link = links.shift();
          if (link) {
            var anchor = container.createChild('a');
            anchor.createTextChild(link.nodeName().toLowerCase());
            anchor.addEventListener('click', revealNode.bind(null, link));
          }
        }
        code = !code;
      }
      details = !details;
    }

    function revealNode(link, event) {
      event.stopPropagation();
      Common.Revealer.reveal(link);
    }
  }

  _reset() {
    /** @type {!Map<!SDK.DOMNode, !Array<!SDK.AnalysisModel.Flag>>} */
    this._flags = new Map();
    /** @type {!Array<!SDK.DOMNode>} */
    this._groups = [];
    /** @type {!Map<!SDK.DOMNode, !Set<!SDK.DOMNode>>} */
    this._descendants = new Map();
    /** @type {!Map<string, number>} */
    this._counts = new Map();
  }

  /**
   * @return {!Map<!SDK.DOMNode, !Array<!SDK.AnalysisModel.Flag>>}
   */
  flags() {
    return this._flags;
  }

  /**
   * @return {!Array<!SDK.DOMNode>}
   */
  flagGroups() {
    return this._groups;
  }

  /**
   * @param {string} level
   * @return {number}
   */
  flagCount(level) {
    return this._counts.get(level) || 0;
  }

  /**
   * @param {!SDK.DOMNode} parent
   * @param {!SDK.DOMNode} descendant
   * @param {boolean} add
   */
  _modifyDescendant(parent, descendant, add) {
    if (!this._descendants.has(parent))
      this._descendants.set(parent, new Set());
    if (add)
      this._descendants.get(parent).add(descendant);
    else
      this._descendants.get(parent).delete(descendant);
  }

  /**
   * @param {!SDK.DOMNode} node
   * @return {!Array<!SDK.AnalysisModel.Flag>}
   */
  flagsForDescendants(node) {
    var descendants = this._descendants.get(node);
    if (!descendants)
      return [];

    return Array.from(descendants).reduce((flags, descendant) => flags.concat(this._flags.get(descendant) || []), []);
  }

  /**
   * @param {!SDK.DOMNode} node
   * @param {!Array<!SDK.AnalysisModel.Flag>} flags
   */
  nodeFlagged(node, flags) {
    var previous = this._flags.get(node);
    if (previous) {
      for (var flag of previous)
        this._counts.set(flag.level, this._counts.get(flag.level) - 1);
    }
    for (var flag of flags)
      this._counts.set(flag.level, (this._counts.get(flag.level) || 0) + 1);
    this._flags.set(node, flags);
    var parent = node;
    while ((parent = parent.parentNode) !== null)
      this._modifyDescendant(parent, node, flags.length !== 0);
    this.dispatchEventToListeners(SDK.AnalysisModel.Events.NodeFlagged, {node: node, flags: flags});
  }

  /**
   * @param {!SDK.DOMNode} node
   */
  addFlagGroup(node) {
    this._groups.push(node);
    this.dispatchEventToListeners(SDK.AnalysisModel.Events.FlagGroup, node);
  }

  clearFlags() {
    this._reset();
    this.dispatchEventToListeners(SDK.AnalysisModel.Events.ClearFlags);
  }

  /**
   * @param {!SDK.DOMNode} node
   * @return {!Set<!SDK.DOMNode>}
   */
  nodeCleared(node) {
    var descendants = new Set(this._descendants.get(node));
    this.nodeFlagged(node, []);
    if (descendants)
      descendants.forEach(descendant => this.nodeFlagged(descendant, []));
    return descendants;
  }
};

SDK.SDKModel.register(SDK.AnalysisModel, SDK.Target.Capability.DOM, true);

/** @enum {symbol} */
SDK.AnalysisModel.Events = {
  NodeFlagged: Symbol('NodeFlagged'),
  FlagGroup: Symbol('FlagGroup'),
  ClearFlags: Symbol('ClearFlags'),
};

/**
 * @unrestricted
 */
SDK.AnalysisModel.Flag = class {
  /**
   * @param {!SDK.AnalysisModel.FlagsDelegate} delegate
   * @param {string} level
   * @param {string} label
   * @param {!Array<!SDK.AnalysisModel.Annotation>=} annotations
   */
  constructor(delegate, level, label, annotations) {
    this.delegate = delegate;
    this.level = level;
    this.label = label;
    this.annotations = annotations || [];
  }
};

/**
 * @unrestricted
 */
SDK.AnalysisModel.Annotation = class {
  /**
   * @param {!SDK.AnalysisModel.Annotation.Types} type
   * @param {*} data
   */
  constructor(type, data) {
    this.type = type;
    this.data = data;
  }
};

/** @enum {symbol} */
SDK.AnalysisModel.Annotation.Types = {
  Attribute: Symbol('Annotation.Types.Attribute'),
};

/**
 * @interface
 */
SDK.AnalysisModel.FlagsDelegate = function() {};

SDK.AnalysisModel.FlagsDelegate.prototype = {
  /**
   * @param {!SDK.DOMNode} node
   */
  highlightFlag(node) {},

  unhighlightFlags() {}
};
