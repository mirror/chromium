// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

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
  var labels = [];
  var values = [];
  for (var i = 0; i < histogram.buckets.length; ++i) {
    labels[i] = histogram.buckets[i].low + '-' + histogram.buckets[i].high;
    values[i] = histogram.buckets[i].count;
  }

  // Add the div and canvas element that will hold the chart.
  var chart_div = document.createElement('div');
  chart_div.id = 'chart_div' + k;
  chart_div.style = 'width: 400px; height: 400px;';
  document.getElementById('histograms').appendChild(chart_div);

  var canvas = document.createElement('canvas');
  canvas.id = 'chart_canvas' + k;
  canvas.style = 'width: 400px; height: 400px;';
  chart_div.appendChild(canvas);

  // Draw the chart.
  var myChart = new Chart(canvas, {
    type: 'bar',
    data: {labels: labels, datasets: [{data: values, borderWidth: 1}]},
    options: {
      title: {display: true, text: histogram.name},
      legend: {display: false}
    }
  });
}

/**
 * Initiates the request for histograms.
 */
function requestHistograms() {
  var query = '';
  if (document.location.pathname)
    query = document.location.pathname.substring(1);
  cr.sendWithPromise('requestHistograms', query)
      .then((histograms) => addHistograms(histograms));
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
  // Restric to only 20 histograms.
  if (i >= 19)
    return;
  i++;
  drawChart(h, i);
  setTimeout(addSingleHistogram, 0);
}

/**
 * Callback from backend with the list of histograms. Builds the UI.
 * @param {array} histograms The list of histograms.
 */
function addHistograms(histograms) {
  $('histograms').innerHTML = '';
  H = histograms;
  addSingleHistogram();
}

/**
 * Load the initial list of histograms.
 */
document.addEventListener('DOMContentLoaded', function() {
  $('refresh').onclick = function() {
    requestHistograms();
    return false;
  };

  requestHistograms();
});
