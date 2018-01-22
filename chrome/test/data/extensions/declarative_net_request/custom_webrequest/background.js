var blockFunction = function(info) {
  if (info.type == "image" || info.type == "stylesheet" || info.url.indexOf("google") != -1)
    return {cancel:true};
};
var extraInfoSpec = ["blocking"];
var filter = {
  urls : ["<all_urls>"]
};

chrome.webRequest.onBeforeRequest.addListener(blockFunction, filter, extraInfoSpec);
