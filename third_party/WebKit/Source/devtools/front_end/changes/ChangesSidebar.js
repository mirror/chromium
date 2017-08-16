// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Changes.ChangesSidebar = class extends UI.Widget {
  /**
   * @param {!WorkspaceDiff.WorkspaceDiff} workspaceDiff
   */
  constructor(workspaceDiff) {
    super();

    var toolbar = new UI.Toolbar('', this.element);
    var persistCheckbox = new UI.ToolbarCheckbox(
        Common.UIString('Persist changes'),
        'Patch changes on reload/navigate.',
        () => WorkspaceDiff.workspaceDiff().setPersistChanges(persistCheckbox.checked()));
    persistCheckbox.setChecked(true);
    toolbar.appendToolbarItem(persistCheckbox);

    this._treeoutline = new UI.TreeOutlineInShadow();
    this._treeoutline.registerRequiredCSS('changes/changesSidebar.css');
    this._treeoutline.setComparator((a, b) => a.titleAsText().compareTo(b.titleAsText()));
    this._treeoutline.addEventListener(UI.TreeOutline.Events.ElementSelected, this._selectionChanged, this);

    this.element.appendChild(this._treeoutline.element);

    /** @type {!Map<!WorkspaceDiff.WorkspaceDiff.UrlDiff, !Changes.ChangesSidebar.UrlDiffTreeElement>} */
    this._treeElements = new Map();
    this._workspaceDiff = workspaceDiff;
    this._workspaceDiff.modifiedUrlDiffs().forEach(this._addUrlDiff.bind(this));
    this._workspaceDiff.addEventListener(
        WorkspaceDiff.Events.ModifiedStatusChanged, this._urlDiffStatusChanged, this);
  }

  /**
   * @return {?WorkspaceDiff.WorkspaceDiff.UrlDiff}
   */
  selectedUrlDiff() {
    return this._treeoutline.selectedTreeElement ? this._treeoutline.selectedTreeElement.urlDiff : null;
  }

  _selectionChanged() {
    this.dispatchEventToListeners(Changes.ChangesSidebar.Events.SelectedUrlDiffChanged);
  }

  /**
   * @param {!Common.Event} event
   */
  _urlDiffStatusChanged(event) {
    if (event.data.isModified) {
      if (!this._treeElements.has(event.data.urlDiff))
        this._addUrlDiff(event.data.urlDiff);
    } else {
      this._removeUrlDiff(event.data.urlDiff);
    }
  }

  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   */
  _removeUrlDiff(urlDiff) {
    var treeElement = this._treeElements.get(urlDiff);
    this._treeElements.delete(urlDiff);
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
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   */
  _addUrlDiff(urlDiff) {
    var treeElement = new Changes.ChangesSidebar.UrlDiffTreeElement(urlDiff);
    this._treeElements.set(urlDiff, treeElement);
    this._treeoutline.appendChild(treeElement);
    if (!this._treeoutline.selectedTreeElement)
      treeElement.select(true);
  }
};

/**
 * @enum {symbol}
 */
Changes.ChangesSidebar.Events = {
  SelectedUrlDiffChanged: Symbol('SelectedUrlDiffChanged')
};

Changes.ChangesSidebar.UrlDiffTreeElement = class extends UI.TreeElement {
  /**
   * @param {!WorkspaceDiff.WorkspaceDiff.UrlDiff} urlDiff
   */
  constructor(urlDiff) {
    super();
    this.urlDiff = urlDiff;
    //this.listItemElement.classList.add('navigator-' + uiSourceCode.contentType().name() + '-tree-item');

    var iconType = 'largeicon-navigator-file';
    // if (this.uiSourceCode.contentType() === Common.resourceTypes.Snippet)
    //   iconType = 'largeicon-navigator-snippet';
    var defaultIcon = UI.Icon.create(iconType, 'icon');
    this.setLeadingIcons([defaultIcon]);

    this._eventListeners = [
    ];

    this._updateTitle();
  }

  _updateTitle() {
    var parsedURL = this.urlDiff.parsedURL();
    var titleText = parsedURL.displayName;
    // if (this.uiSourceCode.isDirty() || Persistence.persistence.hasUnsavedCommittedChanges(this.uiSourceCode))
    //   titleText = '*' + titleText;
    this.title = titleText;

    var tooltip = parsedURL.url;
    // if (this.uiSourceCode.contentType().isFromSourceMap())
    //   tooltip = Common.UIString('%s (from source map)', this.uiSourceCode.displayName());
    this.tooltip = tooltip;
  }

  dispose() {
    Common.EventTarget.removeEventListeners(this._eventListeners);
  }
};