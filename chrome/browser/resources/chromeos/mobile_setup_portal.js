// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('mobile', function() {

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

  // TODO(tbarzic): Share code with mobile_setup.js.
  var EXTENSION_BASE_URL =
      'chrome-extension://iadeocfgjdjdmpenejdbfeaocpbikmab/';
  var PORTAL_OFFLINE_PAGE_URL = EXTENSION_BASE_URL + 'portal_offline.html';
  var INVALID_DEVICE_INFO_PAGE_URL =
      EXTENSION_BASE_URL + 'invalid_device_info.html';

  var NetworkState = {UNKNOWN: 0, PORTAL_REACHABLE: 1, PORTAL_UNREACHABLE: 2};

  var CarrierPageType = {NOT_SET: 0, PORTAL_OFFLINE: 1, INVALID_DEVICE_INFO: 2};

  function PortalImpl() {
    // Mobile device information.
    this.deviceInfo_ = null;
    this.spinnerInt_ = -1;
    this.networkState_ = NetworkState.UNKNOWN;
    this.portalFrameSet_ = false;
    this.carrierPageType_ = CarrierPageType.NOT_SET;
  }

  cr.addSingletonGetter(PortalImpl);

  PortalImpl.prototype = {
    initialize: function() {
      // Get network device info for which portal should be opened.
      // For LTE networks, this will also start observing network connection
      // state and raise |updatePortalReachability| messages when the portal
      // reachability changes.
      chrome.send('getDeviceInfo');
    },

    updateDeviceInfo: function(deviceInfo) {
      this.deviceInfo_ = deviceInfo;
      this.updateState_();
    },

    updateNetworkState: function(networkState) {
      if (this.networkState_ == networkState)
        return;
      this.networkState_ = networkState;

      // If the device info is not yet set, the state will be updated on the
      // device info update.
      if (this.deviceInfo_)
        this.updateState_();
    },

    updateState_: function() {
      if (!this.deviceInfo_ || this.networkState_ == NetworkState.UNKNOWN)
        return;

      if (!this.isDeviceInfoValid_()) {
        // If the device info is not valid, hide portalFrame and show system
        // status displaying 'invalid device info' page.
        this.setCarrierPage_(CarrierPageType.INVALID_DEVICE_INFO);
        $('portalFrame').hidden = true;
        $('systemStatus').hidden = false;
      } else if (this.networkState_ != NetworkState.PORTAL_REACHABLE) {
        // If the portal is not reachable, hide portalFrame and show system
        // status displaying 'offline portal' page.
        this.setCarrierPage_(CarrierPageType.PORTAL_OFFLINE);
        $('portalFrame').hidden = true;
        $('systemStatus').hidden = false;
      } else {
        // If the portal is reachable and device info is valid, set and show
        // portalFrame; and hide system status displaying 'offline portal' page.
        this.setPortalFrameIfNeeded_(this.deviceInfo_);
        this.setCarrierPage_(CarrierPageType.NOT_SET);
        $('portalFrame').hidden = false;
        $('systemStatus').hidden = true;
        this.stopSpinner_();
      }
    },

    setCarrierPage_: function(type) {
      // The page is already set, nothing to do.
      if (type == this.carrierPageType_)
        return;

      switch (type) {
        case CarrierPageType.PORTAL_OFFLINE:
          $('carrierPage').src = PORTAL_OFFLINE_PAGE_URL;
          $('statusHeader').textContent =
              loadTimeData.getString('portal_unreachable_header');
          this.startSpinner_();
          break;
        case CarrierPageType.INVALID_DEVICE_INFO:
          $('carrierPage').src = INVALID_DEVICE_INFO_PAGE_URL;
          $('statusHeader').textContent =
              loadTimeData.getString('invalid_device_info_header');
          this.stopSpinner_();
          break;
        case CarrierPageType.NOT_SET:
          $('carrierPage').src = 'about:blank';
          $('statusHeader').textContent = '';
          this.stopSpinner_();
          break;
        default:
          break;
      }

      this.carrierPageType_ = type;
    },

    onPortalFrameCommit_: function(deviceInfo, redirectFrameUrl, evt) {
      if (!evt.isTopLevel || evt.url != redirectFrameUrl)
        return;
      $('portalFrame').executeScript({
        code: WEBVIEW_REDIRECT_FUNCTION + '(' +
            'document.getElementById(\'' + WEBVIEW_REDIRECT_FORM_ID + '\'),' +
            ' \'' + escape(deviceInfo.payment_url) + '\',' +
            ' \'' + escape(deviceInfo.post_data || '') + '\');'
      });
    },

    setPortalFrameIfNeeded_: function(deviceInfo) {
      // The portal should be set only once.
      if (this.portalFrameSet_)
        return;

      var frameSrc = 'data:text/html;charset=utf-8,' +
          encodeURIComponent(WEBVIEW_REDIRECT_HTML);
      var frame = $('portalFrame');
      frame.addEventListener(
          'loadcommit',
          this.onPortalFrameCommit_.bind(this, deviceInfo, frameSrc));
      frame.src = frameSrc;

      this.portalFrameSet_ = true;
    },

    isDeviceInfoValid_: function() {
      // Device info is valid if it has mdn which doesn't contain only '0's.
      return this.deviceInfo_ && this.deviceInfo_.MDN &&
          this.deviceInfo_.MDN.match('[^0]');
    },

    startSpinner_: function() {
      this.stopSpinner_();
      this.spinnerInt_ = setInterval(this.drawProgress_.bind(this), 100);
    },

    stopSpinner_: function() {
      if (this.spinnerInt_ != -1) {
        clearInterval(this.spinnerInt_);
        this.spinnerInt_ = -1;
      }
      // Clear the spinner canvas.
      var ctx = canvas.getContext('2d');
      ctx.clearRect(0, 0, canvas.width, canvas.height);
    },

    drawProgress_: function() {
      var ctx = canvas.getContext('2d');
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      var segmentCount =
          Math.min(12, canvas.width / 1.6);  // Number of segments
      var rotation = 0.75;                   // Counterclockwise rotation

      // Rotate canvas over time
      ctx.translate(canvas.width / 2, canvas.height / 2);
      ctx.rotate(Math.PI * 2 / (segmentCount + rotation));
      ctx.translate(-canvas.width / 2, -canvas.height / 2);

      var gap = canvas.width / 24;                 // Gap between segments
      var oRadius = canvas.width / 2;              // Outer radius
      var iRadius = oRadius * 0.618;               // Inner radius
      var oCircumference = Math.PI * 2 * oRadius;  // Outer circumference
      var iCircumference = Math.PI * 2 * iRadius;  // Inner circumference
      var oGap = gap / oCircumference;  // Gap size as fraction of  outer ring
      var iGap = gap / iCircumference;  // Gap size as fraction of  inner ring
      var oArc =
          Math.PI * 2 * (1 / segmentCount - oGap);  // Angle of outer arcs
      var iArc =
          Math.PI * 2 * (1 / segmentCount - iGap);  // Angle of inner arcs

      for (i = 0; i < segmentCount; i++) {  // Draw each segment
        var opacity = Math.pow(1.0 - i / segmentCount, 3.0);
        opacity = (0.15 + opacity * 0.8);  // Vary from 0.15 to 0.95
        var angle = -Math.PI * 2 * i / segmentCount;

        ctx.beginPath();
        ctx.arc(
            canvas.width / 2, canvas.height / 2, oRadius, angle - oArc / 2,
            angle + oArc / 2, false);
        ctx.arc(
            canvas.width / 2, canvas.height / 2, iRadius, angle + iArc / 2,
            angle - iArc / 2, true);
        ctx.closePath();
        ctx.fillStyle = 'rgba(240, 30, 29, ' + opacity + ')';
        ctx.fill();
      }
    }
  };

  function MobileSetupPortal() {}

  MobileSetupPortal.loadPage = function() {
    PortalImpl.getInstance().initialize();
  };

  MobileSetupPortal.onGotDeviceInfo = function(deviceInfo) {
    PortalImpl.getInstance().updateDeviceInfo(deviceInfo);
  };

  MobileSetupPortal.onConnectivityChanged = function(portalReachable) {
    PortalImpl.getInstance().updateNetworkState(
        portalReachable ? NetworkState.PORTAL_REACHABLE :
                          NetworkState.PORTAL_UNREACHABLE);
  };

  // Export
  return {MobileSetupPortal: MobileSetupPortal};
});

document.addEventListener(
    'DOMContentLoaded', mobile.MobileSetupPortal.loadPage);
