// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
"use strict";

function setAlarm(event) {
  let minutes = parseFloat(event.target.value);
  console.log("hello")
  chrome.browserAction.setBadgeText({text: 'ON'});
  chrome.alarms.create({delayInMinutes: minutes})
  chrome.storage.sync.set({ "minutes": minutes });
  window.close();
}
//An Alarm delay of less than the minimum 1 minute will fire
// in approximately 1 minute incriments if released
document.getElementById("sampleSecond").addEventListener("click", setAlarm);
document.getElementById("15min").addEventListener("click", setAlarm);
document.getElementById("30min").addEventListener("click", setAlarm);

chrome.alarms.onAlarm.addListener(function() {
  chrome.browserAction.setBadgeText({text: ''});
  chrome.notifications.create({
      type:     "basic",
      iconUrl:  "drink_water.png",
      title:    "Time to Hydrate",
      message:  "Everyday I'm Guzzlin'!",
      buttons: [
        {title: "Keep it Flowing."}
      ],
      priority: 0})
});

chrome.notifications.onButtonClicked.addListener(function(buttonIndex) {
  chrome.storage.sync.get(["minutes"], function(item){
     chrome.browserAction.setBadgeText({text: 'ON'});
     chrome.alarms.create({delayInMinutes: item.minutes});
  });
});
