// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('test_util', function() {
  /**
   * Observes an HTML attribute and fires a promise when it matches a given
   * value.
   * @param {!HTMLElement} target
   * @param {string} attributeName
   * @param {*} attributeValue
   * @return {!Promise}
   */
  function whenAttributeIs(target, attributeName, attributeValue) {
    function isDone() {
      return target.getAttribute(attributeName) == attributeValue;
    }

    return isDone() ? Promise.resolve() : new Promise(function(resolve) {
      new MutationObserver(function(mutations, observer) {
        for (const mutation of mutations) {
          assertEquals('attributes', mutation.type);
          if (mutation.attributeName == attributeName && isDone()) {
            observer.disconnect();
            resolve();
            return;
          }
        }
      }).observe(target, {
        attributes: true,
        childList: false,
        characterData: false
      });
    });
  }

  /**
   * Converts an event occurrence to a promise.
   * @param {string} eventType
   * @param {!HTMLElement} target
   * @return {!Promise} A promise firing once the event occurs.
   */
  function eventToPromise(eventType, target) {
    return new Promise(function(resolve, reject) {
      target.addEventListener(eventType, function f(e) {
        target.removeEventListener(eventType, f);
        resolve(e);
      });
    });
  }

  /**
   * Data-binds two Polymer properties using the property-changed events and
   * set/notifyPath API. Useful for testing components which would normally be
   * used together.
   * @param {!HTMLElement} el1
   * @param {!HTMLElement} el2
   * @param {string} property
   */
  function fakeDataBind(el1, el2, property) {
    const forwardChange = function(el, event) {
      if (event.detail.hasOwnProperty('path'))
        el.notifyPath(event.detail.path, event.detail.value);
      else
        el.set(property, event.detail.value);
    };
    // Add the listeners symmetrically. Polymer will prevent recursion.
    el1.addEventListener(property + '-changed', forwardChange.bind(null, el2));
    el2.addEventListener(property + '-changed', forwardChange.bind(null, el1));
  }

  /**
   * Helper to create an object containing a ContentSettingsType key to array or
   * object value. This is a convenience function that can eventually be
   * replaced with ES6 computed properties.
   * @param {settings.ContentSettingsTypes} contentType The ContentSettingsType
   *     to use as the key.
   * @param {Array<RawSiteException>|DefaultContentSetting} value The value to
   *     map to |contentType|.
   * @return {Object<settings.ContentSettingsTypes,
   *     Array<RawSiteException|DefaultContentSetting>}
   */
  function createObjectWithContentSettingType(contentType, value) {
    const result = {};
    result[contentType] = value;
    return result;
  }

  /**
   * Helper to create a mock {DefaultContentSetting}.
   * @param {Object} override An object with a subset of the properties of
   *     |DefaultContentSetting|. Properties defined in |override| will
   * overwrite the defaults in this function's return value.
   * @return {DefaultContentSetting}
   */
  function createDefaultContentSetting(override) {
    return Object.assign(
        {
          setting: settings.ContentSetting.ASK,
          source: settings.SiteSettingSource.PREFERENCE,
        },
        override);
  }

  /**
   * Helper to create a mock {RawSiteException}.
   * @param {Object} override An object with a subset of the properties of
   *     |RawSiteException|. Properties defined in |override| will overwrite the
   *     defaults in this function's return value.
   * @return {RawSiteException}
   */
  function createRawSiteException(override) {
    return Object.assign(
        {
          embeddingOrigin: '',
          incognito: false,
          origin: 'https://foo.com',
          displayName: '',
          setting: settings.ContentSetting.ALLOW,
          source: settings.SiteSettingSource.PREFERENCE,
        },
        override);
  }

  /**
   * Helper to create a mock |SiteSettingsPref|.
   * @param {Object} override A object containing a subset of the properties in
   *     |SiteSettingsPref| which will overwrite the default |SiteSettingsPref|
   *     returned by this function.
   * @return {SiteSettingsPref}
   */
  function createSiteSettingsPrefs(defaultsOverride, exceptionsOverride) {
    const siteSettingsPref = {
      defaults: {},
      exceptions: {},
    };
    // These test defaults reflect the actual defaults assigned to each
    // ContentSettingType, but keeping these in sync shouldn't matter for tests.
    for (let type in settings.ContentSettingsTypes) {
      siteSettingsPref.defaults[settings.ContentSettingsTypes[type]] =
          createDefaultContentSetting({});
    }
    siteSettingsPref.defaults[settings.ContentSettingsTypes.COOKIES].setting =
        settings.ContentSetting.ALLOW;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.IMAGES].setting =
        settings.ContentSetting.ALLOW;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.JAVASCRIPT]
        .setting = settings.ContentSetting.ALLOW;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.SOUND].setting =
        settings.ContentSetting.ALLOW;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.POPUPS].setting =
        settings.ContentSetting.BLOCK;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.PROTOCOL_HANDLERS]
        .setting = settings.ContentSetting.ALLOW;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.BACKGROUND_SYNC]
        .setting = settings.ContentSetting.ALLOW;
    siteSettingsPref.defaults[settings.ContentSettingsTypes.ADS].setting =
        settings.ContentSetting.BLOCK;
    for (let type in settings.ContentSettingsTypes) {
      siteSettingsPref.exceptions[settings.ContentSettingsTypes[type]] = [];
    }
    Object.assign(siteSettingsPref.defaults, defaultsOverride);
    Object.assign(siteSettingsPref.exceptions, exceptionsOverride);
    return siteSettingsPref;
  }

  return {
    eventToPromise: eventToPromise,
    fakeDataBind: fakeDataBind,
    whenAttributeIs: whenAttributeIs,
    createObjectWithContentSettingType: createObjectWithContentSettingType,
    createDefaultContentSetting: createDefaultContentSetting,
    createRawSiteException: createRawSiteException,
    createSiteSettingsPrefs: createSiteSettingsPrefs,
  };

});
