/** * @fileoverview Script to remove CSS prefixed rules that don't apply to * Chromium.
 */
var fs = require('fs');
var glob = require('glob').sync;
var path = require('path');
/**
 * Strips CSS rules/attributes from the file in-place.
 * @param {string} filename The filename to process.
 */
function processFile(filename) {
  var fileContents = fs.readFileSync(filename, {'encoding': 'utf-8'});
  var lines = fileContents.split('\n');
  var processedFileContents = reconstructFile(lines, findLinesToRemove(lines));
  fs.writeFileSync(filename, processedFileContents);
  console.log('processedFile', filename);
}
/**
 * @return {!Array<string>} lines
 * @return {!Array<number>} The lines numbers that need to be removed in
 *     ascinding order.
 */
function findLinesToRemove(lines) {
  // Regex used to detect CSS attributes or even entire CSS rules to be removed.
  var cssPropertiesToRemove = [
    '-moz-appearance',
    '-moz-box-sizing',
    '-moz-flex-basis',
    '-moz-user-select',

    '-ms-align-content',
    '-ms-align-self',
    '-ms-flex',
    '-ms-flex-align',
    '-ms-flex-basis',
    '-ms-flexbox',
    '-ms-flex-direction',
    '-ms-flex-pack',
    '-ms-flex-wrap',
    '-ms-inline-flexbox',
    '-ms-user-select',

    '-webkit-align-content',
    '-webkit-align-items',
    '-webkit-align-self',
    '-webkit-animation',
    '-webkit-animation-duration',
    '-webkit-animation-iteration-count',
    '-webkit-animation-name',
    '-webkit-animation-timing-function',
    '-webkit-flex',
    '-webkit-flex-direction',
    '-webkit-flex-wrap',
    '-webkit-inline-flex',
    '-webkit-justify-content',
    '-webkit-transform',
    '-webkit-transform-origin',
    '-webkit-transition',
    '-webkit-transition-delay',
    '-webkit-transition-property',
    '-webkit-user-select',
  ].map(p => new RegExp(p));

  return lines.reduce(function(soFar, line, index) {
    if (cssPropertiesToRemove.some(p => p.test(line)))
      soFar.push(index);
    return soFar;
  }, []);
}
/**
 * Generates the final processed file.
 * @param {!Array<string>} lines The lines to be process.
 * @param {!Array<number>} linesToRemove Indices of the lines to be removed in
 *     ascending order.
 * @return {string} The contents of the processed file.
 */
function reconstructFile(lines, linesToRemove) {
  var matchWhiteSpaceRegex = /\s+/;
  // Process line numbers in descinding order, such that the array can be
  // modified in-place.
  linesToRemove.reverse().forEach(function(lineIndex) {
    lines.splice(lineIndex, 1);
  });
  return lines.join('\n');
}
var srcDir = path.resolve(path.join(__dirname, 'components-chromium'));
var htmlFiles = glob(srcDir + '/**/*.html');
var cssFiles = glob(srcDir + '/**/*.css');
htmlFiles.concat(cssFiles).forEach(processFile);
console.log('DONE');
