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

    function filterAddressProperties(addresses) {
      return addresses.map(address => {
        var filteredAddress = {};
        ['fullNames', 'addressLevel1', 'addressLevel2', 'postalCode',
         'sortingCode', 'phoneNumbers', 'emailAddresses']
            .forEach(property => {
              filteredAddress[property] = address[property];
            });
        return filteredAddress;
      });
    }

    chrome.test.listenOnce(
        chrome.autofillPrivate.onAddressListChanged,
        chrome.test.callbackPass(function(addressList) {
          chrome.test.assertEq(
              [{
                fullNames: [NAME],
                addressLevel1: ADDRESS_LEVEL1,
                addressLevel2: ADDRESS_LEVEL2,
                postalCode: POSTAL_CODE,
                sortingCode: SORTING_CODE,
                phoneNumbers: [PHONE],
                emailAddresses: [EMAIL]
              }],
              filterAddressProperties(addressList));
        }));

    chrome.autofillPrivate.getAddressList(
        chrome.test.callbackPass(function(addressList) {
          chrome.test.assertEq([], addressList);
          chrome.autofillPrivate.saveAddress({
            fullNames: [NAME],
            addressLevel1: ADDRESS_LEVEL1,
            addressLevel2: ADDRESS_LEVEL2,
            addressLevel3: ADDRESS_LEVEL3,
            postalCode: POSTAL_CODE,
            sortingCode: SORTING_CODE,
            countryCode: COUNTRY_CODE,
            phoneNumbers: [PHONE],
            emailAddresses: [EMAIL]
          });
        }));
  },

  function updateExistingAddress() {
    // Original information for the profile. Should be identical to the ones in
    // the addNewAddress function.
    var COMPANY_NAME = 'Company name';
    var ADDRESS_LEVEL1 = 'Address level 1';
    var ADDRESS_LEVEL2 = 'Address level 2';
    var ADDRESS_LEVEL3 = 'Address level 3';
    var POSTAL_CODE = 'Postal code';
    var SORTING_CODE = 'Sorting code';
    var COUNTRY_CODE = 'US';
    var EMAIL = 'johndoe@gmail.com';

    // The information that will be updated. It should be different than the
    // information in the addNewAddress function.
    var UPDATED_NAME = 'UpdatedName';
    var UPDATED_PHONE = '1 987-987-9876'

    function filterAddressProperties(addresses) {
      return addresses.map(address => {
        var filteredAddress = {};
        ['fullNames', 'addressLevel1', 'addressLevel2', 'postalCode',
         'sortingCode', 'phoneNumbers', 'emailAddresses']
            .forEach(property => {
              filteredAddress[property] = address[property];
            });
        return filteredAddress;
      });
    }

    chrome.test.listenOnce(
        chrome.autofillPrivate.onAddressListChanged,
        chrome.test.callbackPass(function(addressList) {
          chrome.test.assertEq(
              [{
                fullNames: [UPDATED_NAME],
                addressLevel1: ADDRESS_LEVEL1,
                addressLevel2: ADDRESS_LEVEL2,
                postalCode: POSTAL_CODE,
                sortingCode: SORTING_CODE,
                phoneNumbers: [UPDATED_PHONE],
                emailAddresses: [EMAIL]
              }],
              filterAddressProperties(addressList));
        }));

    chrome.autofillPrivate.getAddressList(
        chrome.test.callbackPass(function(addressList) {
          // The address from the addNewAddress function should still be there.
          chrome.test.assertEq(1, addressList.length);
          // Update it by saving an address with the same guid and using some
          // different information.
          chrome.autofillPrivate.saveAddress({
            guid: addressList[0].guid,
            fullNames: [UPDATED_NAME],
            addressLevel1: ADDRESS_LEVEL1,
            addressLevel2: ADDRESS_LEVEL2,
            addressLevel3: ADDRESS_LEVEL3,
            postalCode: POSTAL_CODE,
            sortingCode: SORTING_CODE,
            countryCode: COUNTRY_CODE,
            phoneNumbers: [UPDATED_PHONE],
            emailAddresses: [EMAIL]
          });
        }));
  },

  function addNewCreditCard() {
    var NAME = 'Name';
    var NUMBER = '4111 1111 1111 1111';
    var EXP_MONTH = '02';
    var EXP_YEAR = '2999';

    function filterCardProperties(cards) {
      return cards.map(cards => {
        var filteredCards = {};
        ['name', 'cardNumber', 'expirationMonth', 'expirationYear'].forEach(
            property => {
              filteredCards[property] = cards[property];
            });
        return filteredCards;
      });
    }

    chrome.test.listenOnce(
        chrome.autofillPrivate.onCreditCardListChanged,
        chrome.test.callbackPass(function(cardList) {
          chrome.test.assertEq(
              [{
                name: NAME,
                cardNumber: NUMBER,
                expirationMonth: EXP_MONTH,
                expirationYear: EXP_YEAR
              }],
              filterCardProperties(cardList));
        }));

    chrome.autofillPrivate.getCreditCardList(
        chrome.test.callbackPass(function(cardList) {
          chrome.test.assertEq([], cardList);
          chrome.autofillPrivate.saveCreditCard({
            name: NAME,
            cardNumber: NUMBER,
            expirationMonth: EXP_MONTH,
            expirationYear: EXP_YEAR
          });
        }));
  },

  function updateExistingCreditCard() {
    // Original information for the card. Should be identical to the ones in the
    // addNewCredirCard function.
    var NUMBER = '4111 1111 1111 1111';
    var EXP_MONTH = '02';

    // The information that will be updated. It should be different than the
    // information in the addNewCreditCard function.
    var UPDATED_NAME = 'UpdatedName';
    var UPDATED_EXP_YEAR = '2888';

    function filterCardProperties(cards) {
      return cards.map(cards => {
        var filteredCards = {};
        ['name', 'cardNumber', 'expirationMonth', 'expirationYear'].forEach(
            property => {
              filteredCards[property] = cards[property];
            });
        return filteredCards;
      });
    }

    chrome.test.listenOnce(
        chrome.autofillPrivate.onCreditCardListChanged,
        chrome.test.callbackPass(function(cardList) {
          chrome.test.assertEq(
              [{
                name: UPDATED_NAME,
                cardNumber: NUMBER,
                expirationMonth: EXP_MONTH,
                expirationYear: UPDATED_EXP_YEAR
              }],
              filterCardProperties(cardList));
        }));

    chrome.autofillPrivate.getCreditCardList(
        chrome.test.callbackPass(function(cardList) {
          // The card from the addNewCreditCard function should still be there.
          chrome.test.assertEq(1, cardList.length);
          // Update it by saving a card with the same guid and using some
          // different information.
          chrome.autofillPrivate.saveCreditCard({
            guid: cardList[0].guid,
            name: UPDATED_NAME,
            cardNumber: NUMBER,
            expirationMonth: EXP_MONTH,
            expirationYear: UPDATED_EXP_YEAR
          });
        }));
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

/** @const */
var TESTS_FOR_CONFIG = {
  'addAndUpdateAddress': ['addNewAddress', 'updateExistingAddress'],
  'addAndUpdateCreditCard': ['addNewCreditCard', 'updateExistingCreditCard']
};

var testConfig = window.location.search.substring(1);
var testsToRun = TESTS_FOR_CONFIG[testConfig] || [testConfig];
chrome.test.runTests(availableTests.filter(function(op) {
  return testsToRun.includes(op.name);
}));
