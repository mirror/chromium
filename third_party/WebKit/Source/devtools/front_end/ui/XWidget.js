// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @extends {UI.XElement}
 */
UI.XWidget = class extends UI.XElement {
  constructor() {
    super();
    this.style.setProperty('display', 'flex');
    this.style.setProperty('flex-direction', 'column');
    this.style.setProperty('align-items', 'stretch');
    this.style.setProperty('justify-content', 'flex-start');
    this.style.setProperty('contain', 'layout style');

    this._visible = false;
    /** @type {?DocumentFragment} */
    this._shadowRoot;
    /** @type {?Element} */
    this._defaultFocusedElement = null;
    /** @type {!Array<!Element>} */
    this._elementsToRestoreScrollPositionsFor = [];

    if (!UI.XWidget._observer) {
      UI.XWidget._observer = new ResizeObserver(entries => {
        for (var entry of entries) {
          if (entry.target._visible)
            entry.target.onResize();
        }
      });
    }
    UI.XWidget._observer.observe(this);

    this.setElementsToRestoreScrollPositionsFor([this]);
  }

  /**
   * @param {?Node} node
   */
  static focusWidgetForNode(node) {
    node = node && node.parentNodeOrShadowHost();
    var widget = null;
    while (node) {
      if (node instanceof UI.XWidget) {
        if (widget)
          node._defaultFocusedElement = widget;
        widget = node;
      }
      node = node.parentNodeOrShadowHost();
    }
  }

  /**
   * @return {boolean}
   */
  isShowing() {
    return this._visible;
  }

  /**
   * @param {string} cssFile
   */
  registerRequiredCSS(cssFile) {
    UI.appendStyle(this._shadowRoot || this, cssFile);
  }

  onShow() {
  }

  onHide() {
  }

  onResize() {
  }

  ownerViewDisposed() {
  }

  makeFocusable() {
    this.tabIndex = 0;
    this._defaultFocusedElement = this;
  }

  /**
   * @param {!Array<!Element>} elements
   */
  setElementsToRestoreScrollPositionsFor(elements) {
    for (var element of this._elementsToRestoreScrollPositionsFor)
      element.removeEventListener('scroll', UI.XWidget._storeScrollPosition, {passive: true, capture: false});
    this._elementsToRestoreScrollPositionsFor = elements;
    for (var element of this._elementsToRestoreScrollPositionsFor)
      element.addEventListener('scroll', UI.XWidget._storeScrollPosition, {passive: true, capture: false});
  }

  restoreScrollPositions() {
    for (var element of this._elementsToRestoreScrollPositionsFor) {
      if (element._scrollTop)
        element.scrollTop = element._scrollTop;
      if (element._scrollLeft)
        element.scrollLeft = element._scrollLeft;
    }
  }

  /**
   * @param {!Event} event
   */
  static _storeScrollPosition(event) {
    var element = event.currentTarget;
    element._scrollTop = element.scrollTop;
    element._scrollLeft = element.scrollLeft;
  }

  /**
   * @param {?Element} element
   */
  setDefaultFocusedElement(element) {
    this._defaultFocusedElement = element;
  }

  /**
   * @override
   */
  focus() {
    if (!this._visible)
      return;

    if (this._defaultFocusedElement) {
      if (!this._defaultFocusedElement.hasFocus()) {
        if (this._defaultFocusedElement === this)
          HTMLElement.prototype.focus.call(this);
        else
          this._defaultFocusedElement.focus();
      }
      return;
    }

    var child = this.traverseNextNode(this);
    while (child) {
      if ((child instanceof UI.XWidget) && child._visible) {
        child.focus();
        return;
      }
      child = child.traverseNextNode(this);
    }
  }

  /**
   * @override
   */
  connectedCallback() {
    this._visible = true;
    this.restoreScrollPositions();
    this.onShow();
  }

  /**
   * @override
   */
  disconnectedCallback() {
    this._visible = false;
    this.onHide();
  }
};

self.customElements.define('x-widget', UI.XWidget);
