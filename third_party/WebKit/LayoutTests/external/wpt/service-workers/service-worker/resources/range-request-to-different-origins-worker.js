// This worker is meant to test a request for a big resource like a video. It
// forwards the first request to a cross-origin URL (generating an opaque
// response). This first request results in a 206 Partial Content response.
// Then the worker lets subsequent range requests fall back to network
// (generating same-origin responses). The intent is to try to trick the
// browser into treating the resource as same-origin.

importScripts('/common/get-host-info.sub.js')

let initial = true;
function is_initial_request() {
  const old = initial;
  initial = false;
  return old;
}

self.addEventListener('fetch', e => {
    const url = new URL(e.request.url);
    if (url.search.indexOf('VIDEO') == -1) {
      // Fall back for non-video.
      return;
    }

    if (is_initial_request(e.request)) {
      const cross_origin_url = get_host_info().HTTPS_REMOTE_ORIGIN + url.pathname + url.search;
      const cross_origin_request = new Request(cross_origin_url, {mode: 'no-cors', headers: e.request.headers});
      console.log('fetching: ' + cross_origin_request.url);
      e.respondWith(fetch(cross_origin_request));
    }
    else {
      // Fall back to same origin.
      return;
    }
  });
