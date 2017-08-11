(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Verifies Browser.getVersion method.');
  var response = await dp.Browser.getVersion();
  check('protocol_version', /^\d+\.\d+$/); // e.g. 1.2
  check('revision', /^@[0-9abcdef]+$/); // e.g. @7d9cc1464f836e6d8c1ab9396a48c656df153d58
  check('user_agent', /Chrome/); // e.g. Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.77.34.5 Safari/537.36
  check('v8_version', /^\d+\.\d+\.\d+$/); // e.g. 6.2.196
  testRunner.completeTest();

  function check(fieldName, regex) {
    var status = regex.test(response.result[fieldName]) ? 'OK' : 'FAIL';
    testRunner.log(`version.${fieldName}: ${status}`);
  }
})
