// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.assistant.assist_actions;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.TextView;

import org.json.JSONObject;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.assistant.page_actions.PageAction;
import org.chromium.chrome.browser.assistant.page_actions.PageActionList;

class AssistActionMessage extends AssistAction implements View.OnClickListener {
    public static AssistActionMessage parse(JSONObject json) {
        if (json == null) return null;
        String message = json.optString("message");
        if (message == null || "".equals(message)) return null;
        AssistActionMessage action = new AssistActionMessage(message);
        action.mOnTap = PageActionList.parse(json.optJSONObject("on_tap"));
        return action;
    }

    private final String mMessage;
    private PageAction mOnTap;
    private TextView mTextView;
    private ImageButton mCloseButton;

    private AssistActionMessage(String message) {
        mMessage = message;
    }

    @Override
    public void buildView(Context context, FrameLayout bottomBar) {
        LayoutInflater inflater = LayoutInflater.from(context);
        View view = inflater.inflate(R.layout.assistant_message, bottomBar);

        mTextView = (TextView) view.findViewById(R.id.message);
        mTextView.setText(mMessage);
        mTextView.setOnClickListener(this);

        mCloseButton = (ImageButton) view.findViewById(R.id.close_button);
        mCloseButton.setOnClickListener(this);
    }

    @Override
    public void destroyView() {
        if (mTextView != null) {
            mTextView.setOnClickListener(null);
            mTextView = null;
        }
        if (mCloseButton != null) {
            mCloseButton.setOnClickListener(null);
            mCloseButton = null;
        }
        if (mOnTap != null) {
            mOnTap.cancel();
            mOnTap = null;
        }
    }

    @Override
    public void onClick(View view) {
        if (view == mTextView) {
            if (mOnTap != null) {
                mOnTap.doPageAction(getDevtools(), (succeeded) -> {
                    onActionResult(succeeded ? ActionResult.ACTION_OK : ActionResult.ACTION_FAILED);
                });
            }
        } else if (view == mCloseButton) {
            onActionResult(ActionResult.CLOSE);
        }
    }
}
