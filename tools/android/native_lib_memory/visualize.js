// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

let codePages = undefined;
let residency = undefined;

function readFile(files, type) {
  let file = files[0];
  if (!file) return;
  let reader = new FileReader();
  reader.onload = function(e) {
    let contents = e.target.result;
    let jsonData = JSON.parse(contents);
    if (type == "code_pages") {
      codePages = jsonData;
    } else if (type == "residency") {
      residency = jsonData;
    }
  }
  reader.readAsText(file);
}

function go() {
  let residencyData = processResidency(codePages, residency);
  createGraph(codePages, residencyData);
}

// Returns:
// {timestamp: [{"offset": adjusted_offset, "status": "incore"}, ...], ...}
// (or out_of_core).
function processResidency(codePages, residency) {
  let result = {};
  // Aligns the residency data.
  //
  // To align the residency data with the code pages data:
  // - .text is the first range
  // - The offset is the difference between min_code_page_offset
  //   and smallest_residency_offset.
  let offsets = codePages.map((x) => x.offset).sort((a, b) => a - b);
  let minCodePageOffset = offsets[0];
  let range = [residency.ranges[0], residency.ranges[1]];
  let offset = range[0] - minCodePageOffset;

  // Align all the residency data.
  let timestamps = Object.keys(residency.residency).map((x) => +x);
  for (let i = 0; i < timestamps.length; i++) {
    let timestamp = timestamps[i];
    result[timestamp] = [];
    let timestampData = residency.residency[timestamp];
    // Resident pages.
    let incore = timestampData[0];
    for (let j = 0; j < incore.length; j++) {
      let pageAddress = incore[j];
      if (incore[j] >= range[0] && incore[j] < range[1]) {
        result[timestamp].push(
            {"offset": pageAddress - offset, "status": "incore"});
      }
    }
    // Non-resident pages.
    let outOfCore = timestampData[1];
    for (let j = 0; j < outOfCore.length; j++) {
      let pageAddress = outOfCore[j];
      if (outOfCore[j] >= range[0] && outOfCore[j] < range[1]) {
        result[timestamp].push(
            {"offset": pageAddress - offset, "status": "out_of_core"});
      }
    }
  }

  return result;
}

// Returns the fill color for an item.
function getFillColor(d) {
  if (d.type == "code_page") {
    return pageToFillColor(d.data);
  } else if (d.type == "residency") {
    return residencyFillColor(d.data);
  }
}

// Prefixes, color, description.
const COLOR_MAPPING = [
  [["third_party/WebKit"], "darksalmon", "Blink"],
  [["v8"], "green", "V8"],
  [["base"], "purple", "//base"],
  [["content"], "blue", "//content"],
  [["components"], "pink", "//components"],
  [["chrome/android"], "lightgreen", "//chrome/android"],
  [["chrome"], "darkblue", "//chrome"],
  [["net", "third_party/boringssl"], "black", "Net"],
  [["third_party/webrtc", "third_party/opus", "third_party/usrsctp"],
   "orange", "WebRTC"],
  [["third_party/ffmpeg", "third_party/libvpx"], "darkred", "Media"],
  [["third_party/icu"], "yellow", "ICU"],
  [["skia"], "red", "Skia"]];

// Returns the fill color for a code page item.
function pageToFillColor(page) {
  const sizeAndFilename = page.size_and_filenames[0];
  if (!sizeAndFilename) return "lightgrey";

  const dominantFilename = sizeAndFilename[1];
  for (let i = 0; i < COLOR_MAPPING.length; i++) {
    const prefixes = COLOR_MAPPING[i][0];
    const color = COLOR_MAPPING[i][1];
    for (let j = 0; j < prefixes.length; j++) {
      if (dominantFilename.startsWith(prefixes[j])) return color;
    }
  }
  return "darkgrey";
}

