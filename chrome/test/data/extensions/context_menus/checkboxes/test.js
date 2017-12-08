// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

window.onload = function() {
  chrome.contextMenus.create({
    id: 'checkbox1',
    type: 'checkbox',
    title: 'Checkbox 1',
    onclick: function() {
      chrome.test.sendMessage('onclick checkbox 1');
    }
  }, function() {
    chrome.test.sendMessage('created checkbox 1', function() {
      createSecondCheckbox();
    });
  });
};

function createSecondCheckbox() {
  chrome.contextMenus.create({
    id: 'checkbox2',
    type: 'checkbox',
    title: 'Checkbox 2',
    onclick: function() {
      chrome.test.sendMessage('onclick checkbox 2');
    }
  }, function() {
    chrome.test.sendMessage('created checkbox 2', function() {
      chrome.contextMenus.update('checkbox2', {checked: true}, function() {
        chrome.test.sendMessage('checkbox2 checked');
      });
      createNormalMenuItem();
    });
  });
}

function createNormalMenuItem() {
  chrome.contextMenus.create({
    id: 'item1',
    title: 'Item 1',
    onclick: function() {
      chrome.test.sendMessage('onclick normal item');
      chrome.contextMenus.update('checkbox2', {checked: false}, function() {
        chrome.test.sendMessage('checkbox2 unchecked');
      });
    }
  }, function() {
    chrome.test.sendMessage('created normal item');
  });
}
