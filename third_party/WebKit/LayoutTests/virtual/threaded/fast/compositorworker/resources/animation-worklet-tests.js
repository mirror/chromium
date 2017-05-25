function runInAnimationWorklet(code) {
  return window.animationWorklet.addModule(
    URL.createObjectURL(new Blob([code], {type: 'text/javascript'}))
  );
}

// Load test cases in worklet context in sequence and wait until they are resolved.
function runTests(testcases) {
  if (window.testRunner) {
    testRunner.waitUntilDone();
    testRunner.dumpAsText();
  }

  const runInSequence = testcases.reduce((chain, testcase) => {
    return chain.then( _ => {
        return runInAnimationWorklet(testcase);
    });
  }, Promise.resolve());

  // TODO(majidvp): Figure out how to wait on AnimationWorklet, which runs on a separate thread,
  // to complete its tasks instead of a timeout.
  runInSequence.then(_ => {
    setTimeout(_ => {
      if (window.testRunner)
        testRunner.notifyDone();
     }, 100);
  });
}

const testcases = Array.prototype.map.call(document.querySelectorAll('script[type$=worklet]'), ($el) => {
  return $el.textContent;
});

runTests(testcases);
