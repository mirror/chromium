// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

var uiHandler;

function initialize() {
  uiHandler = new mojom.DiscardsDetailsProviderPtr;
  Mojo.bindInterface(
      mojom.DiscardsDetailsProvider.name, mojo.makeRequest(uiHandler).handle);

  // Maintains state about the table, its contents and the current sort order.
  var tabDiscardsInfoTableBody = $('tab-discards-info-table-body');
  var infos = [];
  var sortKey = 'utilityRank';
  var sortReverse = false;
  var updateTimer = null;

  // Set the column sort handlers.
  var tabDiscardsInfoTableHeader = $('tab-discards-info-table-header');
  var headers = tabDiscardsInfoTableHeader.children;
  for (var i = 0; i < headers.length; ++i) {
    headers[i].addEventListener('click', (e) => {
      var newSortKey = e.target.getAttribute('sort-key');

      // Skip columns that aren't explicitly labeled with a sort-key attribute.
      if (newSortKey == null)
        return;

      // Reverse the sort key if the key itself hasn't changed.
      if (sortKey == newSortKey) {
        sortReverse = !sortReverse;
      } else {
        sortKey = newSortKey;
        sortReverse = false;
      }

      // Undecorate the old sort column, and decorate the new one.
      var oldSortColumn = document.querySelector('.sort-column');
      oldSortColumn.classList.remove('sort-column');
      e.target.classList.add('sort-column');
      if (sortReverse)
        e.target.setAttribute('sort-reverse', '');
      else
        e.target.removeAttribute('sort-reverse');

      renderTabDiscardsInfoTable();
    });
  }

  // Setup the "Discard a tab now" links.
  var discardNow = $('discard-now-link');
  var discardNowUrgent = $('discard-now-urgent-link');
  function generateDiscardNowListener(urgent) {
    return function(e) {
      if (e.target.classList.contains('inactive-link'))
        return;
      e.target.classList.remove('active-link');
      e.target.classList.add('inactive-link');
      uiHandler.discard(urgent).then((response) => {
        stableUpdateTabDiscardsInfoTable();
        e.target.classList.remove('inactive-link');
        e.target.classList.add('active-link');
      });
    };
  }
  discardNow.addEventListener('click', generateDiscardNowListener(false));
  discardNowUrgent.addEventListener('click', generateDiscardNowListener(true));

  // Populate the table, and kick off a timer to do this repeatedly. This timer
  // can be canceled and rescheduled by stableUpdateTabDiscardsInfoTable if the
  // table is force updated by a user action.
  periodicUpdateTabDiscardsInfoTable();

  // Install a timer that updates the page periodically.
  function periodicUpdateTabDiscardsInfoTable() {
    stableUpdateTabDiscardsInfoTableImpl();
    updateTimer = setTimeout(periodicUpdateTabDiscardsInfoTable, 1000);
  }

  function clearTabDiscardsInfoTable() {
    tabDiscardsInfoTableBody.innerHTML = '';
  }

  /**
   * Compares two TabDiscardsInfos based on the data in the provided sort-key.
   */
  function compareTabDiscardsInfos(sortKey, a, b) {
    var val1 = a[sortKey];
    var val2 = b[sortKey];

    // Compares strings.
    if (sortKey == 'title' || sortKey == 'tabUrl') {
      val1 = val1.toLowerCase();
      val2 = val2.toLowerCase();
      if (val1 == val2)
        return 0;
      return val1 > val2 ? 1 : -1;
    }

    // Compares boolean fields.
    if ([
          'isApp', 'isInternal', 'isMedia', 'isPinned', 'isDiscarded',
          'isAutoDiscardable'
        ].includes(sortKey)) {
      if (val1 == val2)
        return 0;
      return val1 ? 1 : -1;
    }

    // Compares numeric fields.
    if (['discardCount', 'utilityRank', 'lastActiveSeconds'].includes(
            sortKey)) {
      return val1 - val2;
    }

    assertNotReached('Unsupported sort key: ' + sortKey);
    return 0;
  }

  function sortTabDiscardsInfoTable() {
    infos = infos.sort((a, b) => {
      return (sortReverse ? -1 : 1) * compareTabDiscardsInfos(sortKey, a, b);
    });
  }

  function lastActiveToString(secondsAgo) {
    if (secondsAgo < 60)
      return 'just now';

    // Minutes ago.
    var minutesAgo = (secondsAgo / 60).toFixed(0);
    if (minutesAgo <= 60) {
      return minutesAgo.toString() + ' minute' + (minutesAgo == 1 ? '' : 's') +
          ' ago';
    }

    // Hours and minutes and ago.
    var hoursAgo = (secondsAgo / 60 / 60).toFixed(0);
    minutesAgo = minutesAgo % 60;
    if (hoursAgo <= 23) {
      var s = hoursAgo.toString() + ' hour' + (hoursAgo == 1 ? '' : 's');
      if (minutesAgo > 0) {
        s += ' and ' + minutesAgo.toString() + ' minute' +
            (minutesAgo == 1 ? '' : 's');
      }
      s += ' ago';
      return s;
    }

    // Days ago.
    var daysAgo = (secondsAgo / 60 / 60 / 24).toFixed(1);
    if (daysAgo < 7) {
      return daysAgo.toString() + ' day' + (daysAgo == 1 ? '' : 's') + ' ago';
    }

    // Weeks ago.
    var weeksAgo = (secondsAgo / 60 / 60 / 24 / 7).toFixed(0);
    if (weeksAgo < 4) {
      return 'over ' + weeksAgo.toString() + ' week' +
          (weeksAgo == 1 ? '' : 's') + ' ago';
    }

    // Months ago.
    var monthsAgo = (secondsAgo / 60 / 60 / 24 / 7 / 4).toFixed(0);
    if (monthsAgo < 12) {
      return 'over ' + monthsAgo.toString() + ' month' +
          (monthsAgo == 1 ? '' : 's') + ' ago';
    }

    // Years ago.
    var yearsAgo = (secondsAgo / 60 / 60 / 24 / 7 / 52).toFixed(0);
    return 'over ' + yearsAgo.toString() + ' year' +
        (yearsAgo == 1 ? '' : 's') + ' ago';
  }

  function createTabDiscardsInfoTableRow(info) {
    var utilityRankCell = createElementWithClassName('td', 'utility-rank-cell');
    utilityRankCell.textContent = info.utilityRank.toString();

    var titleCell = createElementWithClassName('td', 'title-cell');
    var faviconDiv = createElementWithClassName('div', 'favicon-div');
    var titleDiv = createElementWithClassName('div', 'title-div');
    if (info.faviconUrl.length > 0) {
      var faviconImg = createElementWithClassName('img', 'favicon');
      faviconImg.setAttribute('width', 16);
      faviconImg.setAttribute('height', 16);
      faviconImg.setAttribute('src', info.faviconUrl);
      faviconDiv.appendChild(faviconImg);
    }
    titleDiv.textContent = info.title;
    titleCell.appendChild(faviconDiv);
    titleCell.appendChild(titleDiv);

    var tabUrlCell = createElementWithClassName('td', 'tab-url-cell');
    tabUrlCell.textContent = info.tabUrl;

    var isAppCell =
        createElementWithClassName('td', 'is-app-cell boolean-cell');
    isAppCell.textContent = info.isApp ? '✔' : '';

    var isInternalCell =
        createElementWithClassName('td', 'is-internal-cell boolean-cell');
    isInternalCell.textContent = info.isInternal ? '✔' : '';

    var isMediaCell =
        createElementWithClassName('td', 'is-media-cell boolean-cell');
    isMediaCell.textContent = info.isMedia ? '✔' : '';

    var isPinnedCell =
        createElementWithClassName('td', 'is-pinned-cell boolean-cell');
    isPinnedCell.textContent = info.isPinned ? '✔' : '';

    var isDiscardedCell =
        createElementWithClassName('td', 'is-discarded-cell boolean-cell');
    isDiscardedCell.textContent = info.isDiscarded ? '✔' : '';

    var discardCountCell =
        createElementWithClassName('td', 'discard-count-cell');
    discardCountCell.textContent = info.discardCount.toString();

    var isAutoDiscardableCell = createElementWithClassName(
        'td', 'is-auto-discardable-cell boolean-cell');
    var isAutoDiscardableDiv =
        createElementWithClassName('div', 'is-auto-discardable-div');
    isAutoDiscardableDiv.textContent = info.isAutoDiscardable ? '✔' : '\xa0';
    var isAutoDiscardableLink =
        createElementWithClassName('div', 'is-auto-discardable-link');
    isAutoDiscardableLink.textContent = 'Toggle';
    isAutoDiscardableLink.classList.add('active-link');
    var listener = function(e) {
      e.target.removeEventListener('click', listener);
      e.target.classList.remove('active-link');
      e.target.classList.add('inactive-link');
      uiHandler.setAutoDiscardable(info.id, !info.isAutoDiscardable)
          .then((response) => {
            stableUpdateTabDiscardsInfoTable();
          });
    };
    isAutoDiscardableLink.addEventListener('click', listener);
    isAutoDiscardableCell.appendChild(isAutoDiscardableDiv);
    isAutoDiscardableCell.appendChild(isAutoDiscardableLink);

    var lastActiveCell = createElementWithClassName('td', 'last-active-cell');
    lastActiveCell.textContent = lastActiveToString(info.lastActiveSeconds);

    var discardLinksCell =
        createElementWithClassName('td', 'discard-links-cell');
    var discardLink = createElementWithClassName('div', 'discard-link');
    discardLink.textContent = 'Discard';
    var discardUrgentLink =
        createElementWithClassName('div', 'discard-urgent-link');
    discardUrgentLink.textContent = 'Urgent Discard';
    if (info.isDiscarded) {
      discardLink.classList.add('inactive-link');
      discardUrgentLink.classList.add('inactive-link');
    } else {
      discardLink.classList.add('active-link');
      discardUrgentLink.classList.add('active-link');
      function generateDiscardListener(urgent) {
        return function(e) {
          e.target.removeEventListener('click', listener);
          e.target.classList.remove('active-link');
          e.target.classList.add('inactive-link');
          uiHandler.discardById(info.id, urgent).then((response) => {
            stableUpdateTabDiscardsInfoTable();
          });
        };
      }
      discardLink.addEventListener('click', generateDiscardListener(false));
      discardUrgentLink.addEventListener(
          'click', generateDiscardListener(true));
    }
    discardLinksCell.appendChild(discardLink);
    discardLinksCell.appendChild(discardUrgentLink);

    var row = document.createElement('tr');
    row.appendChild(utilityRankCell);
    row.appendChild(titleCell);
    row.appendChild(tabUrlCell);
    row.appendChild(isAppCell);
    row.appendChild(isInternalCell);
    row.appendChild(isMediaCell);
    row.appendChild(isPinnedCell);
    row.appendChild(isDiscardedCell);
    row.appendChild(discardCountCell);
    row.appendChild(isAutoDiscardableCell);
    row.appendChild(lastActiveCell);
    row.appendChild(discardLinksCell);

    return row;
  }

  function renderTabDiscardsInfoTable() {
    clearTabDiscardsInfoTable();
    sortTabDiscardsInfoTable();

    infos.forEach((info) => {
      tabDiscardsInfoTableBody.appendChild(createTabDiscardsInfoTableRow(info));
    });
  }

  function stableUpdateTabDiscardsInfoTable() {
    if (updateTimer)
      window.clearTimeout(updateTimer);
    periodicUpdateTabDiscardsInfoTable();
  }

  function stableUpdateTabDiscardsInfoTableImpl() {
    uiHandler.getTabDiscardsInfo().then((response) => {
      var newInfos = response.infos;
      var stableInfos = [];

      // Update existing infos in place, remove old ones, and append new ones.
      // This tries to keep the existing ordering stable so that clicking links
      // is minimally disruptive.
      for (var i = 0; i < infos.length; ++i) {
        var oldInfo = infos[i];
        var newInfo = null;
        for (var j = 0; j < newInfos.length; ++j) {
          if (newInfos[j].id == oldInfo.id) {
            newInfo = newInfos[j];
            break;
          }
        }

        // Old infos that have corresponding new infos as pushed first, in the
        // current order of the old infos.
        if (newInfo != null)
          stableInfos.push(newInfo);
      }

      // Make sure info about new tabs is appended to the end, in the order they
      // were originally returned.
      for (var i = 0; i < newInfos.length; ++i) {
        var newInfo = newInfos[i];
        var oldInfo = null;
        for (var j = 0; j < infos.length; ++j) {
          if (infos[j].id == newInfo.id) {
            oldInfo = infos[j];
            break;
          }
        }

        // Entirely new information (has no corresponding old info) is appended
        // to the end.
        if (oldInfo == null)
          stableInfos.push(newInfo);
      }

      // Swap out the current info with the new stably sorted information.
      infos = stableInfos;

      // Clear and regenerate the tab discards info table using the current sort
      // order.
      clearTabDiscardsInfoTable();
      infos.forEach((info) => {
        tabDiscardsInfoTableBody.appendChild(
            createTabDiscardsInfoTableRow(info));
      });
    });
  }
}

document.addEventListener('DOMContentLoaded', initialize);
