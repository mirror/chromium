// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @unrestricted
 */
Audits2.Audits2Dialog = class {
  constructor(onAuditComplete) {
    this._onAuditComplete = onAuditComplete;

    this._protocolService = new Audits2.ProtocolService();
    this._protocolService.registerStatusCallback(msg => this._updateStatus(Common.UIString(msg)));
  }

  /**
   * @param {!Element} dialogRenderElement
   */
  render(dialogRenderElement) {
    this._dialog = new UI.Dialog();
    this._dialog.setOutsideClickCallback(event => event.consume(true));
    var root = UI.createShadowRootWithCoreStyles(this._dialog.contentElement, 'audits2/audits2Dialog.css');
    var auditsViewElement = root.createChild('div', 'audits2-view');

    var closeButton = auditsViewElement.createChild('div', 'dialog-close-button', 'dt-close-button');
    closeButton.addEventListener('click', () => this._cancelAndClose());

    var uiElement = auditsViewElement.createChild('div');
    var headerElement = uiElement.createChild('header');
    this._headerTitleElement = headerElement.createChild('p');
    this._headerTitleElement.textContent = Common.UIString('Audits to perform');
    uiElement.appendChild(headerElement);

    this._auditSelectorForm = uiElement.createChild('form', 'audits2-form');

    for (var preset of Audits2.Audits2Panel.Presets) {
      preset.setting.setTitle(preset.title);
      var checkbox = new UI.ToolbarSettingCheckbox(preset.setting);
      var row = this._auditSelectorForm.createChild('div', 'vbox audits2-launcher-row');
      row.appendChild(checkbox.element);
      row.createChild('span', 'audits2-launcher-description dimmed').textContent = preset.description;
    }

    this._statusView = new Audits2.Audits2Dialog.StatusView();
    this._statusView.render(uiElement);
    this._dialogHelpText = uiElement.createChild('div', 'audits2-dialog-help-text');

    var buttonsRow = uiElement.createChild('div', 'audits2-dialog-buttons hbox');
    this._startButton =
        UI.createTextButton(Common.UIString('Run audit'), this._start.bind(this), '', true /* primary */);
    this._startButton.autofocus = true;
    buttonsRow.appendChild(this._startButton);
    this._cancelButton = UI.createTextButton(Common.UIString('Cancel'), this._cancel.bind(this));
    buttonsRow.appendChild(this._cancelButton);

    this._dialog.setSizeBehavior(UI.GlassPane.SizeBehavior.SetExactWidthMaxHeight);
    this._dialog.setMaxContentSize(new UI.Size(500, 400));
    this._dialog.show(dialogRenderElement);
    auditsViewElement.tabIndex = 0;
    auditsViewElement.focus();
  }

  hide() {
    if (!this._dialog)
      return;

    this._dialog.hide();

    delete this._dialog;
    delete this._statusView;
    delete this._startButton;
    delete this._cancelButton;
    delete this._auditSelectorForm;
    delete this._headerTitleElement;
    delete this._emulationEnabledBefore;
    delete this._emulationOutlineEnabledBefore;
  }

  setStartEnabled(isEnabled) {
    if (this._dialogHelpText)
      this._dialogHelpText.classList.toggle('hidden', isEnabled);

    if (this._startButton)
      this._startButton.disabled = !isEnabled;
  }

  setHelpText(text) {
    if (this._dialogHelpText)
      this._dialogHelpText.textContent = text;
  }

  _updateStatus(message) {
    if (!this._statusView)
      return;
    this._statusView.updateStatus(message);
  }

  _cancelAndClose() {
    this._cancel();
    this.hide();
  }

  async _cancel() {
    if (this._auditRunning) {
      this._updateStatus(Common.UIString('Cancelling\u2026'));
      await this._stopAndReattach();

      if (this._statusView)
        this._statusView.reset();
    } else {
      this.hide();
    }
  }

  /**
   * @return {!Promise<undefined>}
   */
  _getInspectedURL() {
    var mainTarget = SDK.targetManager.mainTarget();
    var runtimeModel = mainTarget.model(SDK.RuntimeModel);
    var executionContext = runtimeModel && runtimeModel.defaultExecutionContext();
    var inspectedURL = mainTarget.inspectedURL();
    if (!executionContext)
      return Promise.resolve(inspectedURL);

    // Evaluate location.href for a more specific URL than inspectedURL provides so that SPA hash navigation routes
    // will be respected and audited.
    return executionContext
        .evaluate(
            {
              expression: 'window.location.href',
              objectGroup: 'audits',
              includeCommandLineAPI: false,
              silent: false,
              returnByValue: true,
              generatePreview: false
            },
            /* userGesture */ false, /* awaitPromise */ false)
        .then(result => {
          if (!result.exceptionDetails && result.object) {
            inspectedURL = result.object.value;
            result.object.release();
          }

          return inspectedURL;
        });
  }

  _start() {
    var emulationModel = self.singleton(Emulation.DeviceModeModel);
    this._emulationEnabledBefore = emulationModel.enabledSetting().get();
    this._emulationOutlineEnabledBefore = emulationModel.deviceOutlineSetting().get();
    emulationModel.enabledSetting().set(true);
    emulationModel.deviceOutlineSetting().set(true);
    emulationModel.toolbarControlsEnabledSetting().set(false);

    for (var device of Emulation.EmulatedDevicesList.instance().standard()) {
      if (device.title === 'Nexus 5X')
        emulationModel.emulate(Emulation.DeviceModeModel.Type.Device, device, device.modes[0], 1);
    }
    this._dialog.setCloseOnEscape(false);

    var categoryIDs = [];
    for (var preset of Audits2.Audits2Panel.Presets) {
      if (preset.setting.get())
        categoryIDs.push(preset.configID);
    }
    Host.userMetrics.actionTaken(Host.UserMetrics.Action.Audits2Started);

    return Promise.resolve()
        .then(_ => this._getInspectedURL())
        .then(url => this._auditURL = url)
        .then(_ => this._protocolService.attach())
        .then(_ => {
          this._auditRunning = true;
          this._updateButton(this._auditURL);
          this._updateStatus(Common.UIString('Loading\u2026'));
        })
        .then(_ => this._protocolService.startLighthouse(this._auditURL, categoryIDs))
        .then(lighthouseResult => {
          if (lighthouseResult && lighthouseResult.fatal) {
            const error = new Error(lighthouseResult.message);
            error.stack = lighthouseResult.stack;
            throw error;
          }

          if (!lighthouseResult)
            throw new Error('Auditing failed to produce a result');

          return this._stopAndReattach().then(() => this._onAuditComplete(lighthouseResult));
        })
        .catch(err => {
          if (err instanceof Error)
            this._statusView.renderBugReport(err, this._auditURL);
        });
  }

  /**
   * @return {!Promise<undefined>}
   */
  async _stopAndReattach() {
    await this._protocolService.detach();

    var emulationModel = self.singleton(Emulation.DeviceModeModel);
    emulationModel.enabledSetting().set(this._emulationEnabledBefore);
    emulationModel.deviceOutlineSetting().set(this._emulationOutlineEnabledBefore);
    emulationModel.toolbarControlsEnabledSetting().set(true);
    Emulation.InspectedPagePlaceholder.instance().update(true);

    Host.userMetrics.actionTaken(Host.UserMetrics.Action.Audits2Finished);
    var resourceTreeModel = SDK.targetManager.mainTarget().model(SDK.ResourceTreeModel);
    // reload to reset the page state
    await resourceTreeModel.navigate(this._auditURL);
    this._auditRunning = false;
    this._updateButton(this._auditURL);
  }

  _updateButton(auditURL) {
    if (!this._dialog)
      return;
    this._startButton.classList.toggle('hidden', this._auditRunning);
    this._startButton.disabled = this._auditRunning;
    this._statusView.setVisible(this._auditRunning);
    this._auditSelectorForm.classList.toggle('hidden', this._auditRunning);
    if (this._auditRunning) {
      var parsedURL = (auditURL || '').asParsedURL();
      var pageHost = parsedURL && parsedURL.host;
      this._headerTitleElement.textContent =
          pageHost ? ls`Auditing ${pageHost}\u2026` : ls`Auditing your web page\u2026`;
    } else {
      this._headerTitleElement.textContent = Common.UIString('Audits to perform');
    }
  }
};

