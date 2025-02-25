// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {SDK.SDKModelObserver<!SDK.ServiceWorkerManager>}
 * @unrestricted
 */
Audits2.Audits2Panel = class extends UI.Panel {
  constructor() {
    super('audits2');
    this.registerRequiredCSS('audits2/lighthouse/report-styles.css');
    this.registerRequiredCSS('audits2/audits2Panel.css');

    this._protocolService = new Audits2.ProtocolService();

    var toolbar = new UI.Toolbar('', this.element);

    var newButton = new UI.ToolbarButton(Common.UIString('New audit\u2026'), 'largeicon-add');
    toolbar.appendToolbarItem(newButton);
    newButton.addEventListener(UI.ToolbarButton.Events.Click, this._showDialog.bind(this));

    var downloadButton = new UI.ToolbarButton(Common.UIString('Download report'), 'largeicon-download');
    toolbar.appendToolbarItem(downloadButton);
    downloadButton.addEventListener(UI.ToolbarButton.Events.Click, this._downloadSelected.bind(this));

    toolbar.appendSeparator();

    this._reportSelector = new Audits2.ReportSelector();
    toolbar.appendToolbarItem(this._reportSelector.comboBox());

    var clearButton = new UI.ToolbarButton(Common.UIString('Clear all'), 'largeicon-clear');
    toolbar.appendToolbarItem(clearButton);
    clearButton.addEventListener(UI.ToolbarButton.Events.Click, this._clearAll.bind(this));

    this._auditResultsElement = this.contentElement.createChild('div', 'audits2-results-container');
    this._dropTarget = new UI.DropTarget(
        this.contentElement, [UI.DropTarget.Type.File], Common.UIString('Drop audit file here'),
        this._handleDrop.bind(this));

    for (var preset of Audits2.Audits2Panel.Presets)
      preset.setting.addChangeListener(this._refreshDialogUI.bind(this));
    this._showLandingPage();
    SDK.targetManager.observeModels(SDK.ServiceWorkerManager, this);
    SDK.targetManager.addEventListener(SDK.TargetManager.Events.InspectedURLChanged, this._refreshDialogUI, this);
  }

  /**
   * @override
   * @param {!SDK.ServiceWorkerManager} serviceWorkerManager
   */
  modelAdded(serviceWorkerManager) {
    if (this._manager)
      return;

    this._manager = serviceWorkerManager;
    this._serviceWorkerListeners = [
      this._manager.addEventListener(SDK.ServiceWorkerManager.Events.RegistrationUpdated, this._refreshDialogUI, this),
      this._manager.addEventListener(SDK.ServiceWorkerManager.Events.RegistrationDeleted, this._refreshDialogUI, this),
    ];

    this._refreshDialogUI();
  }

  /**
   * @override
   * @param {!SDK.ServiceWorkerManager} serviceWorkerManager
   */
  modelRemoved(serviceWorkerManager) {
    if (!this._manager || this._manager !== serviceWorkerManager)
      return;

    Common.EventTarget.removeEventListeners(this._serviceWorkerListeners);
    this._manager = null;
    this._serviceWorkerListeners = null;
    this._refreshDialogUI();
  }

  /**
   * @return {boolean}
   */
  _hasActiveServiceWorker() {
    if (!this._manager)
      return false;

    var mainTarget = SDK.targetManager.mainTarget();
    if (!mainTarget)
      return false;

    var inspectedURL = mainTarget.inspectedURL().asParsedURL();
    var inspectedOrigin = inspectedURL && inspectedURL.securityOrigin();
    for (var registration of this._manager.registrations().values()) {
      if (registration.securityOrigin !== inspectedOrigin)
        continue;

      for (var version of registration.versions.values()) {
        if (version.controlledClients.length > 1)
          return true;
      }
    }

    return false;
  }

  /**
   * @return {boolean}
   */
  _hasAtLeastOneCategory() {
    return Audits2.Audits2Panel.Presets.some(preset => preset.setting.get());
  }

  /**
   * @return {?string}
   */
  _unauditablePageMessage() {
    if (!this._manager)
      return null;

    var mainTarget = SDK.targetManager.mainTarget();
    var inspectedURL = mainTarget && mainTarget.inspectedURL();
    if (inspectedURL && !/^(http|chrome-extension)/.test(inspectedURL)) {
      return Common.UIString(
          'Can only audit HTTP/HTTPS pages and Chrome extensions. ' +
          'Navigate to a different page to start an audit.');
    }

    // Audits don't work on most undockable targets (extension popup pages, remote debugging, etc).
    // However, the tests run in a content shell which is not dockable yet audits just fine,
    // so disable this check when under test.
    if (!Host.isUnderTest() && !Runtime.queryParam('can_dock'))
      return Common.UIString('Can only audit tabs. Navigate to this page in a separate tab to start an audit.');

    return null;
  }

  _refreshDialogUI() {
    if (!this._dialog)
      return;

    var hasActiveServiceWorker = this._hasActiveServiceWorker();
    var hasAtLeastOneCategory = this._hasAtLeastOneCategory();
    var unauditablePageMessage = this._unauditablePageMessage();
    var isDisabled = hasActiveServiceWorker || !hasAtLeastOneCategory || !!unauditablePageMessage;

    var helpText = '';
    if (hasActiveServiceWorker) {
      helpText = Common.UIString(
          'Multiple tabs are being controlled by the same service worker. ' +
          'Close your other tabs on the same origin to audit this page.');
    } else if (!hasAtLeastOneCategory) {
      helpText = Common.UIString('At least one category must be selected.');
    } else if (unauditablePageMessage) {
      helpText = unauditablePageMessage;
    }

    this._dialog.setHelpText(helpText);
    this._dialog.setStartEnabled(!isDisabled);
  }

  _clearAll() {
    this._reportSelector.clearAll();
    this._showLandingPage();
  }

  _downloadSelected() {
    this._reportSelector.downloadSelected();
  }

  _showLandingPage() {
    if (this._reportSelector.comboBox().size())
      return;

    this._auditResultsElement.removeChildren();
    var landingPage = this._auditResultsElement.createChild('div', 'vbox audits2-landing-page');
    var landingCenter = landingPage.createChild('div', 'vbox audits2-landing-center');
    landingCenter.createChild('div', 'audits2-logo');
    var text = landingCenter.createChild('div', 'audits2-landing-text');
    text.createChild('span', 'audits2-landing-bold-text').textContent = Common.UIString('Audits');
    text.createChild('span').textContent = Common.UIString(
        ' help you identify and fix common problems that affect' +
        ' your site\'s performance, accessibility, and user experience. ');
    var link = text.createChild('span', 'link');
    link.textContent = Common.UIString('Learn more');
    link.addEventListener(
        'click', () => InspectorFrontendHost.openInNewTab('https://developers.google.com/web/tools/lighthouse/'));

    var newButton = UI.createTextButton(
        Common.UIString('Perform an audit\u2026'), this._showDialog.bind(this), '', true /* primary */);
    landingCenter.appendChild(newButton);
    this.setDefaultFocusedElement(newButton);
  }

  _showDialog() {
    this._dialog = new Audits2.Audits2Dialog(result => this._buildReportUI(result), this._protocolService);
    this._dialog.render(this._auditResultsElement);
    this._refreshDialogUI();
  }

  _hideDialog() {
    if (!this._dialog)
      return;

    this._dialog.hide();
    delete this._dialog;
  }

  /**
   * @param {!ReportRenderer.ReportJSON} lighthouseResult
   */
  _buildReportUI(lighthouseResult) {
    if (lighthouseResult === null)
      return;

    var optionElement =
        new Audits2.ReportSelector.Item(lighthouseResult, this._auditResultsElement, this._showLandingPage.bind(this));
    this._reportSelector.prepend(optionElement);
    this._hideDialog();
  }

  /**
   * @param {!DataTransfer} dataTransfer
   */
  _handleDrop(dataTransfer) {
    var items = dataTransfer.items;
    if (!items.length)
      return;
    var item = items[0];
    if (item.kind === 'file') {
      var entry = items[0].webkitGetAsEntry();
      if (!entry.isFile)
        return;
      entry.file(file => {
        var reader = new FileReader();
        reader.onload = () => this._loadedFromFile(/** @type {string} */ (reader.result));
        reader.readAsText(file);
      });
    }
  }

  /**
   * @param {string} profile
   */
  _loadedFromFile(profile) {
    var data = JSON.parse(profile);
    if (!data['lighthouseVersion'])
      return;
    this._buildReportUI(/** @type {!ReportRenderer.ReportJSON} */ (data));
  }
};

