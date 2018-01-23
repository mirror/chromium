importAutomationScript('/pointerevents/pointerevent_common_input.js');

const WHEEL_SOURCE_TYPE = 2;

function smoothScroll(pixels_to_scroll, start_x, start_y,
    direction, speed_in_pixels_s) {
  return new Promise((resolve, reject) => {
    chrome.gpuBenchmarking.smoothScrollBy(pixels_to_scroll, resolve, start_x,
        start_y, WHEEL_SOURCE_TYPE, direction, speed_in_pixels_s);
  });
}

function waitForAnimationEnd() {
  return new Promise((resolve, reject) => {
    function tick(frames) {
      if (frames >= 40)
        resolve();
      else
        requestAnimationFrame(tick.bind(this, frames + 1));
    }
    tick(0);
  });
}

function inject_input() {
  return smoothScroll(200, 200, 500, 'up', 4000).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 500, 'up', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 500, 'up', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 500, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  }).then(() => {
    return smoothScroll(200, 200, 100, 'up', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return smoothScroll(200, 200, 100, 'left', 4000);
  }).then(() => {
    return waitForAnimationEnd();
  }).then(() => {
    return mouseClickInTarget('#btnDone');
  });
}