Audits2.Audits2Dialog.StatusView = class {
  constructor() {
    this._statusView = null;
    this._progressWrapper = null;
    this._progressBar = null;
    this._statusText = null;

    this._textChangedAt = 0;
    this._fastFactsQueued = Audits2.Audits2Dialog.StatusView.FastFacts.slice();
    this._currentPhase = null;
    this._scheduledTextChangeTimeout = null;
    this._scheduledFastFactTimeout = null;
  }

  /**
   * @param {!Element} parentElement
   */
  render(parentElement) {
    this.reset();

    this._statusView = parentElement.createChild('div', 'audits2-status vbox hidden');
    this._progressWrapper = this._statusView.createChild('div', 'audits2-progress-wrapper');
    this._progressBar = this._progressWrapper.createChild('div', 'audits2-progress-bar');
    this._statusText = this._statusView.createChild('div', 'audits2-status-text');

    this.updateStatus(Common.UIString('Loading...'));
  }

  reset() {
    this._resetProgressBarClasses();
    clearTimeout(this._scheduledFastFactTimeout);

    this._textChangedAt = 0;
    this._fastFactsQueued = Audits2.Audits2Dialog.StatusView.FastFacts.slice();
    this._currentPhase = null;
    this._scheduledTextChangeTimeout = null;
    this._scheduledFastFactTimeout = null;
  }

  /**
   * @param {boolean} isVisible
   */
  setVisible(isVisible) {
    this._statusView.classList.toggle('hidden', !isVisible);

    if (!isVisible)
      clearTimeout(this._scheduledFastFactTimeout);
  }

  /**
   * @param {?string} message
   */
  updateStatus(message) {
    if (!message || !this._statusText)
      return;

    if (message.startsWith('Cancel')) {
      this._commitTextChange(Common.UIString('Cancelling\u2026'));
      clearTimeout(this._scheduledFastFactTimeout);
      return;
    }

    var nextPhase = this._getPhaseForMessage(message);
    if (!nextPhase && !this._currentPhase) {
      this._commitTextChange(Common.UIString('Lighthouse is warming up\u2026'));
      clearTimeout(this._scheduledFastFactTimeout);
    } else if (nextPhase && (!this._currentPhase || this._currentPhase.order < nextPhase.order)) {
      this._currentPhase = nextPhase;
      this._scheduleTextChange(Common.UIString(nextPhase.message));
      this._scheduleFastFactCheck();
      this._resetProgressBarClasses();
      this._progressBar.classList.add(nextPhase.progressBarClass);
    }
  }

  /**
   * @param {string} message
   * @return {?Audits2.Audits2Dialog.StatusView.StatusPhases}
   */
  _getPhaseForMessage(message) {
    return Audits2.Audits2Dialog.StatusView.StatusPhases.find(phase => message.startsWith(phase.statusMessagePrefix));
  }

  _resetProgressBarClasses() {
    if (!this._progressBar)
      return;

    this._progressBar.className = 'audits2-progress-bar';
  }

  _scheduleFastFactCheck() {
    if (!this._currentPhase || this._scheduledFastFactTimeout)
      return;

    this._scheduledFastFactTimeout = setTimeout(() => {
      this._updateFastFactIfNecessary();
      this._scheduledFastFactTimeout = null;

      this._scheduleFastFactCheck();
    }, 100);
  }

  _updateFastFactIfNecessary() {
    var now = performance.now();
    if (now - this._textChangedAt < Audits2.Audits2Dialog.StatusView.fastFactRotationInterval)
      return;
    if (!this._fastFactsQueued.length)
      return;

    var fastFactIndex = Math.floor(Math.random() * this._fastFactsQueued.length);
    this._scheduleTextChange(ls`\ud83d\udca1 ${this._fastFactsQueued[fastFactIndex]}`);
    this._fastFactsQueued.splice(fastFactIndex, 1);
  }

  /**
   * @param {string} text
   */
  _commitTextChange(text) {
    if (!this._statusText)
      return;
    this._textChangedAt = performance.now();
    this._statusText.textContent = text;
  }

  /**
   * @param {string} text
   */
  _scheduleTextChange(text) {
    if (this._scheduledTextChangeTimeout)
      clearTimeout(this._scheduledTextChangeTimeout);

    var msSinceLastChange = performance.now() - this._textChangedAt;
    var msToTextChange = Audits2.Audits2Dialog.StatusView.minimumTextVisibilityDuration - msSinceLastChange;

    this._scheduledTextChangeTimeout = setTimeout(() => {
      this._commitTextChange(text);
    }, Math.max(msToTextChange, 0));
  }

  /**
   * @param {!Error} err
   * @param {string} auditURL
   */
  renderBugReport(err, auditURL) {
    console.error(err);
    clearTimeout(this._scheduledFastFactTimeout);
    clearTimeout(this._scheduledTextChangeTimeout);
    this._resetProgressBarClasses();
    this._progressBar.classList.add('errored');

    this._commitTextChange('');
    this._statusText.createTextChild(Common.UIString('Ah, sorry! We ran into an error: '));
    this._statusText.createChild('em').createTextChild(err.message);
    if (Audits2.Audits2Dialog.StatusView.KnownBugPatterns.some(pattern => pattern.test(err.message))) {
      var message = Common.UIString(
          'Try to navigate to the URL in a fresh Chrome profile without any other tabs or ' +
          'extensions open and try again.');
      this._statusText.createChild('p').createTextChild(message);
    } else {
      this._renderBugReportLink(err, auditURL);
    }
  }

  /**
   * @param {!Error} err
   * @param {string} auditURL
   */
  _renderBugReportLink(err, auditURL) {
    var baseURI = 'https://github.com/GoogleChrome/lighthouse/issues/new?';
    var title = encodeURI('title=DevTools Error: ' + err.message.substring(0, 60));

    var issueBody = `
**Initial URL**: ${auditURL}
**Chrome Version**: ${navigator.userAgent.match(/Chrome\/(\S+)/)[1]}
**Error Message**: ${err.message}
**Stack Trace**:
\`\`\`
${err.stack}
\`\`\`
    `;
    var body = '&body=' + encodeURIComponent(issueBody.trim());
    var reportErrorEl = UI.XLink.create(
        baseURI + title + body, Common.UIString('Report this bug'), 'audits2-link audits2-report-error');
    this._statusText.appendChild(reportErrorEl);
  }
};


