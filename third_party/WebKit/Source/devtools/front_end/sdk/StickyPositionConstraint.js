// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

SDK.StickyPositionConstraint = class {
  /**
   * @param {?SDK.LayerTreeBase} layerTree
   * @param {!Protocol.LayerTree.StickyPositionConstraint} constraint
   * @struct
   */
  constructor(layerTree, constraint) {
    /** @type {!Protocol.DOM.Rect} */
    this._stickyBoxRect = constraint.stickyBoxRect;
    /** @type {!Protocol.DOM.Rect} */
    this._containingBlockRect = constraint.containingBlockRect;
    /** @type {?SDK.Layer} */
    this._nearestLayerShiftingStickyBox = null;
    if (layerTree && constraint.nearestLayerShiftingStickyBox)
      this._nearestLayerShiftingStickyBox = layerTree.layerById(constraint.nearestLayerShiftingStickyBox);

    /** @type {?SDK.Layer} */
    this._nearestLayerShiftingContainingBlock = null;
    if (layerTree && constraint.nearestLayerShiftingContainingBlock)
      this._nearestLayerShiftingContainingBlock = layerTree.layerById(constraint.nearestLayerShiftingContainingBlock);
  }

  /**
   * @return {!Protocol.DOM.Rect}
   */
  stickyBoxRect() {
    return this._stickyBoxRect;
  }

  /**
   * @return {!Protocol.DOM.Rect}
   */
  containingBlockRect() {
    return this._containingBlockRect;
  }

  /**
   * @return {?SDK.Layer}
   */
  nearestLayerShiftingStickyBox() {
    return this._nearestLayerShiftingStickyBox;
  }

  /**
   * @return {?SDK.Layer}
   */
  nearestLayerShiftingContainingBlock() {
    return this._nearestLayerShiftingContainingBlock;
  }
};
