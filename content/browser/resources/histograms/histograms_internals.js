// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Whether the live mode is active of not.
var live_mode = false;

var previous_query = '';
var current_query = 'StartingQuery';

var charts = {};
var charts_highlight_duration = {};

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
 * Returns whether the labels were changed.
 */
function didLabelsChange(current_labels, new_labels) {
  if (current_labels.length != new_labels.length)
    return true;

  for (var i = 0; i < current_labels.length; ++i) {
    if (current_labels[i].localeCompare(new_labels[i]) != 0)
      return true;
  }

  return false;
}

/**
 * Returns whether the values were changed.
 */
function didValuesChange(current_values, new_values) {
  if (current_values.length != new_values.length)
    return true;

  for (var i = 0; i < current_values.length; ++i) {
    if (current_values[i] != new_values[i])
      return true;
  }

  return false;
}

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
  chart_div.style = 'width: 50%; height: 400px; padding: 20px 5px;';
  var h3 = document.createElement('h3')
  h3.innerHTML = histogram.name;
  chart_div.appendChild(h3);
  $('histograms').appendChild(chart_div);

  var canvas = document.createElement('canvas');
  canvas.id = 'chart_canvas' + k;
  chart_div.appendChild(canvas);

  // Draw the chart.
  var myChart = new Chart(canvas, {
    type: 'bar',
    data: {
      labels: labels_and_values[0],
      datasets: [{
        data: labels_and_values[1],
        borderWidth: 1,
        backgroundColor: 'rgba(0, 0, 255, 0.4)'
      }]
    },
    options: {
      // title: {display: true, text: histogram.name},
      legend: {display: false},
      animation: false,
      maintainAspectRatio: false,
      scales: {
        xAxes: [{ticks: {beginAtZero: true}, barPercentage: 0.7}],
        yAxes: [{ticks: {beginAtZero: true}, barPercentage: 0.7}],
      }
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
  if (!h)
    return;
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
  histograms = histograms.slice(0, 19);
  if (current_query.localeCompare(previous_query) == 0) {
    // It's the same query, refresh the graphs if something changed.
    refreshHistograms(histograms);
  } else {
    // It's a new query, remove the charts and draw new ones.
    charts = {};
    addHistograms(histograms);
  }
}

function refreshHistograms(histograms) {
  for (var i = 0; i < histograms.length; ++i) {
    chart = charts[histograms[i].name];
    if (chart) {
      // Put the histogram data in a readable format for the visualisation API:
      // an array for the labels and an array for the values.
      var labels_and_values = getLabelsAndValues(histograms[i]);

      var background_color = 'rgba(0, 0, 255, 0.4)';

      var needs_updating =
          didLabelsChange(chart.data.labels, labels_and_values[0]) ||
          didValuesChange(chart.data.datasets[0].data, labels_and_values[1]);
      if (needs_updating) {
        charts_highlight_duration[histograms[i].name] = 5;
        background_color = 'rgba(255, 0, 0, 0.6)';
      }

      // Check the status of the timer of the color change.
      if (charts_highlight_duration[histograms[i].name] == 0) {
        // Needs to be updated to go back to the normal color.
        needs_updating = true;
        charts_highlight_duration[histograms[i].name] =
            charts_highlight_duration[histograms[i].name] - 1;
      } else if (charts_highlight_duration[histograms[i].name] > 0) {
        charts_highlight_duration[histograms[i].name] =
            charts_highlight_duration[histograms[i].name] - 1;
      }

      if (needs_updating) {
        var newDataObject = {
          labels: labels_and_values[0],
          datasets: [{
            data: labels_and_values[1],
            borderWidth: 1,
            backgroundColor: background_color
          }]
        };

        chart.config.data = newDataObject;
        chart.update();
      }
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
  requestHistograms($('filter_input').value);
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
  $('filter_input').addEventListener('keyup', function(event) {
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
