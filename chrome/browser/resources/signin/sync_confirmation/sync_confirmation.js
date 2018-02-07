/* Copyright 2015 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. */

cr.define('sync.confirmation', function() {
  'use strict';

  /**
   * Records the Sync consent.
   * @param {!Array<!HTMLElement>} path Path of the click event. Must contain
   *     a consent confirmation element.
   */
  function recordConsent(path) {
    var consentConfirmation;
    for (var i = 0; i < path.length; i++) {
      console.log(path[i]);
      if (path[i].hasAttribute('consent-confirmation')) {
        consentConfirmation = path[i].innerHTML.trim();
        break;
      }
    }
    assert(consentConfirmation);

    // Get the consent description from visible elements.
    var consentDescription =
        Array.from(document.querySelectorAll('[consent-description]'))
            .filter(element => element.clientWidth * element.clientHeight > 0)
            .map(element => element.innerHTML.trim());
    assert(consentDescription);

    chrome.send('recordConsent', [consentDescription, consentConfirmation]);
  }

  function onConfirm(e) {
    recordConsent(e.path);
    chrome.send('confirm');
  }

  function onUndo(e) {
    chrome.send('undo');
  }

  function onGoToSettings(e) {
    recordConsent(e.path);
    chrome.send('goToSettings');
  }

  function initialize() {
    document.addEventListener('keydown', onKeyDown);
    $('confirmButton').addEventListener('click', onConfirm);
    $('undoButton').addEventListener('click', onUndo);
    if (loadTimeData.getBoolean('isSyncAllowed')) {
      $('settingsLink').addEventListener('click', onGoToSettings);
      $('profile-picture').addEventListener('load', onPictureLoaded);
      $('syncDisabledDetails').hidden = true;
    } else {
      $('syncConfirmationDetails').hidden = true;
    }

    // Prefer using |document.body.offsetHeight| instead of
    // |document.body.scrollHeight| as it returns the correct height of the
    // even when the page zoom in Chrome is different than 100%.
    chrome.send('initializedWithSize', [document.body.offsetHeight]);
  }

  function clearFocus() {
    document.activeElement.blur();
  }

  function setUserImageURL(url) {
    if (loadTimeData.getBoolean('isSyncAllowed')) {
      $('profile-picture').src = url;
    }
  }

  function onPictureLoaded(e) {
    if (loadTimeData.getBoolean('isSyncAllowed')) {
      $('picture-container').classList.add('loaded');
    }
  }

  function onKeyDown(e) {
    // If the currently focused element isn't something that performs an action
    // on "enter" being pressed and the user hits "enter", perform the default
    // action of the dialog, which is "OK, Got It".
    if (e.key == 'Enter' &&
        !/^(A|PAPER-(BUTTON|CHECKBOX))$/.test(document.activeElement.tagName)) {
      $('confirmButton').click();
      e.preventDefault();
    }
  }

  // TODO(scottchen): clearFocus and setUserImageURL are called directly by the
  // C++ handler. C++ handlers should not be calling JS functions by name
  // anymore. They should be firing events with FireWebuiListener and have the
  // page itself decide whether to listen or not listen to the event.
  return {
    clearFocus: clearFocus,
    initialize: initialize,
    setUserImageURL: setUserImageURL
  };
});

document.addEventListener('DOMContentLoaded', sync.confirmation.initialize);
