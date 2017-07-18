// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for site-details. */
suite('SiteDetails', function() {
  /**
   * A site list element created before each test.
   * @type {SiteDetails}
   */
  var testElement;

  /**
   * An example pref with 1 pref in each category.
   * @type {SiteSettingsPref}
   */
  var prefs;

  // Initialize a site-details before each test.
  setup(function() {
    prefs = {
      defaults: {
        auto_downloads: {
          setting: settings.ContentSetting.ASK,
        },
        background_sync: {
          setting: settings.ContentSetting.ALLOW,
        },
        camera: {
          setting: settings.ContentSetting.ASK,
        },
        geolocation: {
          setting: settings.ContentSetting.ASK,
        },
        images: {
          setting: settings.ContentSetting.ALLOW,
        },
        javascript: {
          setting: settings.ContentSetting.ALLOW,
        },
        mic: {
          setting: settings.ContentSetting.ASK,
        },
        notifications: {
          setting: settings.ContentSetting.ASK,
        },
        plugins: {
          setting: settings.ContentSetting.ASK,
        },
        popups: {
          setting: settings.ContentSetting.BLOCK,
        },
        unsandboxed_plugins: {
          setting: settings.ContentSetting.ASK,
        },
      },
      exceptions: {
        auto_downloads: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
        background_sync: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
        camera: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'extension',
          },
        ],
        geolocation: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.BLOCK,
            source: 'policy',
          },
        ],
        images: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.DEFAULT,
            source: 'preference',
          },
        ],
        javascript: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
        mic: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
        notifications: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
        plugins: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
        popups: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.DEFAULT,
            source: 'preference',
          },
        ],
        unsandboxed_plugins: [
          {
            embeddingOrigin: 'https://foo-allow.com:443',
            origin: 'https://foo-allow.com:443',
            setting: settings.ContentSetting.ALLOW,
            source: 'preference',
          },
        ],
      }
    };

    browserProxy = new TestSiteSettingsPrefsBrowserProxy();
    settings.SiteSettingsPrefsBrowserProxyImpl.instance_ = browserProxy;
    PolymerTest.clearBody();
    testElement = document.createElement('site-details');
    document.body.appendChild(testElement);
  });

  test('usage heading shows on storage available', function() {
    // Remove the current website-usage-private-api element.
    var parent = testElement.$.usageApi.parentNode;
    testElement.$.usageApi.remove();

    // Replace it with a mock version.
    Polymer({
      is: 'mock-website-usage-private-api',

      fetchUsageTotal: function(origin) {
        testElement.storedData_ = '1 KB';
      },
    });
    var api = document.createElement('mock-website-usage-private-api');
    testElement.$.usageApi = api;
    Polymer.dom(parent).appendChild(api);

    browserProxy.setPrefs(prefs);
    testElement.origin = 'https://foo-allow.com:443';

    Polymer.dom.flush();

    // Expect usage to be rendered.
    assertTrue(!!testElement.$$('#usage'));
  });

  test('correct pref settings are shown', function() {
    browserProxy.setPrefs(prefs);
    testElement.origin = 'https://foo-allow.com:443';

    return browserProxy.whenCalled('getOriginPermissions').then(function() {
      testElement.root.querySelectorAll('site-details-permission')
          .forEach(function(siteDetailsPermission) {
            // Verify settings match the values specified in |prefs|.
            var expectedSetting = settings.ContentSetting.ALLOW;
            if (siteDetailsPermission.category ==
                settings.ContentSettingsTypes.GEOLOCATION) {
              expectedSetting = settings.ContentSetting.BLOCK;
            } else if (
                siteDetailsPermission.category ==
                    settings.ContentSettingsTypes.IMAGES ||
                siteDetailsPermission.category ==
                    settings.ContentSettingsTypes.POPUPS) {
              expectedSetting = settings.ContentSetting.DEFAULT;
            }
            assertEquals(expectedSetting, siteDetailsPermission.site.setting);
            assertEquals(
                expectedSetting, siteDetailsPermission.$.permission.value);
          });
    });
  });
});
