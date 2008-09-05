// Copyright 2006 Google Inc.
// All Rights Reserved.

// Author: aarati@google.com

function setUp() {
}

var cases_ = [ 'aarati', 'JP', 'aarati',
               'aarati@gmail.com', 'JP', 'aarati',
               'aarati@gmail', 'JP', 'aarati',
               'y6.m4.n6.r6.@ezweb.ne.jp', 'JP', 'y6.m4.n6.r6.',
               'snoopy..0_0..86325-tomomin2@docomo.ne.jp', 'JP',
               'snoopy..0_0..86325-tomomin2',
               'masahiro.....urano@docomo.ne.jp', 'JP', 'masahiro.....urano',
               'masahiro.....urano', 'JP', 'masahiro.....urano',
               'imode.yubin@docomo.ne.jp', 'JP', 'imode.yubin',
               'imode.yubin', 'JP', 'imode.yubin',
               'aarati', 'US', undefined,
               'aarati@gmail.com', 'US', undefined,
               'aarati@gmail', 'US', undefined,

               // no regexp -- return given number without changes
               '6501234567', 'US', '6501234567',
               '+1-650-123-4567', 'US', '+1-650-123-4567',
               '+16501234567', 'US', '+16501234567',
               '(650) 123-4567', 'US', '(650) 123-4567',
               '650-123-4567', 'US', '650-123-4567',
               '6501234567', 'CA', '6501234567',
               '+1-650-123-4567', 'CA', '+1-650-123-4567',
               '+16501234567', 'CA', '+16501234567',
               '(650) 123-4567', 'CA', '(650) 123-4567',
               '650-123-4567', 'CA', '650-123-4567',
               '6501234567', 'GB', '6501234567',
               '+1-650-123-4567', 'GB', '+1-650-123-4567',

               '+44-7-123-456-789', 'GB', '+44-7-123-456-789',
               '44-7123456789', 'GB', '44-7123456789',

               '33612345678', 'FR', '33612345678',
               '33-612345678', 'FR', '33-612345678',

               '34612345678', 'ES', '34612345678',
               '34-612345678', 'ES', '34-612345678',


               '49123', 'DE', '49123',
               '49-1234', 'DE', '49-1234',
               '49-12345', 'DE', '49-12345',
               '49-123456', 'DE', '49-123456',
               '491234567', 'DE', '491234567',
               '+491234567', 'DE', '+491234567',
               '49-1234567', 'DE', '49-1234567',
               '49-12345678', 'DE', '49-12345678',
               '49-123456789', 'DE', '49-123456789',
               '49-1234567890', 'DE', '49-1234567890',
               '49-12345678901', 'DE', '49-12345678901',
               '0721 133 4542', 'DE', '0721 133 4542',

               '86 13123456789', 'CN', '86 13123456789',

               '82123456789', 'KR', '82123456789',
               '821234567890', 'KR', '821234567890',

               '31612345678', 'NL', '31612345678',
               '31-612345678', 'NL', '31-612345678',

               '61-4-12345678', 'AU', '61-4-12345678',

               '62-1234567890', 'ID', '62-1234567890',
               '62-12345678901', 'ID', '62-12345678901',

               '64-212345678', 'NZ', '64-212345678',

               '63-1234567890', 'PH', '63-1234567890',

               '65-12345678', 'SG', '65-12345678',

               '66-12345678', 'TH', '66-12345678',

               '90-1234567890', 'TR', '90-1234567890',

               '55-123456780', 'BR', '55-123456780',
               '55-1234567890', 'BR', '55-1234567890',

               '1-234-567890', 'VG', '1-234-567890'
];

function testSE_SanitizeDeviceAddress() {
  for (var i = 0; i < cases_.length; i+=3) {
    var deviceAddress = cases_[i];
    var country = cases_[i+1];
    var answer = cases_[i+2];

    var sanitized = SE_SanitizeDeviceAddress(deviceAddress, country);
    var string = deviceAddress + ', ' + country + ': '
                 + answer + ' != ' + sanitized;
    assertEquals(string, answer, sanitized);
  }
}

var supportedCountries_ = [
    'GB', 'JP', 'FR', 'ES', 'NL', 'DE', 'AU', 'ID', 'NZ',
    'PH', 'SG', 'TH', 'TR', 'BR', 'IN', 'CA', 'US', 'MY', 'AD', 'AE', 'AR',
    'AT', 'AZ', 'BA', 'BE', 'BG', 'BH', 'CH', 'CU', 'CY', 'CZ', 'DK', 'EE',
    'EG', 'FI', 'FO', 'GI', 'GR', 'HK', 'HR', 'HU', 'IE', 'IL', 'IQ', 'IS',
    'IT', 'JM', 'KE', 'KW', 'KZ', 'LB', 'LI', 'LK', 'LT', 'LU', 'LV', 'MA',
    'MO', 'MT', 'MU', 'MZ', 'NG', 'NO', 'OM', 'PE', 'PL', 'PT', 'RO', 'RU',
    'SA', 'SC', 'SE', 'SI', 'SK', 'TW', 'TZ', 'UA', 'VG', 'ZA' ];

var unSupportedCountries_ = [ 'CN', 'KR', 'XX', 'UN', null, undefined];

function testSE_isSMSSupportedCountry() {
  for (var i = 0; i < supportedCountries_.length; ++i) {
    var country = supportedCountries_[i];
    assertTrue("Should be supported: " + country,
               SE_isSMSSupportedCountry(country));
  }

  for (var i = 0; i < unSupportedCountries_.length; ++i) {
    var country = unSupportedCountries_[i];
    assertFalse("Should not be supported: " + country,
                SE_isSMSSupportedCountry(country));
  }
}
