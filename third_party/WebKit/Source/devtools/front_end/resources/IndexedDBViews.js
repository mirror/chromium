/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
Resources.IDBDatabaseView = class extends UI.VBox {
  /**
   * @param {!Resources.IndexedDBModel} model
   * @param {?Resources.IndexedDBModel.Database} database
   */
  constructor(model, database) {
    super();

    this._model = model;
    var databaseName = database ? database.databaseId.name : Common.UIString('Loading\u2026');

    this._reportView = new UI.ReportView(databaseName);
    this._reportView.show(this.contentElement);

    var bodySection = this._reportView.appendSection('');
    this._securityOriginElement = bodySection.appendField(Common.UIString('Security origin'));
    this._versionElement = bodySection.appendField(Common.UIString('Version'));

    var footer = this._reportView.appendSection('').appendRow();
    this._clearButton = UI.createTextButton(
        Common.UIString('Delete database'), () => this._deleteDatabase(), Common.UIString('Delete database'));
    footer.appendChild(this._clearButton);

    this._refreshButton = UI.createTextButton(
        Common.UIString('Refresh database'), () => this._refreshDatabaseButtonClicked(),
        Common.UIString('Refresh database'));
    footer.appendChild(this._refreshButton);

    if (database)
      this.update(database);
  }

  _refreshDatabase() {
    this._securityOriginElement.textContent = this._database.databaseId.securityOrigin;
    this._versionElement.textContent = this._database.version;
  }

  _refreshDatabaseButtonClicked() {
    this._model.refreshDatabase(this._database.databaseId);
  }

  /**
   * @param {!Resources.IndexedDBModel.Database} database
   */
  update(database) {
    this._database = database;
    this._reportView.setTitle(this._database.databaseId.name);
    this._refreshDatabase();
    this._updatedForTests();
  }

  _updatedForTests() {
    // Sniffed in tests.
  }

  async _deleteDatabase() {
    var ok = await UI.ConfirmDialog.show(
        Common.UIString('Please confirm delete of "%s" database.', this._database.databaseId.name), this.element);
    if (ok)
      this._model.deleteDatabase(this._database.databaseId);
  }
};

/**
 * @unrestricted
 */
