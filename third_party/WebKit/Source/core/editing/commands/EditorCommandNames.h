// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EditorCommandNames_h
#define EditorCommandNames_h

namespace blink {

// Must be ordered in a case-folding manner for binary search. Covered by unit
// tests in EditingCommandTest.cpp (not able to use static_assert)
#define FOR_EACH_BLINK_EDITING_COMMAND_NAME(V)    \
  V(kAlignCenter)                                  \
  V(kAlignJustified)                               \
  V(kAlignLeft)                                    \
  V(kAlignRight)                                   \
  V(kBackColor)                                    \
  V(kBackwardDelete)                               \
  V(kBold)                                         \
  V(kCopy)                                         \
  V(kCreateLink)                                   \
  V(kCut)                                          \
  V(kDefaultParagraphSeparator)                    \
  V(kDelete)                                       \
  V(kDeleteBackward)                               \
  V(kDeleteBackwardByDecomposingPreviousCharacter) \
  V(kDeleteForward)                                \
  V(kDeleteToBeginningOfLine)                      \
  V(kDeleteToBeginningOfParagraph)                 \
  V(kDeleteToEndOfLine)                            \
  V(kDeleteToEndOfParagraph)                       \
  V(kDeleteToMark)                                 \
  V(kDeleteWordBackward)                           \
  V(kDeleteWordForward)                            \
  V(kFindString)                                   \
  V(kFontName)                                     \
  V(kFontSize)                                     \
  V(kFontSizeDelta)                                \
  V(kForeColor)                                    \
  V(kFormatBlock)                                  \
  V(kForwardDelete)                                \
  V(kHiliteColor)                                  \
  V(kIgnoreSpelling)                               \
  V(kIndent)                                       \
  V(kInsertBacktab)                                \
  V(kInsertHorizontalRule)                         \
  V(kInsertHTML)                                   \
  V(kInsertImage)                                  \
  V(kInsertLineBreak)                              \
  V(kInsertNewline)                                \
  V(kInsertNewlineInQuotedContent)                 \
  V(kInsertOrderedList)                            \
  V(kInsertParagraph)                              \
  V(kInsertTab)                                    \
  V(kInsertText)                                   \
  V(kInsertUnorderedList)                          \
  V(kItalic)                                       \
  V(kJustifyCenter)                                \
  V(kJustifyFull)                                  \
  V(kJustifyLeft)                                  \
  V(kJustifyNone)                                  \
  V(kJustifyRight)                                 \
  V(kMakeTextWritingDirectionLeftToRight)          \
  V(kMakeTextWritingDirectionNatural)              \
  V(kMakeTextWritingDirectionRightToLeft)          \
  V(kMoveBackward)                                 \
  V(kMoveBackwardAndModifySelection)               \
  V(kMoveDown)                                     \
  V(kMoveDownAndModifySelection)                   \
  V(kMoveForward)                                  \
  V(kMoveForwardAndModifySelection)                \
  V(kMoveLeft)                                     \
  V(kMoveLeftAndModifySelection)                   \
  V(kMovePageDown)                                 \
  V(kMovePageDownAndModifySelection)               \
  V(kMovePageUp)                                   \
  V(kMovePageUpAndModifySelection)                 \
  V(kMoveParagraphBackward)                        \
  V(kMoveParagraphBackwardAndModifySelection)      \
  V(kMoveParagraphForward)                         \
  V(kMoveParagraphForwardAndModifySelection)       \
  V(kMoveRight)                                    \
  V(kMoveRightAndModifySelection)                  \
  V(kMoveToBeginningOfDocument)                    \
  V(kMoveToBeginningOfDocumentAndModifySelection)  \
  V(kMoveToBeginningOfLine)                        \
  V(kMoveToBeginningOfLineAndModifySelection)      \
  V(kMoveToBeginningOfParagraph)                   \
  V(kMoveToBeginningOfParagraphAndModifySelection) \
  V(kMoveToBeginningOfSentence)                    \
  V(kMoveToBeginningOfSentenceAndModifySelection)  \
  V(kMoveToEndOfDocument)                          \
  V(kMoveToEndOfDocumentAndModifySelection)        \
  V(kMoveToEndOfLine)                              \
  V(kMoveToEndOfLineAndModifySelection)            \
  V(kMoveToEndOfParagraph)                         \
  V(kMoveToEndOfParagraphAndModifySelection)       \
  V(kMoveToEndOfSentence)                          \
  V(kMoveToEndOfSentenceAndModifySelection)        \
  V(kMoveToLeftEndOfLine)                          \
  V(kMoveToLeftEndOfLineAndModifySelection)        \
  V(kMoveToRightEndOfLine)                         \
  V(kMoveToRightEndOfLineAndModifySelection)       \
  V(kMoveUp)                                       \
  V(kMoveUpAndModifySelection)                     \
  V(kMoveWordBackward)                             \
  V(kMoveWordBackwardAndModifySelection)           \
  V(kMoveWordForward)                              \
  V(kMoveWordForwardAndModifySelection)            \
  V(kMoveWordLeft)                                 \
  V(kMoveWordLeftAndModifySelection)               \
  V(kMoveWordRight)                                \
  V(kMoveWordRightAndModifySelection)              \
  V(kOutdent)                                      \
  V(kOverWrite)                                    \
  V(kPaste)                                        \
  V(kPasteAndMatchStyle)                           \
  V(kPasteGlobalSelection)                         \
  V(kPrint)                                        \
  V(kRedo)                                         \
  V(kRemoveFormat)                                 \
  V(kScrollLineDown)                               \
  V(kScrollLineUp)                                 \
  V(kScrollPageBackward)                           \
  V(kScrollPageForward)                            \
  V(kScrollToBeginningOfDocument)                  \
  V(kScrollToEndOfDocument)                        \
  V(kSelectAll)                                    \
  V(kSelectLine)                                   \
  V(kSelectParagraph)                              \
  V(kSelectSentence)                               \
  V(kSelectToMark)                                 \
  V(kSelectWord)                                   \
  V(kSetMark)                                      \
  V(kStrikethrough)                                \
  V(kStyleWithCSS)                                 \
  V(kSubscript)                                    \
  V(kSuperscript)                                  \
  V(kSwapWithMark)                                 \
  V(kToggleBold)                                   \
  V(kToggleItalic)                                 \
  V(kToggleUnderline)                              \
  V(kTranspose)                                    \
  V(kUnderline)                                    \
  V(kUndo)                                         \
  V(kUnlink)                                       \
  V(kUnscript)                                     \
  V(kUnselect)                                     \
  V(kUseCSS)                                       \
  V(kYank)                                         \
  V(kYankAndSelect)

}  // namespace blink

#endif  // EditorCommandNames_h
