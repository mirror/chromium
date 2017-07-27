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

    /** @type {!Map<!SDK.DOMNode, !Array<!SDK.AnalysisModel.Flag>>} */
    this._flags = new Map();
    /** @type {!Map<!SDK.DOMNode, !Set<!SDK.DOMNode>>} */
    this._descendants = new Map();
  }

  /**
   * @return {!Map<!SDK.DOMNode, !Array<!SDK.AnalysisModel.Flag>>}
   */
  flags() {
    return this._flags;
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
   * @param {!Array<!SDK.AnalysisModel.Flag>} flags
   */
  nodeFlagged(node, flags) {
    this._flags.set(node, flags);
    var parent = node;
    while ((parent = parent.parentNode) !== null)
      this._modifyDescendant(parent, node, flags.length !== 0);
  }
};

SDK.SDKModel.register(SDK.AnalysisModel, SDK.Target.Capability.DOM, true);

/**
 * @unrestricted
 */
SDK.AnalysisModel.Flag = class {
  /**
   * @param {string} level
   * @param {string} label
   * @param {!Array<!SDK.AnalysisModel.Annotation>=} annotations
   */
  constructor(level, label, annotations) {
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