/** @type {!Array.<!RegExp>} */
Audits2.Audits2Dialog.StatusView.KnownBugPatterns = [
  /Parsing problem/,
  /Read failed/,
  /Tracing.*already started/,
  /^Unable to load.*page/,
  /^You must provide a url to the runner/,
  /^You probably have multiple tabs open/,
];

/** @typedef {{message: string, progressBarClass: string, order: number}} */
Audits2.Audits2Dialog.StatusView.StatusPhases = [
  {
    progressBarClass: 'loading',
    message: 'Lighthouse is loading your page with throttling to measure performance on a mobile device on 3G.',
    statusMessagePrefix: 'Loading page',
    order: 10,
  },
  {
    progressBarClass: 'gathering',
    message: 'Lighthouse is gathering information about the page to compute your score.',
    statusMessagePrefix: 'Retrieving',
    order: 20,
  },
  {
    progressBarClass: 'auditing',
    message: 'Almost there! Lighthouse is now generating your own special pretty report!',
    statusMessagePrefix: 'Evaluating',
    order: 30,
  }
];

Audits2.Audits2Dialog.StatusView.FastFacts = [
  '1MB takes a minimum of 5 seconds to download on a typical 3G connection [Source: WebPageTest and DevTools 3G definition].',
  'Rebuilding Pinterest pages for performance increased conversion rates by 15% [Source: WPO Stats]',
  'BBC has seen a loss of 10% of their users for every extra second of page load [Source: WPO Stats]',
  'By reducing the response size of JSON needed for displaying comments, Instagram saw increased impressions [Source: WPO Stats]',
  'Walmart saw a 1% increase in revenue for every 100ms improvement in page load [Source: WPO Stats]',
  'If a site takes >1 second to become interactive, users lose attention, and their perception of completing the page task is broken [Source: Google Developers Blog]',
  '75% of global mobile users in 2016 were on 2G or 3G [Source: GSMA Mobile]',
  'The average user device costs less than 200 USD. [Source: International Data Corporation]',
  '53% of all site visits are abandoned if page load takes more than 3 seconds [Source: Google DoubleClick blog]',
  '19 seconds is the average time a mobile web page takes to load on a 3G connection [Source: Google DoubleClick blog]',
  '14 seconds is the average time a mobile web page takes to load on a 4G connection [Source: Google DoubleClick blog]',
  '70% of mobile pages take nearly 7 seconds for the visual content above the fold to display on the screen. [Source: Think with Google]',
  'As page load time increases from one second to seven seconds, the probability of a mobile site visitor bouncing increases 113%. [Source: Think with Google]',
  'As the number of elements on a page increases from 400 to 6,000, the probability of conversion drops 95%. [Source: Think with Google]',
  '70% of mobile pages weigh over 1MB, 36% over 2MB, and 12% over 4MB. [Source: Think with Google]',
  'Lighthouse only simulates mobile performance; to measure performance on a real device, try WebPageTest.org [Source: Lighthouse team]',
];

