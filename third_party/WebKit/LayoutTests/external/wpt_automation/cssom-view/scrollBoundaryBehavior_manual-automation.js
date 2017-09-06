importAutomationScript('/pointerevents/pointerevent_common_input.js');

function inject_input() {
  return touchScrollInTarget('#blue', 'down').then(function() {
    return touchScrollInTarget('#blue', 'right');
  }).then(function() {
    return touchScrollInTarget('#blue', 'down');
  }).then(function() {
    return touchTapInTarget('#btnDone');
  }).then(function() {
    return touchScrollInTarget('#blue', 'right');
  }).then(function() {
    return touchScrollInTarget('#blue', 'down');
  }).then(function() {
    return touchTapInTarget('#btnDone');
  }).then(function() {
    return touchScrollInTarget('#blue', 'right');
  }).then(function() {
    return touchScrollInTarget('#blue', 'down');
  }).then(function() {
    return touchTapInTarget('#btnDone');
  }).then(function() {
    return touchScrollInTarget('#green', 'left');
  }).then(function() {
    return touchScrollInTarget('#green', 'up');
  }).then(function() {
    return touchTapInTarget('#btnDone');
  });
}