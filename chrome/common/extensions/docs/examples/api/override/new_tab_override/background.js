chrome.browserAction.onClicked.addListener(function() {
  chrome.tabs.create({
    url: chrome.extension.getURL("tab_override.html")
  })
})
