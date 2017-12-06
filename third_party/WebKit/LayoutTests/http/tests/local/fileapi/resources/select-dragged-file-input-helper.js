'use strict';

// fileInputTest - verifies <input type=file> single file selection.
//
// Uses eventSender.beginDragWithFiles and related methods to select
// using drag-n-drop because that is currently the only file selection
// mechanism available to Blink layout tests, likely leading to the
// observed renderer crashes on POSIX-like systems using non-UTF-8
// locales.
//
// Fields in the parameter object:
//
// - fileNameSource: purely explanatory and gives a clue about which
//   character encoding is the source for the non-7-bit-ASCII parts of
//   the fileBaseName, or Unicode if no smaller-than-Unicode source
//   contains all the characters. Used in the test name.
// - fileBaseName: the not-necessarily-just-7-bit-ASCII file basename
//   for the test file. Used in the test name.
//
// NOTE: This does not correctly account for varying representation of
// combining characters across platforms and filesystems due to
// Unicode normalization or similar platform-specific normalization
// rules. For that reason none of the tests exercise such characters
// or character sequences.
const fileInputTest = ({
  fileNameSource,
  fileBaseName,
}) => {
  promise_test(async testCase => {

    if (document.readyState !== 'complete') {
      await new Promise(resolve => addEventListener('load', resolve));
    }
    assert_own_property(
        window,
        'eventSender',
        'This test relies on eventSender.beginDragWithFiles');

    const fileInput = Object.assign(document.createElement('input'), {
      type: 'file',
    });

    // This element must be at the top of the viewport so it can be dragged to.
    document.body.prepend(fileInput);
    testCase.add_cleanup(() => {
      document.body.removeChild(fileInput);
    });

    await new Promise(resolve => {
      fileInput.onchange = event => {
        assert_equals(
            fileInput.files[0].name,
            fileBaseName,
          `Dropped file should be ${fileBaseName}`);
        // Removes c:\fakepath\ or other pseudofolder and returns just the
        // final component of filePath; allows both / and \ as segment
        // delimiters.
        const baseNameOfFilePath = filePath => filePath.split(/[\/\\]/).pop();
        // For historical reasons .value will be prefixed with
        // c:\fakepath\, but the basename should match the dropped file
        // name exposed through the newer .files[0].name API. This check
        // verifies that assumption.
        assert_equals(
            fileInput.files[0].name,
            baseNameOfFilePath(fileInput.value),
            `The basename of the field's value should match its files[0].name`);
        resolve();
      };
      eventSender.beginDragWithFiles([`resources/${fileBaseName}`]);
      const centerX = fileInput.offsetLeft + fileInput.offsetWidth / 2;
      const centerY = fileInput.offsetTop + fileInput.offsetHeight / 2;
      eventSender.mouseMoveTo(centerX, centerY);
      eventSender.mouseUp();
    });
  }, `Select ${fileBaseName} (${fileNameSource}) in a file input`);
};
