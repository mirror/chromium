// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.contextualsearch;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.mojo.bindings.DeserializationException;
import org.chromium.blink.mojom.UnhandledTapNotifierService;
import org.chromium.blink.mojom.UnhandledTapInfo;

import org.chromium.mojo.system.MojoException;
import org.chromium.services.service_manager.InterfaceFactory;
import org.chromium.content_public.browser.RenderFrameHost;

/**
 * Implementation of mojo UnhandledTapNotifierServce.
 * Receives notification that an unhandled tap was recieved by Blink.
 */
public class UnhandledTapNotifierServiceImpl implements UnhandledTapNotifierService {
    // TODO(donnd): needed??
    private final RenderFrameHost mRenderFrameHost;

    public UnhandledTapNotifierServiceImpl(RenderFrameHost renderFrameHost) {
        System.out.println("ctxs java mojo UnhandledTapNotifierServiceImpl constructor!");
        assert renderFrameHost != null;

        mRenderFrameHost = renderFrameHost;
    }

    @Override
    public void showUnhandledTapUiIfNeeded(UnhandledTapInfo unhandledTapInfo) {
        System.out.println("ctxs java mojo ShowUnhandledTapUIIfNeeded !!!");
    }

    // TODO(donnd): remove both of these?
    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {
        close();
    }

    /**
     * A factory class to register the UnhandledTapNotifierService interface.
     */
    public static class Factory implements InterfaceFactory<UnhandledTapNotifierService> {
        private final RenderFrameHost mRenderFrameHost;

        public Factory(RenderFrameHost renderFrameHost) {
            System.out.println("ctxs java mojo factory constructor!");
            mRenderFrameHost = renderFrameHost;
        }

        @Override
        public UnhandledTapNotifierService createImpl() {
            System.out.println("ctxs java mojo factory service impl creation!");
            assert mRenderFrameHost != null;
            return new UnhandledTapNotifierServiceImpl(mRenderFrameHost);
        }
    }
}