Resources.IDBDataView = class extends UI.SimpleView {
  /**
   * @param {!Resources.IndexedDBModel} model
   * @param {!Resources.IndexedDBModel.DatabaseId} databaseId
   * @param {!Resources.IndexedDBModel.ObjectStore} objectStore
   * @param {?Resources.IndexedDBModel.Index} index
   */
  constructor(model, databaseId, objectStore, index) {
    super(Common.UIString('IDB'));
    this.registerRequiredCSS('resources/indexedDBViews.css');

    this._model = model;
    this._databaseId = databaseId;
    this._isIndex = !!index;
    this._expandController = new ObjectUI.ObjectPropertiesSectionExpandController();

    this._pendingUpdatePromises = [];
    this._pendingUpdateMainPromise = null;

    this.element.classList.add('indexed-db-data-view', 'storage-view');

    this._refreshButton = new UI.ToolbarButton(Common.UIString('Refresh'), 'largeicon-refresh');
    this._refreshButton.addEventListener(UI.ToolbarButton.Events.Click, this._refreshButtonClicked, this);

    this._deleteSelectedButton = new UI.ToolbarButton(Common.UIString('Delete selected'), 'largeicon-delete');
    this._deleteSelectedButton.addEventListener(UI.ToolbarButton.Events.Click, () => this._deleteButtonClicked(null));

    this._clearButton = new UI.ToolbarButton(Common.UIString('Clear object store'), 'largeicon-clear');
    this._clearButton.addEventListener(UI.ToolbarButton.Events.Click, this._clearButtonClicked, this);

    this._createEditorToolbar();

    this._pageSize = 50;
    this._skipCount = 0;

    this.update(objectStore, index);
    this._entries = [];
  }

  /**
   * @override
   */
  wasShown() {
    super.wasShown();
    SDK.targetManager.addModelListener(
        Resources.IndexedDBModel, Resources.IndexedDBModel.Events.IndexedDBContentUpdated,
        this._indexedDBContentUpdated, this);
    this.refreshData();
  }

  /**
   * @override
   */
  willHide() {
    SDK.targetManager.removeModelListener(
        Resources.IndexedDBModel, Resources.IndexedDBModel.Events.IndexedDBContentUpdated,
        this._indexedDBContentUpdated, this);
    super.willHide();
  }

  /**
   * @param {!Common.Event} event
   */
  _indexedDBContentUpdated(event) {
    if (this._databaseId.equals(event.data.databaseId) && this._objectStore.name === event.data.objectStoreName)
      this.refreshData();
  }

  _waitForAllUpdates() {
    if (this._pendingUpdatePromises.length === 0)
      return Promise.resolve();
    var promises = this._pendingUpdatePromises;
    this._pendingUpdatePromises = [];
    return Promise.all(promises).then(() => this._waitForAllUpdates());
  }

  /**
   * @return {!Promise}
   */
  async getPendingUpdatePromise() {
    if (this._pendingUpdateMainPromise)
      return this._pendingUpdateMainPromise;
    this._pendingUpdateMainPromise = this._waitForAllUpdates().then(() => this._pendingUpdateMainPromise = null);
    return this._pendingUpdateMainPromise;
  }

  /**
   * @return {!DataGrid.DataGrid}
   */
  _createDataGrid() {
    var keyPath = this._isIndex ? this._index.keyPath : this._objectStore.keyPath;

    var columns = /** @type {!Array<!DataGrid.DataGrid.ColumnDescriptor>} */ ([]);
    columns.push({id: 'number', title: Common.UIString('#'), sortable: false, width: '50px'});
    columns.push(
        {id: 'key', titleDOMFragment: this._keyColumnHeaderFragment(Common.UIString('Key'), keyPath), sortable: false});
    if (this._isIndex) {
      columns.push({
        id: 'primaryKey',
        titleDOMFragment: this._keyColumnHeaderFragment(Common.UIString('Primary key'), this._objectStore.keyPath),
        sortable: false
      });
    }
    columns.push({id: 'value', title: Common.UIString('Value'), sortable: false});

    var dataGrid = new DataGrid.DataGrid(
        columns, undefined, this._deleteButtonClicked.bind(this), this._updateData.bind(this, true));
    dataGrid.setStriped(true);
    dataGrid.addEventListener(DataGrid.DataGrid.Events.SelectedNode, event => this._updateToolbarEnablement(), this);
    return dataGrid;
  }

  /**
   * @param {string} prefix
   * @param {*} keyPath
   * @return {!DocumentFragment}
   */
  _keyColumnHeaderFragment(prefix, keyPath) {
    var keyColumnHeaderFragment = createDocumentFragment();
    keyColumnHeaderFragment.createTextChild(prefix);
    if (keyPath === null)
      return keyColumnHeaderFragment;

    keyColumnHeaderFragment.createTextChild(' (' + Common.UIString('Key path: '));
    if (Array.isArray(keyPath)) {
      keyColumnHeaderFragment.createTextChild('[');
      for (var i = 0; i < keyPath.length; ++i) {
        if (i !== 0)
          keyColumnHeaderFragment.createTextChild(', ');
        keyColumnHeaderFragment.appendChild(this._keyPathStringFragment(keyPath[i]));
      }
      keyColumnHeaderFragment.createTextChild(']');
    } else {
      var keyPathString = /** @type {string} */ (keyPath);
      keyColumnHeaderFragment.appendChild(this._keyPathStringFragment(keyPathString));
    }
    keyColumnHeaderFragment.createTextChild(')');
    return keyColumnHeaderFragment;
  }

  /**
   * @param {string} keyPathString
   * @return {!DocumentFragment}
   */
  _keyPathStringFragment(keyPathString) {
    var keyPathStringFragment = createDocumentFragment();
    keyPathStringFragment.createTextChild('"');
    var keyPathSpan = keyPathStringFragment.createChild('span', 'source-code indexed-db-key-path');
    keyPathSpan.textContent = keyPathString;
    keyPathStringFragment.createTextChild('"');
    return keyPathStringFragment;
  }

  _createEditorToolbar() {
    var editorToolbar = new UI.Toolbar('data-view-toolbar', this.element);

    editorToolbar.appendToolbarItem(this._refreshButton);
    editorToolbar.appendToolbarItem(this._clearButton);
    editorToolbar.appendToolbarItem(this._deleteSelectedButton);

    editorToolbar.appendToolbarItem(new UI.ToolbarSeparator());

    this._pageBackButton = new UI.ToolbarButton(Common.UIString('Show previous page'), 'largeicon-play-back');
    this._pageBackButton.addEventListener(UI.ToolbarButton.Events.Click, this._pageBackButtonClicked, this);
    editorToolbar.appendToolbarItem(this._pageBackButton);

    this._pageForwardButton = new UI.ToolbarButton(Common.UIString('Show next page'), 'largeicon-play');
    this._pageForwardButton.setEnabled(false);
    this._pageForwardButton.addEventListener(UI.ToolbarButton.Events.Click, this._pageForwardButtonClicked, this);
    editorToolbar.appendToolbarItem(this._pageForwardButton);

    this._keyInputElement = UI.createInput('toolbar-input');
    editorToolbar.appendToolbarItem(new UI.ToolbarItem(this._keyInputElement));
    this._keyInputElement.placeholder = Common.UIString('Start from key');
    this._keyInputElement.addEventListener('paste', this._keyInputChanged.bind(this), false);
    this._keyInputElement.addEventListener('cut', this._keyInputChanged.bind(this), false);
    this._keyInputElement.addEventListener('keypress', this._keyInputChanged.bind(this), false);
    this._keyInputElement.addEventListener('keydown', this._keyInputChanged.bind(this), false);
  }

  /**
   * @param {!Common.Event} event
   */
  _pageBackButtonClicked(event) {
    this._skipCount = Math.max(0, this._skipCount - this._pageSize);
    this._updateData(false);
  }

  /**
   * @param {!Common.Event} event
   */
  _pageForwardButtonClicked(event) {
    this._skipCount = this._skipCount + this._pageSize;
    this._updateData(false);
  }

  _keyInputChanged() {
    window.setTimeout(this._updateData.bind(this, false), 0);
  }

  refreshData() {
    this._updateData(true);
  }

  /**
   * @param {!Resources.IndexedDBModel.ObjectStore} objectStore
   * @param {?Resources.IndexedDBModel.Index} index
   */
  update(objectStore, index) {
    this._objectStore = objectStore;
    this._index = index;

    if (this._dataGrid)
      this._dataGrid.asWidget().detach();
    this._dataGrid = this._createDataGrid();
    this._dataGrid.asWidget().show(this.element);

    this._skipCount = 0;
    this._updateData(true);
  }

  /**
   * @param {string} keyString
   */
  _parseKey(keyString) {
    var result;
    try {
      result = JSON.parse(keyString);
    } catch (e) {
      result = keyString;
    }
    return result;
  }

  /**
   * @param {boolean} force
   */
  _updateData(force) {
    var key = this._parseKey(this._keyInputElement.value);
    var pageSize = this._pageSize;
    var skipCount = this._skipCount;
    var selected = this._dataGrid.selectedNode ? this._dataGrid.selectedNode.data['number'] : 0;
    var updateDoneCallback;
    this._pendingUpdatePromises.push(new Promise(resolve => updateDoneCallback = resolve));
    selected = Math.max(selected, this._skipCount);  // Page forward should select top entry
    this._refreshButton.setEnabled(false);
    this._clearButton.setEnabled(!this._isIndex);

    if (!force && this._lastKey === key && this._lastPageSize === pageSize && this._lastSkipCount === skipCount)
      return;

    if (this._lastKey !== key || this._lastPageSize !== pageSize) {
      skipCount = 0;
      this._skipCount = 0;
    }
    this._lastKey = key;
    this._lastPageSize = pageSize;
    this._lastSkipCount = skipCount;

    /**
     * @param {!Array.<!Resources.IndexedDBModel.Entry>} entries
     * @param {boolean} hasMore
     * @this {Resources.IDBDataView}
     */
    async function callback(entries, hasMore) {
      var promises = [];
      for (var i = 0; i < entries.length; ++i)
        promises.push(Resources.IDBDataGridNode.create(entries[i], i + skipCount, this._expandController));
      var nodes = await Promise.all(promises);
      this._refreshButton.setEnabled(true);
      this.clear();
      this._entries = entries;
      var selectedNode = null;
      for (var node of nodes) {
        this._dataGrid.rootNode().appendChild(node);
        if (node.data['number'] <= selected)
          selectedNode = node;
      }

      if (selectedNode)
        selectedNode.select();
      this._pageBackButton.setEnabled(!!skipCount);
      this._pageForwardButton.setEnabled(hasMore);
      this._updateToolbarEnablement();
      updateDoneCallback();
    }

    var idbKeyRange = key ? window.IDBKeyRange.lowerBound(key) : null;
    if (this._isIndex) {
      this._model.loadIndexData(
          this._databaseId, this._objectStore.name, this._index.name, idbKeyRange, skipCount, pageSize,
          callback.bind(this));
    } else {
      this._model.loadObjectStoreData(
          this._databaseId, this._objectStore.name, idbKeyRange, skipCount, pageSize, callback.bind(this));
    }
  }

  /**
   * @param {?Common.Event} event
   */
  _refreshButtonClicked(event) {
    this._updateData(true);
  }

  /**
   * @param {!Common.Event} event
   */
  async _clearButtonClicked(event) {
    this._clearButton.setEnabled(false);
    await this._model.clearObjectStore(this._databaseId, this._objectStore.name);
    this._clearButton.setEnabled(true);
    this._updateData(true);
  }

  /**
   * @param {?DataGrid.DataGridNode} node
   */
  async _deleteButtonClicked(node) {
    if (!node) {
      node = this._dataGrid.selectedNode;
      if (!node)
        return;
    }
    var key = /** @type {!SDK.RemoteObject} */ (this._isIndex ? node.entry.primaryKey : node.entry.key);
    var keyValue = /** @type {!Array<?>|!Date|number|string} */ (key.value);
    await this._model.deleteEntries(this._databaseId, this._objectStore.name, window.IDBKeyRange.only(keyValue));
  }

  clear() {
    this._dataGrid.rootNode().removeChildren();
    this._entries = [];
  }

  _updateToolbarEnablement() {
    var empty = !this._dataGrid || this._dataGrid.rootNode().children.length === 0;
    this._clearButton.setEnabled(!empty);
    this._deleteSelectedButton.setEnabled(!empty && this._dataGrid.selectedNode !== null);
  }
};

