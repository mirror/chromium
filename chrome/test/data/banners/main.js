// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PromptAction = {
  CALL_PROMPT_DELAYED: 0,
  CALL_PROMPT_IN_HANDLER: 1,
  CANCEL_PROMPT: 2,
  STASH_EVENT: 3,
};

var stashedEvent = 1;

var gotEventsFrom = ['________', '____'];

function initialize() {
  navigator.serviceWorker.register('service_worker.js');
}

function verifyAppInstalledEvents() {
  function setTitle() {
    window.document.title = 'Got appinstalled: ' +
      gotEventsFrom.join(', ');
  }

  window.addEventListener('appinstalled', () => {
    gotEventsFrom[0] = 'listener';
    setTitle();
  });
  window.onappinstalled = () => {
    gotEventsFrom[1] = 'attr';
    setTitle();
  };
}

function verifyBeforeInstallPromptEvents() {
  function setTitle() {
    window.document.title = 'Got beforeinstallprompt: ' +
      gotEventsFrom.join(', ');
  }

  // Test both the addEventListener and onX attribute versions.
  // When a prompt is shown, each should fire once.
  window.addEventListener('beforeinstallprompt', () => {
    gotEventsFrom[0] = 'listener';
    setTitle();
  });
  window.onbeforeinstallprompt = () => {
    gotEventsFrom[1] = 'attr';
    setTitle();
  };
}

function callPrompt(event) {
  event.prompt();
  event.userChoice.then(function(choiceResult) {
    window.document.title = 'Got userChoice: ' + choiceResult.outcome;
  });
}

function callStashedPrompt() {
  stashedEvent.prompt();
}

function addPromptListener(promptAction) {
  window.addEventListener('beforeinstallprompt', function(e) {
    e.preventDefault();

    switch (promptAction) {
      case PromptAction.CALL_PROMPT_DELAYED:
        setTimeout(callPrompt, 0, e);
        break;
      case PromptAction.CALL_PROMPT_IN_HANDLER:
        callPrompt(e);
        break;
      case PromptAction.CANCEL_PROMPT:
        // Navigate the window to trigger the banner cancellation.
        window.location.href = "/";
        break;
      case PromptAction.STASH_EVENT:
        stashedEvent = e;
        break;
    }
  });
}

function addManifestLinkTag() {
  var url = window.location.href;
  var manifestIndex = url.indexOf("?manifest=");
  var manifestUrl =
      (manifestIndex >= 0) ? url.slice(manifestIndex + 10) : 'manifest.json';
  var linkTag = document.createElement("link");
  linkTag.id = "manifest";
  linkTag.rel = "manifest";
  linkTag.href = manifestUrl;
  document.head.append(linkTag);
}

function changeManifestUrl(newManifestUrl) {
  var linkTag = document.getElementById("manifest");
  linkTag.href = newManifestUrl;
}
