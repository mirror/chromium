'use strict';

if (self.testRunner) {
    testRunner.setBlockThirdPartyCookies(false);
}

const ORIGINAL_HOST  = 'example.test',
      TEST_ROOT = 'not-example.test';
const TEST_HOST = 'cookies.' + TEST_ROOT,
      TEST_SUB  = 'subdomain.' + TEST_HOST;

const STRICT_DOM = 'strict_from_dom',
      LAX_DOM = 'lax_from_dom',
      NORMAL_DOM = 'normal_from_dom';

// Clear the three well-known cookies.
const clearKnownCookies = async opt_options => {
    const cookies = [ STRICT_DOM, LAX_DOM, NORMAL_DOM ];
    for (let name of cookies) await cookieStore.delete(name, opt_options);
};
