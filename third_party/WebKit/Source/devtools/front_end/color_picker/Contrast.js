// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

ColorPicker.ContrastInfo = class {
  constructor() {
    /** @type {!Array<number>} */
    this.hsva = [0, 0, 0, 0];

    /** @type {!Common.Color} */
    this.bgColor = Common.Color.fromRGBA([0, 0, 0, 0]);

    /** @type {?number} */
    this.contrastRatio = null;

    /** @type {?Object<string, number>} */
    this.contrastRatioThresholds = null;

    /** @type {string} */
    this._colorString = "";
  }

  /**
   * @param {!Array<number>} hsva
   */
  setHSVA(hsva) {
    this.hsva = hsva;

    var fgRGBA = [];
    Common.Color.hsva2rgba(hsva, fgRGBA);
    var bgRGBA = this.bgColor.rgba();
    this.contrastRatio = Common.Color.calculateContrastRatio(fgRGBA, bgRGBA);
  }

  /**
   * @param {string} colorString
   */
  setColorString(colorString) {
		this._colorString = colorString;
  }

	/**
   * @return {string}
   */
	colorString() {
		return this._colorString;
	}

  /**
   * @param {?SDK.CSSModel.ContrastInfo} contrastInfo
   */
  setContrastInfo(contrastInfo) {
    this.contrastRatio = null;
    this.contrastRatioThresholds = null;

    // TODO(aboxhall): distinguish between !backgroundColors (no text) and
    // !backgroundColors.length (no computed bg color)
    if (!contrastInfo.backgroundColors || !contrastInfo.backgroundColors.length)
      return;

    if (contrastInfo.computedFontSize && contrastInfo.computedFontWeight && contrastInfo.computedBodyFontSize) {
      var isLargeFont = ColorPicker.ContrastInfo.computeIsLargeFont(
          contrastInfo.computedFontSize, contrastInfo.computedFontWeight, contrastInfo.computedBodyFontSize);

      this.contrastRatioThresholds = ColorPicker.ContrastInfo.contrastThresholds(isLargeFont);
    }

    // TODO(aboxhall): figure out what to do in the case of multiple background colors (i.e. gradients)
    var bgColorText = contrastInfo.backgroundColors[0];
    var bgColor = Common.Color.parse(bgColorText);
    if (!bgColor)
      return;

    this.setBgColor(bgColor);
  }

  /**
   * @param {!Common.Color} bgColor
   */
  setBgColor(bgColor) {
    this.bgColor = bgColor;

    if (!this.hsva)
      return;

    var fgRGBA = [];
    Common.Color.hsva2rgba(this.hsva, fgRGBA);

    // If we have a semi-transparent background color over an unknown
    // background, draw the line for the "worst case" scenario: where
    // the unknown background is the same color as the text.
    if (bgColor.hasAlpha) {
      var blendedRGBA = [];
      Common.Color.blendColors(bgColor.rgba(), fgRGBA, blendedRGBA);
      this.bgColor = new Common.Color(blendedRGBA, Common.Color.Format.RGBA);
    }

    var bgRGBA = this.bgColor.rgba();
    this.contrastRatio = Common.Color.calculateContrastRatio(fgRGBA, bgRGBA);

    //    this._updateUI();
    //    this._bgColorSwatch.setColor(this._bgColor);
  }

  /**
   * @param {boolean} isLargeFont
   * @return {!Object<string, number>}
   */
  static contrastThresholds(isLargeFont) {
    const thresholds = {largeFont: {AA: 3.0, AAA: 4.5}, normalFont: {AA: 4.5, AAA: 7.0}};
    return thresholds[isLargeFont ? 'largeFont' : 'normalFont'];
  }

  /**
   * @param {string} fontSize
   * @param {string} fontWeight
   * @param {?string} bodyFontSize
   * @return {boolean}
   */
  static computeIsLargeFont(fontSize, fontWeight, bodyFontSize) {
    const boldWeights = ['bold', 'bolder', '600', '700', '800', '900'];

    var fontSizePx = parseFloat(fontSize.replace('px', ''));
    var isBold = (boldWeights.indexOf(fontWeight) !== -1);

    if (bodyFontSize) {
      var bodyFontSizePx = parseFloat(bodyFontSize.replace('px', ''));
      if (isBold) {
        if (fontSizePx >= (bodyFontSizePx * 1.2))
          return true;
      } else if (fontSizePx >= (bodyFontSizePx * 1.5)) {
        return true;
      }
      return false;
    }

    var fontSizePt = Math.ceil(fontSizePx * 72 / 96);
    if (isBold)
      return fontSizePt >= 14;
    else
      return fontSizePt >= 18;
  }
};

