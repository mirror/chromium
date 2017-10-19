// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define('main', [
    'mojo/public/js/bindings',
    'mojo/public/js/core',
    'mojo/public/js/connection',
    'chrome/browser/ui/webui/omnibox/omnibox.mojom',
    'content/public/renderer/service_provider',
], function(bindings, core, connection, browser, serviceProvider) {
// cr.define('omniboxDebug', function() {
    'use strict';

var page;

/**
 * A constant that's used to decide what autocomplete result
 * properties to output in what order. This is an array of
 * PresentationInfoRecord() objects; for details see that
 * function.
 * @type {Array.<Object>}
 * @const
 */
var uiInfoSelection_ = [
    {description: 'Index', global: 'responseIndex', isActive: true},
    {description: 'Id', global: 'responseId', isActive: false},
    {description: 'Arg Size', global: 'input_size'},
    {description: 'Query', global: 'original_query', isActive: true},
    {description: 'Time', global: 'time_since_omnibox_started_ms'},
    {description: 'Done', global: 'done'},
    {description: 'Contents', tag: 'contents', isActive: true},
    {description: 'Provider', tag: 'provider_name'},
    {description: 'Type', tag: 'type', isActive: true, class: 'nowrap'},
    {description: 'Relevance', tag: 'relevance', isActive: true},
    {description: 'Starred', tag: 'starred'},
    {description: 'Is History What You Typed Match',
     tag: 'is_history_what_you_typed_match'},
    {description: 'Description', tag: 'description'},
    {description: 'URL', tag: 'destination_url'},
    {description: 'Fill Into Edit', tag: 'fill_into_edit'},
    {description: 'Inline Autocomplete Offset',
     tag: 'inline_autocomplete_offset'},
    {description: 'Deletable', tag: 'deletable'},
    {description: 'From Previous', tag: 'from_previous'},
    {description: 'Transition Type',
     url: 'http://code.google.com/codesearch#OAMlx_jo-ck/src/content/public/' +
     'common/page_transition_types.h&exact_package=chromium&l=24',
     tag: 'transition'},
    {description: 'This Provider Done', tag: 'provider_done'},
    {description: 'Template URL', tag: 'template_url', class: 'nowrap'}
                        ];
var viewModes_ = [
    {description: 'Scraper', isActive: true, flush: true},
    {description: 'Metrics-Per-Position', isActive: false, flush: true},
    {description: 'Input-Files', isActive: false},
    {description: 'Input-Queries', isActive: false},
    {description: 'Dump-Scraper-Results', isActive: false},
    {description: 'SxS', isActive: false}];
var base_scrape_;
function populateDumpSxS() {
  var div = document.createElement('div');
  div.id = 'container-for-output';
  var input = document.createElement('input');
  input.type = 'text';
  input.value = 'MyFile.txt';
  input.placeholder = 'filename.txt';
  div.appendChild(input);
  var button = document.createElement('button');
  button.id = 'fancy-button';
  button.onclick = function() { downloadFile(); };
  button.textContent = 'Create file';
  div.appendChild(button);
  var output = document.createElement('output');
  div.appendChild(output);
  var textarea = document.createElement('textarea');
  textarea.innerHTML = 'You will see json text here once you click on ' +
      '"Create File". After that click on the ling to download the file.';
  textarea.style.display = 'block';
  div.appendChild(textarea);
  var aside = document.createElement('aside');
  aside.id = 'download-help';
  var ul = document.createElement('ul');
  var li = document.createElement('li');
  li.innerHTML = 'This link was created using a';
  var code = document.createElement('code');
  code.innerHTML = 'blob: + hello there';
  li.appendChild(code);
  ul.appendChild(li);
  aside.appendChild(ul);
  var ul = document.createElement('ul');
  ul.id = 'filelist';
  div.appendChild(ul);
  div.appendChild(aside);
  var parent = $('omnibox-text-Dump-Scraper-Results');
  parent.appendChild(div);
}
/**
 * Register our event handlers.
 */
function initialize() {
  base_scrape_ = {'stats' : '',
                  'scrape' : {
      'christos': [
  {
    'suggestion': 'christos',
    'title': 'Google Search',
    'type': 'search-what-you-typed',
    'otherSide': 0
  },
  {
    'suggestion': 'www.christosbridal.com',
    'title': 'Christos Bridal Dresses',
    'type': 'navsuggest',
    'otherSide': 0
  },
  {
    'suggestion': 'christos kozyrakis',
    'title': '',
    'type': 'search-suggest',
    'otherSide': 0
  },
  {
    'suggestion': 'christos papadimitriou',
    'title': 'Google Search',
    'type': 'search-suggest',
    'otherSide': 0
  }
                   ],
      'yahoo': [
  {
    'suggestion': 'yahoo',
    'type': 'search-what-you-typed'
  },
  {
    'suggestion': 'bing mail',
    'type': 'test-prov'
  },
  {
    'suggestion': 'yahoo mail',
    'type': 'search-suggest'
  },
  {
    'suggestion': 'www.yahoo.com',
    'type': 'navsuggest'
  }]}};
  $('query_files').addEventListener('change', handleFileSelect, false);
  $('scrape_files').addEventListener(
      'change', handleScrapeFileSelect, false);
  var id = 'select-columns';
  var configElement = $(id);
  configElement.onchange = updateColumnSelection(configElement);
  var cells = [];
  for (var i = 0; i < uiInfoSelection_.length; i++) {
    var option = document.createElement('configs');
    option.enabled = uiInfoSelection_[i].isActive;
    option.baseContent = uiInfoSelection_[i].description;
    option.textContent = appendCheckMark(option.baseContent, option.enabled);
    option.id = 'config_' + i;
    option.onclick = toggleProperty(i);
    cells.push(option);
  }
  cells.sort(function(a, b) {
      return String(a.baseContent) > String(b.baseContent) ? 1 : -1;});
  for (var i = 0; i < cells.length; i++) {
    configElement.appendChild(cells[i]);
  }
  $('omnibox-input-form').addEventListener(
      'submit', startOmniboxQuery, false);
  var id = 'view-mode';
  var viewElement = $(id);
  var allTables = $('view-tables');
  for (var i = 0; i < viewModes_.length; i++) {
    var option = document.createElement('view-modes');
    option.enabled = viewModes_[i].isActive;
    option.baseContent = viewModes_[i].description;
    option.textContent = appendCheckMark(option.baseContent, option.enabled);
    option.id = i;
    option.onclick = toggleView(i);
    viewElement.appendChild(option);
    var tableElement = document.createElement('table');
    tableElement.id = 'omnibox-text-' + viewModes_[i].description;
    tableElement.style.display = option.enabled ? 'table-cell' : 'none';
    allTables.appendChild(tableElement);
  }
  populateDumpSxS();
  // Create a table to hold all the autocomplete items.
  mainTable_ = document.createElement('table');
  mainTable_.className = 'autocomplete-results-table';
  window.requestFileSystem = window.requestFileSystem ||
      window.webkitRequestFileSystem;
  // window.requestFileSystem(window.TEMPORARY, 1024 * 1024,
  //                         onInitFsForWrite, errorHandler);
  updateQueriesList(queriesList);
}
function appendCheckMark(text, mark) {
  return text + (mark ? ' ✔ ' : ' ✗');
}
function toggleView(column) {
  return function() {
    this.enabled = !this.enabled;
    this.textContent = appendCheckMark(this.baseContent, this.enabled);
    viewModes_[column].isActive = this.enabled;
    var elems = document.getElementsByTagName('view-modes');
    for (var u = 0; u < elems.length; u++) {
      var vis = $(
          'omnibox-text-' + viewModes_[elems[u].id].description);
      if (elems[u].id == column) {
        vis.style.display = 'table-cell';
        continue;
      }
      vis.style.display = 'none';
      viewModes_[elems[u].id].isActive = !this.enabled;
      elems[u].enabled = !this.enabled;
      elems[u].textContent =
          appendCheckMark(elems[u].baseContent, !this.enabled);
    }
  }
}
function toggleProperty(column) {
  return function() {
    this.enabled = !this.enabled;
    this.textContent = this.baseContent + appendCheckMark('', this.enabled);
    uiInfoSelection_[column].isActive = this.enabled;
    var elems1 = document.getElementsByTagName('th-suggest');
    var elems2 = document.getElementsByTagName('td-suggest');
    var elems = Array.prototype.slice.call(elems1).concat(
        Array.prototype.slice.call(elems2));
    for (var u = 0; u < elems.length; u++) {
      if (elems[u].displayIndex === column) {
        elems[u].style.display = this.enabled ? 'table-cell' : 'none';
      }
    }
  }
}
/**
 * Toggle column visibility.
 * @param {string} tname Table name.
 * @param {int} column Column to toggle visibility.
 */
function updateColumnSelection(configElement) {
  return function() {
    var column = configElement.selectedIndex;
    var enabled = !uiInfoSelection_[column].isActive;
    uiInfoSelection_[column].isActive = enabled;
    var option = $('option_' + column);
    option.text = uiInfoSelection_[column].description +
        appendCheckMark('', enabled);
    //alert('ss');
  }
};
var mainTable_;
var responseId_;
var responseIndex_;
/**
 * @type {Array.<Object>} an array of all autocomplete results we've seen
 * for this query. We append to this list once for every call to
 * handleNewAutocompleteResult. For details on the structure of
 * the object inside, see the comments by addResultToOutput.
 */
var progressiveAutocompleteResults = {};
var positionCounts = [];
var queriesList = [
    'christos',
    'facebook',
    'yahoo',
    'microsoft',
    'baidu',
    'Shanghai',
    'beijing',
    'tokyo',
    'alaska',
    'sleep disorder clinics michigan',
    'portal services share point',
    '01boy',
    'http://www.a2zautoforums.com/archive/index.php/t-3168',
    'bologna cream',
    'for a complaint of swelling',
    'css overflow',
    'flights for the makkah',
    'does lakeland, fl. have an airport',
    'bittorrent download',
    'john f kenndey',
    'site:www.utdallas.edu ecsn room',
    'instagram',
    'nortel',
    'iit mumbai',
    'diablo 2',
    'dot geotextile road specification',
    'sql bit "data type"',
    'maxamine histamine',
    'nixon\'s accomplishments',
    'vespers portia',
    'ci497-801',
    'replacement birth certificate stoke on trent',
    'song calling your name dance',
    'map quest',
    'michael jackson hits',
    '+pianist +"wet smoking" +psychi*',
    'analog db-2',
    'ipo today',
    'get help from google now',
    'site:www.accommodationguide.net.au',
    'cheap air tickets from ny to san jose',
    'brown head black body bird'];
function sendQuery(query) {
  return function() {
    var pipe = core.createMessagePipe();
    var stub = connection.bindHandleToStub(pipe.handle0, browser.OmniboxPage);
    bindings.StubBindings(stub).delegate = page;
    page.stub_ = stub;
    page.browser_.startOmniboxQuery(query, query.length, false, false, 0, pipe.handle1);
  }
}
function startOmniboxQuery(event) {
  // First, clear the results of past calls (if any).
  progressiveAutocompleteResults = {};
  responseId_ = 0;
  responseIndex_ = 0;
  positionCounts = {'all-positions': {}};
  // Erase whatever is currently being displayed.
  resetDebugBox();
  var text = $('input-text').value;
  if (text) {
    setTimeout(sendQuery(text), 1000 * 1);
  } else {
    for (var i in queriesList) {
      var text = queriesList[i];
      setTimeout(sendQuery(text), 1000 * i);
    }
  }
  // Cancel the submit action. i.e., don't submit the form. (We handle
  // display the results solely with Javascript.)
  event.preventDefault();
}
/**
 * Returns an HTML Element of type table row that contains the
 * headers we'll use for labeling the columns. If we're in
 * detailed_mode, we use all the headers. If not, we only use ones
 * marked isActive.
 */
function createAutocompleteResultTableHeader() {
  var row = document.createElement('suggest-row');
  row.className = '';
  for (var i = 0; i < uiInfoSelection_.length; i++) {
    var headerCell = document.createElement('th-suggest');
    if (uiInfoSelection_[i].urlLabelForHeader != '') {
      // Wrap header text in URL.
      var linkNode = document.createElement('a');
      linkNode.href = uiInfoSelection_[i].urlLabelForHeader;
      linkNode.textContent = uiInfoSelection_[i].description;
      headerCell.appendChild(linkNode);
    } else {
      // Output header text without a URL.
      headerCell.textContent = uiInfoSelection_[i].description;
      headerCell.className = '.table-header';
    }
    headerCell.displayIndex = i;
    if (!uiInfoSelection_[i].isActive) {
      headerCell.style.display = 'none';
    }
    row.appendChild(headerCell);
  }
  return row;
}
/**
 * @param {Object} autocompleteSuggestion the particular autocomplete
 * suggestion we're in the process of displaying.
 * @param {string} propertyName the particular property of the autocomplete
 * suggestion that should go in this cell.
 * @return {HTMLTableCellElement} that contains the value within this
 * autocompleteSuggestion associated with propertyName.
 */
function createCellForPropertyAndRemoveProperty(
    autocompleteSuggestion, propertyName) {
  var cell = document.createElement('td-suggest');
  if (propertyName in autocompleteSuggestion) {
    if (typeof autocompleteSuggestion[propertyName] == 'boolean') {
      // If this is a boolean, display a checkmark or an X instead of
      // the strings true or false.
      if (autocompleteSuggestion[propertyName]) {
        cell.className = 'check-mark';
        cell.textContent = '✔';
      } else {
        cell.className = 'x-mark';
        cell.textContent = '✗';
      }
    } else {
      var text = String(autocompleteSuggestion[propertyName]);
      // If it's a URL wrap it in an href.
      var re = /^(http|https|ftp|chrome|file):\/\//;
      if (re.test(text)) {
        var aCell = document.createElement('a');
        aCell.textContent = text;
        aCell.href = text;
        cell.appendChild(aCell);
      } else {
        // All other data types (integer, strings, etc.) display their
        // normal toString() output.
        cell.textContent = autocompleteSuggestion[propertyName];
      }
    }
  } // else: if propertyName is undefined, we leave the cell blank
  return cell;
}
function removeChildrenFromNode(node) {
  if (!node || node.childNodes.length == 0) {
    return;
  }
  for (; node.childNodes.length > 0;) {
    node.removeChild(node.childNodes[node.childNodes.length - 1]);
  }
}
function addResult2(j, row, result) {
  row.displayIndex = j;
  if (!uiInfoSelection_[j].isActive) {
    row.style.display = 'none';
  }
  var numResults = result.combined_results.length;
  for (var i = 0; i < numResults; i++) {
    var subRow = document.createElement('sub-row');
    var suggestion = result.combined_results[i];
    if (!suggestion) {
      continue;
    }
    var cell = createCellForPropertyAndRemoveProperty(
        suggestion, uiInfoSelection_[j].tag);
    subRow.appendChild(cell);
    row.appendChild(subRow);
  }
}
function addResult1(row, result) {
  for (var j = 0; j < uiInfoSelection_.length; j++) {
    if (!uiInfoSelection_[j].global) {
      continue;
    }
    // This is a global property for all results.
    var cell = createCellForPropertyAndRemoveProperty(
        result, uiInfoSelection_[j].global);
    cell.displayIndex = j;
    if (!uiInfoSelection_[j].isActive) {
      cell.style.display = 'none';
    }
    row.appendChild(cell);
  }
}
function addResultTableToOutput(table, result, className) {
  // Loop over every autocomplete item and add it as a row in the table.
  var numResults = result.combined_results.length;
  var row = document.createElement('suggest-row');
  row.className = className;
  row.query = result.original_query;
  row.responseId = result.responseId;
  addResult1(row, result);
  for (var j = 0; j < uiInfoSelection_.length; j++) {
    if (uiInfoSelection_[j].global) {
      continue;
    }
    var rowSpanCell = document.createElement('td-suggest');
    if (uiInfoSelection_[j].class) {
      rowSpanCell.className = uiInfoSelection_[j].class;
    }
    addResult2(j, rowSpanCell, result);
    row.appendChild(rowSpanCell);
  }
  table.appendChild(row);
}
function reallocElementById(id, name, table, force) {
  var element = null;
  for (var u = 0; u < table.childNodes.length; u++) {
    if (table.childNodes[u].id == id) {
      element = table.childNodes[u];
      break;
    }
  }
  if (force && element) {
    removeChildrenFromNode(element);
    table.removeChild(element);
    element = null;
  }
  if (!element) {
    element = document.createElement(name);
    element.id = id;
    table.appendChild(element);
  }
  return element;
};
function updateCounts(result, inc) {
  if (!result.combined_results || (result.combined_results.length == 0)) {
    return;
  }
  for (var i = 0; i < result.combined_results.length; i++) {
    var suggestion = result.combined_results[i];
    var type = suggestion.type;
    if (type in positionCounts['all-positions']) {
      positionCounts['all-positions'][type] += inc;
    } else {
      positionCounts['all-positions'][type] = inc;
    }
    if (i in positionCounts) {
      if (type in positionCounts[i]) {
        positionCounts[i][type] += inc;
      } else {
        positionCounts[i][type] = inc;
      }
    } else {
      positionCounts[i] = {};
      positionCounts[i][type] = inc;
    }
  }
  var positionTable =
      $('omnibox-text-Metrics-Per-Position');
  positionTable.className = 'autocomplete-results-table';
  var keys = [];
  for (var position in positionCounts) {
    if (position != 'all-positions') {
      keys.push(position);
    }
  }
  keys.sort();
  var first = true;
  displayOneRow(positionTable, 'all-positions', true, false);
  for (var index in keys) {
    var position = keys[index];
    displayOneRow(positionTable, position, false, false);
  }
  displayOneRow(positionTable, 'all-positions', false, true);
}
function displayOneRow(positionTable, position, isHeader, force) {
  var row = reallocElementById('pos::' + isHeader + position,
                               'suggest-row', positionTable, force);
  row.query = position;
  var tag = isHeader ? 'th-suggest-counts' : 'td-suggest-counts';
  var td = reallocElementById('name', tag, row, false);
  if (isHeader) {
    td.textContent = 'position';
  } else {
    row.className = 'fancy-row';
    td.textContent = position;
  }
  for (var type in positionCounts['all-positions']) {
    var count = positionCounts[position][type];
    var td = reallocElementById(type, tag, row, false);
    var aCell = reallocElementById('text', 'a', td, false);
    if (isHeader) {
      td.className = 'autocomplete-results-table';
      aCell.textContent = type;
    } else if (count) {
      aCell.textContent = count;
    } else {
      aCell.textContent = '';
    }
  }
}
function updateMetrics(result) {
  var query = result.original_query;
  if (query in progressiveAutocompleteResults) {
    updateCounts(progressiveAutocompleteResults[query], -1);
  }
  progressiveAutocompleteResults[query] = result;
  updateCounts(result, 1);
}
function resetDebugBox() {
  for (var i = 0; i < viewModes_.length; i++) {
    if (!viewModes_[i].flush) {
      continue;
    }
    var output = $('omnibox-text-' + viewModes_[i].description);
    removeChildrenFromNode(output);
  }
  removeChildrenFromNode(mainTable_);
  mainTable_.appendChild(createAutocompleteResultTableHeader());
  var output = $('omnibox-text-Scraper');
  output.appendChild(mainTable_);
}
function handleScrapeFileSelect(evt) {
  var files = evt.target.files; // FileList object
  if (files.length == 0) {
    return;
  }
  for (var i = 0, f; f = files[i]; i++) {
    var reader = new FileReader();
    reader.onerror = errorHandler;
    reader.onload = scrapeFilesLoaded;
    reader.readAsText(f);
  }
}
function scrapeFilesLoaded(evt) {
  var original = base_scrape_;
  base_scrape_ = JSON.parse(evt.target.result);
  drawSxS(original);
}
function handleFileSelect(evt) {
  var files = evt.target.files; // FileList object
  if (files.length == 0) {
    return;
  }
  var queriesTable = $('omnibox-text-Input-Queries');
  queriesList = [];
  removeChildrenFromNode(queriesTable);
  var output = $('omnibox-text-Input-Files');
  removeChildrenFromNode(output);
  var th = document.createElement('suggest-row');
  for (var type in files[0]) {
    var a = document.createElement('th-suggest-counts');
    a.textContent = type;
    th.appendChild(a);
  }
  output.appendChild(th);
  for (var i = 0, f; f = files[i]; i++) {
    var tr = document.createElement('suggest-row');
    tr.className = 'fancy-row';
    for (var type in f) {
      var a = document.createElement('td-suggest-counts');
      a.textContent = f[type];
      tr.appendChild(a);
    }
    var reader = new FileReader();
    reader.onerror = errorHandler;
    reader.onload = loaded;
    reader.readAsText(f);
    output.appendChild(tr);
  }
}
function noDuplicates(value, index, ar) {
  if (ar[index - 1] == ar[index]) {
    return false;
    726 } else {
    return true;
  }
}
function loaded(evt) {
  var fileString = evt.target.result;
  var queries = fileString.split('\n');
  queries.sort();
  queriesList = queriesList.concat(queries);
  queriesList = queriesList.filter(noDuplicates);
  updateQueriesList(queriesList);
}
function updateQueriesList(queries) {
  var output = $('omnibox-text-Input-Queries');
  var display = output.style.display;
  output.style.display = 'none';
  removeChildrenFromNode(output);
  for (var q in queries) {
    if (!queries[q]) {
      continue;
    }
    var tr = document.createElement('suggest-row');
    tr.className = 'fancy-row';
    var a = document.createElement('td-suggest-counts');
    a.textContent = q;
    tr.appendChild(a);
    var a = document.createElement('td-suggest-counts');
    a.textContent = queries[q];
    tr.appendChild(a);
    output.appendChild(tr);
  }
  output.style.display = display;
}
function onInitFsForWrite(fs) {
  fs.root.getFile('christos.chrysafis.txt', {create: true},
                  function(fileEntry) {
      // Create a FileWriter object for our FileEntry (log.txt).
      fileEntry.createWriter(function(fileWriter) {
          fileWriter.onwriteend = function(e) {
            console.log('--------- Write completed.');
          };
          fileWriter.onerror = function(e) {
            console.log('--------- Write failed: ' + e.toString());
          };
          // Create a new Blob and write it to log.txt.
          // Note: window.WebKitBlobBuilder in Chrome 12.
          var bb = new window.WebKitBlobBuilder();
          bb.append('Lorem Ipsum');
          fileWriter.write(bb.getBlob('text/plain'));
        }, errorHandler);
    }, errorHandler);
}
function errorHandler(evt) {
  var tag = [];
  for (u in evt) {
    tag += u + '=' + evt[u] + '\n';
  }
  alert(tag);
  //alert('error reading ' + evt.name + ' ' + evt.target.error.name);
  var files = document.querySelectorAll('.dragout');
  for (var i = 0, file; file = files[i]; ++i) {
    file.addEventListener('dragstart', function(e) {
        e.dataTransfer.setData('DownloadURL', this.dataset.downloadurl);
      }, false);
  }
}
function toArray(list) {
  return Array.prototype.slice.call(list || [], 0);
}
var cleanUp = function(a) {
  a.textContent = 'Downloaded';
  a.dataset.disabled = true;
  // Need a small delay for the revokeObjectURL to work properly.
  setTimeout(function() {
      window.URL.revokeObjectURL(a.href);
    }, 1500);
};
function minimizeInfo(result) {
  var small = [];
  for (var i = 0; i < result.combined_results.length; i++) {
    var suggestion = result.combined_results[i];
    var type = suggestion.type;
    small.push({'suggestion': suggestion.contents,
            'title': suggestion.description, 'type': type});
  }
  return small;
}
function jsonOutput() {
  var summary = {};
  for (var query in progressiveAutocompleteResults) {
    summary[query] = minimizeInfo(progressiveAutocompleteResults[query]);
  }
  var toSerialize = {'stats': positionCounts, 'scrape': summary};
  return toSerialize;
}
function queryDifferences(s) {
  var sugx = [[], []];
  for (var i = 0; i < 2; i++) {
    for (var u = 0; u < s[i].length; u++) {
      sugx[i][u] = s[i][u].suggestion + s[i][u].title;
      s[i][u].otherSide = 0;
    }
  }
  var matchCount = 0;
  for (var i = 0; i < 2; i++) {
    for (var u = 0; u < s[i].length; u++) {
      s[i][u].otherSide = 1 + sugx[1 - i].indexOf(s[i][u].suggestion +
                                                  s[i][u].title);
      matchCount += s[i][u].otherSide == u + 1;
    }
  }
  if (matchCount == s[0].length + s[1].length) {
    return '';
  } else {
    return s;
  }
}
function differenceSxS(s1, s2) {
  var summary1 = s1.scrape;
  var summary2 = s2.scrape;
  var union = {};
  for (var query in summary1) {
    union[query] = 'left';
  }
  for (var query in summary2) {
    if (query in union) {
      union[query] = 'both';
    } else {
      union[query] = 'right';
    }
  }
  var diff = {};
  for (var query in union) {
    if (union[query] == 'left') {
      diff[query] = {'left': summary1[query]};
    } else if (union[query] == 'right') {
      diff[query] = {'right': summary2[query]};
    } else {
      var s = [summary1[query], summary2[query]];
      s = queryDifferences(s);
      if (s != '') {
        diff[query] = {'left': s[0], 'right': s[1]};
        884 }
      885 }
  }
  return diff;
}
function addClass(id, className) {
  var element = $(id);
  if (id != -1 && element) {
    removeClass(id, className);
    element.className += ' ' + className;
  }
};
function getATextElementWithHref(url, text) {
  var a = document.createElement('a');
  a.href = url;
  var aText = document.createTextNode(text);
  a.appendChild(aText);
  return a;
};
/**
 * Removes a class from an element.
 * @param {string} id The element id.
 * @param {string} className The name of the class.
 */
function removeClass(id, className) {
  var element = $(id);
  if (id != -1 && element) {
    element.className = element.className.replace(' ' + className, '');
  }
};
function highlightPair(
    className, ids, highlight) {
  var tools = this;
  return function() {
    var length = ids.length;
    for (var u = 0; u < length; u++) {
      if (highlight) {
        addClass(ids[u], className);
      } else {
        removeClass(ids[u], className);
      }
    }
  }
};
function drawSxS(exp) {
  var baseNode = $('omnibox-text-SxS');
  var diff = differenceSxS(exp, base_scrape_);
  removeChildrenFromNode(baseNode);
  var parent = document.createElement('table');
  var count = 0;
  for (var query in diff) {
    var element = diff[query];
    var row = document.createElement('original_query_row');
    var cell = document.createElement('td');
    cell.textContent = query;
    cell.colSpan = '2';
    row.appendChild(cell);
    parent.appendChild(row);
    var leftLength = element.left ? element.left.length : 0;
    var rightLength = element.right ? element.right.length : 0;
    var common = Math.min(leftLength, rightLength);
    var last = Math.max(leftLength, rightLength);
    for (var index = 0; index < last; index++) {
      var row = document.createElement('refinement_row');
      for (var u in {'left': '', 'right': ''}) {
        var cell = document.createElement('td');
        if ((u == 'left' && index < leftLength) ||
            (u == 'right' && index < rightLength)) {
          var x = element[u][index];
          var refinement = x.suggestion;
          var id1 = 'pos_' + count + '_' + index + u;
          var id2 = 'pos_' + count + '_' + (x.otherSide - 1) +
              (u == 'left' ? 'right' : 'left');
          var extraClass = x.otherSide ? 'inCommon' : '';
          cell.className = 'suggestionCell ' + extraClass;
          cell.id = id1;
          cell.otherSide = id2;
          var span = document.createElement('span');
          span.className = 'refinementNum' + index;
          var spanText = document.createTextNode('' + (index + 1) + '.');
          span.appendChild(spanText);
          cell.appendChild(span);
          cell.appendChild(
              getATextElementWithHref(
                  'http://www.google.com/' + encodeURIComponent(refinement),
                  refinement));
          if (x.title) {
            var title = document.createElement('url-title');
            title.textContent = ' ' + x.title;
            cell.appendChild(title);
          }
          var cellText = document.createTextNode(' ' + x.type);
          cell.appendChild(cellText);
          if (x.otherSide) {
            var span = document.createElement('span');
            span.className = 'sameAsMessage';
            var spanText = document.createTextNode('same as O' + x.otherSide);
            span.appendChild(spanText);
            cell.appendChild(span);
            var inputElem = document.createElement('input');
            inputElem.style.display = 'none';
            cell.appendChild(inputElem);
            cell.onmouseover = highlightPair('highlighted', [id1, id2], true);
            cell.onmouseout = highlightPair('highlighted', [id1, id2], false);
          }
        }
        row.appendChild(cell);
      }
      parent.appendChild(row);
    }
    count++;
  }
  baseNode.appendChild(parent);
}
function listResults(entries) {
  // Document fragments can improve performance since they're only appended
  // to the DOM once. Only one browser reflow occurs.
  var fragment = document.createDocumentFragment();
  entries.forEach(function(entry, i) {
      var img = entry.isDirectory ? '<img src="folder-icon.gif">' :
          '<img src="file-icon.gif">';
      var li = document.createElement('li');
      li.innerHTML = [img, '<span>', entry.name, '</span>'].join('');
      fragment.appendChild(li);
    });
  document.querySelector('#filelist').appendChild(fragment);
}
var downloadFile = function() {
  var container = document.querySelector('#container-for-output');
  var textarea = container.querySelector('textarea');
  var output = container.querySelector('output');
  var MIME_TYPE = 'text/plain';
  window.URL = window.webkitURL || window.URL;
  // window.BlobBuilder = window.BlobBuilder || window.WebKitBlobBuilder || window.MozBlobBuilder;
  var prevLink = output.querySelector('a');
  if (prevLink) {
    window.URL.revokeObjectURL(prevLink.href);
    output.innerHTML = '';
  }
  // var bb = new BlobBuilder();
  var jsonOut = jsonOutput();
  drawSxS(jsonOut);
  textarea.textContent = JSON.stringify(jsonOut, null, '\t');
  // bb.append(textarea.textContent);
  var a = document.createElement('a');
  a.download = container.querySelector('input[type="text"]').value;
  // a.href = window.URL.createObjectURL(bb.getBlob(MIME_TYPE));
  a.textContent = a.download;
  a.dataset.downloadurl = [MIME_TYPE, a.download, a.href].join(':');
  a.draggable = true; // Don't really need, but good practice.
  a.classList.add('dragout');
  output.appendChild(a);
  a.onclick = function(e) {
    if ('disabled' in this.dataset) {
      return false;
    }
    cleanUp(this);
  };
};
function drawVisualization() {
  var drawId = 'refinements_visualization';
  var parent = $('omnibox-text-Dump-SxS');
  var divElement = reallocElementById(drawId, 'div', parent, true);
  divElement.style.width = '900px';
  divElement.style.height = '500px';
  var stats = {};
  for (var pos in positionCounts) {
    for (var u in positionCounts[pos]) {
      var line = u;
      var index = 9;
      var value = positionCounts[pos][u];
      var locale = pos;
      if (!stats[locale]) {
        stats[locale] = {};
      }
      stats[locale].left = value;
      stats[locale].right = value;
    }
  }
  var graphData = [['SxS', 'new', 'original']];
  // We want to sort all items by the dictionary key.
  var sortedItems = [];
  for (var u in stats) {
    sortedItems.push([u, stats[u].left, stats[u].right]);
  }
  sortedItems.sort();
  for (var u in sortedItems) {
    graphData.push(sortedItems[u]);
  }
  // Some raw data (not necessarily accurate)
  var data = google.visualization.arrayToDataTable(graphData);
  var title = 'Absolute query coverage ';
  if (batchQueries.language) {
    title += batchQueries.language + '_' + batchQueries.country +
        ' ' + batchQueries.position;
  }
  var options = {
 title: title,
 hAxis: {title: '', gridlines: {count: 10}},
 vAxis: {title: 'Absolute query coverage', gridlines: {count: 10}},
 seriesType: 'bars'
  };
  var body = document.getElementsByTagName('body')[0];
  body.appendChild(divElement);
  var chart = new google.visualization.ComboChart(divElement);
  if (graphData.length > 1) {
    chart.draw(data, options);
  }
};


  function OmniboxPageImpl(browser) {
    this.browser_ = browser;
    initialize();
  }

  OmniboxPageImpl.prototype =
      Object.create(browser.OmniboxPage.stubClass.prototype);

  OmniboxPageImpl.prototype.handleNewAutocompleteResult = function(result) {

/**
 * Called by C++ code when we get an update from the
 * AutocompleteController. We simply append the result to
 * progressiveAutocompleteResults and refresh the page.
 */
  var removeDuplicates = true;
  var query = result.original_query;
  // Counts the number of refreshes with new results we got for this query.
  var refreshCount = 0;
  var className = (responseIndex_ & 1) ? 'odd' : 'even';
  if (removeDuplicates && query in progressiveAutocompleteResults) {
    responseIndex_--;
    refreshCount = progressiveAutocompleteResults[query].refreshCount + 1;
    var deleteList = [];
    var elems = document.getElementsByTagName('suggest-row');
    for (var u = 0; u < elems.length; u++) {
      if (elems[u].query == query) {
        deleteList.push(elems[u]);
      }
    }
    for (var u in deleteList) {
      mainTable_.removeChild(deleteList[u]);
    }
  }
  updateMetrics(result);
  result.refreshCount = refreshCount;
  result.responseId = responseId_++;
  result.responseIndex = responseIndex_++;
  addResultTableToOutput(mainTable_, result, className);

//    progressiveAutocompleteResults.push(result);
//    refresh();
  };

  return function() {
    var browserProxy = connection.bindHandleToProxy(
        serviceProvider.connectToService(
            browser.OmniboxUIHandlerMojo.name),
        browser.OmniboxUIHandlerMojo);
    page = new OmniboxPageImpl(browserProxy);
  };
});

//document.addEventListener('DOMContentLoaded', omniboxDebug.initialize);