/**
 * @override
 */
Audits2.Audits2Panel.ReportRenderer = class extends ReportRenderer {
  /**
   * Provides empty element for left nav
   * @override
   * @returns {!DocumentFragment}
   */
  _renderReportNav() {
    return createDocumentFragment();
  }

  /**
   * @param {!ReportRenderer.ReportJSON} report
   * @override
   * @return {!DocumentFragment}
   */
  _renderReportHeader(report) {
    return createDocumentFragment();
  }
};

class ReportUIFeatures {
  /**
   * @param {!ReportRenderer.ReportJSON} report
   */
  initFeatures(report) {
  }
}

/** @typedef {{type: string, setting: !Common.Setting, configID: string, title: string, description: string}} */
Audits2.Audits2Panel.Preset;

/** @type {!Array.<!Audits2.Audits2Panel.Preset>} */
Audits2.Audits2Panel.Presets = [
  // configID maps to Lighthouse's Object.keys(config.categories)[0] value
  {
    setting: Common.settings.createSetting('audits2.cat_perf', true),
    configID: 'performance',
    title: 'Performance',
    description: 'How long does this app take to show content and become usable'
  },
  {
    setting: Common.settings.createSetting('audits2.cat_pwa', true),
    configID: 'pwa',
    title: 'Progressive Web App',
    description: 'Does this page meet the standard of a Progressive Web App'
  },
  {
    setting: Common.settings.createSetting('audits2.cat_best_practices', true),
    configID: 'best-practices',
    title: 'Best practices',
    description: 'Does this page follow best practices for modern web development'
  },
  {
    setting: Common.settings.createSetting('audits2.cat_a11y', true),
    configID: 'accessibility',
    title: 'Accessibility',
    description: 'Is this page usable by people with disabilities or impairments'
  },
  {
    setting: Common.settings.createSetting('audits2.cat_seo', true),
    configID: 'seo',
    title: 'SEO',
    description: 'Is this page optimized for search engine results ranking'
  },
];

