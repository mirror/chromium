// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

var logWriterSpy;

function verifyLog(assert, index, expectedEvent, expectedPhase) {
  var actual = logWriterSpy.getCall(index).args[0];
  assert.equal(
      actual.type, remoting.ChromotingEvent.Type.CHROMOTING_DOT_COM_MIGRATION,
      'ChromotingEvent.type == CHROMOTING_DOT_COM_MIGRATION');
  assert.equal(
      actual.chromoting_dot_com_migration.event, expectedEvent,
      'ChromotingEvent.chromoting_dot_com_migration.event matched.');
  assert.equal(
      actual.chromoting_dot_com_migration.phase, expectedPhase,
      'ChromotingEvent.chromoting_dot_com_migration.phase matched');
}

QUnit.module('ButterBar', {
  beforeEach: function() {
    var fixture = document.getElementById('qunit-fixture');
    fixture.innerHTML =
        '<div id="butter-bar" hidden>' +
        '  <p>' +
        '    <span id="butter-bar-message"></span>' +
        '    <a id="butter-bar-dismiss" href="#" tabindex="0">' +
        '      <img src="icon_cross.webp" class="close-icon">' +
        '    </a>' +
        '  </p>' +
        '</div>';
    logWriterSpy = sinon.spy();
    this.butterBar = new remoting.ButterBar(logWriterSpy);
    chrome.storage = {
      sync: {
        get: sinon.stub(),
        set: sinon.stub(),
      }
    };

    chrome.i18n.getMessage = function(tag, substituions) {
      switch (tag) {
        case 'WEBSITE_INVITE_BETA':
        case 'WEBSITE_INVITE_STABLE':
        case 'WEBSITE_INVITE_DEPRECATION_1':
        case 'WEBSITE_INVITE_DEPRECATION_2':
          return 'Check out ' + substituions[0] + 'link' + substituions[1];
        default:
          return tag;
      }
    };
  },
  afterEach: function() {
    if (this.clock) {
      this.clock.restore();
    }
  }
});

QUnit.test('should stay hidden if index==-1', function(assert) {
  this.butterBar.currentMessage_ = -1;
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
    sinon.assert.notCalled(logWriterSpy);
  });
});

QUnit.test('should be shown, yellow and dismissable if index==0',
           function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.butterBar.currentMessage_ = 0;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    assert.ok(this.butterBar.dismiss_.hidden == false);
    assert.ok(!this.butterBar.root_.classList.contains('red'));
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.BETA)
  });
});

QUnit.test('should update storage when shown', function(assert) {
  this.butterBar.currentMessage_ = 0;
  this.clock = sinon.useFakeTimers(123);
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.deepEqual(chrome.storage.sync.set.firstCall.args,
                     [{
                       "message-state": {
                         "hidden": false,
                         "index": 0,
                         "timestamp": 123,
                       }
                     }]);
  });
});

QUnit.test(
    'should be shown and should not update local storage if it has already ' +
    'shown, the timeout has not elapsed and it has not been dismissed',
    function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.butterBar.currentMessage_ = 0;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": false,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.clock = sinon.useFakeTimers(remoting.ButterBar.kTimeout_);
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    assert.ok(!chrome.storage.sync.set.called);
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.BETA)
  });
});

QUnit.test('should stay hidden if the timeout has elapsed', function(assert) {
  this.butterBar.currentMessage_ = 0;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": false,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.clock = sinon.useFakeTimers(remoting.ButterBar.kTimeout_+ 1);
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
    sinon.assert.notCalled(logWriterSpy);
  });
});


QUnit.test('should stay hidden if it was previously dismissed',
           function(assert) {
  this.butterBar.currentMessage_ = 0;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": true,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.clock = sinon.useFakeTimers(0);
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == true);
    sinon.assert.notCalled(logWriterSpy);
  });
});


QUnit.test('should be shown if the index has increased', function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;
  this.butterBar.currentMessage_ = 1;
  chrome.storage.sync.get.callsArgWith(1, {
    "message-state": {
      "hidden": true,
      "index": 0,
      "timestamp": 0,
    }
  });
  this.clock = sinon.useFakeTimers(remoting.ButterBar.kTimeout_ + 1);
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.STABLE)
  });
});

QUnit.test('should be red and not dismissable for the final message',
           function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.butterBar.currentMessage_ = 3;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    assert.ok(this.butterBar.root_.hidden == false);
    assert.ok(this.butterBar.dismiss_.hidden == true);
    assert.ok(this.butterBar.root_.classList.contains('red'));
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.DEPRECATION_2)
  });
});

QUnit.test('dismiss button updates local storage', function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.butterBar.currentMessage_ = 0;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    this.clock = sinon.useFakeTimers(0);
    this.butterBar.dismiss_.click();
    // The first call is in response to showing the message; the second is in
    // response to dismissing the message.
    assert.deepEqual(chrome.storage.sync.set.secondCall.args,
                     [{
                       "message-state": {
                         "hidden": true,
                         "index": 0,
                         "timestamp": 0,
                       }
                     }]);
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.BETA)
    verifyLog(
        assert, 1, MigrationEvent.DEPRECATION_NOTICE_DISMISSAL,
        MigrationPhase.BETA)
  });
});

QUnit.test('clicks on URL are reported', function(assert) {
  var MigrationPhase = remoting.ChromotingEvent.ChromotingDotComMigration.Phase;
  var MigrationEvent = remoting.ChromotingEvent.ChromotingDotComMigration.Event;

  this.butterBar.currentMessage_ = 2;
  chrome.storage.sync.get.callsArgWith(1, {});
  return this.butterBar.init().then(() => {
    this.clock = sinon.useFakeTimers(0);
    // Invoke the link.
    this.butterBar.root_.getElementsByTagName('a')[0].click();
    verifyLog(
        assert, 0, MigrationEvent.DEPRECATION_NOTICE_IMPRESSION,
        MigrationPhase.DEPRECATION_1)
    verifyLog(
        assert, 1, MigrationEvent.DEPRECATION_NOTICE_CLICKED,
        MigrationPhase.DEPRECATION_1)
  });
});

}());
