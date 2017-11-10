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
      onchange: event => form.submit(),
    });

    form.appendChild(fileInput);

    // This element must be at the top of the doc so that logging does
    // not cause it to go out of the viewport where it can't be
    // dragged to.
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
    // Rationale for this particular test character sequence:
    //
    // - ABC~ ensures the string starts with something we can read to
    //   ensure it is from the correct source; ~ is used because even
    //   some 1-byte otherwise-ASCII-like parts of ISO-2022-JP
    //   interpret it differently.
    // - ¤ is inside Latin-1 and helps diagnose problems due to
    //   filesystem encoding or locale; it is also the "simplest" case
    //   needing substitution in ISO-2022-JP
    // - • is inside Windows-1252 and helps diagnose problems due to
    //   filesystem encoding or locale and also ensures these aren't
    //   accidentally turned into e.g. control codes
    // - ★ is inside ISO-2022-JP on a non-Kanji page and makes correct
    //   output easier to spot
    // - 星 is inside ISO-2022-JP on a Kanji page and makes correct
    //   output easier to spot
    // - 🌟 is outside the BMP and makes incorrect surrogate pair
    //   substitution detectable and ensures substitutions work
    //   correctly immediately after Kanji 2-byte ISO-2022-JP
    // - 星 repeated here ensures the correct codec state is used
    //   after a non-BMP substitution
    // - ★ repeated here also makes correct output easier to spot
    // - • is inside Windows-1252 and again helps diagnose problems
    //   due to filesystem encoding or locale and also ensures
    //   substitutions work correctly immediately after non-Kanji
    //   2-byte ISO-2022-JP
    // - ¤ is inside Latin-1 and helps diagnose problems due to
    //   filesystem encoding or locale; again it is a "simple"
    //   substitution case and also ensures
    // - ~XYZ ensures earlier errors don't lead to misencoding of
    //   simple ASCII
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
