'use strict';

// Rationale for this particular test character sequence, which is
// used in filenames and also in file contents:
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
//
// Overall the near-symmetry makes common I18N mistakes like
// off-by-1-after-non-BMP easier to spot.
const kTestChars = 'ABC~¤•★星🌟星★•¤~XYZ';
const kTestFallbackIso2022jp =
      'ABC~¤•\x1b$B!z@1\x1b(B🌟\x1b$B@1!z\x1b(B•¤~XYZ'.replace(
            /[^\0-\x7f]/gu,
          x => `&#${x.codePointAt(0)};`);

// Web server hosting helper CGIs
const kWebServer = 'http://127.0.0.1:8000';

const formPostFileUploadTest = ({
  fileNameEncoding,
  fileBaseName,
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
        'This test relies on eventSender.beginDragWithFiles');

    const formTargetFrame = Object.assign(document.createElement('iframe'), {
      name: 'formtargetframe',
    });
    document.body.prepend(formTargetFrame);

    const form = Object.assign(document.createElement('form'), {
      acceptCharset: formEncoding,
      action: `${kWebServer}/xmlhttprequest/resources/post-echo.cgi`,
      method: 'POST',
      enctype: 'multipart/form-data',
      target: formTargetFrame.name,
    });
    // This element must be at the top of the viewport so it can be dragged to.
    document.body.prepend(form);

    const fileInput = Object.assign(document.createElement('input'), {
      type: 'file',
      name: 'file',
    });
    form.appendChild(fileInput);

    const fileToDropLines = (await (await fetch(
        `${kWebServer}/local/fileapi/resources/write-temp-file.cgi?filename=${
           encodeURIComponent(fileBaseName)
         }&data=${
           encodeURIComponent(kTestChars)
         }`, { method: 'post' })).text()).split('\n');
    assert_equals(
        fileToDropLines.length,
        3,
        `CGI response should have three lines but got ${fileToDropLines}`);
    const [shouldBeOk, fileToDropAcp, fileToDrop] = fileToDropLines;
    assert_equals(
        shouldBeOk,
        'OK',
        `CGI response should begin with OK but got ${fileToDropLines}`);
    assert_equals(
        fileToDrop.split(/[\/\\]/).pop(),
        fileBaseName,
        `Unicode ${fileToDrop} basename should be ${fileBaseName}`);
    assert_equals(
        fileToDropAcp.split(/[\/\\]/).pop().replace(/[^ -~]/g, '?'),
        fileBaseName.replace(/[^ -~]/g, '?'),
        `ACP ${fileToDropAcp} basename should resemble ${fileBaseName}`);
    fileInput.onchange = event => {
      if (fileInput.files[0].name !== fileBaseName) {
        assert_equals(
            fileInput.files[0].name,
            fileToDropAcp.split(/[\/\\]/).pop(),
            `Dropped file should be ${fileToDrop} or ${fileToDropAcp}`);
      }
      form.submit();
    };
    try {
      await new Promise(resolve => {
        formTargetFrame.onload = resolve;
        eventSender.beginDragWithFiles([fileToDropAcp]);
        const centerX = fileInput.offsetLeft + fileInput.offsetWidth / 2;
        const centerY = fileInput.offsetTop + fileInput.offsetHeight / 2;
        eventSender.mouseMoveTo(centerX, centerY);
        eventSender.mouseUp();
      });
    } finally {
      const cleanupErrors = await (await fetch(
          `${kWebServer}/local/fileapi/resources/reset-temp-file.cgi?filename=${
             encodeURIComponent(fileToDrop)
           }`, { method: 'post' })).text();
      assert_equals(cleanupErrors, 'OK', 'Temp file cleanup should not fail');
    }

    const formDataText = formTargetFrame.contentDocument.body.innerText;
    const formDataLines = formDataText.split('\n');
    if (formDataLines.length && !formDataLines[formDataLines.length - 1]) {
      --formDataLines.length;
    }
    assert_greater_than(
        formDataLines.length,
        2,
        `Multipart form data should have at least 3 lines: ${
           JSON.stringify(formDataText)
         }`);
    const boundary = formDataLines[0];
    assert_equals(
        formDataLines[formDataLines.length - 1],
        boundary + '--',
        `Multipart form data should end with ${boundary}--: ${
           JSON.stringify(formDataText)
         }`);
    const expectedText = [
      boundary,
      `Content-Disposition: form-data; name="file"; ` +
          `filename="${expectedEncodedBaseName}"`,
      'Content-Type: text/plain',
      '',
      kTestChars,
      boundary + '--',
    ].join('\n');
    assert_true(
        formDataText.startsWith(expectedText),
        `Unexpected multipart-shaped form data received: ${
           JSON.stringify(formDataText)
         } Expected: ${JSON.stringify(expectedText)}`);
  }, `Upload ${fileNameEncoding}-named file in ${formEncoding} form`);
};