Audits2.ReportSelector = class {
  constructor() {
    this._comboBox = new UI.ToolbarComboBox(this._handleChange.bind(this), 'audits2-report');
    this._comboBox.setMaxWidth(270);
    this._comboBox.setMinWidth(200);
    this._itemByOptionElement = new Map();
  }

  /**
   * @param {!Event} event
   */
  _handleChange(event) {
    var item = this._selectedItem();
    if (item)
      item.select();
  }

  /**
   * @return {!Audits2.ReportSelector.Item}
   */
  _selectedItem() {
    var option = this._comboBox.selectedOption();
    return this._itemByOptionElement.get(option);
  }

  /**
   * @return {!UI.ToolbarComboBox}
   */
  comboBox() {
    return this._comboBox;
  }

  /**
   * @param {!Audits2.ReportSelector.Item} item
   */
  prepend(item) {
    var optionEl = item.optionElement();
    var selectEl = this._comboBox.selectElement();

    this._itemByOptionElement.set(optionEl, item);
    selectEl.insertBefore(optionEl, selectEl.firstElementChild);
    this._comboBox.select(optionEl);
    item.select();
  }

  clearAll() {
    for (var elem of this._comboBox.options())
      this._itemByOptionElement.get(elem).delete();
  }

  downloadSelected() {
    var item = this._selectedItem();
    item.download();
  }
};

