// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import org.chromium.chrome.browser.autofill.CardType;
import org.chromium.payments.mojom.BasicCardNetwork;
import org.chromium.payments.mojom.BasicCardType;
import org.chromium.payments.mojom.PaymentMethodData;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/** Basic-card utils */
public class BasicCardUtils {
    /** The method name for any type of card. */
    public static final String BASIC_CARD_METHOD_NAME = "basic-card";

    /** The total number of all possible card types (i.e., credit, debit, prepaid, unknown). */
    public static final int TOTAL_NUMBER_OF_CARD_TYPES = 4;

    /** @return A set of card networks (e.g., "visa", "amex") accepted by "basic-card" method. */
    public static Set<String> convertBasicCardToNetworks(PaymentMethodData data) {
        // Merchant website does not support any issuer networks.
        if (data == null) return null;

        // Merchant website supports all issuer networks.
        Map<Integer, String> networks = getNetworks();
        if (data.supportedNetworks == null || data.supportedNetworks.length == 0) {
            return new HashSet<>(networks.values());
        }

        // Merchant website supports some issuer networks.
        Set<String> result = new HashSet<>();
        for (int i = 0; i < data.supportedNetworks.length; i++) {
            String network = networks.get(data.supportedNetworks[i]);
            if (network != null) result.add(network);
        }
        return result;
    }

    /**
     * @return A set of card types (e.g., CardType.DEBIT, CardType.PREPAID)
     *         accepted by "basic-card" method.
     */
    public static Set<Integer> convertBasicCardToTypes(PaymentMethodData data) {
        Set<Integer> result = new HashSet<>();
        result.add(CardType.UNKNOWN);

        Map<Integer, Integer> cardTypes = getCardTypes();
        if (!isBasicCardTypeSpecified(data)) {
            // Merchant website supports all card types.
            result.addAll(cardTypes.values());
        } else {
            // Merchant website supports some card types.
            for (int i = 0; i < data.supportedTypes.length; i++) {
                Integer cardType = cardTypes.get(data.supportedTypes[i]);
                if (cardType != null) result.add(cardType);
            }
        }

        return result;
    }

    /** @return True if supported card type is specified in data for "basic-card" method. */
    public static boolean isBasicCardTypeSpecified(PaymentMethodData data) {
        return data != null && data.supportedTypes != null && data.supportedTypes.length != 0;
    }

    /** @return True if supported card network is specified in data for "basic-card" method. */
    public static boolean isBasicCardNetworkSpecified(PaymentMethodData data) {
        return data != null && data.supportedNetworks != null && data.supportedNetworks.length != 0;
    }

    /** @return a complete map of BasicCardNetworks to strings. */
    public static Map<Integer, String> getNetworks() {
        Map<Integer, String> networks = new HashMap<>();
        networks.put(BasicCardNetwork.AMEX, "amex");
        networks.put(BasicCardNetwork.DINERS, "diners");
        networks.put(BasicCardNetwork.DISCOVER, "discover");
        networks.put(BasicCardNetwork.JCB, "jcb");
        networks.put(BasicCardNetwork.MASTERCARD, "mastercard");
        networks.put(BasicCardNetwork.MIR, "mir");
        networks.put(BasicCardNetwork.UNIONPAY, "unionpay");
        networks.put(BasicCardNetwork.VISA, "visa");
        return networks;
    }

    private static Map<Integer, Integer> getCardTypes() {
        Map<Integer, Integer> cardTypes = new HashMap<>();
        cardTypes.put(BasicCardType.CREDIT, CardType.CREDIT);
        cardTypes.put(BasicCardType.DEBIT, CardType.DEBIT);
        cardTypes.put(BasicCardType.PREPAID, CardType.PREPAID);
        return cardTypes;
    }

    private BasicCardUtils() {}
}