Resources.IDBDataGridNode = class extends DataGrid.DataGridNode {
  /**
   * @param {number} index
   * @param {!Resources.IndexedDBModel.Entry} entry
   * @param {!Element} keyElement
   * @param {!Element} primaryKeyElement
   * @param {!Element} valueElement
   */
  constructor(index, entry, keyElement, primaryKeyElement, valueElement) {
    super({number: index, key: keyElement, primaryKey: primaryKeyElement, value: valueElement}, false);
    this.selectable = true;
    this.entry = entry;
  }

  /**
   * @param {!SDK.RemoteObject} key
   * @return {?string}
   */
  static _keyToStringId(key) {
    if (key.value !== undefined)
      return `${key.value}:${key.type}`;
    if (key.subtype === 'date')
      return `${key.description}:date`;
    return null;
  }

  /**
   * @param {!SDK.RemoteObject} value
   * @param {?string} id
   * @param {string} field
   * @param {!ObjectUI.ObjectPropertiesSectionExpandController} expandController
   * @return {!Promise<!Element>}
   */
  static async _createElement(value, id, field, expandController) {
    var objectElement = ObjectUI.ObjectPropertiesSection.defaultObjectPresentation(value, undefined, true);
    var section = ObjectUI.ObjectPropertiesSection.fromElement(objectElement);
    if (id && section)
      await expandController.watchSection(`${id}:${field}`, section);
    return objectElement;
  }

  /**
   * @param {!Resources.IndexedDBModel.Entry} entry
   * @param {number} index
   * @param {!ObjectUI.ObjectPropertiesSectionExpandController} expandController
   * @return {!Promise<!Resources.IDBDataGridNode>}
   */
  static async create(entry, index, expandController) {
    var id = Resources.IDBDataGridNode._keyToStringId(entry.primaryKey);
    var elements = await Promise.all([
      Resources.IDBDataGridNode._createElement(entry.key, id, 'key', expandController),
      Resources.IDBDataGridNode._createElement(entry.primaryKey, id, 'primaryKey', expandController),
      Resources.IDBDataGridNode._createElement(entry.value, id, 'value', expandController)
    ]);
    return new Resources.IDBDataGridNode(index, entry, elements[0], elements[1], elements[2]);
  }

  /**
   * @override
   * @param {string} columnIdentifier
   * @return {!Element}
   */
  createCell(columnIdentifier) {
    var cell = super.createCell(columnIdentifier);
    switch (columnIdentifier) {
      case 'value':
      case 'key':
      case 'primaryKey':
        cell.removeChildren();
        cell.appendChild(/** @type {!Element} */ (this.data[columnIdentifier]));
        break;
      default:
    }
    return cell;
  }
};
