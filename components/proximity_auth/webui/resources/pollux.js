/* Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

Pollux = {
  controller_: null,

  /**
   * The possible states that an assertion session can be in.
   */
  AssertionState: {
    NOT_STARTED: 0,
    ADVERTISING: 1,
    CONNECTED: 2,
    COMPLETED_WITH_ASSERTION: 3,
    TIMED_OUT: 4,
    UNEXPECTED_DISCONNECT: 5,
    ERROR: 6,

    toString: function(state) {
      switch (state) {
        case Pollux.AssertionState.NOT_STARTED:
          return 'NOT STARTED';
        case Pollux.AssertionState.ADVERTISING:
          return 'ADVERTISING';
        case Pollux.AssertionState.CONNECTED:
          return 'CONNECTED';
        case Pollux.AssertionState.COMPLETED_WITH_ASSERTION:
          return 'ASSERTION:';
        case Pollux.AssertionState.TIMED_OUT:
          return 'TIMED OUT';
        case Pollux.AssertionState.UNEXPECTED_DISCONNECT:
          return 'DISCONNECTED';
        case Pollux.AssertionState.ERROR:
          return 'ERROR';
        default:
          return 'INVALID';
      }
    },
  },

  /**
   * Initializes the debug UI.
   */
  init: function() {
    Pollux.controller_ = new PolluxController();
  }
};

/**
 * Interface with the native WebUI component for Pollux events.
 */
PolluxInterface = {
  /**
   * Called when a new master key is created.
   */
  onMasterKeyCreated: function(masterKey) {
    throw 'Not Implemented';
  },

  onCloudPairingKeysDerived: function(irk, lk) {
    throw 'Not Implemented';
  },

  /**
   * Called when a new challenge is created.
   */
  onChallengeCreated: function(nonce, challenge, eid, sessionKey) {
    throw 'Not Implemented';
  },

  /**
   * Called when the assertion session state changes.
   */
  onAssertionStateChanged: function(newState) {
    throw 'Not Implemented';
  },

  /**
   * Called when an assertion is received from the remote device.
   */
  onGotAssertion: function(assertion) {
    throw 'Not Implemented';
  },
};

class PolluxController {
  constructor() {
    // Cloud pairing elements:
    this.masterKeyInput_ = document.getElementById('master-key-input');
    this.newMasterKeyButton_ = document.getElementById('new-master-key');
    this.lkInput_ = document.getElementById('lk-input');
    this.irkInput_ = document.getElementById('irk-input');
    this.deriveCloudKeysButton_ = document.getElementById('derive-cloud-keys');

    // Assertion elements:
    this.newChallengeButton_ = document.getElementById('new-challenge');
    this.nonceInput_ = document.getElementById('nonce-input');
    this.challengeInput_ = document.getElementById('challenge-input');
    this.eidInput_ = document.getElementById('eid-input');
    this.sessionKeyInput_ = document.getElementById('sk-input');
    this.getAssertionButton_ = document.getElementById('get-assertion');
    this.authStateElement_ = document.getElementById('authenticator-state');
    this.assertionStateElement_ = document.getElementById('assertion-state');
    this.assertionValueElement_ = document.getElementById('assertion-value');

    this.newMasterKeyButton_.onclick = this.createMasterKey_.bind(this);
    this.deriveCloudKeysButton_.onclick = this.deriveCloudKeys_.bind(this);
    this.newChallengeButton_.onclick = this.createNewChallenge_.bind(this);
    this.getAssertionButton_.onclick = this.getAssertion_.bind(this);

    PolluxInterface = this;
  }

  onMasterKeyCreated(masterKey) {
    console.log('master key:');
    console.log(masterKey);
    this.masterKeyInput_.value = masterKey;
    this.irkInput_.value = '';
    this.lkInput_.value = '';
  }

  onCloudPairingKeysDerived(irk, lk) {
    console.log('cloud keys:');
    console.log(irk);
    console.log(lk);
    this.irkInput_.value = irk;
    this.lkInput_.value = lk;
  }

  onChallengeCreated(nonce, challenge, eid, sessionKey) {
    console.log("challenge created:");
    console.log(nonce);
    console.log(challenge);
    console.log(eid);
    console.log(sessionKey);
    this.nonceInput_.value = nonce;
    this.challengeInput_.value = challenge;
    this.eidInput_.value = eid;
    this.sessionKeyInput_.value = sessionKey;
  }

  onAssertionStateChanged(newState) {
    console.log('assertion state changed: ' + newState);

    this.assertionStateElement_.textContent =
        Pollux.AssertionState.toString(newState);

    if (newState != Pollux.AssertionState.COMPLETED_WITH_ASSERTION) {
      this.assertionValueElement_.textContent ='' 
    }

    if (newState == Pollux.AssertionState.ADVERTISING ||
        newState == Pollux.AssertionState.CONNECTED) {
      this.getAssertionButton_.textContent = 'Stop Assertion';
      this.getAssertionButton_.onclick = this.stopAssertion_.bind(this);
    } else {
      this.getAssertionButton_.textContent = 'Get Assertion';
      this.getAssertionButton_.onclick = this.getAssertion_.bind(this);
    }
  }

  onGotAssertion(assertion) {
    console.log('got assertion: ' + assertion);
    this.assertionValueElement_.textContent = assertion;
  }

  createMasterKey_() {
    chrome.send('createMasterKey');
  }

  deriveCloudKeys_() {
    chrome.send('deriveCloudPairingKeys', [this.masterKeyInput_.value]);
  }

  createNewChallenge_() {
    chrome.send('createNewChallenge',
                [this.irkInput_.value, this.lkInput_.value]);
  }

  getAssertion_() {
    chrome.send('getAssertion',
        [this.challengeInput_.value, this.eidInput_.value,
        this.sessionKeyInput_.value]);
  }

  stopAssertion_() {
    chrome.send('stopAssertion');
  }
}

document.addEventListener('DOMContentLoaded', function() {
  WebUI.onWebContentsInitialized();
  Logs.init();
  Pollux.init();
});