/** @const */
Audits2.Audits2Dialog.StatusView.fastFactRotationInterval = 6000;
/** @const */
Audits2.Audits2Dialog.StatusView.minimumTextVisibilityDuration = 3000;

Audits2.ProtocolService = class extends Common.Object {
  constructor() {
    super();
    /** @type {?Protocol.InspectorBackend.Connection} */
    this._rawConnection = null;
    /** @type {?Services.ServiceManager.Service} */
    this._backend = null;
    /** @type {?Promise} */
    this._backendPromise = null;
    /** @type {?function(string)} */
    this._status = null;
  }

  /**
   * @return {!Promise<undefined>}
   */
  attach() {
    return SDK.targetManager.interceptMainConnection(this._dispatchProtocolMessage.bind(this)).then(rawConnection => {
      this._rawConnection = rawConnection;
    });
  }

  /**
   * @param {string} auditURL
   * @param {!Array<string>} categoryIDs
   * @return {!Promise<!ReportRenderer.ReportJSON>}
   */
  startLighthouse(auditURL, categoryIDs) {
    return this._send('start', {url: auditURL, categoryIDs});
  }

  /**
   * @return {!Promise<!Object|undefined>}
   */
  detach() {
    return Promise.resolve().then(() => this._send('stop')).then(() => this._backend.dispose()).then(() => {
      delete this._backend;
      delete this._backendPromise;
      return this._rawConnection.disconnect();
    });
  }

  /**
   *  @param {function (string): undefined} callback
   */
  registerStatusCallback(callback) {
    this._status = callback;
  }

  /**
   * @param {string} message
   */
  _dispatchProtocolMessage(message) {
    this._send('dispatchProtocolMessage', {message: message});
  }

  _initWorker() {
    this._backendPromise =
        Services.serviceManager.createAppService('audits2_worker', 'Audits2Service').then(backend => {
          if (this._backend)
            return;
          this._backend = backend;
          this._backend.on('statusUpdate', result => this._status(result.message));
          this._backend.on('sendProtocolMessage', result => this._sendProtocolMessage(result.message));
        });
  }

  /**
   * @param {string} message
   */
  _sendProtocolMessage(message) {
    this._rawConnection.sendMessage(message);
  }

  /**
   * @param {string} method
   * @param {!Object=} params
   * @return {!Promise<!ReportRenderer.ReportJSON>}
   */
  _send(method, params) {
    if (!this._backendPromise)
      this._initWorker();

    return this._backendPromise.then(_ => this._backend.send(method, params));
  }
};
