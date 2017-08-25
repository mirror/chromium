// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Changes.ChangesSidebar = class extends UI.Widget {
  /**
   * @param {!WorkspaceDiff.WorkspaceDiff} workspaceDiff
   */
  constructor(workspaceDiff) {
    super();
    this._treeoutline = new UI.TreeOutlineInShadow();
    this._treeoutline.registerRequiredCSS('changes/changesSidebar.css');
    this._treeoutline.setComparator((a, b) => a.titleAsText().compareTo(b.titleAsText()));
    this._treeoutline.addEventListener(UI.TreeOutline.Events.ElementSelected, this._selectionChanged, this);

    this.element.appendChild(this._treeoutline.element);

    /** @type {!Map<!Workspace.UISourceCode, !Changes.ChangesSidebar.UISourceCodeTreeElement>} */
    this._treeElements = new Map();
    this._workspaceDiff = workspaceDiff;
    this._workspaceDiff.modifiedUISourceCodes().forEach(this._addUISourceCode.bind(this));
    this._workspaceDiff.addEventListener(
        WorkspaceDiff.Events.ModifiedStatusChanged, this._uiSourceCodeMofiedStatusChanged, this);
  }

  /**
   * @return {?Workspace.UISourceCode}
   */
  selectedUISourceCode() {
    return this._treeoutline.selectedTreeElement ? this._treeoutline.selectedTreeElement.uiSourceCode : null;
  }

  _selectionChanged() {
    this.dispatchEventToListeners(Changes.ChangesSidebar.Events.SelectedUISourceCodeChanged);
  }

  /**
   * @param {!Common.Event} event
   */
  _uiSourceCodeMofiedStatusChanged(event) {
    var uiSourceCode = /** @type {!Workspace.UISourceCode} */ (event.data.uiSourceCode);
    if (uiSourceCode.project().type() === Workspace.projectTypes.Network &&
        Bindings.changeableNetworkProject.uiSourceCodeForURL(uiSourceCode.url())) {
      this._removeUISourceCodeIfNeeded(uiSourceCode);
      return;
    }
    if (event.data.isModified)
      this._addUISourceCode(uiSourceCode);
    else
      this._removeUISourceCodeIfNeeded(uiSourceCode);
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  _removeUISourceCodeIfNeeded(uiSourceCode) {
    var treeElement = this._treeElements.get(uiSourceCode);
    if (!treeElement)
      return;
    this._treeElements.delete(uiSourceCode);
    if (this._treeoutline.selectedTreeElement === treeElement) {
      var nextElementToSelect = treeElement.previousSibling || treeElement.nextSibling;
      if (nextElementToSelect) {
        nextElementToSelect.select(true);
      } else {
        treeElement.deselect();
        this._selectionChanged();
      }
    }
    this._treeoutline.removeChild(treeElement);
    treeElement.dispose();
  }

  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  _addUISourceCode(uiSourceCode) {
    var treeElement = new Changes.ChangesSidebar.UISourceCodeTreeElement(uiSourceCode);
    this._treeElements.set(uiSourceCode, treeElement);
    this._treeoutline.appendChild(treeElement);
    if (!this._treeoutline.selectedTreeElement)
      treeElement.select(true);
  }
};

/**
 * @enum {symbol}
 */
Changes.ChangesSidebar.Events = {
  SelectedUISourceCodeChanged: Symbol('SelectedUISourceCodeChanged')
};

Changes.ChangesSidebar.UISourceCodeTreeElement = class extends UI.TreeElement {
  /**
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  constructor(uiSourceCode) {
    super();
    this.uiSourceCode = uiSourceCode;
    this._interceptor = new Changes.ChangesSidebar._Interceptor(true /* enabled */, uiSourceCode);
    this.listItemElement.classList.add('navigator-' + uiSourceCode.contentType().name() + '-tree-item');

    var iconType = 'largeicon-navigator-file';
    if (this.uiSourceCode.contentType() === Common.resourceTypes.Snippet)
      iconType = 'largeicon-navigator-snippet';
    var defaultIcon = UI.Icon.create(iconType, 'icon');
    this.setLeadingIcons([defaultIcon]);

    this._eventListeners = [
      uiSourceCode.addEventListener(Workspace.UISourceCode.Events.TitleChanged, this._updateTitle, this),
      uiSourceCode.addEventListener(Workspace.UISourceCode.Events.WorkingCopyChanged, this._updateTitle, this),
      uiSourceCode.addEventListener(Workspace.UISourceCode.Events.WorkingCopyCommitted, this._updateTitle, this)
    ];

    this._updateTitle();
  }

  _updateTitle() {
    var titleText = this.uiSourceCode.displayName();
    if (this.uiSourceCode.isDirty() || Persistence.persistence.hasUnsavedCommittedChanges(this.uiSourceCode))
      titleText = '*' + titleText;
    this.title = titleText;

    var tooltip = this.uiSourceCode.url();
    if (this.uiSourceCode.contentType().isFromSourceMap())
      tooltip = Common.UIString('%s (from source map)', this.uiSourceCode.displayName());
    this.tooltip = tooltip;
    this._interceptor.setURL(this.uiSourceCode.url());
  }

  dispose() {
    Common.EventTarget.removeEventListeners(this._eventListeners);
  }
};

Changes.ChangesSidebar._Interceptor = class extends SDK.RequestInterceptor {
  /**
   * @param {boolean} enabled
   * @param {!Workspace.UISourceCode} uiSourceCode
   */
  constructor(enabled, uiSourceCode) {
    super(enabled, new Set([uiSourceCode.url()]));
    this._currentURL = uiSourceCode.url();
    this._uiSourceCode = uiSourceCode;
  }

  /**
   * @param {string} url
   */
  setURL(url) {
    if (this._currentURL === url)
      return;
    this._currentURL = url;
    this.setPatterns(new Set([url]));
  }

  /**
   * @override
   * @param {!SDK.InterceptedRequest} interceptedRequest
   * @return {!Promise}
   */
  handle(interceptedRequest) {
    interceptedRequest.continueRequestWithContent(this._uiSourceCode.workingCopy(), this._uiSourceCode.mimeType());
    return Promise.resolve();
  }
};
