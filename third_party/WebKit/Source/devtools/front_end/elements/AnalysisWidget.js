/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * @unrestricted
 */
Elements.AnalysisWidget = class extends UI.ThrottledWidget {
  constructor() {
    super();
    this.registerRequiredCSS('elements/analysisWidget.css');
    SDK.targetManager.addModelListener(
        SDK.AnalysisModel, SDK.AnalysisModel.Events.NodeFlagged, this._nodeFlagged, this);
    SDK.targetManager.addModelListener(SDK.AnalysisModel, SDK.AnalysisModel.Events.FlagGroup, this._flagGroup, this);
    SDK.targetManager.addModelListener(SDK.AnalysisModel, SDK.AnalysisModel.Events.ClearFlags, this._clearList, this);

    this._flagList = this.contentElement.createChild('div', 'flag-list');
    this._flagList.addEventListener('mouseleave', () => this.flagDelegate.unhighlightFlags());
    this._emptyElement = this.contentElement.createChild('div', 'gray-info-message');
    this._emptyElement.textContent = Common.UIString('No DOM issues');

    this._clearList();

    for (var analysisModel of SDK.targetManager.models(SDK.AnalysisModel)) {
      for (var node of analysisModel.flagGroups())
        this._listHierarchyForNode(node);
      for (var entry of analysisModel.flags()) {
        this._flagNode(
            /** @type {!SDK.DOMNode} */ (entry[0]),
            /** @type {!Array<!SDK.AnalysisModel.Flag>} */ (entry[1]));
      }
    }
  }

  _clearList() {
    /** @type {!Map<!SDK.DOMNode, {elements: !Array<!Element>}>} */
    this._flags = new Map();
    /** @type {?SDK.AnalysisModel.FlagsDelegate} */
    this.flagDelegate = null;
    /** @type {!Map<!SDK.DOMNode, !Element>} */
    this._flagGroups = new Map();
    this._flagList.removeChildren();
    this._emptyElement.classList.remove('hidden');
  }

  /**
   * @param {!Common.Event} event
   */
  _nodeFlagged(event) {
    this._flagNode(
        /** @type {!SDK.DOMNode} */ (event.data.node),
        /** @type {!Array<!SDK.AnalysisModel.Flag>} */ (event.data.flags));
  }

  /**
   * @param {!Common.Event} event
   */
  _flagGroup(event) {
    this._listHierarchyForNode(/** @type {!SDK.DOMNode} */ (event.data));
  }

  /**
   * @param {!SDK.DOMNode} node
   * @return {!Element}
   */
  _listHierarchyForNode(node) {
    var previous = null;
    while (node !== null) {
      var groupExists = this._flagGroups.has(node);
      var group = groupExists ? this._flagGroups.get(node) : this._flagList.createChild('div', 'flag-group');
      if (previous)
        group.appendChild(previous);
      if (groupExists)
        return group;
      previous = group;
      this._flagGroups.set(node, group);
      node = node.parentNode;
    }
    return /** @type {!Element} */ (previous);
  }

  /**
   * @param {!SDK.DOMNode} node
   * @param {!Array<!SDK.AnalysisModel.Flag>} flags
   */
  _flagNode(node, flags) {
    if (!this._flags.has(node))
      this._flags.set(node, {elements: []});

    // Reset the effects of the previous flags.
    var previous = this._flags.get(node);
    previous.elements.forEach(element => element.remove());

    // Add the effects of the new flags.
    for (var flag of flags) {
      var item = this._listHierarchyForNode(node).createChild('span', `flag ${flag.level}`);
      SDK.AnalysisModel.appendFlagContentToNode(node.domModel(), flag, item, true);
      item.addEventListener('mouseenter', () => {
        this.flagDelegate = flag.delegate;
        this.flagDelegate.highlightFlag(node);
      });
      item.addEventListener('click', /** @type {function()} */ (Common.Revealer.reveal.bind(Common.Revealer, node)));
      previous.elements.push(item);
    }
    if (flags.length > 0)
      this._emptyElement.classList.add('hidden');
  }
};
