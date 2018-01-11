// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import android.text.TextUtils;

import org.chromium.base.CollectionUtil;
import org.chromium.base.VisibleForTesting;

import java.util.HashSet;
import java.util.Locale;

/**
 * Implements a simple first-cut heuristic for whether a Tap is on an entity or not.
 * This is intended to be a proof-of-concept that entities are worth tapping upon.
 * This implementation only recognizes one simple pattern that we recognize as a proper noun: two
 * camel-case words that are not at the beginning of a sentence, in a page that we think is in a
 * Roman language that uses upper-case for proper nouns.
 * <p>
 * This is not a robust implementation -- it's only really suitable as a strong-positive signal
 * that can sometimes be extracted from the page content.  Future implementations could use CLD or
 * some translate infrastructure to determine if the page is really in a language that uses
 * camel-case for proper nouns (e.g. exclude German because it capitalizes all nouns).  Leveraging
 * an on-device entity recognizer is another natural extension to this idea.
 * <p>
 * The current algorithm is designed to have relatively high precision at the expense of very low
 * recall (lots of false-negatives, but patterns that are "recognized" should usually be entities).
 * <p>
 * We implement suppression, but only really apply that for testing and interactive demo purposes.
 */
class ContextualSearchEntityHeuristic extends ContextualSearchHeuristic {
    private static final int INVALID_OFFSET = ContextualSearchContext.INVALID_OFFSET;

    // Ordered by popularity, and vetted with a simple web search for capitalization rules.
    // Do not add German!
    private static final HashSet<String> ROMAN_CASE_SENSITIVE_COUNTRIES =
            CollectionUtil.newHashSet("es", // Spanish
                    "en", // English,
                    "pt", // Portuguese
                    "ru", // Russian
                    "fr", // French,
                    "it" // Italian
                    );

    private final boolean mIsSuppressionEnabled;
    private final boolean mIsConditionSatisfied;
    private final boolean mIsContextUppercaseForProperNouns;
    private final boolean mIsProbablyProperNoun;

    /**
     * Constructs a heuristic to determine if the current Tap looks like it was on a name or not.
     * @param contextualSearchContext The current {@link ContextualSearchContext} so we can figure
     *        out the words around what has been tapped.
     */
    ContextualSearchEntityHeuristic(ContextualSearchContext contextualSearchContext) {
        this(contextualSearchContext, ContextualSearchFieldTrial.isNotAnEntitySuppressionEnabled());
    }

    /**
     * Constructs an instance for testing.
     */
    static ContextualSearchEntityHeuristic testInstance(
            ContextualSearchContext contextualSearchContext, boolean isEnabled) {
        return new ContextualSearchEntityHeuristic(contextualSearchContext, isEnabled);
    }

    /**
     * Constructs an instance of a {@link ContextualSearchHeuristic} that provides a signal for a
     * tap that is probably on a proper noun for a user of a capitalizing language.
     * @param contextualSearchContext The current {@link ContextualSearchContext} so we can figure
     *                                out the words around what has been tapped.
     * @param isEnabled Whether or not to enable suppression.
     */
    private ContextualSearchEntityHeuristic(
            ContextualSearchContext contextualSearchContext, boolean isEnabled) {
        mIsSuppressionEnabled = isEnabled;
        mIsContextUppercaseForProperNouns =
                isContextUppercaseForProperNouns(contextualSearchContext);
        mIsProbablyProperNoun = mIsContextUppercaseForProperNouns
                && isTapOnTwoCamelCaseWordsMidSentence(contextualSearchContext);
        mIsConditionSatisfied = !mIsProbablyProperNoun;
    }

    @Override
    protected boolean isConditionSatisfiedAndEnabled() {
        return mIsSuppressionEnabled && mIsConditionSatisfied;
    }

    @Override
    protected void logResultsSeen(boolean wasSearchContentViewSeen, boolean wasActivatedByTap) {
        if (wasActivatedByTap) {
            ContextualSearchUma.logTapOnEntitySeen(
                    wasSearchContentViewSeen, !mIsConditionSatisfied);
        }
    }

    @Override
    protected void logRankerTapSuppression(ContextualSearchRankerLogger logger) {
        logger.logFeature(ContextualSearchRankerLogger.Feature.IS_ENTITY, mIsProbablyProperNoun);
        logger.logFeature(ContextualSearchRankerLogger.Feature.IS_ENTITY_ELIGIBLE,
                mIsContextUppercaseForProperNouns);
    }

    @VisibleForTesting
    protected boolean isProbablyEntityBasedOnCase() {
        return mIsProbablyProperNoun;
    }

