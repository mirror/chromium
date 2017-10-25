// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('mobile.util', function() {

  var WEBVIEW_REDIRECT_FUNCTION = '(function(form, paymentUrl, postData) {' +
      'function addInputElement(form, name, value) {' +
      '  var input = document.createElement(\'input\');' +
      '  input.type = \'hidden\';' +
      '  input.name = name;' +
      '  input.value = value;' +
      '  form.appendChild(input);' +
      '}' +
      'function initFormFromPostData(form, postData) {' +
      '  if (!postData) return;' +
      '  var pairs = postData.split(\'&\');' +
      '  pairs.forEach(pairStr => {' +
      '    var pair = pairStr.split(\'=\');' +
      '    if (pair.length == 2)' +
      '      addInputElement(form, pair[0], pair[1]);' +
      '    else if (pair.length == 1)' +
      '      addInputElement(form, pair[0], true);' +
      '  });' +
      '}' +
      'form.action = unescape(paymentUrl);' +
      'form.method = \'POST\';' +
      'initFormFromPostData(form, unescape(postData));' +
      'form.submit();' +
      '})';

  var WEBVIEW_REDIRECT_FORM_ID = 'redirectForm';

  var WEBVIEW_REDIRECT_HTML = '<html><body>' +
      '<form id="' + WEBVIEW_REDIRECT_FORM_ID + '"></form>' +
      '</body></html>';

  function initializeWebviewRedirectForm(
      webview, deviceInfo, webviewSrc, commitEvent) {
    if (!commitEvent.isTopLevel || commitEvent.url != webviewSrc)
      return;

    webview.executeScript({
      code: WEBVIEW_REDIRECT_FUNCTION + '(' +
          'document.getElementById(\'' + WEBVIEW_REDIRECT_FORM_ID + '\'),' +
          ' \'' + escape(deviceInfo.payment_url) + '\',' +
          ' \'' + escape(deviceInfo.post_data || '') + '\');'
    });
  }

  function postDeviceDataToWebview(webview, deviceInfo) {
    var webviewSrc = 'data:text/html;charset=utf-8,' +
        encodeURIComponent(WEBVIEW_REDIRECT_HTML);
    webview.addEventListener(
        'loadcommit',
        initializeWebviewRedirectForm.bind(
            this, webview, deviceInfo, webviewSrc));
    webview.src = webviewSrc;
  }

  return {postDeviceDataToWebview: postDeviceDataToWebview};
});
