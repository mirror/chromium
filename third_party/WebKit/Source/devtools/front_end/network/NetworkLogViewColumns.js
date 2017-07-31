// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @unrestricted
 */
Network.NetworkLogViewColumns = class {
  /**
   * @param {!Network.NetworkLogView} networkLogView
   * @param {!Network.NetworkTransferTimeCalculator} timeCalculator
   * @param {!Network.NetworkTransferDurationCalculator} durationCalculator
   * @param {!Common.Setting} networkLogLargeRowsSetting
   */
  constructor(networkLogView, timeCalculator, durationCalculator, networkLogLargeRowsSetting) {
    this._networkLogView = networkLogView;

    /** @type {!Common.Setting} */
    this._persistantSettings = Common.settings.createSetting('networkLogColumns', {});

    this._networkLogLargeRowsSetting = networkLogLargeRowsSetting;
    this._networkLogLargeRowsSetting.addChangeListener(this._updateRowsSize, this);

    /** @type {!Map<string, !Array<number>>} */
    this._eventDividers = new Map();
    this._eventDividersShown = true;

    this._gridMode = true;

    /** @type {!Array.<!Network.NetworkLogViewColumns.Descriptor>} */
    this._columns = [];

    /** @type {?Element} */
    this._waterfallInnerContainer = null;
    this._hasScrollbar = false;

    /** @type {!Components.Linkifier} */
    this._popupLinkifier = new Components.Linkifier();

    /** @type {!Map<string, !Network.NetworkTimeCalculator>} */
    this._calculatorsMap = new Map();
    this._calculatorsMap.set(Network.NetworkLogViewColumns._calculatorTypes.Time, timeCalculator);
    this._calculatorsMap.set(Network.NetworkLogViewColumns._calculatorTypes.Duration, durationCalculator);

    this._setupDataGrid();
    this._setupWaterfall();
  }

  /**
   * @param {!Network.NetworkLogViewColumns.Descriptor} columnConfig
   * @return {!DataGrid.DataGrid.ColumnDescriptor}
   */
  static _convertToDataGridDescriptor(columnConfig) {
    return /** @type {!DataGrid.DataGrid.ColumnDescriptor} */ ({
      id: columnConfig.id,
      title: columnConfig.title,
      sortable: columnConfig.sortable,
      align: columnConfig.align,
      nonSelectable: columnConfig.nonSelectable,
      weight: columnConfig.weight
    });
  }

  wasShown() {
    this._updateRowsSize();
  }

  willHide() {
    this._popoverHelper.hidePopover();
  }

  reset() {
    if (this._popoverHelper)
      this._popoverHelper.hidePopover();
    this._eventDividers.clear();
  }

  _setupDataGrid() {
    var defaultColumns = Network.NetworkLogViewColumns._defaultColumns;

    var defaultColumnConfig = Network.NetworkLogViewColumns._defaultColumnConfig;

    this._columns = /** @type {!Array<!Network.NetworkLogViewColumns.Descriptor>} */ ([]);
    for (var currentConfigColumn of defaultColumns) {
      var columnConfig = /** @type {!Network.NetworkLogViewColumns.Descriptor} */ (
          Object.assign({}, defaultColumnConfig, currentConfigColumn));
      if (columnConfig.subtitle)
        columnConfig.titleDOMFragment = this._makeHeaderFragment(columnConfig.title, columnConfig.subtitle);
      this._columns.push(columnConfig);
    }
    this._loadCustomColumnsAndSettings();

    this._popoverHelper = new UI.PopoverHelper(this._networkLogView.element, this._getPopoverRequest.bind(this));
    this._popoverHelper.setHasPadding(true);
    this._popoverHelper.setTimeout(300, 300);

    /** @type {!DataGrid.SortableDataGrid<!Network.NetworkNode>} */
    this._dataGrid =
        new DataGrid.SortableDataGrid(this._columns.map(Network.NetworkLogViewColumns._convertToDataGridDescriptor));
    this._dataGrid.element.addEventListener('mousedown', event => {
      if (!this._dataGrid.selectedNode && event.button)
        event.consume();
    }, true);
    this._dataGrid.scrollContainer.addEventListener('scroll', () => this._popoverHelper.hidePopover(), {passive: true});

    this._updateColumns();
    this._dataGrid.addEventListener(DataGrid.DataGrid.Events.SortingChanged, this._sortHandler, this);
    this._dataGrid.setHeaderContextMenuCallback(this._innerHeaderContextMenu.bind(this));

    this._activeWaterfallSortId = Network.NetworkLogViewColumns.WaterfallSortIds.StartTime;
    this._dataGrid.markColumnAsSortedBy(
        Network.NetworkLogViewColumns._initialSortColumn, DataGrid.DataGrid.Order.Ascending);
  }

  _setupWaterfall() {
    this._waterfallColumn = new Network.NetworkWaterfallColumn(this._networkLogView.calculator());

    var waterfallColumnConfig = this._columns.find(columnDescriptor => columnDescriptor.id === 'waterfall');
    if (waterfallColumnConfig) {
      waterfallColumnConfig.sortingFunction = (a, b) =>
          Network.NetworkRequestNode.RequestPropertyComparator(this._activeWaterfallSortId, a, b);
    }

    this._dataGrid.addEventListener(
        DataGrid.ViewportDataGrid.Events.ViewportCalculated,
        event => this._redrawWaterfallColumn(/** @type {!DataGrid.ViewportDataGrid.ViewportState} */ (event.data)));

    this._waterfallColumn.contentElement.classList.add('network-waterfall-view');

    this._dataGrid.addEventListener(DataGrid.DataGrid.Events.ColumnsResized, () => {
      if (!this._waterfallColumn.isShowing())
        return;
      this._configureTimeDividers();
      this._waterfallColumn.onResize();
    });

    this.switchViewMode(false);
  }

  /**
   * @param {!DataGrid.ViewportDataGrid.ViewportState=} viewportState
   */
  _redrawWaterfallColumn(viewportState) {
    if (!this._dataGrid.asWidget().isShowing())
      return;
    if (!this._waterfallInnerContainer) {
      var visibleColumns = this._dataGrid.visibleColumns();
      var columnIndex = visibleColumns.findIndex(columnDescriptor => columnDescriptor.id === 'waterfall');
      if (columnIndex === -1)
        return;
      var topFillerRowElement = this._dataGrid.topFillerRowElement().children[columnIndex];
      topFillerRowElement.classList.add('waterfall-overlay');
      var waterfallContainer = topFillerRowElement.createChild('div', 'waterfall-container');
      this._waterfallInnerContainer = waterfallContainer.createChild('div');
      var contentElement = createElementWithClass('div', 'waterfall-root vbox flex-auto');

      var shadowRoot = UI.createShadowRootWithCoreStyles(this._waterfallInnerContainer);
      shadowRoot.appendChild(contentElement);
      this._waterfallColumn.show(contentElement);
    }
    /** @type {!Array<!Network.NetworkNode>|undefined} */
    var visibleNodes;
    if (viewportState) {
      visibleNodes = /** @type {!Array<!Network.NetworkNode>} */ (viewportState.visibleNodes);
      this._hasScrollbar = viewportState.contentHeight > viewportState.clientHeight ||
          !!(viewportState.topPadding || viewportState.bottomPadding);
      this._resizeWaterfallIfNeeded(viewportState.contentHeight);
    }
    this._configureTimeDividers();
    this._waterfallColumn.update(visibleNodes, this._eventDividersShown ? this._eventDividers : undefined);
  }

  /**
   * @param {number} contentHeight
   */
  _resizeWaterfallIfNeeded(contentHeight) {
    var hasScrollbarClass = this._waterfallInnerContainer.classList.contains('network-log-grid-has-scrollbar');
    var resized = false;
    if (this._hasScrollbar && !hasScrollbarClass) {
      this._waterfallInnerContainer.classList.add('network-log-grid-has-scrollbar');
      resized = true;
    } else if (!this._hasScrollbar && hasScrollbarClass) {
      this._waterfallInnerContainer.classList.remove('network-log-grid-has-scrollbar');
      resized = true;
    }
    var newHeightPx = contentHeight + 'px';
    if (!this._hasScrollbar)
      newHeightPx = this._dataGrid.scrollContainer.offsetHeight + 'px';
    if (this._waterfallInnerContainer.style.height !== newHeightPx) {
      this._waterfallInnerContainer.style.height = newHeightPx;
      resized = true;
    }
    if (resized)
      this._waterfallColumn.onResize();
  }

  _configureTimeDividers() {
    var calculator = this._networkLogView.calculator();
    var dividersData = PerfUI.TimelineGrid.calculateDividerOffsets(calculator, /* freeZoneAtLeft */ 100);
    this._waterfallColumn.setDividersData(dividersData);
    var headerElement = this._dataGrid.headerTableHeader('waterfall');
    var waterfallTimeDividersContainer = headerElement.ownerDocument.getElementById('waterfall-dividers-container');
    if (!waterfallTimeDividersContainer) {
      waterfallTimeDividersContainer = headerElement.createChild('div');
      waterfallTimeDividersContainer.id = 'waterfall-dividers-container';
    }
    waterfallTimeDividersContainer.removeChildren();
    for (var offsetInfo of dividersData.offsets) {
      var dividerElement = waterfallTimeDividersContainer.createChild('div', 'header-divider');
      var labelElement = dividerElement.createChild('span', 'header-divider-label');
      dividerElement.style.left = (100 * offsetInfo.position / waterfallTimeDividersContainer.clientWidth) + '%';
      labelElement.textContent = calculator.formatValue(offsetInfo.time, dividersData.precision);
    }
  }

  /**
   * @param {!Network.NetworkTimeCalculator} x
   */
  setCalculator(x) {
    this._waterfallColumn.setCalculator(x);
  }

  scheduleRefresh() {
    this._waterfallColumn.scheduleDraw();
  }

  _updateRowsSize() {
    var largeRows = !!this._networkLogLargeRowsSetting.get();

    this._dataGrid.element.classList.toggle('small', !largeRows);
    this._dataGrid.scheduleUpdate();

    this._waterfallColumn.setRowHeight(largeRows ? 41 : 21);
  }

  /**
   * @param {!Element} element
   */
  show(element) {
    this._dataGrid.asWidget().show(element);
  }

  /**
   * @return {!DataGrid.SortableDataGrid<!Network.NetworkNode>} dataGrid
   */
  dataGrid() {
    return this._dataGrid;
  }

  sortByCurrentColumn() {
    this._sortHandler();
  }

  _sortHandler() {
    var columnId = this._dataGrid.sortColumnId();
    this._networkLogView.removeAllNodeHighlights();

    var columnConfig = this._columns.find(columnConfig => columnConfig.id === columnId);
    if (!columnConfig || !columnConfig.sortingFunction)
      return;

    this._dataGrid.sortNodes(columnConfig.sortingFunction, !this._dataGrid.isSortOrderAscending());
    this._networkLogView.dataGridSorted();
  }

  _updateColumns() {
    if (!this._dataGrid)
      return;
    var visibleColumns = /** @type {!Object.<string, boolean>} */ ({});
    if (this._gridMode) {
      for (var columnConfig of this._columns)
        visibleColumns[columnConfig.id] = columnConfig.visible;
    } else {
      visibleColumns.name = true;
    }
    // First detach the waterfall since DataGrid does not know how to handle widgets.
    if (this._waterfallInnerContainer) {
      this._waterfallColumn.detach();
      this._waterfallInnerContainer = null;
    }
    this._dataGrid.setColumnsVisiblity(visibleColumns);
  }

  /**
   * @param {boolean} gridMode
   */
  switchViewMode(gridMode) {
    if (this._gridMode === gridMode)
      return;
    this._gridMode = gridMode;
    if (gridMode) {
      if (this._dataGrid.selectedNode)
        this._dataGrid.selectedNode.selected = false;
    } else {
      this._networkLogView.removeAllNodeHighlights();
    }
    this._networkLogView.element.classList.toggle('brief-mode', !gridMode);
    this._updateColumns();
  }

  /**
   * @param {!Network.NetworkLogViewColumns.Descriptor} columnConfig
   */
  _toggleColumnVisibility(columnConfig) {
    this._loadCustomColumnsAndSettings();
    columnConfig.visible = !columnConfig.visible;
    this._saveColumnsSettings();
    this._updateColumns();
  }

  _saveColumnsSettings() {
    var saveableSettings = {};
    for (var columnConfig of this._columns)
      saveableSettings[columnConfig.id] = {visible: columnConfig.visible, title: columnConfig.title};

    this._persistantSettings.set(saveableSettings);
  }

  _loadCustomColumnsAndSettings() {
    var savedSettings = this._persistantSettings.get();
    var columnIds = Object.keys(savedSettings);
    for (var columnId of columnIds) {
      var setting = savedSettings[columnId];
      var columnConfig = this._columns.find(columnConfig => columnConfig.id === columnId);
      if (!columnConfig)
        columnConfig = this._addCustomHeader(setting.title, columnId);
      if (columnConfig.hideable && typeof setting.visible === 'boolean')
        columnConfig.visible = !!setting.visible;
      if (typeof setting.title === 'string' && columnId !== 'waterfall')
        columnConfig.title = setting.title;
    }
  }

  /**
   * @param {string} title
   * @param {string} subtitle
   * @return {!DocumentFragment}
   */
  _makeHeaderFragment(title, subtitle) {
    var fragment = createDocumentFragment();
    fragment.createTextChild(title);
    var subtitleDiv = fragment.createChild('div', 'network-header-subtitle');
    subtitleDiv.createTextChild(subtitle);
    return fragment;
  }

  /**
   * @param {!UI.ContextMenu} contextMenu
   */
  _innerHeaderContextMenu(contextMenu) {
    var columnConfigs = this._columns.filter(columnConfig => columnConfig.hideable);
    var nonResponseHeaders = columnConfigs.filter(columnConfig => !columnConfig.isResponseHeader);
    for (var columnConfig of nonResponseHeaders) {
      contextMenu.appendCheckboxItem(
          columnConfig.title, this._toggleColumnVisibility.bind(this, columnConfig), columnConfig.visible);
    }

    contextMenu.appendSeparator();

    var responseSubMenu = contextMenu.appendSubMenuItem(Common.UIString('Response Headers'));
    var responseHeaders = columnConfigs.filter(columnConfig => columnConfig.isResponseHeader);
    for (var columnConfig of responseHeaders) {
      responseSubMenu.appendCheckboxItem(
          columnConfig.title, this._toggleColumnVisibility.bind(this, columnConfig), columnConfig.visible);
    }

    responseSubMenu.appendSeparator();
    responseSubMenu.appendItem(
        Common.UIString('Manage Header Columns\u2026'), this._manageCustomHeaderDialog.bind(this));

    contextMenu.appendSeparator();

    var waterfallSortIds = Network.NetworkLogViewColumns.WaterfallSortIds;
    var waterfallSubMenu = contextMenu.appendSubMenuItem(Common.UIString('Waterfall'));
    waterfallSubMenu.appendCheckboxItem(
        Common.UIString('Start Time'), setWaterfallMode.bind(this, waterfallSortIds.StartTime),
        this._activeWaterfallSortId === waterfallSortIds.StartTime);
    waterfallSubMenu.appendCheckboxItem(
        Common.UIString('Response Time'), setWaterfallMode.bind(this, waterfallSortIds.ResponseTime),
        this._activeWaterfallSortId === waterfallSortIds.ResponseTime);
    waterfallSubMenu.appendCheckboxItem(
        Common.UIString('End Time'), setWaterfallMode.bind(this, waterfallSortIds.EndTime),
        this._activeWaterfallSortId === waterfallSortIds.EndTime);
    waterfallSubMenu.appendCheckboxItem(
        Common.UIString('Total Duration'), setWaterfallMode.bind(this, waterfallSortIds.Duration),
        this._activeWaterfallSortId === waterfallSortIds.Duration);
    waterfallSubMenu.appendCheckboxItem(
        Common.UIString('Latency'), setWaterfallMode.bind(this, waterfallSortIds.Latency),
        this._activeWaterfallSortId === waterfallSortIds.Latency);

    contextMenu.show();

    /**
     * @param {!Network.NetworkLogViewColumns.WaterfallSortIds} sortId
     * @this {Network.NetworkLogViewColumns}
     */
    function setWaterfallMode(sortId) {
      var calculator = this._calculatorsMap.get(Network.NetworkLogViewColumns._calculatorTypes.Time);
      var waterfallSortIds = Network.NetworkLogViewColumns.WaterfallSortIds;
      if (sortId === waterfallSortIds.Duration || sortId === waterfallSortIds.Latency)
        calculator = this._calculatorsMap.get(Network.NetworkLogViewColumns._calculatorTypes.Duration);
      this._networkLogView.setCalculator(calculator);

      this._activeWaterfallSortId = sortId;
      this._dataGrid.markColumnAsSortedBy('waterfall', DataGrid.DataGrid.Order.Ascending);
      this._sortHandler();
    }
  }

  _manageCustomHeaderDialog() {
    var customHeaders = [];
    for (var columnConfig of this._columns) {
      if (columnConfig.isResponseHeader)
        customHeaders.push({title: columnConfig.title, editable: columnConfig.isCustomHeader});
    }
    var manageCustomHeaders = new Network.NetworkManageCustomHeadersView(
        customHeaders, headerTitle => !!this._addCustomHeader(headerTitle), this._changeCustomHeader.bind(this),
        this._removeCustomHeader.bind(this));
    var dialog = new UI.Dialog();
    manageCustomHeaders.show(dialog.contentElement);
    dialog.setSizeBehavior(UI.GlassPane.SizeBehavior.MeasureContent);
    dialog.show(this._networkLogView.element);
  }

  /**
   * @param {string} headerId
   * @return {boolean}
   */
  _removeCustomHeader(headerId) {
    headerId = headerId.toLowerCase();
    var index = this._columns.findIndex(columnConfig => columnConfig.id === headerId);
    if (index === -1)
      return false;
    this._columns.splice(index, 1);
    this._dataGrid.removeColumn(headerId);
    this._saveColumnsSettings();
    this._updateColumns();
    return true;
  }

  /**
   * @param {string} headerTitle
   * @param {string=} headerId
   * @param {number=} index
   * @return {?Network.NetworkLogViewColumns.Descriptor}
   */
  _addCustomHeader(headerTitle, headerId, index) {
    if (!headerId)
      headerId = headerTitle.toLowerCase();
    if (index === undefined)
      index = this._columns.length - 1;

    var currentColumnConfig = this._columns.find(columnConfig => columnConfig.id === headerId);
    if (currentColumnConfig)
      return null;

    var columnConfig = /** @type {!Network.NetworkLogViewColumns.Descriptor} */ (
        Object.assign({}, Network.NetworkLogViewColumns._defaultColumnConfig, {
          id: headerId,
          title: headerTitle,
          isResponseHeader: true,
          isCustomHeader: true,
          visible: true,
          sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, headerId)
        }));
    this._columns.splice(index, 0, columnConfig);
    if (this._dataGrid)
      this._dataGrid.addColumn(Network.NetworkLogViewColumns._convertToDataGridDescriptor(columnConfig), index);
    this._saveColumnsSettings();
    this._updateColumns();
    return columnConfig;
  }

  /**
   * @param {string} oldHeaderId
   * @param {string} newHeaderTitle
   * @param {string=} newHeaderId
   * @return {boolean}
   */
  _changeCustomHeader(oldHeaderId, newHeaderTitle, newHeaderId) {
    if (!newHeaderId)
      newHeaderId = newHeaderTitle.toLowerCase();
    oldHeaderId = oldHeaderId.toLowerCase();

    var oldIndex = this._columns.findIndex(columnConfig => columnConfig.id === oldHeaderId);
    var oldColumnConfig = this._columns[oldIndex];
    var currentColumnConfig = this._columns.find(columnConfig => columnConfig.id === newHeaderId);
    if (!oldColumnConfig || (currentColumnConfig && oldHeaderId !== newHeaderId))
      return false;

    this._removeCustomHeader(oldHeaderId);
    this._addCustomHeader(newHeaderTitle, newHeaderId, oldIndex);
    return true;
  }

  /**
   * @param {!Event} event
   * @return {?UI.PopoverRequest}
   */
  _getPopoverRequest(event) {
    if (!this._gridMode)
      return null;
    var hoveredNode = this._networkLogView.hoveredNode();
    if (!hoveredNode)
      return null;

    var request = hoveredNode.request();
    if (this._dataGrid.columnIdFromNode(/** @type {!Node} */ (event.target)) === 'waterfall') {
      if (!request)
        return null;
      return this._waterfallColumn.getPopoverRequest(request, event.offsetX, event.offsetY);
    }

    var anchor = event.target.enclosingNodeOrSelfWithClass('network-script-initiated');
    if (!anchor)
      return null;
    var initiator = request ? request.initiator() : null;
    if (!initiator || !initiator.stack)
      return null;
    return {
      box: anchor.boxInWindow(),
      show: popover => {
        var manager = anchor.request ? SDK.NetworkManager.forRequest(anchor.request) : null;
        var content = Components.DOMPresentationUtils.buildStackTracePreviewContents(
            manager ? manager.target() : null, this._popupLinkifier, initiator.stack);
        popover.contentElement.appendChild(content);
        return Promise.resolve(true);
      },
      hide: this._popupLinkifier.reset.bind(this._popupLinkifier)
    };
  }

  /**
   * @param {!Array<number>} times
   * @param {string} className
   */
  addEventDividers(times, className) {
    // TODO(allada) Remove this and pass in the color.
    var color = 'transparent';
    switch (className) {
      case 'network-blue-divider':
        color = 'hsla(240, 100%, 80%, 0.7)';
        break;
      case 'network-red-divider':
        color = 'rgba(255, 0, 0, 0.5)';
        break;
      default:
        return;
    }
    var currentTimes = this._eventDividers.get(color) || [];
    this._eventDividers.set(color, currentTimes.concat(times));
    this._networkLogView.scheduleRefresh();
  }

  hideEventDividers() {
    this._eventDividersShown = true;
    this._redrawWaterfallColumn();
  }

  showEventDividers() {
    this._eventDividersShown = false;
    this._redrawWaterfallColumn();
  }

  /**
   * @param {number} time
   */
  selectFilmStripFrame(time) {
    this._eventDividers.set(Network.NetworkLogViewColumns._filmStripDividerColor, [time]);
    this._redrawWaterfallColumn();
  }

  clearFilmStripFrame() {
    this._eventDividers.delete(Network.NetworkLogViewColumns._filmStripDividerColor);
    this._redrawWaterfallColumn();
  }
};

