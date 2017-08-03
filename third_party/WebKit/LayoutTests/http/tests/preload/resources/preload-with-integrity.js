// Call testRunner.notifyDone after a 5s delay, because the sanity checks
// for preloads run shortly after the page has loaded.
if (window.testRunner) {
  setTimeout(function() { testRunner.notifyDone(); }, 5000);
}

console.log("preload-with-integrity.js loaded.");