Audits2.ReportSelector.Item = class {
  /**
   * @param {!ReportRenderer.ReportJSON} lighthouseResult
   * @param {!Element} resultsView
   * @param {function()} showLandingCallback
   */
  constructor(lighthouseResult, resultsView, showLandingCallback) {
    this._lighthouseResult = lighthouseResult;
    this._resultsView = resultsView;
    this._showLandingCallback = showLandingCallback;
    /** @type {?Element} */
    this._reportContainer = null;


    var url = new Common.ParsedURL(lighthouseResult.url);
    var timestamp = lighthouseResult.generatedTime;
    this._element = createElement('option');
    this._element.label = `${url.domain()} ${new Date(timestamp).toLocaleString()}`;
  }

  select() {
    this._renderReport();
  }

  /**
   * @return {!Element}
   */
  optionElement() {
    return this._element;
  }

  delete() {
    if (this._element)
      this._element.remove();
    this._showLandingCallback();
  }

  download() {
    var url = new Common.ParsedURL(this._lighthouseResult.url).domain();
    var timestamp = this._lighthouseResult.generatedTime;
    var fileName = `${url}-${new Date(timestamp).toISO8601Compact()}.json`;
    Workspace.fileManager.save(fileName, JSON.stringify(this._lighthouseResult), true);
  }

  _renderReport() {
    this._resultsView.removeChildren();
    if (this._reportContainer) {
      this._resultsView.appendChild(this._reportContainer);
      return;
    }

    this._reportContainer = this._resultsView.createChild('div', 'lh-vars lh-root lh-devtools');

    var dom = new DOM(/** @type {!Document} */ (this._resultsView.ownerDocument));
    var detailsRenderer = new Audits2.DetailsRenderer(dom);
    var categoryRenderer = new CategoryRenderer(dom, detailsRenderer);
    var renderer = new Audits2.Audits2Panel.ReportRenderer(dom, categoryRenderer);

    var templatesHTML = Runtime.cachedResources['audits2/lighthouse/templates.html'];
    var templatesDOM = new DOMParser().parseFromString(templatesHTML, 'text/html');
    if (!templatesDOM)
      return;

    renderer.setTemplateContext(templatesDOM);
    renderer.renderReport(this._lighthouseResult, this._reportContainer);
  }
};

Audits2.DetailsRenderer = class extends DetailsRenderer {
  /**
   * @param {!DOM} dom
   */
  constructor(dom) {
    super(dom);
    this._onMainFrameNavigatedPromise = null;
  }

  /**
   * @override
   * @param {!DetailsRenderer.NodeDetailsJSON} item
   * @return {!Element}
   */
  renderNode(item) {
    var element = super.renderNode(item);
    this._replaceWithDeferredNodeBlock(element, item);
    return element;
  }

  /**
   * @param {!Element} origElement
   * @param {!DetailsRenderer.NodeDetailsJSON} detailsItem
   */
  async _replaceWithDeferredNodeBlock(origElement, detailsItem) {
    var mainTarget = SDK.targetManager.mainTarget();
    if (!this._onMainFrameNavigatedPromise) {
      var resourceTreeModel = mainTarget.model(SDK.ResourceTreeModel);
      this._onMainFrameNavigatedPromise = resourceTreeModel.once(SDK.ResourceTreeModel.Events.MainFrameNavigated);
    }

    await this._onMainFrameNavigatedPromise;

    var domModel = mainTarget.model(SDK.DOMModel);
    if (!detailsItem.path)
      return;

    var nodeId = await domModel.pushNodeByPathToFrontend(detailsItem.path);

    if (!nodeId)
      return;
    var node = domModel.nodeForId(nodeId);
    if (!node)
      return;

    var element = Components.DOMPresentationUtils.linkifyNodeReference(node, undefined, detailsItem.snippet);
    origElement.title = '';
    origElement.textContent = '';
    origElement.appendChild(element);
  }
};
