'use strict';

// Rationale for this particular test character sequence, which is
// used in filenames and also in file contents:
//
// - ABC~ ensures the string starts with something we can read to
//   ensure it is from the correct source; ~ is used because even
//   some 1-byte otherwise-ASCII-like parts of ISO-2022-JP
//   interpret it differently.
// - ‾¥ are inside a single-byte range of ISO-2022-JP and help
//   diagnose problems due to filesystem encoding or locale
// - ≈ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - ¤ is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale; it is also the "simplest" case
//   needing substitution in ISO-2022-JP
// - ･ is inside a single-byte range of ISO-2022-JP in some variants
//   and helps diagnose problems due to filesystem encoding or locale;
//   on the web it is distinct when decoding but unified when encoding
// - ・ is inside a double-byte range of ISO-2022-JP and helps
//   diagnose problems due to filesystem encoding or locale
// - • is inside Windows-1252 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures these aren't
//   accidentally turned into e.g. control codes
// - ∙ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - · is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures HTML named
//   character references (e.g. &middot;) are not used
// - ☼ is inside IBM437 shadowing C0 and helps diagnose problems due to
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
// - ☼ is inside IBM437 shadowing C0 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures these aren't
//   accidentally turned into e.g. control codes and also ensures
//   substitutions work correctly immediately after non-Kanji
//   2-byte ISO-2022-JP
// - · is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale and also ensures HTML named
//   character references (e.g. &middot;) are not used
// - ∙ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - • is inside Windows-1252 and again helps diagnose problems
//   due to filesystem encoding or locale
// - ・ is inside a double-byte range of ISO-2022-JP and helps
//   diagnose problems due to filesystem encoding or locale
// - ･ is inside a single-byte range of ISO-2022-JP in some variants
//   and helps diagnose problems due to filesystem encoding or locale;
//   on the web it is distinct when decoding but unified when encoding
// - ¤ is inside Latin-1 and helps diagnose problems due to
//   filesystem encoding or locale; again it is a "simple"
//   substitution case and also ensures
// - ≈ is inside IBM437 and helps diagnose problems due to filesystem
//   encoding or locale
// - ¥‾ are inside a single-byte range of ISO-2022-JP and help
//   diagnose problems due to filesystem encoding or locale
// - ~XYZ ensures earlier errors don't lead to misencoding of
//   simple ASCII
//
// Overall the near-symmetry makes common I18N mistakes like
// off-by-1-after-non-BMP easier to spot. All the characters
// are also allowed in Windows Unicode filenames.
const kTestChars = 'ABC~‾¥≈¤･・•∙·☼★星🌟星★☼·∙•・･¤≈¥‾~XYZ';

// NOTE: The expected interpretation of ISO-2022-JP according to
// https://encoding.spec.whatwg.org/#iso-2022-jp-encoder unifies
// single-byte and double-byte katakana.
const kTestFallbackIso2022jp =
      ('ABC~\x1B(J~\\≈¤\x1B$B!&!&\x1B(B•∙·☼\x1B$B!z@1\x1B(B🌟' +
       '\x1B$B@1!z\x1B(B☼·∙•\x1B$B!&!&\x1B(B¤≈\x1B(J\\~\x1B(B~XYZ').replace(
             /[^\0-\x7F]/gu,
           x => `&#${x.codePointAt(0)};`);

// NOTE: \uFFFD is used here to replace Windows-1252 bytes to match
// how we will see them in the reflected POST bytes in a frame using
// UTF-8 byte interpretation. The bytes will actually be intact, but
// this code cannot tell and does not really care.
const kTestFallbackWindows1252 =
      'ABC~‾\xA5≈\xA4･・\x95∙\xB7☼★星🌟星★☼\xB7∙\x95・･\xA4≈\xA5‾~XYZ'.replace(
            /[^\0-\xFF]/gu,
          x => `&#${x.codePointAt(0)};`).replace(/[\x80-\xFF]/g, '\uFFFD');

const kTestFallbackXUserDefined =
      kTestChars.replace(/[^\0-\x7F]/gu, x => `&#${x.codePointAt(0)};`);

// Web server hosting helper CGIs
const kWebServer = 'http://127.0.0.1:8000';

