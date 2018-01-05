// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var tests = [
  /**
   * Test named destinations.
   */
  function testParamsParser() {
    var paramsParser = new OpenPDFParamsParser(function(name) {
      if (name == 'RU') {
        paramsParser.onNamedDestinationReceived({pageNumber: 26});
      } else if (name == 'US') {
        paramsParser.onNamedDestinationReceived({pageNumber: 0});
      } else if (name == 'UY') {
        paramsParser.onNamedDestinationReceived({pageNumber: 22});
      } else if (name == 'DestWithFit') {
        paramsParser.onNamedDestinationReceived(
            {pageNumber: 10, viewType: 'Fit'});
      } else if (name == 'DestWithFitH') {
        paramsParser.onNamedDestinationReceived(
            {pageNumber: 10, viewType: 'FitH,700'});
      } else if (name == 'DestWithXYZ') {
        paramsParser.onNamedDestinationReceived(
            {pageNumber: 10, viewType: 'XYZ,111,222,333'});
      } else {
        paramsParser.onNamedDestinationReceived({pageNumber: -1});
      }
    });

    var url = "http://xyz.pdf";

    // Checking #nameddest.
    paramsParser.getViewportFromUrlParams(`${url}#RU`, function(params) {
      chrome.test.assertEq(26, params.page);
    });

    // Checking #nameddest=name.
    paramsParser.getViewportFromUrlParams(
        `${url}#nameddest=US`, function(params) {
          chrome.test.assertEq(0, params.page);
        });

    // Checking #page=pagenum nameddest.The document first page has a pagenum
    // value of 1.
    paramsParser.getViewportFromUrlParams(`${url}#page=6`, function(params) {
      chrome.test.assertEq(5, params.page);
    });

    // Checking #zoom=scale.
    paramsParser.getViewportFromUrlParams(`${url}#zoom=200`, function(params) {
      chrome.test.assertEq(2, params.zoom);
    });

    // Checking #zoom=scale,left,top.
    paramsParser.getViewportFromUrlParams(
        `${url}#zoom=200,100,200`, function(params) {
          chrome.test.assertEq(2, params.zoom);
          chrome.test.assertEq(100, params.position.x);
          chrome.test.assertEq(200, params.position.y);
        });

    // Checking #nameddest=name and zoom=scale.
    paramsParser.getViewportFromUrlParams(
        `${url}#nameddest=UY&zoom=150`, function(params) {
          chrome.test.assertEq(22, params.page);
          chrome.test.assertEq(1.5, params.zoom);
        });

    // Checking #page=pagenum and zoom=scale.
    paramsParser.getViewportFromUrlParams(
        `${url}#page=2&zoom=250`, function(params) {
          chrome.test.assertEq(1, params.page);
          chrome.test.assertEq(2.5, params.zoom);
        });

    // Checking #nameddest=name and zoom=scale,left,top.
    paramsParser.getViewportFromUrlParams(
        `${url}#nameddest=UY&zoom=150,100,200`, function(params) {
          chrome.test.assertEq(22, params.page);
          chrome.test.assertEq(1.5, params.zoom);
          chrome.test.assertEq(100, params.position.x);
          chrome.test.assertEq(200, params.position.y);
        });

    // Checking #nameddest=name with a nameddest that specifies the "view".
    paramsParser.getViewportFromUrlParams(
        `${url}#nameddest=DestWithFit`, function(params) {
          chrome.test.assertEq(10, params.page);
          chrome.test.assertEq(FittingType.FIT_TO_PAGE, params.view);
        });

    // Checking #nameddest=name with a nameddest that specifies the "view" with
    // a parameter.
    paramsParser.getViewportFromUrlParams(
        `${url}#nameddest=DestWithFitH`, function(params) {
          chrome.test.assertEq(10, params.page);
          chrome.test.assertEq(FittingType.FIT_TO_WIDTH, params.view);
          chrome.test.assertEq(700, params.viewPosition);
        });

    // Checking #nameddest=name with a nameddest that specifies the "view" with
    // multiple parameters.
    paramsParser.getViewportFromUrlParams(
        `${url}#nameddest=DestWithXYZ`, function(params) {
          chrome.test.assertEq(10, params.page);
          // TODO(hnakashima): Implement XYZ and update this.
          chrome.test.assertEq(undefined, params.view);
        });

    // Checking #page=pagenum and zoom=scale,left,top.
    paramsParser.getViewportFromUrlParams(
        `${url}#page=2&zoom=250,100,200`, function(params) {
          chrome.test.assertEq(1, params.page);
          chrome.test.assertEq(2.5, params.zoom);
          chrome.test.assertEq(100, params.position.x);
          chrome.test.assertEq(200, params.position.y);
        });

    // Checking #view=Fit.
    paramsParser.getViewportFromUrlParams(`${url}#view=Fit`, function(params) {
      chrome.test.assertEq(FittingType.FIT_TO_PAGE, params.view);
      chrome.test.assertEq(undefined, params.viewPosition);
    });
    // Checking #view=FitH.
    paramsParser.getViewportFromUrlParams(`${url}#view=FitH`, function(params) {
      chrome.test.assertEq(FittingType.FIT_TO_WIDTH, params.view);
      chrome.test.assertEq(undefined, params.viewPosition);
    });
    // Checking #view=FitH,[int position].
    paramsParser.getViewportFromUrlParams(
        `${url}#view=FitH,789`, function(params) {
          chrome.test.assertEq(FittingType.FIT_TO_WIDTH, params.view);
          chrome.test.assertEq(789, params.viewPosition);
        });
    // Checking #view=FitH,[float position].
    paramsParser.getViewportFromUrlParams(
        `${url}#view=FitH,7.89`, function(params) {
          chrome.test.assertEq(FittingType.FIT_TO_WIDTH, params.view);
          chrome.test.assertEq(7.89, params.viewPosition);
        });
    // Checking #view=FitV.
    paramsParser.getViewportFromUrlParams(`${url}#view=FitV`, function(params) {
      chrome.test.assertEq(FittingType.FIT_TO_HEIGHT, params.view);
      chrome.test.assertEq(undefined, params.viewPosition);
    });
    // Checking #view=FitV,[int position].
    paramsParser.getViewportFromUrlParams(
        `${url}#view=FitV,123`, function(params) {
          chrome.test.assertEq(FittingType.FIT_TO_HEIGHT, params.view);
          chrome.test.assertEq(123, params.viewPosition);
        });
    // Checking #view=FitV,[float position].
    paramsParser.getViewportFromUrlParams(
        `${url}#view=FitV,1.23`, function(params) {
          chrome.test.assertEq(FittingType.FIT_TO_HEIGHT, params.view);
          chrome.test.assertEq(1.23, params.viewPosition);
        });
    // Checking #view=[wrong parameter].
    paramsParser.getViewportFromUrlParams(`${url}#view=FitW`, function(params) {
      chrome.test.assertEq(undefined, params.view);
      chrome.test.assertEq(undefined, params.viewPosition);
    });
    // Checking #view=[wrong parameter],[position].
    paramsParser.getViewportFromUrlParams(
        `${url}#view=FitW,555`, function(params) {
          chrome.test.assertEq(undefined, params.view);
          chrome.test.assertEq(undefined, params.viewPosition);
        });

    // Checking #toolbar=0 to disable the toolbar.
    var uiParams = paramsParser.getUiUrlParams(`${url}#toolbar=0`);
    chrome.test.assertFalse(uiParams.toolbar);
    uiParams = paramsParser.getUiUrlParams(`${url}#toolbar=1`);
    chrome.test.assertTrue(uiParams.toolbar);

    chrome.test.succeed();
  }
];

var scriptingAPI = new PDFScriptingAPI(window, window);
scriptingAPI.setLoadCallback(function() {
  chrome.test.runTests(tests);
});