    /**
     * @return Whether the tap is on a proper noun.
     */
    private boolean isTapOnTwoCamelCaseWordsMidSentence(
            ContextualSearchContext contextualSearchContext) {
        // Check common cases that we can quickly reject.
        String tappedWord = contextualSearchContext.getWordTapped();
        if (TextUtils.isEmpty(tappedWord)
                || !isCapitalizedCamelCase(tappedWord)) {
            return false;
        }

        // Check if the tapped word is the first word of a two-word entity.
        if (isTwoWordCamelCaseSpaceSeparatedEntity(contextualSearchContext, tappedWord,
                    contextualSearchContext.getWordTappedOffset(),
                    contextualSearchContext.getWordFollowingTap(),
                    contextualSearchContext.getWordFollowingTapOffset())) {
            return true;
        }

        // Otherwise the tapped word needs to be the second word of a two-word entity.
        return isTwoWordCamelCaseSpaceSeparatedEntity(contextualSearchContext,
                    contextualSearchContext.getWordPreviousToTap(),
                    contextualSearchContext.getWordPreviousToTapOffset(), tappedWord,
                    contextualSearchContext.getWordTappedOffset());
    }

    /**
     * Considers whether the given words at the given offsets are probably a two-word entity based
     * on our simple rules for capitalization: both camel-case and separated by just a single space
     * and not following whitespace that precedes a character that commonly ends a sentence.
     * @param contextualSearchContext The {@link ContextualSearchContext} that the words came from.
     * @param firstWord The first word of a possible entity.
     * @param firstWordOffset The offset of the first word.
     * @param secondWord The second word of a possible entity.
     * @param secondWordOffset The offset of the second word.
     * @return Whether the words are probably an entity.
     */
    private boolean isTwoWordCamelCaseSpaceSeparatedEntity(
            ContextualSearchContext contextualSearchContext, String firstWord, int firstWordOffset,
            String secondWord, int secondWordOffset) {
        if (!isCapitalizedCamelCase(firstWord) || !isCapitalizedCamelCase(secondWord)) return false;

        if (!isWhitespaceThatDoesntEndASentence(contextualSearchContext, firstWordOffset - 1)) {
            return false;
        }

        // Check that there's just one separator character.
        if (firstWordOffset + firstWord.length() + 1 != secondWordOffset) return false;

        // Check that it's a space.
        return isWhitespaceAtOffset(contextualSearchContext, secondWordOffset - 1);
    }

    /**
     * Scans previous characters starting from the given offset in the given context.
     * @return Whether there is whitespace that doesn't end a sentence at the given offset.
     */
    private boolean isWhitespaceThatDoesntEndASentence(
            ContextualSearchContext contextualSearchContext, int offset) {
        int whitespaceScanOffset = offset;
        while (whitespaceScanOffset > 0
                && isWhitespaceAtOffset(contextualSearchContext, whitespaceScanOffset)) {
            --whitespaceScanOffset;
        }
        return whitespaceScanOffset > 0
                && !isEndOfSentenceChar(contextualSearchContext.getSurroundingText().charAt(
                           whitespaceScanOffset));
    }

    /**
     * @return Whether the given character is often used to end a sentence (with high precision, low
     *         recall).
     */
    private boolean isEndOfSentenceChar(char c) {
        return c == '.' || c == '?' || c == '!' || c == ':';
    }

    /**
     * @return {@code true} if the word starts with an upper-case letter and has at least one letter
     *         that is not considered upper-case.
     */
    private boolean isCapitalizedCamelCase(String word) {
        if (TextUtils.isEmpty(word) || word.length() <= 1) return false;

        Character firstChar = word.charAt(0);
        return Character.isUpperCase(firstChar)
                && !word.toUpperCase(Locale.getDefault()).equals(word);
    }

    /**
     * @return Whether the surrounding text has a whitespace character at the given offset.
     */
    private boolean isWhitespaceAtOffset(
            ContextualSearchContext contextualSearchContext, int offset) {
        if (offset == INVALID_OFFSET) return false;

        Character charAtOffset = contextualSearchContext.getSurroundingText().charAt(offset);
        return Character.isWhitespace(charAtOffset);
    }

    /**
     * Detects the language of the Context and returns whether that language uses upper-case for
     * proper nouns.
     * @return Whether the language of the Context uses capitalization for proper nouns.
     */
    private boolean isContextUppercaseForProperNouns(
            ContextualSearchContext contextualSearchContext) {
        return ROMAN_CASE_SENSITIVE_COUNTRIES.contains(
                contextualSearchContext.getDetectedLanguage());
    }
}
