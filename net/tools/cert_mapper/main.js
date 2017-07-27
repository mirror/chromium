"use strict";

var dom = (function() {
  return {
    add: function(parent, tagName, optText, optClassName) {
      var node = parent.ownerDocument.createElement(tagName);
      parent.appendChild(node);

      if (optText != undefined)
        dom.addText(node, optText);
      if (optClassName != undefined)
        node.className = optClassName;

      return node;
    },

    addLink: function(parent, text, href) {
      var node = dom.add(parent, 'a');
      dom.addText(node, text);
      node.href = href;
      return node;
    },

    addActionLink: function(parent, text, onclick) {
      var node = dom.addLink(parent, text, 'javascript:void(0)');
      node.onclick = onclick;
      return node;
    },

    addText: function(parent, text) {
      var textNode = parent.ownerDocument.createTextNode(text);
      parent.appendChild(textNode);
      return textNode;
    },

    clear: function(node) {
      node.innerHTML = '';
    },
  };
})();

function changeSort(parent, metricsItem, newSort) {
  setSort(metricsItem, newSort);
  drawMetricsItem(parent, metricsItem);
}

function setSort(metricsItem, newSort) {
  function getSortProperty(bucket) {
    let prop = metricsItem.sort || SORT_BY_COUNT;

    switch (Math.abs(prop)) {
      case SORT_BY_COUNT:
        return bucket.total;
      case SORT_BY_NAME:
        return bucket.name;
    }
    return undefined;
  }

  if (metricsItem.sort && Math.abs(metricsItem.sort) == Math.abs(newSort)) {
    // Reverse sort order.
    metricsItem.sort *= -1;
  } else {
    metricsItem.sort = newSort;
  }

  metricsItem.buckets.sort(function(a, b) {
    var multiplier = metricsItem.sort < 0 ? -1 : 1;

    var propA = getSortProperty(a);
    var propB = getSortProperty(b);

    var result = 0;

    if (propA < propB) {
      result = -1;
    } else if (propA > propB) {
      result = 1;
    }

    return result * multiplier;
  });
}

function drawOverviewPage() {
  var parent = document.body;
  dom.clear(parent);

  for (let metricsItem of g_data.metrics) {
    let div = dom.add(parent, 'div', undefined, 'metricsItem');
    drawMetricsItem(div, metricsItem);
  }
}

function prettyPrintBigNumber(x) {
  let origStr = x + '';
  let result = '';

  for (let i = 0; i < origStr.length; ++i) {
    let ch = origStr.charAt(origStr.length - 1 - i);

    if (i > 0 && i % 3 == 0)
      result = ',' + result;

    result = ch + result
  }

  return result;
}

function drawMetricsItem(parent, metricsItem) {
  dom.clear(parent);

  let includeSamplesColumn = false;
  for (let bucket of metricsItem.buckets) {
    if (bucket.samples.length > 0)
      includeSamplesColumn = true;
  }

  dom.add(parent, 'h3', metricsItem.name);
  dom.add(parent, 'p', 'Total: ' + prettyPrintBigNumber(metricsItem.total));

  var table = dom.add(parent, 'table');
  var thead = dom.add(table, 'thead');
  var theadTr = dom.add(thead, 'tr');

  var countTh = dom.add(theadTr, 'th');
  countTh.colSpan = 2;
  let countSortIndicator = '';
  let nameSortIndicator = '';

  let kUp = ' \u25B2';
  let kDown = ' \u25BC';

  if (metricsItem.sort == SORT_BY_COUNT) {
    countSortIndicator = kUp;
  } else if (metricsItem.sort == -SORT_BY_COUNT) {
    countSortIndicator = kDown;
  } else if (metricsItem.sort == SORT_BY_NAME) {
    nameSortIndicator = kUp;
  } else if (metricsItem.sort == -SORT_BY_NAME) {
    nameSortIndicator = kDown;
  }

  var countSortCol = dom.addActionLink(countTh, 'Count' + countSortIndicator, changeSort.bind(null, parent, metricsItem, SORT_BY_COUNT));

  var samplesTh;
  if (includeSamplesColumn)
    samplesTh = dom.add(theadTr, 'th');

  var descriptionTh = dom.add(theadTr, 'th');
  var descriptionSortCol = dom.addActionLink(descriptionTh, 'Name' + nameSortIndicator, changeSort.bind(null, parent, metricsItem, SORT_BY_NAME));

  var tbody = dom.add(table, 'tbody');

  for (let bucket of metricsItem.buckets) {
    var tr = dom.add(tbody, 'tr');
    dom.add(tr, 'td', prettyPrintBigNumber(bucket.total), 'colTotal');
    dom.add(tr, 'td', (100 * bucket.total / metricsItem.total).toFixed(3) + ' %', 'colPercent');

    if (includeSamplesColumn) {
      var samplesTd = dom.add(tr, 'td', undefined, 'colSamples');
      if (bucket.samples.length > 0)
        dom.addLink(samplesTd, bucket.samples.length + ' samples', '#samples/' + encodeURIComponent(bucket.id));
    }

    dom.add(tr, 'td', bucket.name, 'colName');
  }
}

function getQueryParams() {
  var params = {};

  var parts = window.location.hash.substr(1).split('&');
  for(var i = 0; i < parts.length; i++) {
    var keyAndValue = parts[i].split('=');
    params[decodeURIComponent(keyAndValue[0])] = decodeURIComponent(keyAndValue[1]);
  }

  return params;
}

function findSamplesForBucket(bucketId) {
  for (let metricsItem of g_data.metrics) {
    for (let bucket of metricsItem.buckets) {
      if (bucket.id == bucketId)
        return bucket.samples;
    }
  }
  return null;
}

function drawSamplesPage(bucketId) {
  var parent = document.body;
  dom.clear(parent);

  parent = dom.add(parent, 'div', undefined, 'samplesDiv');

  let samples = findSamplesForBucket(bucketId);
  if (!samples) {
    dom.add(parent, 'h3', 'No bucket for id=' + bucketId);
    return;
  }

  dom.add(parent, 'h3', 'Samples');

  var ul = dom.add(parent, 'ul');

  for (let sampleUrl of samples) {
    var li = dom.add(ul, 'li');
    dom.addLink(li, sampleUrl, sampleUrl);
  }
}

function onHashChange() {
  let hash = window.location.hash;

  // Draw the samples page.
  let regex = /^#samples\/([0-9]+)$/;
  let match = regex.exec(hash);

  if (match) {
    drawSamplesPage(parseInt(match[1]));
    return;
  }

  // Otherwise draw the overview page.
  drawOverviewPage();
}

function onLoad() {
  // Do an initial sorting of the data, and also assign an ID to each bucket.
  let bucketId = 0;
  for (let metricsItem of g_data.metrics) {
    for (let bucket of metricsItem.buckets) {
      bucket.id = bucketId++;
    }
    setSort(metricsItem, -SORT_BY_COUNT);
  }

  // Draw the current page based on the URL.
  onHashChange();
}

var SORT_BY_COUNT = 1;
var SORT_BY_NAME = 2;

document.addEventListener("DOMContentLoaded", onLoad);
window.addEventListener("hashchange", onHashChange);

