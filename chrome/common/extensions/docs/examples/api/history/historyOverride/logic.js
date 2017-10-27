// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const kMillisecondsPerWeek = 1000 * 60 * 60 * 24 * 7;
const kOneWeekAgo = (new Date).getTime() - kMillisecondsPerWeek;
let historyDiv = document.getElementById('historyDiv');
const kColors = ['#4688F1', '#E8453C', '#F9BB2D', '#3AA757'];

function constructHistory(historyItems) {
  console.log(historyItems)
  let template = document.getElementById('history-item');
  for (let item of historyItems) {
    let randomColor = kColors[Math.floor(Math.random() * kColors.length)];
    template.style.backgroundColor = randomColor
    let titleLink = template.querySelector('.titleLink, a');
    let pageName = template.querySelector('.pageName, p');
    let removeButton = template.querySelector('.removeButton, button');
    removeButton.setAttribute('id', item.url);
    let checkbox = template.querySelector('.removeCheck, input');
    checkbox.setAttribute('value', item.url);
    let favicon = document.createElement('img');
    let host = new URL(item.url).host;
    titleLink.href = item.url;
    favicon.src = 'chrome://favicon/' + item.url;
    titleLink.textContent = host;
    titleLink.appendChild(favicon)
    pageName.innerText = item.title;
    if( item.title === '') {
          pageName.innerText = host;
    }
    historyDiv.appendChild(template.cloneNode(true));
  }
}

chrome.history.search({
      'text': '',
      'startTime': kOneWeekAgo,
      'maxResults': 99
    }, constructHistory);

document.getElementById('searchSubmit').onclick = function() {
  historyDiv.innerHTML = " "
  let searchQuery = document.getElementById('searchInput').value
  chrome.history.search({
        'text': searchQuery,
        'startTime': kOneWeekAgo
      }, constructHistory)
}

document.getElementById('historyDiv').onclick = function(element) {
  let elementClicked = element.path[0].tagName
  if (elementClicked == 'BUTTON'){
    let deleteItem = element.path[0].id.toString();
    chrome.history.deleteUrl({url: deleteItem}, function() {
      location.reload()
    })
  }
}

document.getElementById('deleteSelected').onclick = function() {
  let checkboxes = document.getElementsByTagName('input');
  for (var i =0; i<checkboxes.length; i++) {
    if (checkboxes[i].checked == true){
        chrome.history.deleteUrl({url: checkboxes[i].value})
    }
  }
  location.reload()
}

document.getElementById('removeAll').onclick = function() {
  chrome.history.deleteAll(function(){
    location.reload();
  })
}

document.getElementById('seeAll').onclick = function() {
    location.reload();
}
