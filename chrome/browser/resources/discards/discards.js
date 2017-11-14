// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

/**
 * Initializes this page. Invoked by the DOMContentLoaded event.
 */
function initialize() {
  let uiHandler = new mojom.DiscardsDetailsProviderPtr;
  Mojo.bindInterface(
      mojom.DiscardsDetailsProvider.name, mojo.makeRequest(uiHandler).handle);

  let tabDiscardsInfoTableBody = $('tab-discards-info-table-body');
  let infos = [];
  let sortKey = 'utilityRank';
  let sortReverse = false;
  let updateTimer = null;

  // Set the column sort handlers.
  let tabDiscardsInfoTableHeader = $('tab-discards-info-table-header');
  let headers = tabDiscardsInfoTableHeader.children;
  for (let i = 0; i < headers.length; ++i) {
    headers[i].addEventListener('click', (e) => {
      let newSortKey = e.target.getAttribute('sort-key');

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
      let oldSortColumn = document.querySelector('.sort-column');
      oldSortColumn.classList.remove('sort-column');
      e.target.classList.add('sort-column');
      if (sortReverse)
        e.target.setAttribute('sort-reverse', '');
      else
        e.target.removeAttribute('sort-reverse');

      sortTabDiscardsInfoTable();
      renderTabDiscardsInfoTable();
    });
  }

  // Setup the "Discard a tab now" links.
  let discardNow = $('discard-now-link');
  let discardNowUrgent = $('discard-now-urgent-link');
  let discardListener = function(e) {
    if (e.target.hasAttribute('disabled'))
      return;
    e.target.setAttribute('disabled', '');
    let urgent = e.target.id.includes('urgent');
    uiHandler.discard(urgent).then((response) => {
      stableUpdateTabDiscardsInfoTable();
      e.target.removeAttribute('disabled');
    });
  };
  discardNow.addEventListener('click', discardListener);
  discardNowUrgent.addEventListener('click', discardListener);

  // Populate the table, and kick off a timer to do this repeatedly. This timer
  // can be canceled and rescheduled by stableUpdateTabDiscardsInfoTable if the
  // table is force updated by a user action.
  periodicUpdateTabDiscardsInfoTable();

  /**
   * Install a timer that updates the page periodically. The timer is stored in
   * |updateTimer| so that it may be canceled by other logic, as necessary.
   */
  function periodicUpdateTabDiscardsInfoTable() {
    stableUpdateTabDiscardsInfoTableImpl();
    updateTimer = setTimeout(periodicUpdateTabDiscardsInfoTable, 1000);
  }

  /**
   * Ensures the discards info table has the appropriate length.
   */
  function ensureTabDiscardsInfoTableLength() {
    let rows = tabDiscardsInfoTableBody.querySelectorAll('tr');
    if (rows.length < infos.length) {
      for (let i = rows.length; i < infos.length; ++i) {
        let row = createEmptyTabDiscardsInfoTableRow();
        row.setAttribute('row-index', i.toString());
        tabDiscardsInfoTableBody.appendChild(row);
      }
    } else if (rows.length > infos.length) {
      for (let i = infos.length; i < rows.length; ++i) {
        tabDiscardsInfoTableBody.removeChild(rows[i]);
      }
    }
  }

  /**
   * Compares two TabDiscardsInfos based on the data in the provided sort-key.
   * @param sortKey The key of the sort. See the "sort-key" attribute of the
   *                table headers for valid sort-keys.
   * @param a The first value being compared.
   * @param b The second value being compared.
   * @returns A negative number if a < b, 0 if a == b, and a positive number if
   *          a > b.
   */
  function compareTabDiscardsInfos(sortKey, a, b) {
    let val1 = a[sortKey];
    let val2 = b[sortKey];

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

  /**
   * Sorts the tab discards info data in |infos| according to the current
   * |sortKey|.
   */
  function sortTabDiscardsInfoTable() {
    infos = infos.sort((a, b) => {
      return (sortReverse ? -1 : 1) * compareTabDiscardsInfos(sortKey, a, b);
    });
  }

  /**
   * Converts a |secondsAgo| last-active time to a user friendly string.
   * @param secondsAgo The amount of time since the tab was active.
   * @returns An English string representing the last active time.
   */
  function lastActiveToString(secondsAgo) {
    if (secondsAgo < 60)
      return 'just now';

    // Minutes ago.
    let minutesAgo = (secondsAgo / 60).toFixed(0);
    if (minutesAgo <= 60) {
      return minutesAgo.toString() + ' minute' + (minutesAgo == 1 ? '' : 's') +
          ' ago';
    }

    // Hours and minutes and ago.
    let hoursAgo = (secondsAgo / 60 / 60).toFixed(0);
    minutesAgo = minutesAgo % 60;
    if (hoursAgo <= 23) {
      let s = hoursAgo.toString() + ' hour' + (hoursAgo == 1 ? '' : 's');
      if (minutesAgo > 0) {
        s += ' and ' + minutesAgo.toString() + ' minute' +
            (minutesAgo == 1 ? '' : 's');
      }
      s += ' ago';
      return s;
    }

    // Days ago.
    let daysAgo = (secondsAgo / 60 / 60 / 24).toFixed(1);
    if (daysAgo < 7) {
      return daysAgo.toString() + ' day' + (daysAgo == 1 ? '' : 's') + ' ago';
    }

    // Weeks ago.
    let weeksAgo = (secondsAgo / 60 / 60 / 24 / 7).toFixed(0);
    if (weeksAgo < 4) {
      return 'over ' + weeksAgo.toString() + ' week' +
          (weeksAgo == 1 ? '' : 's') + ' ago';
    }

    // Months ago.
    let monthsAgo = (secondsAgo / 60 / 60 / 24 / 7 / 4).toFixed(0);
    if (monthsAgo < 12) {
      return 'over ' + monthsAgo.toString() + ' month' +
          (monthsAgo == 1 ? '' : 's') + ' ago';
    }

    // Years ago.
    let yearsAgo = (secondsAgo / 60 / 60 / 24 / 7 / 52).toFixed(0);
    return 'over ' + yearsAgo.toString() + ' year' +
        (yearsAgo == 1 ? '' : 's') + ' ago';
  }

  /**
   * Returns a string representation of a boolean value for display in a table.
   * @param bool A boolean value.
   * @returns A string representing the bool.
   */
  function boolToString(bool) {
    return bool ? '✔' : '\xa0';
  }

  /**
   * Returns the index of the row in the table that houses the given element.
   */
  function getRowIndex(element) {
    let row = element.closest('tr');
    return parseInt(row.getAttribute('row-index'));
  }

  /**
   * Creates an empty tab discards table row with action-link listeners, etc.
   * By default the links are inactive.
   */
  function createEmptyTabDiscardsInfoTableRow() {
    let row = $('tab-discard-info-row');
    row = document.importNode(row.content, true);
    row = row.querySelector('tr');

    // Set up the listener for the auto-discardable toggle action.
    let isAutoDiscardable = row.querySelector('.is-auto-discardable-link');
    isAutoDiscardable.setAttribute('disabled', '');
    isAutoDiscardable.addEventListener('click', (e) => {
      // Do nothing if the action is currently disabled.
      if (e.target.hasAttribute('disabled'))
        return;
      // Get the info backing this row.
      let info = infos[getRowIndex(e.target)];
      // Disable the action. The update function is responsible for
      // re-enabling actions if necessary.
      e.target.setAttribute('disabled', '');
      // Perform the action.
      uiHandler.setAutoDiscardable(info.id, !info.isAutoDiscardable)
          .then((response) => {
            stableUpdateTabDiscardsInfoTable();
          });
    });

    // Set up the listeners for discard links.
    let discardListener = function(e) {
      // Do nothing if the action is currently disabled.
      if (e.target.hasAttribute('disabled'))
        return;
      // Get the info backing this row.
      let info = infos[getRowIndex(e.target)];
      // Determine whether this is urgent or not.
      let urgent = e.target.classList.contains('discard-urgent-link');
      // Disable the action. The update function is responsible for
      // re-enabling actions if necessary.
      e.target.setAttribute('disabled', '');
      // Perform the action.
      uiHandler.discardById(info.id, urgent).then((response) => {
        stableUpdateTabDiscardsInfoTable();
      });
    };
    let discardLink = row.querySelector('.discard-link');
    let discardUrgentLink = row.querySelector('.discard-urgent-link');
    discardLink.addEventListener('click', discardListener);
    discardUrgentLink.addEventListener('click', discardListener);

    return row;
  }

  /**
   * Updates a tab discards info table row in place. Sets/unsets 'disabled'
   * attributes on action-links as necessary, and populates all contents.
   */
  function updateTabDiscardsInfoTableRow(row, info) {
    // Update the content.
    row.querySelector('.utility-rank-cell').textContent =
        info.utilityRank.toString();
    row.querySelector('.favicon').setAttribute('src', info.faviconUrl);
    row.querySelector('.title-div').textContent = info.title;
    row.querySelector('.title-div').textContent = info.title;
    row.querySelector('.tab-url-cell').textContent = info.tabUrl;
    row.querySelector('.is-app-cell').textContent = boolToString(info.isApp);
    row.querySelector('.is-internal-cell').textContent =
        boolToString(info.isInternal);
    row.querySelector('.is-media-cell').textContent =
        boolToString(info.isMedia);
    row.querySelector('.is-pinned-cell').textContent =
        boolToString(info.isPinned);
    row.querySelector('.is-discarded-cell').textContent =
        boolToString(info.isDiscarded);
    row.querySelector('.discard-count-cell').textContent =
        info.discardCount.toString();
    row.querySelector('.is-auto-discardable-div').textContent =
        boolToString(info.isAutoDiscardable);
    row.querySelector('.last-active-cell').textContent =
        lastActiveToString(info.lastActiveSeconds);

    // Enable/disable action links as appropriate.
    row.querySelector('.is-auto-discardable-link').removeAttribute('disabled');
    let discardLink = row.querySelector('.discard-link');
    let discardUrgentLink = row.querySelector('.discard-urgent-link');
    if (info.isDiscarded) {
      discardLink.setAttribute('disabled', '');
      discardUrgentLink.setAttribute('disabled', '');
    } else {
      discardLink.removeAttribute('disabled');
      discardUrgentLink.removeAttribute('disabled');
    }
  }

  /**
   * Causes the discards info table to be rendered. Reuses existing table rows
   * in place to minimize disruption to the page.
   */
  function renderTabDiscardsInfoTable() {
    ensureTabDiscardsInfoTableLength();
    let rows = tabDiscardsInfoTableBody.querySelectorAll('tr');
    for (let i = 0; i < infos.length; ++i)
      updateTabDiscardsInfoTableRow(rows[i], infos[i]);
  }

  /**
   * Causes the discard info table to be updated in as stable a manner as
   * possible. That is, rows will stay in their relative positions, even if the
   * current sort order is violated. Only the addition or removal of rows (tabs)
   * can cause the layout to change. This cancels the pending timer, does an
   * immediate render, and restarts the periodic timer.
   */
  function stableUpdateTabDiscardsInfoTable() {
    if (updateTimer)
      window.clearTimeout(updateTimer);
    periodicUpdateTabDiscardsInfoTable();
  }

  /**
   * Implementation of the stable table update. Retrieves new data backing the
   * table, updates the existing data in place, and re-renders the table.
   */
  function stableUpdateTabDiscardsInfoTableImpl() {
    uiHandler.getTabDiscardsInfo().then((response) => {
      let newInfos = response.infos;
      let stableInfos = [];

      // Update existing infos in place, remove old ones, and append new ones.
      // This tries to keep the existing ordering stable so that clicking links
      // is minimally disruptive.
      for (let i = 0; i < infos.length; ++i) {
        let oldInfo = infos[i];
        let newInfo = null;
        for (let j = 0; j < newInfos.length; ++j) {
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
      for (let i = 0; i < newInfos.length; ++i) {
        let newInfo = newInfos[i];
        let oldInfo = null;
        for (let j = 0; j < infos.length; ++j) {
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

      // Render the content in place.
      renderTabDiscardsInfoTable();
    });
  }
}

document.addEventListener('DOMContentLoaded', initialize);