ColorPicker.ContrastOverlay = class {
  /**
   * @param {!Element} colorElement
   * @param {!Element} contentElement
   * @param {function(boolean=, !Common.Event=)} toggleMainColorPickerCallback
   */
		constructor(colorElement, contentElement, toggleMainColorPickerCallback) {
		/** @type {!ColorPicker.ContrastInfo} */
    this._contrastInfo = new ColorPicker.ContrastInfo();

    var contrastRatioSVG = colorElement.createSVGChild('svg', 'spectrum-contrast-container fill');
    this._contrastRatioLine = contrastRatioSVG.createSVGChild('path', 'spectrum-contrast-line');

    this._contrastValueBubble = colorElement.createChild('div', 'spectrum-contrast-info');
    this._contrastValueBubble.classList.add('force-white-icons');
    this._contrastValueBubble.createChild('span', 'low-contrast').textContent = Common.UIString('Low contrast');
    this._contrastValue = this._contrastValueBubble.createChild('span', 'value');
    this._contrastValueBubble.appendChild(UI.Icon.create('smallicon-contrast-ratio'));
    this._contrastValueBubble.title = Common.UIString('Click to toggle contrast ratio details');
    this._contrastDetails = new ColorPicker.ContrastDetails(this._contrastInfo, contentElement, toggleMainColorPickerCallback);

    this._contrastValueBubble.addEventListener('mousedown', this._toggleContrastDetails.bind(this), true);
  }

  /**
   * @param {?SDK.CSSModel.ContrastInfo} contrastInfo
   */
  setContrastInfo(contrastInfo) {
    this._contrastInfo.setContrastInfo(contrastInfo);
	}

  /**
   * @param {!Array<number>} hsva
   */
  setHSVA(hsva) {
    this._contrastInfo.setHSVA(hsva);
  }

  /**
   * @param {string} colorString
   */
	setColorString(colorString) {
    this._contrastInfo.setColorString(colorString);
  }


	/**
	 * @param {!Event} event
	 */
  moveAwayFrom(event) {
		var bubble = this._contrastValueBubble;
    if (!bubble.boxInWindow().contains(event.x, event.y))
      return;

		if (bubble.offsetWidth > ((bubble.offsetParent.offsetWidth / 2) - 10))
      bubble.classList.toggle('contrast-info-top');
		else
      bubble.classList.toggle('contrast-info-left');
	}


  /**
   * @param {number} width
   * @param {number} height
   * @param {!AnchorBox} draggerBox
   */
  show(width, height, draggerBox) {
    var bubble = this._contrastValueBubble;
    bubble.classList.remove('hidden');

    this._contrastValue.textContent = this._contrastInfo.contrastRatio.toFixed(2);

    var AA = this._contrastInfo.contrastRatioThresholds['AA'];
    this._drawContrastRatioLine(AA, width, height);

    var passesAA = this._contrastInfo.contrastRatio >= AA;
    bubble.classList.toggle('contrast-fail', !passesAA);

    var dragX = draggerBox.x + (draggerBox.width / 2);
    var dragY = draggerBox.y + (draggerBox.height / 2);
    var bubbleBox = bubble.boxInWindow();
    if (bubbleBox.contains(dragX, dragY) && bubble.offsetParent) {
      if (bubble.offsetWidth > ((bubble.offsetParent.offsetWidth / 2) - 10))
        bubble.classList.toggle('contrast-info-top');
      else
        bubble.classList.toggle('contrast-info-left');
    }
  }

  hide() {
    this._contrastValueBubble.classList.add('hidden');
  }

  /**
   * @param {!Event} event
   */
  _toggleContrastDetails(event) {
    if ('button' in event && event.button !== 0)
      return;
    event.consume();
    this._contrastDetails.toggleVisible();
  }

  _hideContrastDetails() {
    this._contrastDetails.hide();
  }

  /**
   * @param {number} requiredContrast
   * @param {number} width
   * @param {number} height
   */
    _drawContrastRatioLine(requiredContrast, width, height) {
    // TODO(aboxhall): throttle this to avoid being called in rapid succession when using eyedropper

    if (!this._contrastInfo.bgColor || !width || !height)
      return;

    const dS = 0.02;
    const epsilon = 0.002;
    const H = 0;
    const S = 1;
    const V = 2;
    const A = 3;

    var fgRGBA = [];
    var hsva = this._contrastInfo.hsva;
    Common.Color.hsva2rgba(hsva, fgRGBA);
    var bgRGBA = this._contrastInfo.bgColor.rgba();
    var bgLuminance = Common.Color.luminance(bgRGBA);
    var blendedRGBA = [];
    Common.Color.blendColors(fgRGBA, bgRGBA, blendedRGBA);
    var fgLuminance = Common.Color.luminance(blendedRGBA);
    this._contrastValueBubble.classList.toggle('light', fgLuminance > 0.5);
    var fgIsLighter = fgLuminance > bgLuminance;
    var desiredLuminance = Common.Color.desiredLuminance(bgLuminance, requiredContrast, fgIsLighter);

    var lastV = hsva[V];
    var currentSlope = 0;
    var candidateHSVA = [hsva[H], 0, 0, hsva[A]];
    var pathBuilder = [];
    var candidateRGBA = [];
    Common.Color.hsva2rgba(candidateHSVA, candidateRGBA);
    Common.Color.blendColors(candidateRGBA, bgRGBA, blendedRGBA);

    /**
     * Approach the desired contrast ratio by modifying the given component
     * from the given starting value.
     * @param {number} index
     * @param {number} x
     * @param {boolean} onAxis
     * @return {?number}
     */
    function approach(index, x, onAxis) {
      while (0 <= x && x <= 1) {
        candidateHSVA[index] = x;
        Common.Color.hsva2rgba(candidateHSVA, candidateRGBA);
        Common.Color.blendColors(candidateRGBA, bgRGBA, blendedRGBA);
        var fgLuminance = Common.Color.luminance(blendedRGBA);
        var dLuminance = fgLuminance - desiredLuminance;

        if (Math.abs(dLuminance) < (onAxis ? epsilon / 10 : epsilon))
          return x;
        else
          x += (index === V ? -dLuminance : dLuminance);
      }
      return null;
    }

    for (var s = 0; s < 1 + dS; s += dS) {
      s = Math.min(1, s);
      candidateHSVA[S] = s;

      var v = lastV;
      v = lastV + currentSlope * dS;

      v = approach(V, v, s === 0);
      if (v === null)
        break;

      currentSlope = (v - lastV) / dS;

      pathBuilder.push(pathBuilder.length ? 'L' : 'M');
      pathBuilder.push(s * width);
      pathBuilder.push((1 - v) * height);
    }

    if (s < 1 + dS) {
      s -= dS;
      candidateHSVA[V] = 1;
      s = approach(S, s, true);
      if (s !== null)
        pathBuilder = pathBuilder.concat(['L', s * width, -1]);
    }

    this._contrastRatioLine.setAttribute('d', pathBuilder.join(' '));
  }
}

