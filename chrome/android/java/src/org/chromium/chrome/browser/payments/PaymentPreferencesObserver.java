// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.payments;

import org.chromium.base.ThreadUtils;
import org.chromium.chrome.browser.autofill.PersonalDataManager.CreditCard;

import java.util.ArrayList;
import java.util.List;

/** The class to observer operations in 'settings/Autofill and payment'. */
public class PaymentPreferencesObserver {
    private static final List<Observer> sObservers = new ArrayList<>();
    private static PaymentPreferencesObserver sPaymentPreferencesObserver;

    public static PaymentPreferencesObserver getInstance() {
        ThreadUtils.assertOnUiThread();

        if (sPaymentPreferencesObserver == null) {
            sPaymentPreferencesObserver = new PaymentPreferencesObserver();
        }
        return sPaymentPreferencesObserver;
    }

    // Avoid accident instantiation.
    private PaymentPreferencesObserver() {}

    public interface Observer {
        void onAddressUpdated(AutofillAddress address);
        void onAddressDeleted(String guid);
        void onCreditCardUpdated(CreditCard card);
        void onCreditCardDeleted(String guid);
    }

    public void registerObserver(Observer observer) {
        sObservers.add(observer);
    }

    public void unregisterObserver(Observer observer) {
        sObservers.remove(observer);
    }

    public void notifyOnAddressUpdated(AutofillAddress address) {
        for (Observer observer : sObservers) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    observer.onAddressUpdated(address);
                }
            });
        }
    }

    public void notifyOnAddressDeleted(String guid) {
        for (Observer observer : sObservers) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    observer.onAddressDeleted(guid);
                }
            });
        }
    }

    public void notifyOnCreditCardUpdated(CreditCard card) {
        for (Observer observer : sObservers) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    observer.onCreditCardUpdated(card);
                }
            });
        }
    }

    public void notifyOnCreditCardDeleted(String guid) {
        for (Observer observer : sObservers) {
            ThreadUtils.postOnUiThread(new Runnable() {
                @Override
                public void run() {
                    observer.onCreditCardDeleted(guid);
                }
            });
        }
    }
}