const formPostFileUploadTest = ({
  fileNameSource,
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

    try {
      // Used to verify that the browser agrees with the test about
      // which form charset is used.
      form.appendChild(Object.assign(document.createElement('input'), {
        type: 'hidden',
        name: '_charset_',
      }));

      // Used to verify that the browser agrees with the test about
      // field value replacement and encoding independently of file system
      // idiosyncracies.
      form.appendChild(Object.assign(document.createElement('input'), {
        type: 'hidden',
        name: 'filename',
        value: fileBaseName,
      }));

      // Same, but with name and value reversed to ensure field names
      // get the same treatment.
      form.appendChild(Object.assign(document.createElement('input'), {
        type: 'hidden',
        name: fileBaseName,
        value: 'filename',
      }));

      const fileInput = Object.assign(document.createElement('input'), {
        type: 'file',
        name: 'file',
      });
      form.appendChild(fileInput);

      const fileToDropLines = (await (await fetch(
          `${kWebServer}/local/fileapi/resources/write-temp-file.cgi` +
            `?filename=${
               encodeURIComponent(fileBaseName)
             }&data=${encodeURIComponent(kTestChars)}`,
          { method: 'post' })).text()).split('\n');
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
          fileToDropAcp.split(/[\/\\]/).pop().replace(/[^ -~]/g, '?').substr(
              0,
              6).toLowerCase(),
          fileBaseName.replace(/[^ -~]/g, '?').substr(0, 6).toLowerCase(),
          `ACP ${fileToDropAcp} basename should resemble ${fileBaseName}`);
      const fileBaseNameAcp = fileToDropAcp.split(/[\/\\]/).pop();
      const filesToDrop = [fileToDrop];
      fileInput.onchange = event => {
        if (!fileInput.files[0].name && filesToDrop[0] !== fileToDropAcp) {
          // Dropping the Unicode filename failed. Try again with the
          // ACP filename.
          filesToDrop[0] = fileToDropAcp;
          eventSender.beginDragWithFiles(filesToDrop);
          const centerX = fileInput.offsetLeft + fileInput.offsetWidth / 2;
          const centerY = fileInput.offsetTop + fileInput.offsetHeight / 2;
          eventSender.mouseMoveTo(centerX, centerY);
          eventSender.mouseUp();
          return;
        }
        if (fileInput.files[0].name !== fileBaseName ||
            fileInput.value.split(/[\/\\]/).pop() !== fileBaseName) {
          assert_equals(
              fileInput.files[0].name,
              fileBaseNameAcp,
              `Dropped file should be ${fileBaseName} or ${fileBaseNameAcp}`);
        }
        form.submit();
      };
      try {
        await new Promise(resolve => {
          formTargetFrame.onload = resolve;
          eventSender.beginDragWithFiles(filesToDrop);
          const centerX = fileInput.offsetLeft + fileInput.offsetWidth / 2;
          const centerY = fileInput.offsetTop + fileInput.offsetHeight / 2;
          eventSender.mouseMoveTo(centerX, centerY);
          eventSender.mouseUp();
        });
      } finally {
        const cleanupErrors = await (await fetch(
            `${kWebServer}/local/fileapi/resources/reset-temp-file.cgi` +
              `?filename=${encodeURIComponent(fileToDrop)}`,
            { method: 'post' })).text();
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
        'Content-Disposition: form-data; name="_charset_"',
        '',
        formEncoding,
        boundary,
        'Content-Disposition: form-data; name="filename"',
        '',
        expectedEncodedBaseName,
        boundary,
        `Content-Disposition: form-data; name="${expectedEncodedBaseName}"`,
        '',
        'filename',
        boundary,
        `Content-Disposition: form-data; name="file"; ` +
            `filename="${expectedEncodedBaseName}"`,
        'Content-Type: text/plain',
        '',
        kTestChars,
        boundary + '--',
      ].join('\n');
      const expectedTextAcp = [
        boundary,
        'Content-Disposition: form-data; name="_charset_"',
        '',
        formEncoding,
        boundary,
        'Content-Disposition: form-data; name="filename"',
        '',
        expectedEncodedBaseName,
        boundary,
        `Content-Disposition: form-data; name="${expectedEncodedBaseName}"`,
        '',
        'filename',
        boundary,
        `Content-Disposition: form-data; name="file"; ` +
            `filename="${
               fileBaseNameAcp.split(fileBaseName).join(expectedEncodedBaseName)
             }"`,
        'Content-Type: text/plain',
        '',
        kTestChars,
        boundary + '--',
      ].join('\n');
      assert_true(
          formDataText.startsWith(expectedText) ||
            formDataText.startsWith(expectedTextAcp),
          `Unexpected multipart-shaped form data received:\n${
           formDataText
         }\nExpected:\n${expectedText}` +
            ((expectedText !== expectedTextAcp)
             ? `\nor:\n${expectedTextAcp}`
             : ''));
    } finally {
      document.body.removeChild(formTargetFrame);
      document.body.removeChild(form);
    }
  }, `Upload ${fileBaseName} (${fileNameSource}) in ${formEncoding} form`);
};
