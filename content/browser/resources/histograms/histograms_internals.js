

function requestHistograms() {
  chrome.send('requestHistograms');
}


/**
 * Callback from backend with the list of crashes. Builds the UI.
 * @param {boolean} enabled Whether or not crash reporting is enabled.
 * @param {boolean} dynamicBackend Whether the crash backend is dynamic.
 * @param {boolean} manualUploads Whether the manual uploads are supported.
 * @param {array} crashes The list of crashes.
 * @param {string} version The browser version.
 * @param {string} os The OS name and version.
 */
function onHistogramsReceived(histograms) {
  console.log(histograms);
}