Network.NetworkLogViewColumns._initialSortColumn = 'waterfall';

/**
 * @typedef {{
 *     id: string,
 *     title: string,
 *     titleDOMFragment: (!DocumentFragment|undefined),
 *     subtitle: (string|null),
 *     visible: boolean,
 *     weight: number,
 *     hideable: boolean,
 *     nonSelectable: boolean,
 *     sortable: boolean,
 *     align: (?DataGrid.DataGrid.Align|undefined),
 *     isResponseHeader: boolean,
 *     sortingFunction: (!function(!Network.NetworkNode, !Network.NetworkNode):number|undefined),
 *     isCustomHeader: boolean
 * }}
 */
Network.NetworkLogViewColumns.Descriptor;

/** @enum {string} */
Network.NetworkLogViewColumns._calculatorTypes = {
  Duration: 'Duration',
  Time: 'Time'
};

/**
 * @type {!Object} column
 */
Network.NetworkLogViewColumns._defaultColumnConfig = {
  subtitle: null,
  visible: false,
  weight: 6,
  sortable: true,
  hideable: true,
  nonSelectable: true,
  isResponseHeader: false,
  alwaysVisible: false,
  isCustomHeader: false
};

/**
 * @type {!Array.<!Network.NetworkLogViewColumns.Descriptor>} column
 */
