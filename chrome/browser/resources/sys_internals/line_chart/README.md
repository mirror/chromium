# Line Chart

## Overview

LineChart is a library to create a RWD HTML canvas line chart. User can zoom or
scroll the chart to observe the data.

Every line in the line chart is maintain by a |DataSeries|. There is a menu at
the left of the line chart to control the visibility of data series. Under the
chart there is a dummy scrollbar to show the current position of the chart.


## Usage

Create a |LineChart| object and pass a HTML div to it. |LineChart| will
initialize the HTML canvas and other objects. Also create several |DataSeries|
to maintain our data points. After that, we can add the data points to our data
series. Every data points include value and time, and its time must be increasing.

To show the data series on the line chart, we must setup the |SubChart| first.
A |SubChart| is a line chart which its data series share the same unit set. For
now there are two sub chart, "LEFT" and "RIGHT", which shows its label at left
or right side. We can give the |SubChart| a unit set, so the data series of the
sub chart will present with this unit set. See |LineChart.setSubChart()| for
details.

After setup the sub chart, we can add data series to the line chart.
|LineChart| will automatically show the button in the menu and render the
canvas.

To show another set of data series, just call |LineChart.setSubChart()| again
and add the data series.


## Example

``` js
const lineChart = new LineChart.LineChart($('id-of-root'));
const align = LineChart.LabelAlign.RIGHT;
lineChart.setSubChart(align, ['B', 'KB', 'MB', 'GB'], 1024);

const memory = new LineChart.DataSeries(
                    /* Title*/ 'Memory', /* Color */ '#aabbcc');
const swap = new LineChart.DataSeries(
                    /* Title*/ 'Swap', /* Color */ '#ccbbaa');

lineChart.addDataSeries(align, memory);
lineChart.addDataSeries(align, swap);

function updateData(newMemoryData, newSwapData) {
  /* Use current time as the time tick. */
  const time = Date.now();

  memory.addDataPoint(newMemoryData, time);
  swap.addDataPoint(newSwapData, time);

  /* Update the end time of the line chart to show the new data. */
  lineChart.updateEndTime(time);
}
```
