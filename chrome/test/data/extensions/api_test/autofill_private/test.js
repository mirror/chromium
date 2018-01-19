// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This just tests the interface. It does not test for specific results, only
// that callbacks are correctly invoked, expected parameters are correct,
// and failures are detected.

var availableTests = [
  function getCountryList() {
    var handler = function(countries) {
      var numSeparators = 0;
      var countBeforeSeparator = 0;
      var countAfterSeparator = 0;

      var beforeSeparator = true;

      chrome.test.assertTrue(countries.length > 1,
          'Expected more than one country');

      countries.forEach(function(country) {
        // Expecting to have both |name| and |countryCode| or neither.
        chrome.test.assertEq(!!country.name, !!country.countryCode);

        if (country.name) {
          if (beforeSeparator)
            ++countBeforeSeparator;
          else
            ++countAfterSeparator;
        } else {
          beforeSeparator = false;
          ++numSeparators;
        }
      });

      chrome.test.assertEq(1, numSeparators);
      chrome.test.assertEq(1, countBeforeSeparator);
      chrome.test.assertTrue(countAfterSeparator > 1,
          'Expected more than one country after the separator');

      chrome.test.succeed();
    };

    chrome.autofillPrivate.getCountryList(handler);
  },

  function getAddressComponents() {
    var COUNTRY_CODE = 'US';

    var handler = function(components) {
      chrome.test.assertTrue(!!components.components);
      chrome.test.assertTrue(!!components.components[0]);
      chrome.test.assertTrue(!!components.components[0].row);
      chrome.test.assertTrue(!!components.components[0].row[0]);
      chrome.test.assertTrue(!!components.components[0].row[0].field);
      chrome.test.assertTrue(!!components.components[0].row[0].fieldName);
      chrome.test.assertTrue(!!components.languageCode);
      chrome.test.succeed();
    }

    chrome.autofillPrivate.getAddressComponents(COUNTRY_CODE, handler);
  },

  function addNewAddress() {
    var NAME = 'Name';
    var COMPANY_NAME = 'Company name';
    var ADDRESS_LEVEL1 = 'Address level 1';
    var ADDRESS_LEVEL2 = 'Address level 2';
    var ADDRESS_LEVEL3 = 'Address level 3';
    var POSTAL_CODE = 'Postal code';
    var SORTING_CODE = 'Sorting code';
    var COUNTRY_CODE = 'US';
    var PHONE = '1 123-123-1234';
    var EMAIL = 'johndoe@gmail.com';

    var numCalls = 0;
    var handler = function(addressList) {
      numCalls++;

      if (numCalls == 1) {
        chrome.test.assertEq(addressList.length, 0);
      } else {
        chrome.test.assertEq(addressList.length, 1);
        var address = addressList[0];
        chrome.test.assertEq(address.fullNames[0], NAME);
        chrome.test.assertEq(address.addressLevel1, ADDRESS_LEVEL1);
        chrome.test.assertEq(address.addressLevel2, ADDRESS_LEVEL2);
        chrome.test.assertEq(address.addressLevel3, ADDRESS_LEVEL3);
        chrome.test.assertEq(address.postalCode, POSTAL_CODE);
        chrome.test.assertEq(address.sortingCode, SORTING_CODE);
        chrome.test.assertEq(address.countryCode, COUNTRY_CODE);
        chrome.test.assertEq(address.phoneNumbers[0], PHONE);
        chrome.test.assertEq(address.emailAddresses[0], EMAIL);
        chrome.test.succeed();
      }
    }

    chrome.autofillPrivate.onAddressListChanged.addListener(handler);
    chrome.autofillPrivate.getAddressList(handler);
    chrome.autofillPrivate.saveAddress({fullNames: [NAME],
      addressLevel1: ADDRESS_LEVEL1, addressLevel2: ADDRESS_LEVEL2,
      addressLevel3: ADDRESS_LEVEL3, postalCode: POSTAL_CODE,
      sortingCode: SORTING_CODE, countryCode: COUNTRY_CODE,
      phoneNumbers: [PHONE], emailAddresses: [EMAIL]});
  },

  function updateExistingAddress() {
    var NAME = 'Name';
    var COMPANY_NAME = 'Company name';
    var ADDRESS_LEVEL1 = 'Address level 1';
    var ADDRESS_LEVEL2 = 'Address level 2';
    var ADDRESS_LEVEL3 = 'Address level 3';
    var POSTAL_CODE = 'Postal code';
    var SORTING_CODE = 'Sorting code';
    var COUNTRY_CODE = 'US';
    var PHONE = '1 123-123-1234';
    var EMAIL = 'johndoe@gmail.com';

    var UPDATED_NAME = 'UpdatedName';

    var generated_guid = "123";

    var numCalls = 0;
    var handler = function(addressList) {
      numCalls++;

      if (numCalls == 1) {
        chrome.test.assertEq(addressList.length, 0);
      } else if (numCalls == 2) {
        chrome.test.assertEq(addressList.length, 1);
        var address = addressList[0];
        generated_guid = address.guid;
        chrome.autofillPrivate.saveAddress({guid: generated_guid,
          fullNames: [UPDATED_NAME]});
      } else {
        chrome.test.assertEq(addressList.length, 1);
        var address = addressList[0];
        chrome.test.assertEq(address.guid, generated_guid);
        chrome.test.assertEq(address.fullNames[0], UPDATED_NAME);
        chrome.test.assertEq(address.addressLevel1, ADDRESS_LEVEL1);
        chrome.test.assertEq(address.addressLevel2, ADDRESS_LEVEL2);
        chrome.test.assertEq(address.addressLevel3, ADDRESS_LEVEL3);
        chrome.test.assertEq(address.postalCode, POSTAL_CODE);
        chrome.test.assertEq(address.sortingCode, SORTING_CODE);
        chrome.test.assertEq(address.countryCode, COUNTRY_CODE);
        chrome.test.assertEq(address.phoneNumbers[0], PHONE);
        chrome.test.assertEq(address.emailAddresses[0], EMAIL);
        chrome.test.succeed();
      }
    }

    chrome.autofillPrivate.onAddressListChanged.addListener(handler);
    chrome.autofillPrivate.getAddressList(handler);
    chrome.autofillPrivate.saveAddress({fullNames: [NAME],
      addressLevel1: ADDRESS_LEVEL1, addressLevel2: ADDRESS_LEVEL2,
      addressLevel3: ADDRESS_LEVEL3, postalCode: POSTAL_CODE,
      sortingCode: SORTING_CODE, countryCode: COUNTRY_CODE,
      phoneNumbers: [PHONE], emailAddresses: [EMAIL]});
  },

  function addNewCreditCard() {
    var NAME = 'Name';
    var NUMBER = "4111 1111 1111 1111";
    var EXP_MONTH = "02";
    var EXP_YEAR = "2999";

    var numCalls = 0;
    var handler = function(creditCardList) {
      numCalls++;

      if (numCalls == 1) {
        chrome.test.assertEq(creditCardList.length, 0);
      } else {
        chrome.test.assertEq(creditCardList.length, 1);
        var creditCard = creditCardList[0];
        chrome.test.assertEq(creditCard.name, NAME);
        chrome.test.assertEq(creditCard.cardNumber, NUMBER);
        chrome.test.assertEq(creditCard.expirationMonth, EXP_MONTH);
        chrome.test.assertEq(creditCard.expirationYear, EXP_YEAR);
        chrome.test.succeed();
      }
    }

    chrome.autofillPrivate.onCreditCardListChanged.addListener(handler);
    chrome.autofillPrivate.getCreditCardList(handler);
    chrome.autofillPrivate.saveCreditCard({name: NAME, cardNumber: NUMBER,
      expirationMonth: EXP_MONTH, expirationYear: EXP_YEAR});
  },

  function updateExistingCreditCard() {
    var NAME = 'Name';
    var NUMBER = "4111 1111 1111 1111";
    var EXP_MONTH = "02";
    var EXP_YEAR = "2999";

    var UPDATED_NAME = 'UpdatedName';

    var generated_guid = "123";

    var numCalls = 0;
    var handler = function(creditCardList) {
      numCalls++;

      if (numCalls == 1) {
        chrome.test.assertEq(creditCardList.length, 0);
      } else if (numCalls == 2) {
        chrome.test.assertEq(creditCardList.length, 1);
        var creditCard = creditCardList[0];
        generated_guid = creditCard.guid;
        chrome.autofillPrivate.saveCreditCard({guid: generated_guid,
          name: UPDATED_NAME});
      } else {
        chrome.test.assertEq(creditCardList.length, 1);
        var creditCard = creditCardList[0];
        chrome.test.assertEq(creditCard.guid, generated_guid);
        chrome.test.assertEq(creditCard.name, UPDATED_NAME);
        chrome.test.assertEq(creditCard.cardNumber, NUMBER);
        chrome.test.assertEq(creditCard.expirationMonth, EXP_MONTH);
        chrome.test.assertEq(creditCard.expirationYear, EXP_YEAR);
        chrome.test.succeed();
      }
    }

    chrome.autofillPrivate.onCreditCardListChanged.addListener(handler);
    chrome.autofillPrivate.getCreditCardList(handler);
    chrome.autofillPrivate.saveCreditCard({name: NAME, cardNumber: NUMBER,
      expirationMonth: EXP_MONTH, expirationYear: EXP_YEAR});
  },

  function removeEntry() {
    var NAME = 'Name';
    var guid;

    var numCalls = 0;
    var handler = function(creditCardList) {
      numCalls++;

      if (numCalls == 1) {
        chrome.test.assertEq(creditCardList.length, 0);
      } else if (numCalls == 2) {
        chrome.test.assertEq(creditCardList.length, 1);
        var creditCard = creditCardList[0];
        chrome.test.assertEq(creditCard.name, NAME);

        guid = creditCard.guid;
        chrome.autofillPrivate.removeEntry(guid);
      } else {
        chrome.test.assertEq(creditCardList.length, 0);
        chrome.test.succeed();
      }
    }

    chrome.autofillPrivate.onCreditCardListChanged.addListener(handler);
    chrome.autofillPrivate.getCreditCardList(handler);
    chrome.autofillPrivate.saveCreditCard({name: NAME});
  },

  function validatePhoneNumbers() {
    var COUNTRY_CODE = 'US';
    var ORIGINAL_NUMBERS = ['1-800-123-4567'];
    var FIRST_NUMBER_TO_ADD = '1-800-234-5768';
    // Same as original number, but without formatting.
    var SECOND_NUMBER_TO_ADD = '18001234567';

    var handler1 = function(validateNumbers) {
      chrome.test.assertEq(validateNumbers.length, 1);
      chrome.test.assertEq('1-800-123-4567', validateNumbers[0]);

      chrome.autofillPrivate.validatePhoneNumbers({
        phoneNumbers: validatedNumbers.concat(FIRST_NUMBER_TO_ADD),
        indexOfNewNumber: 0,
        countryCode: COUNTRY_CODE
      }, handler2);
    }

    var handler2 = function(validatedNumbers) {
      chrome.test.assertEq(validateNumbers.length, 2);
      chrome.test.assertEq('1-800-123-4567', validateNumbers[0]);
      chrome.test.assertEq('1-800-234-5678', validateNumbers[1]);

      chrome.autofillPrivate.validatePhoneNumbers({
        phoneNumbers: validatedNumbers.concat(SECOND_NUMBER_TO_ADD),
        indexOfNewNumber: 0,
        countryCode: COUNTRY_CODE
      }, handler3);
    };

    var handler3 = function(validateNumbers) {
      // Newly-added number should not appear since it was the same as an
      // existing number.
      chrome.test.assertEq(validateNumbers.length, 2);
      chrome.test.assertEq('1-800-123-4567', validateNumbers[0]);
      chrome.test.assertEq('1-800-234-5678', validateNumbers[1]);
      chrome.test.succeed();
    }

    chrome.autofillPrivate.validatePhoneNumbers({
      phoneNumbers: ORIGINAL_NUMBERS,
      indexOfNewNumber: 0,
      countryCode: COUNTRY_CODE
    }, handler1);
  },
];

var testToRun = window.location.search.substring(1);
chrome.test.runTests(availableTests.filter(function(op) {
  return op.name == testToRun;
}));
