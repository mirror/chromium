function suspendMediaElement(element, callback) {
  var pollSuspendState = function() {
    if (!window.internals.isMediaElementSuspended(element)) {
      window.requestAnimationFrame(pollSuspendState);
      return;
    }

    callback();
  };

  window.requestAnimationFrame(pollSuspendState);
  window.internals.forceStaleStateForMediaElement(element);
}
