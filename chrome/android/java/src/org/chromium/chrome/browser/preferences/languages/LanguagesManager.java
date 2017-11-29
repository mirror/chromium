// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.preferences.languages;

import org.chromium.chrome.browser.preferences.PrefServiceBridge;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Manages languages details for languages settings.
 *
 * The LanguagesManager is responsible for fetching languages details from native.
 */
class LanguagesManager {
    /**
     * An Observer interfaces that allows other classes to know when native data is updated.
     */
    interface Observer {
        /**
         * Called when the foreground service is destroyed.
         */
        void onDataUpdated();
    }

    private static LanguagesManager sManager;

    private final PrefServiceBridge mPrefServiceBridge;
    private final Map<String, LanguageItem> mLanguagesMap;
    private final List<LanguageItem> mLanguageList;

    private Observer mObserver;

    private LanguagesManager() {
        mPrefServiceBridge = PrefServiceBridge.getInstance();

        // Get all language info from native.
        mLanguagesMap = new HashMap<>();
        mLanguageList = mPrefServiceBridge.getChromeLanguageList();
        for (LanguageItem item : mLanguageList) {
            mLanguagesMap.put(item.getCode(), item);
        }
    }

    private void notifyObserver() {
        if (mObserver != null) mObserver.onDataUpdated();
    }

    /**
     * Sets the observer tracking the user accept languages changes.
     */
    public void setObserver(Observer observer) {
        mObserver = observer;
    }

    /**
     * @return A list of LanguageItems for the current user's accept languages.
     */
    public List<LanguageItem> getUserAcceptLanguageItems() {
        // Always read the latest user accept language code list from native.
        List<String> codes = mPrefServiceBridge.getUserLanguageCodes();

        List<LanguageItem> results = new ArrayList<>();
        // Keep the same order as accept language codes.
        for (String code : codes) {
            LanguageItem item = mLanguagesMap.get(code);
            if (item != null) results.add(item);
        }
        return results;
    }

    /**
     * @return A list of LanguageItems, excluding the current user's accept languages.
     */
    public List<LanguageItem> getLanguageItemsForAdding() {
        // Always read the latest user accept language code list from native.
        List<String> codes = mPrefServiceBridge.getUserLanguageCodes();

        List<LanguageItem> results = new ArrayList<>();
        // Keep the same order as mLanguageList.
        for (LanguageItem item : mLanguageList) {
            if (!codes.contains(item.getCode())) {
                results.add(item);
            }
        }
        return results;
    }

    /**
     * Add a language to the current user's accept languages.
     * @param code The language code to remove.
     */
    public void addToAcceptLanguages(String code) {
        mPrefServiceBridge.updateUserAcceptLanguages(code, true /* is_add */);
        notifyObserver();
    }

    /**
     * Remove a language from the current user's accept languages.
     * @param code The language code to remove.
     */
    public void removeFromAcceptLanguages(String code) {
        mPrefServiceBridge.updateUserAcceptLanguages(code, false /* is_add */);
        notifyObserver();
    }

    /**
     * Get the static instance of ChromePreferenceManager if exists else create it.
     * @return the LanguagesManager singleton.
     */
    public static LanguagesManager getInstance() {
        if (sManager == null) sManager = new LanguagesManager();
        return sManager;
    }

    /**
     * Called to release unused resources.
     */
    public static void recycle() {
        sManager = null;
    }
}
