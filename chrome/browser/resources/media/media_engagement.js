// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

// Allow a function to be provided by tests, which will be called when
// the page has been populated with media engagement details.
var pageIsPopulatedResolver = new PromiseResolver();
function whenPageIsPopulatedForTest() {
  return pageIsPopulatedResolver.promise;
}

(function() {

var uiHandler = null;
var info = null;
var engagementTableBody = null;
var sortReverse = true;
var sortKey = 'total_score';

function initializeProxies() {
  uiHandler = new mojom.MediaEngagementScoreDetailsProviderPtr;
  Mojo.bindInterface(
      mojom.MediaEngagementScoreDetailsProvider.name,
      mojo.makeRequest(uiHandler).handle);
  updateEngagementTable();
}

function initialize() {
  engagementTableBody = $('engagement-table-body');

  // Set table header sort handlers.
  var engagementTableHeader = $('engagement-table-header');
  var headers = engagementTableHeader.children;
  for (var i = 0; i < headers.length; i++) {
    headers[i].addEventListener('click', (e) => {
      var newSortKey = e.target.getAttribute('sort-key');
      if (sortKey == newSortKey) {
        sortReverse = !sortReverse;
      } else {
        sortKey = newSortKey;
        sortReverse = false;
      }
      var oldSortColumn = document.querySelector('.sort-column');
      oldSortColumn.classList.remove('sort-column');
      e.target.classList.add('sort-column');
      if (sortReverse)
        e.target.setAttribute('sort-reverse', '');
      else
        e.target.removeAttribute('sort-reverse');
      renderTable();
    });
  }
}

/**
 * Creates a single row in the engagement table.
 * @param {MediaEngagementScoreDetails} info The info to create the row from.
 * @return {HTMLElement}
 */
function createRow(info) {
  var originCell = createElementWithClassName('td', 'origin-cell');
  originCell.textContent = info.origin.url;

  var visitsCountCell = createElementWithClassName('td', 'visits-count-cell');
  visitsCountCell.textContent = info.visits;

  var mediaPlaybacksCountCell =
      createElementWithClassName('td', 'media-playbacks-count-cell');
  mediaPlaybacksCountCell.textContent = info.mediaPlaybacks;

  var lastPlaybackTimeCell =
      createElementWithClassName('td', 'last-playback-time-cell');
  if (info.last_media_playback_time)
    lastPlaybackTimeCell.textContent =
        new Date(info.lastMediaPlaybackTime).toISOString();

  var totalScoreCell = createElementWithClassName('td', 'total-score-cell');
  totalScoreCell.textContent = info.totalScore;

  var engagementBar = createElementWithClassName('div', 'engagement-bar');
  engagementBar.style.width = (info.totalScore * 10) + 'px';

  var engagementBarCell =
      createElementWithClassName('td', 'engagement-bar-cell');
  engagementBarCell.appendChild(engagementBar);

  var row = document.createElement('tr');
  row.appendChild(originCell);
  row.appendChild(visitsCountCell);
  row.appendChild(mediaPlaybacksCountCell);
  row.appendChild(lastPlaybackTimeCell);
  row.appendChild(totalScoreCell);
  row.appendChild(engagementBarCell);
  return row;
}

/**
 * Remove all rows from the engagement table.
 */
function clearTable() {
  engagementTableBody.innerHTML = '';
}

/**
 * Sort the engagement info based on |sortKey| and |sortReverse|.
 */
function sortInfo() {
  info.sort((a, b) => {
    return (sortReverse ? -1 : 1) * compareTableItem(sortKey, a, b);
  });
}

/**
 * Compares two MediaEngagementScoreDetails objects based on |sortKey|.
 * @param {string} sortKey The name of the property to sort by.
 * @return {number} A negative number if |a| should be ordered before
 * |b|, a positive number otherwise.
 */
function compareTableItem(sortKey, a, b) {
  var val1 = a[sortKey];
  var val2 = b[sortKey];

  // Compare the hosts of the origin ignoring schemes.
  if (sortKey == 'origin')
    return new URL(val1.url).host > new URL(val2.url).host ? 1 : -1;

  if (sortKey == 'visits' || sortKey == 'media_playbacks' ||
      sortKey == 'last_media_playback_time' || sortKey == 'total_score') {
    return val1 - val2;
  }

  assertNotReached('Unsupported sort key: ' + sortKey);
  return 0;
}

/**
 * Rounds the supplied value to two decimal places of accuracy.
 * @param {number} score
 * @return {number}
 */
function roundScore(score) {
  return Number(Math.round(score * 100) / 100);
}

/**
 * Regenerates the engagement table from |info|.
 */
function renderTable() {
  clearTable();
  sortInfo();

  info.forEach((info) => {
    info.total_score = roundScore(info.total_score);
    engagementTableBody.appendChild(createRow(info));
  });
}

/**
 * Retrieve media engagement info and render the engagement table.
 */
function updateEngagementTable() {
  // Populate engagement table.
  uiHandler.getMediaEngagementScoreDetails().then((response) => {
    info = response.info;
    renderTable(info);
    pageIsPopulatedResolver.resolve();
  });
}

document.addEventListener('DOMContentLoaded', function() {
  initializeProxies();
  initialize();
});
})();
