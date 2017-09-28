/*
 * Copyright (C) 2006, 2007 Apple, Inc.  All rights reserved.
 * Copyright (C) 2012 Google, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/editing/Editor.h"

#include "core/editing/EditingUtilities.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/RelocatablePosition.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/iterators/TextIterator.h"
#include "core/events/KeyboardEvent.h"
#include "core/frame/LocalFrame.h"
#include "core/page/EditorClient.h"
#include "platform/text/UnicodeUtilities.h"
#include "public/platform/WebInputEvent.h"

namespace blink {

namespace {

// This method needs to match Android's KeyCharacterMap.sAccentToCombining.
UChar GetCombiningCharacterFromAccent(UChar32 accent_char) {
  switch (accent_char) {
    case 0x02cb:  // ACCENT_GRAVE
      return 0x0300;
    case 0x00b4:  // ACCENT_ACUTE
      return 0x0301;
    case 0x02c6:  // ACCENT_CIRCUMFLEX
      return 0x0302;
    case 0x02dc:  // ACCENT_TILDE
      return 0x0303;
    case 0x00af:  // ACCENT_MACRON
      return 0x0304;
    case 0x02d8:  // ACCENT_BREVE
      return 0x0306;
    case 0x02d9:  // ACCENT_DOT_ABOVE
      return 0x0307;
    case 0x00a8:  // ACCENT_UMLAUT
      return 0x0308;
    case 0x02c0:  // ACCENT_HOOK_ABOVE
      return 0x0309;
    case 0x02da:  // ACCENT_RING_ABOVE
      return 0x030a;
    case 0x02dd:  // ACCENT_DOUBLE_ACUTE
      return 0x030b;
    case 0x02c7:  // ACCENT_CARON
      return 0x030c;
    case 0x02c8:  // ACCENT_VERTICAL_LINE_ABOVE
      return 0x030d;
    case 0x02bb:  // ACCENT_TURNED_COMMA_ABOVE
      return 0x0312;
    case 0x1fbd:  // ACCENT_COMMA_ABOVE:
      return 0x0313;
    case 0x02bd:  // ACCENT_REVERSED_COMMA_ABOVE:
      return 0x0314;
    case 0x02bc:  // ACCENT_COMMA_ABOVE_RIGHT:
      return 0x0315;
    case 0x0027:  // ACCENT_HORN
      return 0x031b;
    case 0x002e:  // ACCENT_DOT_BELOW
      return 0x0323;
    case 0x00b8:  // ACCENT_CEDILLA
      return 0x0327;
    case 0x02db:  // ACCENT_OGONEK
      return 0x0328;
    case 0x02cc:  // ACCENT_VERTICAL_LINE_BELOW
      return 0x0329;
    case 0x02cd:  // ACCENT_MACRON_BELOW
      return 0x0331;
    case 0x002d:  // ACCENT_STROKE
      return 0x0335;

    case 0x0060:  // ACCENT_GRAVE_LEGACY
      return 0x0300;
    case 0x005e:  // ACCENT_CIRCUMFLEX_LEGACY
      return 0x0302;
    case 0x007e:  // ACCENT_TILDE_LEGACY
      return 0x0303;

    default:
      return 0;
  }
}

UChar32 GetCodePointAt(const String& text, size_t index) {
  UChar32 c;
  U16_GET(text, 0, index, text.length(), c);
  return c;
}

// If the accent char can be combined with key_code, returns the combined
// character. Otherwise returns 0.
// Logic based on Android's KeyBoardMap#getDeadChar().
UChar GetDeadChar(UChar accent, UChar32 key_code) {
  if (key_code == accent || key_code == ' ') {
    // The same dead character typed twice or a dead character followed by a
    // space should both produce the non-combining version of the combining
    // char. In this case we don't even need to compute the combining character.
    return accent;
  }

  UChar combining = GetCombiningCharacterFromAccent(accent);
  if (!combining)
    return 0;

  String dead_key;
  dead_key.append(static_cast<UChar>(key_code));
  dead_key.append(combining);

  Vector<UChar> dead_key_normalized;
  NormalizeCharactersIntoNFCForm(dead_key.Characters16(), dead_key.length(),
                                 dead_key_normalized);

  // Characters could not be combined.
  if (dead_key_normalized.size() > 1)
    return 0;

  return GetCodePointAt(String(dead_key_normalized), 0);
}

}  // anonymous namespace

bool Editor::HandleEditingKeyboardEvent(KeyboardEvent* evt) {
  const WebKeyboardEvent* key_event = evt->KeyEvent();
  // do not treat this as text input if it's a system key event
  if (!key_event || key_event->is_system_key)
    return false;

  String command_name = Behavior().InterpretKeyEvent(*evt);
  Command command = this->CreateCommand(command_name);

  if (key_event->GetType() == WebInputEvent::kRawKeyDown) {
    // WebKit doesn't have enough information about mode to decide how
    // commands that just insert text if executed via Editor should be treated,
    // so we leave it upon WebCore to either handle them immediately
    // (e.g. Tab that changes focus) or let a keypress event be generated
    // (e.g. Tab that inserts a Tab character, or Enter).
    if (command.IsTextInsertion() || command_name.IsEmpty())
      return false;
    return command.Execute(evt);
  }

  if (command.Execute(evt))
    return true;

  if (!Behavior().ShouldInsertCharacter(*evt) || !CanEdit())
    return false;

  const Element* const focused_element =
      frame_->GetDocument()->FocusedElement();
  if (!focused_element) {
    // We may lose focused element by |command.execute(evt)|.
    return false;
  }
  // We should not insert text at selection start if selection doesn't have
  // focus.
  if (!frame_->Selection().SelectionHasFocus())
    return false;

  // Return true to prevent default action. e.g. Space key scroll.
  if (DispatchBeforeInputInsertText(evt->target()->ToNode(), key_event->text) !=
      DispatchEventResult::kNotCanceled)
    return true;

  if (!Behavior().ShouldUseAndroidDeadKeyHandling())
    return InsertText(key_event->text, evt);

  return InsertTextHandlingAndroidDeadKeys(key_event->text, evt);
}

// Logic based on Android's QwertyKeyListener#onKeyDown().
bool Editor::InsertTextHandlingAndroidDeadKeys(
    const String& text,
    KeyboardEvent* triggering_event) {
  bool is_dead_key = (triggering_event->key() == "Dead");
  UChar composed = 0;

  GetFrame().GetDocument()->UpdateStyleAndLayoutIgnorePendingStylesheets();
  const SelectionInDOMTree& selection =
      GetFrameSelection().GetSelectionInDOMTree();
  if (active_dead_key_range_ &&
      active_dead_key_range_->StartPosition().IsEquivalent(
          selection.ComputeStartPosition()) &&
      active_dead_key_range_->EndPosition().IsEquivalent(
          selection.ComputeEndPosition())) {
    if (PlainText(EphemeralRange(selection.ComputeStartPosition(),
                                 selection.ComputeEndPosition()))
            .length() == 1) {
      const String& accent_string = PlainText(EphemeralRange(
          selection.ComputeStartPosition(), selection.ComputeEndPosition()));
      UChar32 accent = GetCodePointAt(accent_string, 0);
      composed = GetDeadChar(accent, triggering_event->keyCode());

      if (composed)
        is_dead_key = false;
    }

    if (!composed) {
      active_dead_key_range_ = nullptr;
      GetFrameSelection().SetSelection(
          SelectionInDOMTree::Builder()
              .Collapse(selection.ComputeEndPosition())
              .Build());
    }
  }

  if (!is_dead_key) {
    if (composed)
      return InsertText(String(&composed, 1), triggering_event);
    return InsertText(text, triggering_event);
  }

  // Handling a dead key.
  const SelectionInDOMTree& selection_before_insert =
      GetFrameSelection().GetSelectionInDOMTree();
  const RelocatablePosition& old_selection_start =
      RelocatablePosition(selection_before_insert.ComputeStartPosition());

  if (!InsertText(text, triggering_event))
    return false;

  const SelectionInDOMTree& selection_after_insert =
      GetFrameSelection().GetSelectionInDOMTree();
  const Position& new_selection_end =
      selection_after_insert.ComputeEndPosition();

  if (old_selection_start.GetPosition() < new_selection_end) {
    const EphemeralRange active_dead_key_ephemeral_range =
        EphemeralRange(old_selection_start.GetPosition(), new_selection_end);
    GetFrameSelection().SetSelection(
        SelectionInDOMTree::Builder()
            .Collapse(old_selection_start.GetPosition())
            .Extend(new_selection_end)
            .Build());
    active_dead_key_range_ =
        Range::Create(*GetFrame().GetDocument(),
                      old_selection_start.GetPosition(), new_selection_end);
  }

  return true;
}

void Editor::HandleKeyboardEvent(KeyboardEvent* evt) {
  // Give the embedder a chance to handle the keyboard event.
  if (Client().HandleKeyboardEvent(frame_) || HandleEditingKeyboardEvent(evt))
    evt->SetDefaultHandled();
}

}  // namespace blink
