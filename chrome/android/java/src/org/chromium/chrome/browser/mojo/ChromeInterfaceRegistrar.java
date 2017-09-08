// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.mojo;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.contextualsearch.UnhandledTapNotifierServiceImpl;
import org.chromium.chrome.browser.installedapp.InstalledAppProviderFactory;
import org.chromium.chrome.browser.payments.PaymentRequestFactory;
import org.chromium.chrome.browser.webshare.ShareServiceImplementationFactory;
import org.chromium.content_public.browser.InterfaceRegistrar;
import org.chromium.content_public.browser.RenderFrameHost;
import org.chromium.content_public.browser.WebContents;
import org.chromium.installedapp.mojom.InstalledAppProvider;
import org.chromium.payments.mojom.PaymentRequest;
import org.chromium.services.service_manager.InterfaceRegistry;
import org.chromium.webshare.mojom.ShareService;

// TODO(donnd): mojo Needed even for Java?
import org.chromium.blink.mojom.UnhandledTapNotifierService;

@SuppressWarnings("MultipleTopLevelClassesInFile")

/** Registers mojo interface implementations exposed to C++ code at the Chrome layer. */
class ChromeInterfaceRegistrar {
    @CalledByNative
    private static void registerMojoInterfaces() {
        System.out.println("ctxs registerMojoInterfaces...");
        InterfaceRegistrar.Registry.addWebContentsRegistrar(
                new ChromeWebContentsInterfaceRegistrar());
        InterfaceRegistrar.Registry.addRenderFrameHostRegistrar(
                new ChromeRenderFrameHostInterfaceRegistrar());
        // HACK Just install us now!!!!!!!!!!!!!!!
       // InterfaceRegistrar.Registry.applyRenderFrameHostRegistrars(interfaceRegistry, renderFrameHost);
    }

    private static class ChromeWebContentsInterfaceRegistrar
            implements InterfaceRegistrar<WebContents> {
        @Override
        public void registerInterfaces(InterfaceRegistry registry, final WebContents webContents) {
            System.out.println("ctxs WebContents registry addInterface!");
            registry.addInterface(
                    ShareService.MANAGER, new ShareServiceImplementationFactory(webContents));
        }
    }

    private static class ChromeRenderFrameHostInterfaceRegistrar
            implements InterfaceRegistrar<RenderFrameHost> {
        @Override
        public void registerInterfaces(
                InterfaceRegistry registry, final RenderFrameHost renderFrameHost) {
            // TODO(donnd): mojo Is this is the right place?
            System.out.println("ctxs RenderFrameHost registry addInterface!");
            registry.addInterface(
                     UnhandledTapNotifierService.MANAGER, 
                     new UnhandledTapNotifierServiceImpl.Factory(renderFrameHost));
            registry.addInterface(
                    PaymentRequest.MANAGER, new PaymentRequestFactory(renderFrameHost));
            registry.addInterface(
                    InstalledAppProvider.MANAGER, new InstalledAppProviderFactory(renderFrameHost));
        }
    }
}
