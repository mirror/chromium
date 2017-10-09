// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('SiteDataTest', function() {
  /** @type {SiteDataElement} */
  var siteData;

  /** @type {TestLocalDataBrowserProxy} */
  var testBrowserProxy;

  setup(function() {
    testBrowserProxy = new TestLocalDataBrowserProxy();
    settings.LocalDataBrowserProxyImpl.instance_ = testBrowserProxy;
    siteData = document.createElement('site-data');
  });

  teardown(function() {
    siteData.remove();
  });

  test('remove button (trash) calls remove on origin', function(done) {
    var sites = [
      {site: 'Hello', id: '1', localData: 'Cookiez!'},
    ];
    testBrowserProxy.setCookieList(sites);
    /** @type {SiteDataElement} */
    listenOnce(siteData, 'site-data-list-complete', () => {
      Polymer.dom.flush();
      var button = siteData.$$('#siteItem').querySelector('.icon-delete-gray');
      assertTrue(!!button);
      assertEquals(button.is, 'paper-icon-button-light');
      MockInteractions.tap(button);
      testBrowserProxy.whenCalled('removeItem').then(function(path) {
        assertEquals('Hello', path);
        done();
      });
    });
    document.body.appendChild(siteData);
  });

  test('remove button hidden when no search results', function(done) {
    listenOnce(siteData, 'site-data-list-complete', () => {
      assertEquals(2, siteData.$.list.items.length);
      listenOnce(siteData, 'site-data-list-complete', () => {
        assertEquals(1, siteData.$.list.items.length);
        done();
      });
      siteData.filter = 'Hello';
    });
    var sites = [
      {site: 'Hello', id: '1', localData: 'Cookiez!'},
      {site: 'World', id: '2', localData: 'Cookiez!'},
    ];
    testBrowserProxy.setCookieList(sites);
    /** @type {SiteDataElement} */
    document.body.appendChild(siteData);
  });

  test('calls reloadCookies() when created', function() {
    document.body.appendChild(siteData);
    return testBrowserProxy.whenCalled('reloadCookies');
  });

  test('calls reloadCookies() when visited again', function() {
    document.body.appendChild(siteData);
    settings.navigateTo(settings.routes.SITE_SETTINGS_COOKIES);
    testBrowserProxy.reset();
    settings.navigateTo(settings.routes.SITE_SETTINGS_SITE_DATA);
    return testBrowserProxy.whenCalled('reloadCookies');
  });
});
