/**
 * @param {string} message
 */
function output(message) {
  if (!self._output)
    self._output = [];
  self._output.push('[page] ' + message);
}
