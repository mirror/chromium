// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

UI.Fragment.registerTemplate('x-report', ['title', 'subtitle', 'url', 'toolbar'], (element, params, children) => {
  // clang-format off
  return UI.Fragment.cached`
    <vbox style='background: white; flex: none'>
      <vbox style='flex: none; border-bottom: 1px solid rgb(230, 230, 230); padding: 12px 24px;'>
        <hbox style='font-size: 15px'>${params.title}</hbox>
        <hbox style='font-size: 12px; margin-top: 10px' class=${params.subtitle ? '' : 'hidden'}>${params.subtitle}</hbox>
        <hbox style='font-size: 12px; margin-top: 10px' class=${params.url ? '' : 'hidden'}>${params.url}</hbox>
        <hbox style='margin: 5px 0 -8px -8px;' class=${params.toolbar ? '' : 'hidden'}>${params.toolbar}</hbox>
      </vbox>
      <vbox>${children}</vbox>
    </vbox>
  `;
  // clang-format on
});

UI.Fragment.registerTemplate('x-report-section', ['title', 'toolbar'], (element, params, children) => {
  // clang-format off
  return UI.Fragment.cached`
    <vbox style='flex: none; padding: 12px; border-bottom: 1px solid rgb(230, 230, 230);'>
      <hbox style='align-items: center; margin-left: 18px'>
        <hbox style='flex: auto; font-weight: bold; color: #555; white-space: nowrap;
                     text-overflow: ellipsis; overflow: hidden;'>${params.title}</hbox>
        ${params.toolbar}
      </hbox>
      <vbox>${children}</vbox>
    </vbox>
  `;
  // clang-format on
});

UI.Fragment.registerTemplate('x-report-field', ['title'], (element, params, children) => {
  // clang-format off
  return UI.Fragment.cached`
    <hbox style='margin-top: 8px; line-height: 28px;'>
      <hbox style='flex: 0 0 128px; white-space: pre; justify-content: flex-end;
                   color: #888; padding: 0 6px;'>${params.title}</hbox>
      <div style='flex: auto; white-space: pre; padding: 0 6px;'>${children}</div>
    </hbox>
  `;
  // clang-format on
});
