'use strict';

const formPostFileUploadTest = ({
  fileNameEncoding,
  fileToDrop,
  formEncoding,
  expectedEncodedBaseName,
}) => {
  promise_test(async testCase => {

    if (document.readyState !== 'complete') {
      await new Promise(resolve => addEventListener('load', resolve));
    }
    assert_own_property(
        window,
        'eventSender',
        'This test relies on eventSender.beginDragWithFiles ' +
          '-- to manually test this behavior search for ' +
          '"What steps will reproduce the problem?" in ' +
          'https://crbug.com/661819');

    const target = Object.assign(document.createElement('iframe'), {
      name: 'target',
    });
    document.body.prepend(target);

    const form = Object.assign(document.createElement('form'), {
      acceptCharset: formEncoding,
      action: 'http://127.0.0.1:8000/xmlhttprequest/resources/post-echo.cgi',
      method: 'POST',
      enctype: 'multipart/form-data',
      target: target.name,
    });

    const fileInput = Object.assign(document.createElement('input'), {
      type: 'file',
      name: 'file',
      ondragenter: event => event.preventDefault(),
      ondragover: event => event.preventDefault(),
      ondrop: event => {
        assert_not_equals(
            event.dataTransfer.types.indexOf('Files'),
            -1,
            'event.dataTransfer must contain a File object on drop.');
        event.preventDefault();
        fileInput.files = event.dataTransfer.files;
        form.submit();
      },
    });
    fileInput.style.cssText = 'width: 100px; height: 100px';

    form.appendChild(fileInput);

    // Important that we put this element at the top of the doc so that logging
    // does not cause it to go out of view (where it can't be dragged
    // to)
    document.body.prepend(form);

    await new Promise(resolve => {
      target.onload = resolve;
      eventSender.beginDragWithFiles([fileToDrop]);
      const centerX = fileInput.offsetLeft + fileInput.offsetWidth / 2;
      const centerY = fileInput.offsetTop + fileInput.offsetHeight / 2;
      eventSender.mouseMoveTo(centerX, centerY);
      eventSender.mouseUp();
    });

    const formDataText = target.contentDocument.body.innerText;
    const formDataLines = formDataText.split('\n');
    if (formDataLines.length && !formDataLines[formDataLines.length - 1]) {
      --formDataLines.length;
    }
    assert_greater_than(
        formDataLines.length,
        2,
        'Multipart form data should have at least 3 lines:\n' +
          JSON.stringify(formDataText));
    assert_equals(
        formDataLines[formDataLines.length - 1],
        formDataLines[0] + '--',
        'Form data not multipart-shaped (missing expected boundaries):\n' +
          formDataText);
    const boundary = formDataLines[0];
    const expectedText = [
      boundary,
      `Content-Disposition: form-data; name="file"; ` +
          `filename="${expectedEncodedBaseName}"`,
      'Content-Type: text/plain',
      '',
      'ABC~¤•★星🌟星★•¤~XYZ',
      '',
      boundary + '--',
    ].join('\n');
    assert_true(
        formDataText.startsWith(expectedText),
        'Unexpected multipart-shaped form data received:\n' +
          formDataText +
          '\nExpected:\n' +
          expectedText);
  }, 'Upload ' + fileNameEncoding + '-named file in ' + formEncoding + ' form');
};
