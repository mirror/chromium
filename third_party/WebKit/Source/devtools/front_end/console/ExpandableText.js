// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Console.ExpandableText = class {
  /**
   * @param {string} text
   * @return {!DocumentFragment}
   */
  static createFragment(text) {
    var fragment = createDocumentFragment();
    fragment.textContent = text.slice(0, Console.ExpandableText.MaxLength);

    var hiddenText = text.slice(Console.ExpandableText.MaxLength);
    var expandButton = fragment.createChild('span', 'console-expand-button');
    expandButton.title = Common.UIString('Expand truncated content (%s more characters)', hiddenText.length);

    expandButton.addEventListener('click', () => {
      if (expandButton.parentElement)
        expandButton.parentElement.insertBefore(createTextNode(hiddenText), expandButton);
      expandButton.remove();
    });

    return fragment;
  }
};

/**
 * @const
 * @type {number}
 */
Console.ExpandableText.MaxLength = 10000;
