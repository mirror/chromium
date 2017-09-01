// Read the file and call displayData.
function readFile(files) {
  console.log("readFile");
  var file = files[0];
  if (!file) {
    return;
  }
  var reader = new FileReader();
  reader.onload = function(e) {
    var contents = e.target.result;
    console.log("Read the data");
    createGraph(JSON.parse(contents));
  }
  reader.readAsText(file);
}

function pageToFillColor(page) {
  const prefixToColor = {"third_party/Webkit": "darksalmon", "v8": "olivegreen",
                         "base": "purple", "content": "blue", "components": "pink",
                         "chrome": "darkblue", "net": "black",
                         "third_party/webrtc": "orange", "third_party/icu": "yellow"};
  const prefixes = Object.keys(prefixToColor);
  let size_and_filename = page.size_and_filenames[0];
  if (!size_and_filename) {
    return "lightgrey";
  }

  let dominant_filename = size_and_filename[1];
  for (var i = 0; i < prefixes.length; i++) {
    let prefix = prefixes[i];
    if (dominant_filename.startsWith(prefix)) {
      return prefixToColor[prefix];
    }
  }
  return "darkgrey";
}

// Takes the json data and displays it.
function createGraph(data) {
  const PAGE_SIZE = 4096;

  let offsets = data.map((x) => x.offset).sort((a, b) => a - b);
  let minOffset = +offsets[0];
  let maxOffset = +offsets[offsets.length - 1] + PAGE_SIZE;
  let lanes = 1;  // Number of data tracks.

  let margins = [20, 15, 15, 120]  // top right bottom left
  let width = window.innerWidth - margins[1] - margins[3]
  let height = 500 - margins[0] - margins[2]
  let miniHeight = lanes * 12 + 50
  let mainHeight = height - miniHeight - 50

  //scales
  let globalXScale = d3.scale.linear()
      .domain([minOffset, maxOffset])
      .range([0, width]);

  let zoomedXScale = d3.scale.linear()
      .domain([minOffset, maxOffset])
      .range([0, width]);

  let mainYScale = d3.scale.linear()
      .domain([0, lanes])
      .range([0, mainHeight]);

  let miniYScale = d3.scale.linear()
      .domain([0, lanes])
      .range([0, miniHeight]);

  let globalXAxis = d3.svg.axis()
      .scale(globalXScale)
      .orient("bottom")
      .ticks(10 * PAGE_SIZE);

  let zoomedXAxis = d3.svg.axis()
      .scale(zoomedXScale)
      .orient("bottom")
      .ticks(10 * PAGE_SIZE);

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

  // Main lines and text
  main.append("g").selectAll(".laneLines")
      .data(["Dominant Component"])
      .enter().append("line")
      .attr("x1", margins[1])
      .attr("y1", (d) => mainYScale(0))
      .attr("x2", width)
      .attr("y2", (d) => mainYScale(0))
      .attr("stroke", "lightgray");

  main.append("g").selectAll(".laneText")
      .data(["Dominant Component"])
      .enter().append("text")
      .text((d) => d)
      .attr("x", -margins[1])
      .attr("y", (d, index) => mainYScale(index + .5))
      .attr("dy", ".5ex")
      .attr("text-anchor", "end")
      .attr("class", "laneText");

  let itemRects = main.append("g")
      .attr("clip-path", "url(#clip)");

  // Mini lines and text
  mini.append("g").selectAll(".laneLines")
      .data(["lanes"])
      .enter().append("line")
      .attr("x1", margins[1])
      .attr("y1", (d) => miniYScale(0))
      .attr("x2", width)
      .attr("y2", (d) => miniYScale(0))
      .attr("stroke", "lightgray");

  mini.append("g").selectAll(".laneText")
      .data(["Dominant Component"])
      .enter().append("text")
      .text((d) => d)
      .attr("x", -margins[1])
      .attr("y", (d, i) => miniYScale(i + .5))
      .attr("dy", ".5ex")
      .attr("text-anchor", "end")
      .attr("class", "laneText");

  console.log(data.length)
  //mini item rects
  mini.append("g").selectAll("miniItems")
      .data(data)
      .enter().append("rect")
      .attr("class", (d) => "miniItem")
      .attr("x", (d) => globalXScale(d.offset))
      .attr("y", (d) => miniYScale(.5) - 5)
      .attr("width", (d) => globalXScale(d.offset + PAGE_SIZE) - globalXScale(d.offset))
      .attr("height", 10)
      .style("fill", pageToFillColor);

  //mini labels
  // mini.append("g").selectAll(".miniLabels")
  //     .data(items)
  //     .enter().append("text")
  //     .text(function(d) {return d.id;})
  //     .attr("x", function(d) {return x(d.start);})
  //     .attr("y", function(d) {return y2(d.lane + .5);})
  //     .attr("dy", ".5ex");

  //brush
  let brush = d3.svg.brush()
      .x(globalXScale)
      .on("brush", display);

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
    console.log("min = " + minExtent + " max = " + maxExtent);
    visItems = data.filter((d) => d.offset < maxExtent &&
        d.offset + PAGE_SIZE > minExtent);
    console.log("Visible items " + visItems.length);

    mini.select(".brush")
	.call(brush.extent([minExtent, maxExtent]));

    zoomedXScale.domain([minExtent, maxExtent]);

    //update main item rects
    rects = itemRects.selectAll("rect")
	.data(visItems, (d) => d.offset)
	.attr("x", (d) => zoomedXScale(d.offset))
	.attr("width", (d) => zoomedXScale(d.offset + PAGE_SIZE) - zoomedXScale(d.offset));

    rects.enter().append("rect")
	.attr("class", (d) => "mainItem")
	.attr("x", (d) =>  zoomedXScale(d.offset))
	.attr("y", (d) => mainYScale(0) + 10)
	.attr("width", (d) => zoomedXScale(d.offset + PAGE_SIZE) - zoomedXScale(d.offset))
	.attr("height", (d) => .8 * mainYScale(1))
        .style("fill", pageToFillColor);

    // let dragRect = rects.enter().append("rect")
    //     .attr("class", (d, i) => "mainItem") //midi
    //     .attr("x", (d) => zoomedXScale(d.offset))
    //     .attr("y", (d) => mainYScale(0) + 10)
    //     .attr("width", (d) => zoomedXScale(d.offset + PAGE_SIZE) - zoomedXScale(d.offset))
    //     .attr("height", (d) => .8 * mainYScale(1))
    //     .attr("cursor", "move")
    //     .call(drag);

    rects.exit().remove();

    //update the item labels
    // labels = itemRects.selectAll("text")
    //     .data(visItems, function (d) { return d.id; })
    //     .attr("x", function(d) {return zoomedXScale(Math.max(d.offset, minExtent) + 2);});

    // labels.enter().append("text")
    //     .text(function(d) {return d.id;})
    //     .attr("x", function(d) {return x1(Math.max(d.offset, minExtent));})
    //     .attr("y", function(d) {return y1(.5);})
    //     .attr("text-anchor", "start");

    // labels.exit().remove();
  }

  function dragmove(d) {
    d3.select(this).attr("x", d.x = Math.max(0, d3.event.x));
  }
}