ColorPicker.ContrastDetails = class {
  /**
   * @param {!ColorPicker.ContrastInfo} contrastInfo
   * @param {!Element} contentElement
   * @param {function(boolean=, !Common.Event=)} toggleMainColorPickerCallback
   */
		constructor(contrastInfo, contentElement, toggleMainColorPickerCallback) {
		/** @type {!ColorPicker.ContrastInfo} */
    this._contrastInfo = contrastInfo;

		/** @type {function(boolean=, !Common.Event=)} */
		this._toggleMainColorPicker = toggleMainColorPickerCallback;

    this._contrastDetails = contentElement.createChild('div', 'spectrum-contrast-details');
    var contrastValueRow = this._contrastDetails.createChild('div');
    contrastValueRow.createTextChild(Common.UIString('Contrast Ratio'));

    this._contrastValueBubble = contrastValueRow.createChild('span', 'contrast-details-value force-white-icons');
    this._contrastValue = this._contrastValueBubble.createChild('span');
    this._contrastValueBubble.appendChild(UI.Icon.create('smallicon-checkmark-square'));
    this._contrastValueBubble.appendChild(UI.Icon.create('smallicon-checkmark-behind'));
    this._contrastValueBubble.appendChild(UI.Icon.create('smallicon-no'));
    this._contrastValueBubble.addEventListener('mouseenter', this._toggleContrastValueHovered.bind(this));
    this._contrastValueBubble.addEventListener('mouseleave', this._toggleContrastValueHovered.bind(this));

    var toolbar = new UI.Toolbar('', contrastValueRow);
    var closeButton = new UI.ToolbarButton('Hide contrast ratio details', 'largeicon-delete');
    closeButton.addEventListener(UI.ToolbarButton.Events.Click, this.hide.bind(this));
    toolbar.appendToolbarItem(closeButton);

    this._contrastThresholds = this._contrastDetails.createChild('div', 'contrast-thresholds');
    this._contrastAA = this._contrastThresholds.createChild('div', 'contrast-threshold');
    this._contrastAA.appendChild(UI.Icon.create('smallicon-checkmark-square'));
    this._contrastAA.appendChild(UI.Icon.create('smallicon-no'));
    this._contrastPassFailAA = this._contrastAA.createChild('span', 'contrast-pass-fail');
    var aaLink = this._contrastAA.appendChild(UI.createExternalLink(
        'https://www.w3.org/TR/UNDERSTANDING-WCAG20/visual-audio-contrast-contrast.html', 'AA', 'wcag-link'));
    aaLink.textContent = '';
    aaLink.appendChild(UI.Icon.create('mediumicon-info'));

    this._contrastAAA = this._contrastThresholds.createChild('div', 'contrast-threshold');
    this._contrastAAA.appendChild(UI.Icon.create('smallicon-checkmark-square'));
    this._contrastAAA.appendChild(UI.Icon.create('smallicon-no'));
    this._contrastPassFailAAA = this._contrastAAA.createChild('span', 'contrast-pass-fail');
    var aaaLink = this._contrastAAA.appendChild(UI.createExternalLink(
        'https://www.w3.org/TR/UNDERSTANDING-WCAG20/visual-audio-contrast7.html', 'AAA', 'wcag-link'));
    aaaLink.textContent = '';
    aaaLink.appendChild(UI.Icon.create('mediumicon-info'));

    var bgColorRow = this._contrastDetails.createChild('div');
    bgColorRow.createTextChild(Common.UIString('Background color:'));
    this._bgColorSwatch = new ColorPicker.Spectrum.Swatch(bgColorRow, 'contrast');

    this._bgColorPicker = bgColorRow.createChild('button', 'background-color-picker');
    this._bgColorPicker.appendChild(UI.Icon.create('largeicon-eyedropper'));
    this._bgColorPicker.addEventListener('click', this._toggleBackgroundColorPicker.bind(this, undefined));
    this._bgColorPickedBound = this._bgColorPicked.bind(this);
  }

  update() {
    this._contrastValue.textContent = this._contrastInfo.contrastRatio.toFixed(2);

    var AA = this._contrastInfo.contrastRatioThresholds['AA'];
    var AAA = this._contrastInfo.contrastRatioThresholds['AAA'];

    var passesAA = this._contrastInfo.contrastRatio >= AA;
    this._contrastPassFailAA.innerHTML = passesAA ? Common.UIString('Passes <strong>AA (%s)</strong>', AA.toFixed(1)) :
                                                    Common.UIString('Fails <strong>AA (%s)</strong>', AA.toFixed(1));
    this._contrastAA.classList.toggle('pass', passesAA);
    this._contrastAA.classList.toggle('fail', !passesAA);
    var passesAAA = this._contrastInfo.contrastRatio >= AAA;
    this._contrastPassFailAAA.innerHTML = passesAAA ?
        Common.UIString('Passes <strong>AAA (%s)</strong>', AAA.toFixed(1)) :
        Common.UIString('Fails <strong>AAA (%s)</strong>', AAA.toFixed(1));
    this._contrastAAA.classList.toggle('pass', passesAAA);
    this._contrastAAA.classList.toggle('fail', !passesAAA);

    this._contrastValueBubble.classList.toggle('contrast-fail', !passesAA);
    this._contrastValueBubble.classList.toggle('contrast-aa', passesAA);
    this._contrastValueBubble.classList.toggle('contrast-aaa', passesAAA);
    this._contrastValueBubble.style.color = this._contrastInfo.colorString();
    var icons = this._contrastValueBubble.querySelectorAll('[is=ui-icon]');
    for (var i = 0; i < icons.length; i++)
      icons[i].style.setProperty('background', this._contrastInfo.colorString(), 'important');

    var isWhite = (this._contrastInfo.bgColor.hsla()[2] > 0.9);
    this._contrastValueBubble.style.background = /** @type {string} */ (this._contrastInfo.bgColor.asString(Common.Color.Format.RGBA));
    this._contrastValueBubble.classList.toggle('contrast-color-white', isWhite);

    if (isWhite)
      this._contrastValueBubble.style.removeProperty('border-color');
    else
				this._contrastValueBubble.style.borderColor = /** @type {string} */ (this._contrastInfo.bgColor.asString(Common.Color.Format.RGBA));
  }

  toggleVisible() {
    this._contrastDetails.classList.toggle('visible');
    if (this._contrastDetails.classList.contains('visible'))
      this._toggleMainColorPicker(false);
    else
      this._toggleBackgroundColorPicker(false);
  }

  hide() {
    this._contrastDetails.classList.remove('visible');
    this._toggleMainColorPicker(false);
  }

  /**
   * @param {boolean=} enabled
   */
  _toggleBackgroundColorPicker(enabled) {
    if (enabled === undefined) {
      this._bgColorPicker.classList.toggle('active');
      enabled = this._bgColorPicker.classList.contains('active');
    } else {
      this._bgColorPicker.classList.toggle('active', enabled);
    }
    UI.ARIAUtils.setPressed(this._bgColorPicker, enabled);

    InspectorFrontendHost.setEyeDropperActive(enabled);
    if (enabled) {
      InspectorFrontendHost.events.addEventListener(
          InspectorFrontendHostAPI.Events.EyeDropperPickedColor, this._bgColorPickedBound);
    } else {
      InspectorFrontendHost.events.removeEventListener(
          InspectorFrontendHostAPI.Events.EyeDropperPickedColor, this._bgColorPickedBound);
    }
  }

  /**
   * @param {!Common.Event} event
   */
  _bgColorPicked(event) {
    var rgbColor = /** @type {!{r: number, g: number, b: number, a: number}} */ (event.data);
    var rgba = [rgbColor.r, rgbColor.g, rgbColor.b, (rgbColor.a / 2.55 | 0) / 100];
    var color = Common.Color.fromRGBA(rgba);
    this._contrastInfo.setBgColor(color);
		this.update()
    InspectorFrontendHost.bringToFront();
  }

  /**
   * @param {!Event} event
   */
  _toggleContrastValueHovered(event) {
    if (!this._contrastValueBubble.classList.contains('contrast-fail'))
      return;
    if (event.type === 'mouseenter') {
      this._contrastValueBubble.style.color = '#333';
      this._contrastValueBubble.style.background = 'white';
      var icons = this._contrastValueBubble.querySelectorAll('[is=ui-icon]');
      for (var i = 0; i < icons.length; i++)
        icons[i].style.setProperty('background', 'black', 'important');
    } else {
      this._contrastValueBubble.style.color = this._contrastInfo.colorString();
      this._contrastValueBubble.style.background = /** @type {string} */ (this._contrastInfo.bgColor.asString(Common.Color.Format.RGBA));
      var icons = this._contrastValueBubble.querySelectorAll('[is=ui-icon]');
      for (var i = 0; i < icons.length; i++)
        icons[i].style.setProperty('background', this._contrastInfo.colorString(), 'important');
    }
  }
}
