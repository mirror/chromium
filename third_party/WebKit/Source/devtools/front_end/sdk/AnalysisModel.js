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

    this._flags = new Map();
    this._potential = [];
    this._descendants = new Map();
  }

  /**
   * @return {!Array<!SDK.AnalysisModel.Flag>}
   */
  flags() {
    return this._flags;
  }

  _modifyDescendant(parent, descendant, add) {
    if (!this._descendants.has(parent))
      this._descendants.set(parent, new Set());
    if (add)
      this._descendants.get(parent).add(descendant);
    else
      this._descendants.get(parent).delete(descendant);
  }

  getFlagsForDescendants(node) {
    var descendants = this._descendants.get(node);
    if (!descendants)
      return [];

    return Array.from(descendants)
      .reduce((flags, descendant) => flags.concat(this._flags.get(descendant) || []), []);
  }

  _elementFlagged(node, flags) {
    this._flags.set(node, flags);
    var parent = node;
    while ((parent = parent.parentNode) !== null)
      this._modifyDescendant(parent, node, flags.length !== 0);
    this.dispatchEventToListeners(
      SDK.AnalysisModel.Events.ElementFlagged, {
        node: node,
        flags: flags
      });
  }

  _potentialFlags(node) {
    this._potential.push(node);
    this.dispatchEventToListeners(
      SDK.AnalysisModel.Events.PotentialFlags, node);
  }

  _clearFlags() {
    this.dispatchEventToListeners(
      SDK.AnalysisModel.Events.ClearFlags);
  }

  _appendFlagContentToNode(flag, node) {
    var code = false;
    for (var segment of flag.label.split('`')) {
      if (!code)
        node.createTextChild(segment);
      else
        node.createChild('code').createTextChild(segment);
      code = !code;
    }
  }
};

SDK.SDKModel.register(SDK.AnalysisModel, SDK.Target.Capability.AllForTests, true);

/** @enum {symbol} */
SDK.AnalysisModel.Events = {
  ElementFlagged: Symbol('ElementFlagged'),
  PotentialFlags: Symbol('PotentialFlags'),
  ClearFlags: Symbol('ClearFlags'),
};

SDK.AnalysisModel.Flag = class {
  /**
   * @param {!SDK.AnalysisModel} analysisModel
   * @param {!Elements.ElementsTreeOutline} treeOutline
   * @param {string} level
   * @param {string} label
   */
  constructor(analysisModel, treeOutline, level, label) {
    this.analysisModel = analysisModel;
    this.treeOutline = treeOutline;
    this.level = level;
    this.label = label;
  }
};