function buildColorLegend() {
  let table = document.getElementById("colors-legend-table");
  for (let i = 0; i < COLOR_MAPPING.length; i++) {
    let color = COLOR_MAPPING[i][1];
    let name = COLOR_MAPPING[i][2];
    let row = document.createElement("tr");
    row.setAttribute("align", "left");
    let colorRectangle = document.createElement("td");
    colorRectangle.setAttribute("class", "legend-rectangle");
    colorRectangle.setAttribute("style", `background-color: ${color}`);
    let label = document.createElement("td");
    label.innerHTML = name;
    row.appendChild(colorRectangle);
    row.appendChild(label);
    table.appendChild(row);
  }
}
buildColorLegend();

// Returns the fill color for a resident / non resident page.
function residencyFillColor(d) {
  if (d.status == "incore") {
    return "green";
  } else {
    return "red";
  }
}

// Returns the legend for a page item.
function getLegend(d) {
  if (d.type == "code_page") pageLegend(d.data);
}

// Returns the legend for a code page item.
function pageLegend(page) {
  let legend = document.getElementById("legend");
  legend.style.display = "block";

  let title = document.getElementById("legend-title");
  let sizeAndFilename = page.size_and_filenames[0];
  let filename = "";
  if (sizeAndFilename) filename = sizeAndFilename[1];

  title.innerHTML = `
    <b>Page offset:</b> ${page.offset}
    <br/>
    <b>Accounted for:</b> ${page.accounted_for}
    <br/>
    <b>Dominant filename:</b> ${filename}`;

  let table = document.getElementById("legend-table");
  table.innerHTML = "";
  let header = document.createElement("tr");
  header.setAttribute("align", "left");
  header.innerHTML = "<th>Size</th><th>Filename</th>";
  table.appendChild(header);
  for (let i = 0; i < page.size_and_filenames.length; i++) {
    let row = document.createElement("tr");
    row.setAttribute("align", "left");
    row.innerHTML = `<td>${page.size_and_filenames[i][0]}</td>
                     <td>${page.size_and_filenames[i][1]}</td>`;
    table.appendChild(row);
  }
}

// Returns the lane index (from 0) for an item.
function typeToLane(d) {
  return d.type == "code_page" ? 0 : 1;
}

