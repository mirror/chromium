// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @implements {SDK.SDKModelObserver<!SDK.ResourceTreeModel>}
 * @unrestricted
 */
Resources.AppManifestView = class extends UI.VBox {
  constructor() {
    super(true);

    this.contentElement.style.setProperty('background', '#f9f9f9');
    this.contentElement.style.setProperty('overflow', 'auto');

    this._themeColorSwatch = InlineEditor.ColorSwatch.create();
    this._backgroundColorSwatch = InlineEditor.ColorSwatch.create();
    this._toolbar = new UI.Toolbar('');
    this._toolbar.renderAsLinks();
    var addToHomeScreen =
        new UI.ToolbarButton(Common.UIString('Add to homescreen'), undefined, Common.UIString('Add to homescreen'));
    addToHomeScreen.addEventListener(UI.ToolbarButton.Events.Click, this._addToHomescreen, this);
    this._toolbar.appendToolbarItem(addToHomeScreen);

    SDK.targetManager.observeModels(SDK.ResourceTreeModel, this);
  }

  /**
   * @override
   * @param {!SDK.ResourceTreeModel} resourceTreeModel
   */
  modelAdded(resourceTreeModel) {
    if (this._resourceTreeModel)
      return;
    this._resourceTreeModel = resourceTreeModel;
    this._updateManifest();
    resourceTreeModel.addEventListener(SDK.ResourceTreeModel.Events.MainFrameNavigated, this._updateManifest, this);
  }

  /**
   * @override
   * @param {!SDK.ResourceTreeModel} resourceTreeModel
   */
  modelRemoved(resourceTreeModel) {
    if (!this._resourceTreeModel || this._resourceTreeModel !== resourceTreeModel)
      return;
    resourceTreeModel.removeEventListener(SDK.ResourceTreeModel.Events.MainFrameNavigated, this._updateManifest, this);
    delete this._resourceTreeModel;
  }

  _updateManifest() {
    this._resourceTreeModel.fetchAppManifest((url, data, errors) => {
      console.time('manifest');
      for (var i = 0; i < 100; i++)
        this._renderManifest(url, data, errors);
      console.timeEnd('manifest');
    });
  }

  /**
   * @param {string} url
   * @param {?string} data
   * @param {!Array<!Protocol.Page.AppManifestError>} errors
   */
  _renderManifest(url, data, errors) {
    this.contentElement.removeChildren();

    if (!data && !errors.length) {
      var empty = UI.Fragment.cached`
        <cbox overflow=auto flex=auto>
          <vbox x-center color='hsla(0, 0%, 65%, 1)' padding=30px min-width=70px>
            <h2>No manifest detected</h2>
            <div max-width=300px line-height=18px>
              A web manifest allows you to control how your app behaves when launched and displayed to the user.
              <x-link href='https://developers.google.com/web/fundamentals/engage-and-retain/web-app-manifest/?utm_source=devtools'
                  class=devtools-link>Read more about the web manifest.</x-link>
            </div>
          </vbox>
        </cbox>
      `;
      this.contentElement.appendChild(empty.element());
      return;
    }

    var parsedManifest;
    if (data) {
      if (data.charCodeAt(0) === 0xFEFF)
        data = data.slice(1);  // Trim the BOM as per https://tools.ietf.org/html/rfc7159#section-8.1.
      parsedManifest = JSON.parse(data);
    }

    /**
     * @param {string} name
     * @return {string}
     */
    function stringProperty(name) {
      if (!parsedManifest)
        return '';
      var value = parsedManifest[name];
      if (typeof value !== 'string')
        return '';
      return value;
    }
    
    var errorRows = errors.map(error => {
      var label = UI.createLabel(error.message, error.critical ? 'smallicon-error' : 'smallicon-warning');
      return UI.Fragment.cached`<hbox style='margin: 10px 0 2px 18px;'>${label}</hbox>`;
    });

    var startURL = stringProperty('start_url');
    var startURLLink;
    if (startURL) {
      var completeURL = /** @type {string} */ (Common.ParsedURL.completeURL(url, startURL));
      startURLLink = Components.Linkifier.linkifyURL(completeURL, {text: startURL});
    }

    this._themeColorSwatch.hidden = !stringProperty('theme_color');
    var themeColor = Common.Color.parse(stringProperty('theme_color') || 'white') || Common.Color.parse('white');
    this._themeColorSwatch.setColor(/** @type {!Common.Color} */ (themeColor));

    this._backgroundColorSwatch.hidden = !stringProperty('background_color');
    var backgroundColor =
        Common.Color.parse(stringProperty('background_color') || 'white') || Common.Color.parse('white');
    this._backgroundColorSwatch.setColor(/** @type {!Common.Color} */ (backgroundColor));

    var icons = (parsedManifest['icons'] || []).map(icon => {
      var title = (icon['sizes'] || '') + '\n' + (icon['type'] || '');
      var src = Common.ParsedURL.completeURL(url, icon['src']);
      return UI.Fragment.cached`
        <x-report-field><title>${title}</title>
          <img src=${src} stlye='max-width: 200px; max-height: 200px;'></img>
        </x-report-field>
      `;
    });

    var report = UI.Fragment.cached`
      <x-report>
        <title>App Manifest</title>
        <url>${Components.Linkifier.linkifyURL(url)}</url>
        <x-report-section class=${errorRows.length ? '' : 'hidden'}><title>Errors and warnings</title>${errorRows}</x-report-section>
        <x-report-section><title>Identity</title><toolbar>${this._toolbar.element}</toolbar>
          <x-report-field><title>Name</title>${stringProperty('name')}</x-report-field>
          <x-report-field><title>Short name</title>${stringProperty('short_name')}</x-report-field>
        </x-report-section>
        <x-report-section><title>Presentation</title>
          <x-report-field><title>Start URL</title>${startURLLink}</x-report-field>
          <x-report-field><title>Theme color</title>${this._themeColorSwatch}</x-report-field>
          <x-report-field><title>Background color</title>${this._backgroundColorSwatch}</x-report-field>
          <x-report-field><title>Orientation</title>${stringProperty('orientation')}</x-report-field>
          <x-report-field><title>Display</title>${stringProperty('display')}</x-report-field>
        </x-report-section>
        <x-report-section><title>Icons</title>${icons}</x-report-section>
      </x-report>
    `;
    this.contentElement.appendChild(report.element());
  }

  /**
   * @param {!Common.Event} event
   */
  _addToHomescreen(event) {
    var target = SDK.targetManager.mainTarget();
    if (target && target.hasBrowserCapability()) {
      target.pageAgent().requestAppBanner();
      Common.console.show();
    }
  }
};

