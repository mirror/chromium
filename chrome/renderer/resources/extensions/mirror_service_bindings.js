// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var mirrorService;

define('mirror_service_bindings', [
    'content/public/renderer/frame_interfaces',
    'media/mojo/interfaces/mirror_service.mojom'
], function(frameInterfaces,
            mirrorServiceMojom) {
  'use strict';

  /**
   * Creates a new MirrorService.
   * @param {!mirrorServiceMojom.MirrorServiceProxyPtr} service
   * @constructor
   */
  function MirrorService(service) {
    /**
     * The Mojo service proxy. Allows extension code to connect to the mirror
     * service.
     * @type {!mirrorServiceMojom.MirrorServiceProxyPtr}
     */
    this.service_ = service;
  }

  /**
   * Returns definitions of Mojo core and generated Mojom classes that can be
   * used directly by the component.
   * @return {!Object}
   */
  MirrorService.prototype.getMojoExports = function() {
    return {
      MirrorServiceProxyPtr: mirrorServiceMojom.MirrorServiceProxyPtr,
      MirrorClient: mirrorServiceMojom.MirrorClient,
      MirrorClientPtr: mirrorServiceMojom.MirrorClientPtr,
      SenderConfig: mirrorServiceMojom.SenderConfig,
      CaptureType: mirrorServiceMojom.CaptureType,
      CaptureParam: mirrorServiceMojom.CaptureParam,
      UdpOptions: mirrorServiceMojom.UdpOptions,
      StreamingParams: mirrorServiceMojom.StreamingParams,
      MirrorError: mirrorServiceMojom.MirrorError,
    };
  };

  MirrorService.prototype.getSupportedSenderConfigs = function(
      hasAudio, hasVideo) {
    return this.service_.getSupportedSenderConfigs(hasAudio, hasVideo);
  };

  MirrorService.prototype.start = function(capture_param, udp_param,
      audio_config, video_config, streaming_param, client) {
    return this.service_.start(capture_param, udp_param, audio_config,
                               video_config, streaming_param, client);
  };

  MirrorService.prototype.stop = function() {
    this.service_.stop();
  };

  MirrorService.prototype.shutdown = function() {
    this.service_.shutdown();
  }

  mirrorService = new MirrorService(new mirrorServiceMojom.MirrorServiceProxyPtr(
      frameInterfaces.getInterface(mirrorServiceMojom.MirrorServiceProxy.name)));

  return mirrorService;
});
