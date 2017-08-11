// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_search_page_hotword', function() {
  function generateSearchEngineInfo() {
    var searchEngines0 = settings_search.createSampleSearchEngine(
        true, false, false);
    searchEngines0.default = true;
    var searchEngines1 = settings_search.createSampleSearchEngine(
        true, false, false);
    searchEngines1.default = false;
    var searchEngines2 = settings_search.createSampleSearchEngine(
        true, false, false);
    searchEngines2.default = false;

    return {
      defaults: [searchEngines0, searchEngines1, searchEngines2],
      others: [],
      extensions: [],
    };
  }

  suite('SearchPageHotwordTests', function() {
    /** @type {?SettingsSearchPageElement} */
    var page = null;

    var browserProxy = null;

    setup(function() {
      browserProxy = new settings_search.TestSearchEnginesBrowserProxy();
      browserProxy.setSearchEnginesInfo(generateSearchEngineInfo());
      settings.SearchEnginesBrowserProxyImpl.instance_ = browserProxy;
      PolymerTest.clearBody();
      page = document.createElement('settings-search-page');
      page.prefs = {
        default_search_provider_data: {
          template_url_data: {},
        },
      };
      document.body.appendChild(page);
    });

    teardown(function() { page.remove(); });

    // Tests the UI when Hotword 'alwaysOn' is true.
    test('HotwordAlwaysOn', function() {
      return browserProxy.whenCalled('getSearchEnginesList').then(function() {
        return browserProxy.whenCalled('getHotwordInfo');
      }).then(function() {
        Polymer.dom.flush();
        assertTrue(page.hotwordInfo_.allowed);
        assertTrue(page.hotwordInfo_.alwaysOn);
        assertFalse(page.hotwordInfo_.enabled);
        assertFalse(browserProxy.hotwordSearchEnabled);
        assertFalse(page.hotwordSearchEnablePref_.value);

        var control = page.$$('#hotwordSearchEnable');
        assertTrue(!!control);
        assertFalse(control.disabled);
        assertFalse(control.checked);
        MockInteractions.tap(control.$.control);
        Polymer.dom.flush();
        return browserProxy.whenCalled('setHotwordSearchEnabled');
      }).then(function() {
        assertTrue(browserProxy.hotwordSearchEnabled);
      });
    });

    // Tests the UI when Hotword 'alwaysOn' is false.
    test('HotwordNotAlwaysOn', function() {
      return browserProxy.whenCalled('getSearchEnginesList').then(function() {
        return browserProxy.whenCalled('getHotwordInfo');
      }).then(function() {
        browserProxy.setHotwordInfo({
          allowed: true,
          enabled: false,
          alwaysOn: false,
          errorMessage: '',
          userName: '',
          historyEnabled: false,
        });
        Polymer.dom.flush();
        assertTrue(page.hotwordInfo_.allowed);
        assertFalse(page.hotwordInfo_.alwaysOn);
        assertFalse(page.hotwordInfo_.enabled);

        var control = page.$$('#hotwordSearchEnable');
        assertTrue(!!control);
        assertFalse(control.disabled);
        assertFalse(control.checked);
        MockInteractions.tap(control.$.control);
        Polymer.dom.flush();
        return browserProxy.whenCalled('setHotwordSearchEnabled');
      }).then(function() {
        assertTrue(browserProxy.hotwordSearchEnabled);
      });
    });
  });
});
