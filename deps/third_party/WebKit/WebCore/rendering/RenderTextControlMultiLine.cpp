/**
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/) 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderTextControlMultiLine.h"

#include "EventNames.h"
#include "Frame.h"
#include "HitTestResult.h"
#include "HTMLTextAreaElement.h"

namespace WebCore {

RenderTextControlMultiLine::RenderTextControlMultiLine(Node* node)
    : RenderTextControl(node)
{
}

RenderTextControlMultiLine::~RenderTextControlMultiLine()
{
    if (node())
        static_cast<HTMLTextAreaElement*>(node())->rendererWillBeDestroyed();
}

void RenderTextControlMultiLine::subtreeHasChanged()
{
    RenderTextControl::subtreeHasChanged();
    formControlElement()->setValueMatchesRenderer(false);

    if (!node()->focused())
        return;

    if (Frame* frame = document()->frame())
        frame->textDidChangeInTextArea(static_cast<Element*>(node()));
}

void RenderTextControlMultiLine::layout()
{
    int oldHeight = height();
    calcHeight();

    int oldWidth = width();
    calcWidth();

    bool relayoutChildren = oldHeight != height() || oldWidth != width();
    RenderBox* innerTextRenderer = innerTextElement()->renderBox();

    // Set the text block height
    int desiredHeight = textBlockHeight();
    if (desiredHeight != innerTextRenderer->height())
        relayoutChildren = true;

    int scrollbarSize = 0;
    if (style()->overflowY() != OHIDDEN)
        scrollbarSize = scrollbarThickness();

    // Set the text block width
    int desiredWidth = textBlockWidth() - scrollbarSize;
    if (style()->htmlHacks())
        // Matches width in IE quirksmode. We can't just remove the CSS padding in 
        // quirks.css because then text will wrap differently than in IE.
        desiredWidth -= 2;
    if (desiredWidth != innerTextRenderer->width())
        relayoutChildren = true;
    innerTextRenderer->style()->setWidth(Length(desiredWidth, Fixed));

    RenderBlock::layoutBlock(relayoutChildren);
}

bool RenderTextControlMultiLine::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
    if (!RenderTextControl::nodeAtPoint(request, result, x, y, tx, ty, hitTestAction))
        return false;

    // Don't check if result.innerNode() is our node().
    // Otherwise this would break layout test:
    // LayoutTests/fast/forms/textarea-scrolled-endline-caret.html.
    hitInnerTextBlock(result, x, y, tx, ty);
    return true;
}

void RenderTextControlMultiLine::forwardEvent(Event* event)
{
    RenderTextControl::forwardEvent(event);
}

int RenderTextControlMultiLine::preferredContentWidth(float charWidth) const
{
    int factor = static_cast<HTMLTextAreaElement*>(node())->cols();
    int scrollbarSize = 0;
    if (style()->overflowY() != OHIDDEN)
        scrollbarSize = scrollbarThickness();
    return static_cast<int>(ceilf(charWidth * factor)) + scrollbarSize;
}

void RenderTextControlMultiLine::adjustControlHeightBasedOnLineHeight(int lineHeight)
{
    setHeight(height() + lineHeight * static_cast<HTMLTextAreaElement*>(node())->rows());
}

int RenderTextControlMultiLine::baselinePosition(bool, bool) const
{
    return height() + marginTop() + marginBottom();
}

int RenderTextControlMultiLine::scrollHeight() const
{
    // Matches IE, where the scrollHeight is the height of the text.
    int numLines = 0;

    RenderBlock* renderer = static_cast<RenderBlock*>(innerTextElement()->renderer());
    for (RootInlineBox* root = renderer->firstRootBox(); root; root = root->nextRootBox()) {
        numLines++;
    }

    if (!numLines)
        numLines = 1;

    return lineHeight(true, true) * numLines + paddingTop() + paddingBottom();
}

void RenderTextControlMultiLine::updateFromElement()
{
    createSubtreeIfNeeded(0);
    RenderTextControl::updateFromElement();

    setInnerTextValue(static_cast<HTMLTextAreaElement*>(node())->value());
}

void RenderTextControlMultiLine::cacheSelection(int start, int end)
{
    static_cast<HTMLTextAreaElement*>(node())->cacheSelection(start, end);
}

PassRefPtr<RenderStyle> RenderTextControlMultiLine::createInnerTextStyle(const RenderStyle* startStyle) const
{
    RefPtr<RenderStyle> textBlockStyle = RenderStyle::create();
    textBlockStyle->inheritFrom(startStyle);

    adjustInnerTextStyle(startStyle, textBlockStyle.get());

    // The inner text block should not ever have scrollbars.
    textBlockStyle->setOverflowX(OVISIBLE);
    textBlockStyle->setOverflowY(OVISIBLE);

    // Set word wrap property based on wrap attribute.
    if (static_cast<HTMLTextAreaElement*>(node())->shouldWrapText()) {
        textBlockStyle->setWhiteSpace(PRE_WRAP);
        textBlockStyle->setWordWrap(BreakWordWrap);
    } else {
        textBlockStyle->setWhiteSpace(PRE);
        textBlockStyle->setWordWrap(NormalWordWrap);
    }

    textBlockStyle->setDisplay(BLOCK);

    return textBlockStyle.release();
}

}
