importAutomationScript('/pointerevents/pointerevent_common_input.js');

function smoothScroll(pixels_to_scroll, start_x, start_y,
    direction, speed_in_pixels_s) {
  return new Promise((resolve, reject) => {
    chrome.gpuBenchmarking.smoothScrollBy(pixels_to_scroll, resolve, start_x,
        start_y, 1, direction, speed_in_pixels_s);
  });
}

function inject_input() {
  return smoothScroll(200, 200, 500, 'up', 4000).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 500, 'up', 4000);
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 500, 'up', 4000);
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 100, 'up', 4000);
  }).then(() => {
    return smoothScroll(200, 200, 100, 'left', 4000);
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  });
}