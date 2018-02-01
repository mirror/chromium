// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Whether the live mode is active of not.
var live_mode = false;

var previous_query = '';
var current_query = 'StartingQuery';

var charts = {};

var sliced_histograms;
var histograms_cache;

/**
 * Returns an array containing the labels and the values for the specified
 * histogram.
 *
 * params:
 *  histogram The histogram from which to extract the labels and data.
 *
 * returns:
 *  An array with the labels at index 0 and the values at index 1.
 */
function getLabelsAndValues(histogram) {
  var labels = [];
  var values = [];
  for (var i = 0; i < histogram.buckets.length; ++i) {
    labels[i] = histogram.buckets[i].low + '-' + histogram.buckets[i].high;
    values[i] = histogram.buckets[i].count;
  }

  return [labels, values];
};

/**
 * Adds a canvas element to the document and draws a chart for the histogram in
 * it.
 *
 * params:
 *    histogram The histogram to draw.
 *    k         The index of the histogram.
 */
function drawChart(histogram, k) {
  // Put the histogram data in a readable format for the visualisation API: an
  // array for the labels and an array for the values.
  var labels_and_values = getLabelsAndValues(histogram);

  // Add the div and canvas element that will hold the chart.
  var chart_div = document.createElement('div');
  chart_div.id = 'chart_div' + k;
  chart_div.style = 'width: 400px; height: 400px;';
  $('histograms').appendChild(chart_div);

  var canvas = document.createElement('canvas');
  canvas.id = 'chart_canvas' + k;
  canvas.style = 'width: 400px; height: 400px;';
  chart_div.appendChild(canvas);

  // Draw the chart.
  var myChart = new Chart(canvas, {
    type: 'bar',
    data: {
      labels: labels_and_values[0],
      datasets: [{
        data: labels_and_values[1],
        borderWidth: 1,
        backgroundColor: 'rgba(0, 0, 255, 0.7)'
      }]
    },
    options: {
      title: {display: true, text: histogram.name},
      legend: {display: false},
      animation: false
    }
  });

  // Add the chart to the charts dictionary.
  charts[histogram.name] = myChart;
}

/**
 * Initiates the drawing of the charts one at a time.
 */
let H;
let i = 0;
function addSingleHistogram() {
  const h = H.pop();
  if (!h) {
    histograms_cache = sliced_histograms;
    return;
  }
  i++;
  drawChart(h, i);
  setTimeout(addSingleHistogram, 0);
}

/**
 * Callback from backend with the list of histograms. Builds the UI.
 * @param {array} histograms The list of histograms.
 */
function receivedHistograms(histograms) {
  // Restric to only 20 histograms.
  sliced_histograms = histograms.slice(0, 19);
  if (current_query.localeCompare(previous_query) == 0) {
    // It's the same query, refresh the graphs if something changed.
    refreshHistograms(sliced_histograms);
  } else {
    // It's a new query, remove the charts and draw new ones.
    charts = {};
    addHistograms(sliced_histograms);
  }
}

function refreshHistograms(histograms) {
  for (var i = 0; i < histograms.length; ++i) {
    chart = charts[histograms[i].name];
    if (chart) {
      // Put the histogram data in a readable format for the visualisation API:
      // an array for the labels and an array for the values.
      var labels_and_values = getLabelsAndValues(histograms[i]);

      var newDataObject = {
        labels: labels_and_values[0],
        datasets: [{
          data: labels_and_values[1],
          borderWidth: 1,
          backgroundColor: 'rgba(0, 0, 255, 0.7)'
        }]
      }

                          chart.config.data = newDataObject;
      chart.update();
    }
  }
}

function addHistograms(histograms) {
  // Remove the current charts and reset the variables for showing the charts
  // one by one.
  $('histograms').innerHTML = '';
  H = histograms;
  i = 0;
  addSingleHistogram();
}

/**
 * Initiates the request of histograms that math the filter prefix.
 */
function filterHistograms() {
  requestHistograms($('filer_input').value);
}

/**
 * Initiates the request for histograms.
 */
function requestHistograms(query) {
  previous_query = current_query;
  current_query = query;
  cr.sendWithPromise('requestHistograms', query)
      .then((histograms) => receivedHistograms(histograms));
}

/**
 * Handles the live mode loop.
 */
function liveModeRefresh() {
  if (live_mode) {
    filterHistograms();
    setTimeout(liveModeRefresh, 1000);
  }
}

/**
 * Setup some fucntions and load the initial list of histograms.
 */
document.addEventListener('DOMContentLoaded', function() {
  // Setup the Filter button.
  $('filter_button').onclick = function() {
    filterHistograms();
  };

  // Setup the filter button to be triggered on enter from the filter input.
  $('filer_input').addEventListener('keyup', function(event) {
    event.preventDefault();
    if (event.keyCode === 13) {
      $('filter_button').click();
    }
  });

  // Setup the Live Mode cehckbox.
  $('live_mode_check').onclick = function() {
    live_mode = $('live_mode_check').checked;
    liveModeRefresh();
  };

  var query = '';
  if (document.location.pathname)
    query = document.location.pathname.substring(1);
  requestHistograms(query);
});
