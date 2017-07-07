// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.androidoverlay;

import android.annotation.TargetApi;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Build;
import android.support.test.filters.MediumTest;
import android.view.Surface;

import org.junit.Assert;

import org.chromium.base.ThreadUtils;
import org.chromium.base.test.util.Feature;
import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.content.browser.ContentViewCore;

import java.util.concurrent.Callable;

/**
 * Pixel tests for DialogOverlayImpl.  These use UiAutomation, so they only run in JB or above.
 */
@MinAndroidSdkLevel(Build.VERSION_CODES.JELLY_BEAN_MR2)
@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class DialogOverlayImplPixelTest extends DialogOverlayImplTestBase {
    // CSS coordinates of a div that we'll try to cover with an overlay.
    private static final int DIV_X_CSS = 10;
    private static final int DIV_Y_CSS = 20;
    private static final int DIV_WIDTH_CSS = 300;
    private static final int DIV_HEIGHT_CSS = 200;

    // Provide a solid-color div that's positioned / sized by DIV_*_CSS.
    private static final String TEST_PAGE_STYLE = "<style>"
            + "div {"
            + "left: " + DIV_X_CSS + "px;"
            + "top: " + DIV_Y_CSS + "px;"
            + "width: " + DIV_WIDTH_CSS + "px;"
            + "height: " + DIV_HEIGHT_CSS + "px;"
            + "position: absolute;"
            + "background: red;"
            + "}"
            + "</style>";
    private static final String TEST_PAGE_DATA_URL = UrlUtils.encodeHtmlDataUri(
            "<html>" + TEST_PAGE_STYLE + "<body><div></div></body></html>");

    // DIV_*_CSS converted to screen pixels.
    int mDivXPx;
    int mDivYPx;
    int mDivWidthPx;
    int mDivHeightPx;

    // Maximum status bar height that we'll work with.  This just lets us restruct the area of the
    // screenshot that we inspect, since it's slow.
    private static final int mStatusBarMaxHeightPx = 200;

    // Area of interest that contains the div, since the whole image is big.
    Rect mAreaOfInterestPx;

    // Screenshot of the test page, before we do anything.
    Bitmap mScreenshotOfPage;

    @Override
    protected String getInitialUrl() {
        return TEST_PAGE_DATA_URL;
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        takeScreenshotOfBackground();
    }

    // Take a screenshot via UiAutomation, which captures all overlays.
    Bitmap takeScreenshot() {
        return getInstrumentation().getUiAutomation().takeScreenshot();
    }

    // Fill |surface| with color |color| and return a screenshot.  Note that we have no idea how
    // long it takes before the image posts, so the screenshot might not reflect it.  Be prepared to
    // retry.  Note that we always draw the same thing, so it's okay if a retry gets a screenshot of
    // a previous surface; they're identical.
    Bitmap fillSurface(Surface surface, int color) {
        Canvas canvas = surface.lockCanvas(null);
        canvas.drawColor(color);
        surface.unlockCanvasAndPost(canvas);
        return takeScreenshot();
    }

    int convertCSSToScreenPixels(int css) {
        ContentViewCore cvc = getContentViewCore();
        return (int) (css * cvc.getPageScaleFactor() * cvc.getDeviceScaleFactor());
    }

    // Since ContentShell makes our solid color div have some textured background, we have to be
    // somewhat lenient here.
    boolean isApproximatelyRed(int color) {
        int r = Color.red(color);
        return Color.blue(color) < r && Color.green(color) < r && Color.red(color) > 100;
    }

    // Take a screenshot, and wait until we get one that has the background div in it.
    void takeScreenshotOfBackground() {
        mAreaOfInterestPx = new Rect();
        for (int retries = 0; retries < 5; retries++) {
            // Compute the div position in screen pixels.  We recompute these since they sometimes
            // take a while to settle, also.
            mDivXPx = convertCSSToScreenPixels(DIV_X_CSS);
            mDivYPx = convertCSSToScreenPixels(DIV_Y_CSS);
            mDivWidthPx = convertCSSToScreenPixels(DIV_WIDTH_CSS);
            mDivHeightPx = convertCSSToScreenPixels(DIV_HEIGHT_CSS);

            // Don't read the whole bitmap.  It's quite big.  Assume that the status bar is only at
            // the top, and that it's at most mStatusBarMaxHeightPx px tall.
            mAreaOfInterestPx.left = mDivXPx;
            mAreaOfInterestPx.top = mDivYPx;
            mAreaOfInterestPx.right = mDivXPx + mDivWidthPx - 1;
            mAreaOfInterestPx.bottom = mDivYPx + mDivHeightPx + mStatusBarMaxHeightPx;

            mScreenshotOfPage = takeScreenshot();

            int area = 0;
            for (int ry = mAreaOfInterestPx.top; ry <= mAreaOfInterestPx.bottom; ry++) {
                for (int rx = mAreaOfInterestPx.left; rx <= mAreaOfInterestPx.right; rx++) {
                    if (isApproximatelyRed(mScreenshotOfPage.getPixel(rx, ry))) area++;
                }
            }

            // It's okay if we have some randomly colored other pixels.
            if (area >= mDivWidthPx * mDivHeightPx) return;

            try {
                Thread.sleep(50);
            } catch (Exception e) {
            }
        }

        Assert.assertTrue(false);
    }

    // Count how many pixels in the div are covered by blue in |blue|, and return it.
    int countDivPixelsCoveredByOverlay(Bitmap blue) {
        // Find pixels that changed from the source color to the target color.  This should avoid
        // issues like changes in the status bar, unless we're really unlucky.  It assumes that the
        // div is actually the expected size; coloring the entire page red would fool this.
        int area = 0;
        for (int ry = mAreaOfInterestPx.top; ry <= mAreaOfInterestPx.bottom; ry++) {
            for (int rx = mAreaOfInterestPx.left; rx <= mAreaOfInterestPx.right; rx++) {
                if (isApproximatelyRed(mScreenshotOfPage.getPixel(rx, ry))
                        && blue.getPixel(rx, ry) == Color.BLUE) {
                    area++;
                }
            }
        }

        return area;
    }

    // Assert that |surface| exactly covers the target div on the page.  Note that we assume that
    // you have not drawn anything to |surface| yet, so that we can still see the div.
    void assertDivIsExactlyCovered(Surface surface) {
        // Draw two colors, and count as the area the ones that change between screenshots.  This
        // lets us notice if the status bar is occluding something, even if it happens to be the
        // same color.  Maybe we should just pick something that's unlikely and use one instead.
        int area = 0;
        int targetArea = mDivWidthPx * mDivHeightPx;
        for (int retries = 0; retries < 10; retries++) {
            // We fill the overlay every time, in case a resize was pending.  Eventually, we should
            // reach a steady-state where the surface is resizd, and this (or a previous) filled-in
            // surface is on the screen.
            Bitmap blue = fillSurface(surface, Color.BLUE);
            area = countDivPixelsCoveredByOverlay(blue);
            if (area == targetArea) return;

            // There are several reasons this can fail besides being broken.  We don't know how long
            // it takes for fillSurface()'s output to make it to the display.  We also don't know
            // how long scheduleLayout() takes.  Just try a few times, since the whole thing should
            // take only a frame or two to settle.
            try {
                Thread.sleep(20);
            } catch (Exception e) {
            }
        }

        // Assert so that we get a helpful message in the log.
        Assert.assertEquals(targetArea, area);
    }

    // Wait for |overlay| to become ready, get its surface, and return it.
    Surface waitForSurface(DialogOverlayImpl overlay) throws Exception {
        Assert.assertNotNull(overlay);
        final Client.Event event = getClient().nextEvent();
        Assert.assertTrue(event.surfaceKey > 0);
        return ThreadUtils.runOnUiThreadBlocking(new Callable<Surface>() {
            @Override
            public Surface call() {
                return DialogOverlayImpl.nativeLookupSurfaceForTesting((int) event.surfaceKey);
            }
        });
    }

    @MediumTest
    @Feature({"AndroidOverlay"})
    public void testInitialPosition() throws Exception {
        // Test that the initial position supplied for the overlay covers the <div> we created.
        final DialogOverlayImpl overlay =
                createOverlay(mDivXPx, mDivYPx, mDivWidthPx, mDivHeightPx);
        Surface surface = waitForSurface(overlay);

        assertDivIsExactlyCovered(surface);
    }

    @MediumTest
    @Feature({"AndroidOverlay"})
    public void testScheduleLayout() throws Exception {
        // Test that scheduleLayout() moves the overlay to cover the <div>.
        final DialogOverlayImpl overlay = createOverlay(0, 0, 10, 10);
        Surface surface = waitForSurface(overlay);

        final org.chromium.gfx.mojom.Rect rect = new org.chromium.gfx.mojom.Rect();
        rect.x = mDivXPx;
        rect.y = mDivYPx;
        rect.width = mDivWidthPx;
        rect.height = mDivHeightPx;

        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                overlay.scheduleLayout(rect);
            }
        });

        assertDivIsExactlyCovered(surface);
    }
}
