// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var microsecondsPerWeek = 1000 * 60 * 60 * 24 * 7;
var oneWeekAgo = (new Date).getTime() - microsecondsPerWeek;
var historyDiv = document.getElementById('historyDiv');
var colors = ['#4688F1', '#E8453C', '#F9BB2D', '#3AA757'];

chrome.history.search({
      'text': '',
      'startTime': oneWeekAgo,
      'maxResults': 99
    },
    function(historyItems) {
      for (var i = 0; i < historyItems.length; ++i) {
        let furl = 'chrome://favicon/' + historyItems[i].url;
        let spliceURL = historyItems[i].url.split('/').slice()
        let div = document.createElement('div');
        let br = document.createElement('br');
        let imgDiv = document.createElement('div');
        let urlDiv = document.createElement('div');
        let removeDiv = document.createElement('div');
        let titleP = document.createElement('p');
        let img = document.createElement('img');
        let a = document.createElement('a');
        let p = document.createElement('p');
        let checkbox = document.createElement('input');
        checkbox.type = 'checkbox';
        let buttonDelete = document.createElement('button');
        div.setAttribute('class', 'history');
        div.style.backgroundColor = colors[Math.floor(Math.random() * colors.length)]
        urlDiv.setAttribute('class', 'urlDiv')
        p.innerHTML = historyItems[i].title
        if( historyItems[i].title === ''){
          p.innerHTML = spliceURL[2]
        }
        p.setAttribute('class', 'urlP')
        urlDiv.appendChild(p)
        titleP.setAttribute('class', 'titleP')
        titleP.innerHTML = spliceURL[2]
        buttonDelete.setAttribute('id', historyItems[i].url)
        buttonDelete.setAttribute('class', 'removeButton')
        checkbox.setAttribute('class', 'removeCheck')
        checkbox.setAttribute('value', historyItems[i].url)
        buttonDelete.textContent = "Delete"
        img.setAttribute('src', furl);
        imgDiv.setAttribute('class', 'imageDiv');
        removeDiv.setAttribute('class', 'removeDiv');
        a.setAttribute("href", historyItems[i].url)
        a.appendChild(img)
        a.appendChild(titleP)
        imgDiv.appendChild(a)
        removeDiv.appendChild(buttonDelete)
        removeDiv.appendChild(checkbox)
        div.appendChild(imgDiv)
        div.appendChild(br)
        div.appendChild(urlDiv)
        div.appendChild(removeDiv)
        historyDiv.appendChild(div);
      }
  });

document.getElementById('searchSubmit').onclick = function() {
  historyDiv.innerHTML = ''
  let searchQuery = document.getElementById('searchInput').value
  console.log(searchQuery)
  chrome.history.search({
        'text': searchQuery,
        'startTime': oneWeekAgo
      },
      function(historyItems) {
        for (var i = 0; i < historyItems.length; ++i) {
          let furl = 'chrome://favicon/' + historyItems[i].url;
          let spliceURL = historyItems[i].url.split('/').slice()
          let div = document.createElement('div');
          let br = document.createElement('br');
          let imgDiv = document.createElement('div');
          let urlDiv = document.createElement('div');
          let removeDiv = document.createElement('div');
          let titleP = document.createElement('p');
          let img = document.createElement('img');
          let a = document.createElement('a');
          let p = document.createElement('p');
          let checkbox = document.createElement('input');
          checkbox.type = 'checkbox';
          let buttonDelete = document.createElement('button');
          div.setAttribute('class', 'history');
          div.style.backgroundColor = colors[Math.floor(Math.random() * colors.length)]
          urlDiv.setAttribute('class', 'urlDiv')
          p.innerHTML = historyItems[i].title
          if( historyItems[i].title === ''){
            p.innerHTML = spliceURL[2]
          }
          p.setAttribute('class', 'urlP')
          urlDiv.appendChild(p)
          titleP.setAttribute('class', 'titleP')
          titleP.innerHTML = spliceURL[2]
          buttonDelete.setAttribute('id', historyItems[i].url)
          buttonDelete.setAttribute('class', 'removeButton')
          checkbox.setAttribute('class', 'removeCheck')
          checkbox.setAttribute('value', historyItems[i].url)
          buttonDelete.textContent = "Delete"
          img.setAttribute('src', furl);
          imgDiv.setAttribute('class', 'imageDiv');
          removeDiv.setAttribute('class', 'removeDiv');
          a.setAttribute("href", historyItems[i].url)
          a.appendChild(img)
          a.appendChild(titleP)
          imgDiv.appendChild(a)
          removeDiv.appendChild(buttonDelete)
          removeDiv.appendChild(checkbox)
          div.appendChild(imgDiv)
          div.appendChild(br)
          div.appendChild(urlDiv)
          div.appendChild(removeDiv)
          historyDiv.appendChild(div);
        }
      })
}

document.getElementById('historyDiv').onclick = function(element) {
  let elementClicked = element.path[0].tagName
  console.log(elementClicked)
  if (elementClicked == 'BUTTON'){
    let deleteItem = element.path[0].id.toString()
    chrome.history.deleteUrl({url: deleteItem}, function() {
      location.reload()
    })
  }
}

document.getElementById('deleteSelected').onclick = function() {
  let checkboxes = document.getElementsByTagName('input');
  for (var i =0; i<checkboxes.length; i++) {
    if (checkboxes[i].checked == true){
        console.log(checkboxes[i].value)
        chrome.history.deleteUrl({url: checkboxes[i].value})
    }
  }
  location.reload()
}

document.getElementById('removeAll').onclick = function() {
  chrome.history.deleteAll(function(){
    location.reload()
  })
}

document.getElementById('seeAll').onclick = function() {
    location.reload()
}