// Takes the json data and displays it.
function createGraph(codePages, residencyData) {
  const PAGE_SIZE = 4096;

  let offsets = codePages.map((x) => x.offset).sort((a, b) => a - b);
  let minOffset = +offsets[0];
  let maxOffset = +offsets[offsets.length - 1] + PAGE_SIZE;

  let lanes = 2;  // Number of data tracks.
  let flatData = codePages.map((page) => ({"type": "code_page", "data": page}));
  let timestamps = Object.keys(residencyData).sort((a, b) => a - b);
  let maxTimestamp = timestamps[timestamps.length - 1];
  let offsetFromFirstDataPoint = maxTimestamp - timestamps[0];
  const maxTimestampResidencyData = residencyData[maxTimestamp];
  flatData = flatData.concat(
      maxTimestampResidencyData.map((d) => ({"type": "residency", "data": d})));

  let margins = [20, 15, 15, 120]  // top right bottom left
  let width = window.innerWidth - margins[1] - margins[3]
  let height = 500 - margins[0] - margins[2]
  let miniHeight = lanes * 12 + 50
  let mainHeight = height - miniHeight - 50

  let globalXScale = d3.scale.linear().domain([minOffset, maxOffset])
      .range([0, width]);
  const globalScalePageWidth = globalXScale(PAGE_SIZE) - globalXScale(0);

  let zoomedXScale = d3.scale.linear().domain([minOffset, maxOffset])
      .range([0, width]);

  let mainYScale = d3.scale.linear().domain([0, lanes]).range([0, mainHeight]);
  let miniYScale = d3.scale.linear().domain([0, lanes]).range([0, miniHeight]);

  let drag = d3.behavior.drag().on("drag", dragmove);

  let chart = d3.select("body")
      .append("svg")
      .attr("width", width + margins[1] + margins[3])
      .attr("height", height + margins[0] + margins[2])
      .attr("class", "chart");

  chart.append("defs").append("clipPath")
      .attr("id", "clip")
      .append("rect")
      .attr("width", width)
      .attr("height", mainHeight);

  let main = chart.append("g")
      .attr("transform", `translate(${margins[3]}, ${margins[0]})`)
      .attr("width", width)
      .attr("height", mainHeight)
      .attr("class", "main");

  let mini = chart.append("g")
      .attr("transform", `translate(${margins[3]}, ${mainHeight + margins[0]})`)
      .attr("width", width)
      .attr("height", miniHeight)
      .attr("class", "mini");

  const labels = ["Component",
                  `Residency at ${offsetFromFirstDataPoint}ms`];
  main.append("g").selectAll(".laneLines")
      .data(labels)
      .enter().append("line")
      .attr("x1", margins[1])
      .attr("y1", (d, i) => mainYScale(i))
      .attr("x2", width)
      .attr("y2", (d, i) => mainYScale(i))
      .attr("stroke", "lightgray");

  main.append("g").selectAll(".laneText")
      .data(labels)
      .enter().append("text")
      .text((d) => d)
      .attr("x", -margins[1])
      .attr("y", (d, i) => mainYScale(i + .5))
      .attr("dy", ".5ex")
      .attr("text-anchor", "end")
      .attr("class", "laneText");

  let itemRects = main.append("g")
      .attr("clip-path", "url(#clip)");

  mini.append("g").selectAll(".laneLines")
      .data(labels)
      .enter().append("line")
      .attr("x1", margins[1])
      .attr("y1", (d, i) => miniYScale(i))
      .attr("x2", width)
      .attr("y2", (d, i) => miniYScale(i))
      .attr("stroke", "lightgray");

  mini.append("g").selectAll(".laneText")
      .data(labels)
      .enter().append("text")
      .text((d) => d)
      .attr("x", -margins[1])
      .attr("y", (d, i) => miniYScale(i + .5))
      .attr("dy", ".5ex")
      .attr("text-anchor", "end");

  mini.append("g").selectAll("miniItems")
      .data(flatData)
      .enter().append("rect")
      .attr("class", (d) => "miniItem")
      .attr("x", (d) => globalXScale(d.data.offset))
      .attr("y", (d) => miniYScale(typeToLane(d) + .5) - 5)
      .attr("width", (d) => globalScalePageWidth)
      .attr("height", 10)
      .style("fill", getFillColor);

  let brush = d3.svg.brush().x(globalXScale).on("brush", display);
  mini.append("g")
      .attr("class", "x brush")
      .call(brush)
      .selectAll("rect")
      .attr("y", 1)
      .attr("height", miniHeight - 1);

  display();

  function display() {
    let rects, labels,
    minExtent = brush.extent()[0];
    maxExtent = brush.extent()[1];
    visibleItems = flatData.filter((d) => d.data.offset < maxExtent &&
        d.data.offset + PAGE_SIZE > minExtent);

    mini.select(".brush").call(brush.extent([minExtent, maxExtent]));
    zoomedXScale.domain([minExtent, maxExtent]);

    // Update the main rects.
    const zoomedXScalePageWidth = zoomedXScale(PAGE_SIZE) - zoomedXScale(0);
    rects = itemRects.selectAll("rect")
        .data(visibleItems, (d) => d.data.offset + d.type)
        .attr("x", (d) => zoomedXScale(d.data.offset))
        .attr("width", (d) => zoomedXScalePageWidth);

    rects.enter().append("rect")
        .attr("class", (d) => "mainItem")
        .attr("x", (d) =>  zoomedXScale(d.data.offset))
        .attr("y", (d) => mainYScale(typeToLane(d)) + 10)
        .attr("width", (d) => zoomedXScalePageWidth)
        .attr("height", (d) => .8 * mainYScale(1))
        .style("fill", getFillColor)
        .on("mouseover", getLegend);

    rects.exit().remove();
  }

  function dragmove(d) {
    d3.select(this).attr("x", d.x = Math.max(0, d3.event.x));
  }
}
