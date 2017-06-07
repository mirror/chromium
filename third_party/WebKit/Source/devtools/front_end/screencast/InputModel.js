// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Screencast.InputModel = class extends SDK.SDKModel {
  /**
   * @param {!SDK.Target} target
   */
  constructor(target) {
    super(target);
    this._inputAgent = target.inputAgent();
    /** @type {?number} */
    this._activeTouchOffsetTop = null;
    this._activeTouchParams = null;
  }

  /**
   * @param {!Event} event
   */
  emitKeyEvent(event) {
    var type;
    switch (event.type) {
      case 'keydown':
        type = 'keyDown';
        break;
      case 'keyup':
        type = 'keyUp';
        break;
      case 'keypress':
        type = 'char';
        break;
      default:
        return;
    }

    var text = event.type === 'keypress' ? String.fromCharCode(event.charCode) : undefined;
    this._inputAgent.invoke_dispatchKeyEvent(
        type, this._modifiersForEvent(event), event.timeStamp / 1000, text, text ? text.toLowerCase() : undefined,
        event.keyIdentifier, event.code, event.key,
        /* windowsVirtualKeyCode */ event.keyCode,
        /* nativeVirtualKeyCode */ event.keyCode,
        /* autoRepeat */ false,
        /* isKeypad */ false,
        /* isSystemKey */ false);
  }

  /**
   * @param {!Event} event
   * @param {number} offsetTop
   * @param {number} zoom
   */
  emitTouchFromMouseEvent(event, offsetTop, zoom) {
    var buttons = {0: 'none', 1: 'left', 2: 'middle', 3: 'right'};
    var types = {
      'mousedown': 'mousePressed',
      'mouseup': 'mouseReleased',
      'mousemove': 'mouseMoved',
      'mousewheel': 'mouseWheel'
    };
    if (!(event.type in types) || !(event.which in buttons))
      return;
    if (event.type !== 'mousewheel' && buttons[event.which] === 'none')
      return;

    if (event.type === 'mousedown' || this._activeTouchOffsetTop === null)
      this._activeTouchOffsetTop = offsetTop;

    var x = Math.round(event.offsetX / zoom);
    var y = Math.round(event.offsetY / zoom);
    y = Math.round(y - this._activeTouchOffsetTop);
    var params = {
      type: types[event.type],
      x: x,
      y: y,
      modifiers: this._modifiersForEvent(event),
      timestamp: event.timeStamp / 1000,
      button: buttons[event.which],
      clickCount: 0
    };
    if (event.type === 'mousewheel') {
      params.deltaX = event.wheelDeltaX / zoom;
      params.deltaY = event.wheelDeltaY / zoom;
    } else {
      this._activeTouchParams = params;
    }
    if (event.type === 'mouseup')
      this._activeTouchOffsetTop = null;
    this._inputAgent.invoke_emulateTouchFromMouseEvent(
        params.type, params.x, params.y, params.timestamp, params.button, params.deltaX, params.deltaY,
        params.modifiers, params.clickCount);
  }

  cancelTouch() {
    if (this._activeTouchOffsetTop === null)
      return;
    var params = this._activeTouchParams;
    this._activeTouchParams = null;
    params.type = 'mouseReleased';
    this._inputAgent.invoke_emulateTouchFromMouseEvent(
        params.type, params.x, params.y, params.timestamp, params.button, params.deltaX, params.deltaY,
        params.modifiers, params.clickCount);
  }

  /**
   * @param {!Event} event
   * @return {number}
   */
  _modifiersForEvent(event) {
    return (event.altKey ? 1 : 0) | (event.ctrlKey ? 2 : 0) | (event.metaKey ? 4 : 0) | (event.shiftKey ? 8 : 0);
  }
};

SDK.SDKModel.register(Screencast.InputModel, SDK.Target.Capability.Input, false);