Network.NetworkLogViewColumns._defaultColumns = [
  {
    id: 'name',
    title: Common.UIString('Name'),
    subtitle: Common.UIString('Path'),
    visible: true,
    weight: 20,
    hideable: false,
    nonSelectable: false,
    alwaysVisible: true,
    sortingFunction: Network.NetworkRequestNode.NameComparator
  },
  {
    id: 'method',
    title: Common.UIString('Method'),
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'requestMethod')
  },
  {
    id: 'status',
    title: Common.UIString('Status'),
    visible: true,
    subtitle: Common.UIString('Text'),
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'statusCode')
  },
  {
    id: 'protocol',
    title: Common.UIString('Protocol'),
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'protocol')
  },
  {
    id: 'scheme',
    title: Common.UIString('Scheme'),
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'scheme')
  },
  {
    id: 'domain',
    title: Common.UIString('Domain'),
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'domain')
  },
  {
    id: 'remoteaddress',
    title: Common.UIString('Remote Address'),
    weight: 10,
    align: DataGrid.DataGrid.Align.Right,
    sortingFunction: Network.NetworkRequestNode.RemoteAddressComparator
  },
  {
    id: 'type',
    title: Common.UIString('Type'),
    visible: true,
    sortingFunction: Network.NetworkRequestNode.TypeComparator
  },
  {
    id: 'initiator',
    title: Common.UIString('Initiator'),
    visible: true,
    weight: 10,
    sortingFunction: Network.NetworkRequestNode.InitiatorComparator
  },
  {
    id: 'cookies',
    title: Common.UIString('Cookies'),
    align: DataGrid.DataGrid.Align.Right,
    sortingFunction: Network.NetworkRequestNode.RequestCookiesCountComparator
  },
  {
    id: 'setcookies',
    title: Common.UIString('Set Cookies'),
    align: DataGrid.DataGrid.Align.Right,
    sortingFunction: Network.NetworkRequestNode.ResponseCookiesCountComparator
  },
  {
    id: 'size',
    title: Common.UIString('Size'),
    visible: true,
    subtitle: Common.UIString('Content'),
    align: DataGrid.DataGrid.Align.Right,
    sortingFunction: Network.NetworkRequestNode.SizeComparator
  },
  {
    id: 'time',
    title: Common.UIString('Time'),
    visible: true,
    subtitle: Common.UIString('Latency'),
    align: DataGrid.DataGrid.Align.Right,
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'duration')
  },
  {
    id: 'priority',
    title: Common.UIString('Priority'),
    sortingFunction: Network.NetworkRequestNode.InitialPriorityComparator
  },
  {
    id: 'connectionid',
    title: Common.UIString('Connection ID'),
    sortingFunction: Network.NetworkRequestNode.RequestPropertyComparator.bind(null, 'connectionId')
  },
  {
    id: 'cache-control',
    isResponseHeader: true,
    title: Common.UIString('Cache-Control'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'cache-control')
  },
  {
    id: 'connection',
    isResponseHeader: true,
    title: Common.UIString('Connection'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'connection')
  },
  {
    id: 'content-encoding',
    isResponseHeader: true,
    title: Common.UIString('Content-Encoding'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'content-encoding')
  },
  {
    id: 'content-length',
    isResponseHeader: true,
    title: Common.UIString('Content-Length'),
    align: DataGrid.DataGrid.Align.Right,
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderNumberComparator.bind(null, 'content-length')
  },
  {
    id: 'etag',
    isResponseHeader: true,
    title: Common.UIString('ETag'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'etag')
  },
  {
    id: 'keep-alive',
    isResponseHeader: true,
    title: Common.UIString('Keep-Alive'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'keep-alive')
  },
  {
    id: 'last-modified',
    isResponseHeader: true,
    title: Common.UIString('Last-Modified'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderDateComparator.bind(null, 'last-modified')
  },
  {
    id: 'server',
    isResponseHeader: true,
    title: Common.UIString('Server'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'server')
  },
  {
    id: 'vary',
    isResponseHeader: true,
    title: Common.UIString('Vary'),
    sortingFunction: Network.NetworkRequestNode.ResponseHeaderStringComparator.bind(null, 'vary')
  },
  {
    id: 'waterfall',
    title: Common.UIString('Waterfall'),
    visible: true,
    hideable: false
    // Sorting function setup in waterfall function.
  }
];

Network.NetworkLogViewColumns._filmStripDividerColor = '#fccc49';

/**
 * @enum {string}
 */
Network.NetworkLogViewColumns.WaterfallSortIds = {
  StartTime: 'startTime',
  ResponseTime: 'responseReceivedTime',
  EndTime: 'endTime',
  Duration: 'duration',
  Latency: 'latency'
};