UI.Fragment.register('x-report', ['title', 'subtitle', 'url', 'toolbar'], (element, params, children) => {
  return UI.Fragment.cached`
    <vbox style='background: white'>
      <vbox style='flex: none; border-bottom: 1px solid rgb(230, 230, 230); padding: 12px 24px;'>
        <hbox style='font-size: 15px'>${params.title}</hbox>
        <hbox style='font-size: 12px; margin-top: 10px' class=${params.subtitle ? '' : 'hidden'}>${params.subtitle}</hbox>
        <hbox style='font-size: 12px; margin-top: 10px' class=${params.url ? '' : 'hidden'}>${params.url}</hbox>
        <hbox style='margin: 5px 0 -8px -8px;' class=${params.toolbar ? '' : 'hidden'}>${params.toolbar}</hbox>
      </vbox>
      <vbox>${children}</vbox>
    </vbox>
  `;
});

UI.Fragment.register('x-report-section', ['title', 'toolbar'], (element, params, children) => {
  return UI.Fragment.cached`
    <vbox style='flex: none; padding: 12px; border-bottom: 1px solid rgb(230, 230, 230);'>
      <hbox style='align-items: center; margin-left: 18px'>
        <hbox style='flex: auto; font-weight: bold; color: #555; white-space: nowrap; text-overflow: ellipsis; overflow: hidden;'>${params.title}</hbox>
        ${params.toolbar}
      </hbox>
      <vbox>${children}</vbox>
    </vbox>
  `;
});

UI.Fragment.register('x-report-field', ['title'], (element, params, children) => {
  return UI.Fragment.cached`
    <hbox style='margin-top: 8px; line-height: 28px;'>
      <hbox style='flex: 0 0 128px; white-space: pre; justify-content: flex-end; color: #888; padding: 0 6px;'>${params.title}</hbox>
      <div style='flex: auto; white-space: pre; padding: 0 6px;'>${children}</div>
    </hbox>
  `;
});
