// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var microsecondsPerWeek = 1000 * 60 * 60 * 24 * 7;
var oneWeekAgo = (new Date).getTime() - microsecondsPerWeek;
var historyDiv = document.getElementById('historyDiv');
var colors = ['#4688F1', '#E8453C', '#F9BB2D', '#3AA757'];

function constructHistory(historyItems) {
    for (var i = 0; i < historyItems.length; ++i) {
      // select random color for this item div
      let randomColor = colors[Math.floor(Math.random() * colors.length)];
      // splices the host url for readability
      let spliceURL = historyItems[i].url.split('/').slice();
      // grabs url of page favicon
      let faviconUrl = 'chrome://favicon/' + historyItems[i].url;
      // create image tag to hold favicon
      let faviconImg = document.createElement('img');
      faviconImg.setAttribute('src', faviconUrl);
      // create a p tag to hold the title of the page
      let titleP = document.createElement('p');
      titleP.innerHTML = spliceURL[2];
      titleP.setAttribute('class', 'titleP');
      // create link around favicon and title
      let titleLink = document.createElement('a');
      titleLink.setAttribute("href", historyItems[i].url);
      titleLink.appendChild(faviconImg);
      titleLink.appendChild(titleP);
      // div holds favicon and page title
      let faviconDiv = document.createElement('div');
      faviconDiv.setAttribute('class', 'imageDiv');
      faviconDiv.appendChild(titleLink);
      // creates p to hold the page name
      let pageNameP = document.createElement('p');
      pageNameP.innerHTML = historyItems[i].title;
      // if page has no title, will use host url
      if( historyItems[i].title === '') {
        pageNameP.innerHTML = spliceURL[2];
      }
      pageNameP.setAttribute('class', 'urlP');
      // create div to hold pageNameP
      let urlDiv = document.createElement('div');
      urlDiv.setAttribute('class', 'urlDiv');
      urlDiv.appendChild(pageNameP);
      // create checkbox elements to remove items in bulk
      let checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      // create delete button to remove single item
      let buttonDelete = document.createElement('button');
      buttonDelete.setAttribute('id', historyItems[i].url);
      buttonDelete.setAttribute('class', 'removeButton');
      checkbox.setAttribute('class', 'removeCheck');
      checkbox.setAttribute('value', historyItems[i].url);
      buttonDelete.textContent = "Delete";
      // create a div to hold remove elements
      let removeDiv = document.createElement('div');
      removeDiv.setAttribute('class', 'removeDiv');
      removeDiv.appendChild(buttonDelete);
      removeDiv.appendChild(checkbox);
      // create div to hold elements
      let itemDiv = document.createElement('div');
      itemDiv.setAttribute('class', 'history');
      itemDiv.style.backgroundColor = randomColor;
      itemDiv.appendChild(faviconDiv);
      itemDiv.appendChild(urlDiv);
      itemDiv.appendChild(removeDiv);
      // append itemDiv historyDiv container
      historyDiv.appendChild(itemDiv);
    }
}

chrome.history.search({
      'text': '',
      'startTime': oneWeekAgo,
      'maxResults': 99
    }, constructHistory);

document.getElementById('searchSubmit').onclick = function() {
  historyDiv.innerHTML = ''
  let searchQuery = document.getElementById('searchInput').value
  chrome.history.search({
        'text': searchQuery,
        'startTime': oneWeekAgo
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
