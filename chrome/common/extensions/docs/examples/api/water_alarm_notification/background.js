// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

document.getElementById('sampleSecond').addEventListener('click', setAlarm);
document.getElementById('15min').addEventListener('click', setAlarm);
document.getElementById('30min').addEventListener('click', setAlarm);

function setAlarm(event){
  var minutes = event.target.value
  chrome.browserAction.setBadgeText({text: "ON"});
  if(minutes === "seconds"){
      chrome.alarms.create({delayInMinutes: 0.1});
  }
  else if(minutes === "15"){
      chrome.alarms.create({delayInMinutes: 15.0});
  }
  else if(minutes === "30"){
      chrome.alarms.create({delayInMinutes: 30.0});
  }
  window.close();
}


chrome.alarms.onAlarm.addListener(function() {

  chrome.browserAction.setBadgeText({text: ""});
  chrome.notifications.create({type: "basic",
      iconUrl: "drink_water.png",
      title: "Time to Hydrate",
      message: "Everyday I'm Guzzlin'!",
      buttons:[
        {title: "Sample Seconds"},
        {title: "15 Minutes"}
      ],
      priority: 0},
      function(id) {
          myNotificationID = id;
      })
});

chrome.notifications.onButtonClicked.addListener(function(notifId, btnIdx) {
    if (notifId === myNotificationID) {
        if (btnIdx === 0) {
            chrome.browserAction.setBadgeText({text: "ON"});
            chrome.alarms.create({delayInMinutes: 0.1});
        } else if (btnIdx === 1) {
          chrome.browserAction.setBadgeText({text: "ON"});
          chrome.alarms.create({delayInMinutes: 15.0});
        }
    }
});
