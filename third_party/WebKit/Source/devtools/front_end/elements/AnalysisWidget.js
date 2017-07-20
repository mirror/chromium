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
        SDK.AnalysisModel, SDK.AnalysisModel.Events.ElementFlagged, this._elementFlagged, this);
    SDK.targetManager.addModelListener(
        SDK.AnalysisModel, SDK.AnalysisModel.Events.PotentialFlags, this._potentialFlags, this);
    SDK.targetManager.addModelListener(SDK.AnalysisModel, SDK.AnalysisModel.Events.ClearFlags, this._clearList, this);

    var toolbar = new UI.Toolbar('flag-toolbar right-toolbar', this.contentElement);
    this._flagList = this.contentElement.createChild('div', 'flag-list');
    this._flagList.addEventListener('mouseleave', () => this._treeOutline.setHoverEffect(null));

    this._clearList();

    for (var analysisModel of SDK.targetManager.models(SDK.AnalysisModel)) {
      for (var potential of analysisModel._potential)
        this._listHierarchyForNode(potential);
      for (var entry of analysisModel.flags())
        this._flagElement(entry[0], entry[1]);
    }

    this._counter = new Main.Main.WarningErrorCounter(() => this._errors, () => this._warnings);
    toolbar.appendToolbarItem(this._counter.item());
  }

  _clearList() {
    this._warnings = 0;
    this._errors = 0;
    this._flags = new Map();
    this._treeOutline = null;
    this._flagGroups = new Map();
    this._flagList.removeChildren();
    if (this._counter)
      this._counter._update();
  }

  /**
   * @param {!Common.Event} event
   */
  _elementFlagged(event) {
    this._flagElement(
        /** @type {!SDK.DOMNode} */ (event.data.node),
        /** @type {!Array<!SDK.AnalysisModel.Flag>} */ (event.data.flags));
    this._counter._update();
  }

  /**
   * @param {!Common.Event} event
   */
  _potentialFlags(event) {
    this._listHierarchyForNode(event.data);
  }

  /**
   * @param {!SDK.DOMNode} node
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
    return previous;
  }

  /**
   * @param {!SDK.DOMNode} node
   * @param {!Array<!SDK.AnalysisModel.Flag>} flags
   */
  _flagElement(node, flags) {
    if (!this._flags.has(node))
      this._flags.set(node, {errors: 0, warnings: 0, elements: []});

    var previous = this._flags.get(node);
    this._errors -= previous.errors;
    this._warnings -= previous.warnings;
    previous.errors = previous.warnings = 0;
    previous.elements.forEach(element => element.remove());
    for (var flag of flags) {
      if (flag.level === 'error') {
        ++this._errors;
        ++previous.errors;
      }
      if (flag.level === 'warning') {
        ++this._warnings;
        ++previous.warnings;
      }
      var span = this._listHierarchyForNode(node).createChild('span', flag.level);
      var icon = UI.Icon.create('smallicon-' + flag.level);
      span.appendChild(icon);
      flag.analysisModel._appendFlagContentToNode(flag, span);
      span.addEventListener('mouseenter', () => {
        this._treeOutline = flag.treeOutline;
        var treeElement = flag.treeOutline.findTreeElement(node);
        flag.treeOutline.setHoverEffect(treeElement);
      });
      span.addEventListener('click', () => {
        Common.Revealer.reveal.call(Common.Revealer, node);
      });
      previous.elements.push(span);
    }
  }
};
