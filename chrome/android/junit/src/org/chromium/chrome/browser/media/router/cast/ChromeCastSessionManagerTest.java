// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.cast;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.Feature;
import org.chromium.chrome.browser.media.router.cast.ChromeCastSessionManager.CastSessionLaunchRequest;
import org.chromium.chrome.browser.media.router.cast.ChromeCastSessionManager.ChromeCastSessionObserver;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Robolectric tests for {@link CastMediaSource} class.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ChromeCastSessionManagerTest {
    private ChromeCastSessionObserver mObserver;
    private CastSessionLaunchRequest mRequest;
    private CastSession mSession;
    private ChromeCastSessionManager mManager;

    // Mocks to be used in scenarios where multiple sessions are launched.
    private ChromeCastSessionObserver mAltObserver;
    private CastSessionLaunchRequest mAltRequest;
    private CastSession mAltSession;

    @Before
    public void setup() {
        mObserver = mock(ChromeCastSessionObserver.class);
        mRequest = mock(CastSessionLaunchRequest.class);
        mSession = mock(CastSession.class);

        doReturn(mObserver).when(mRequest).getSessionObserver();

        mManager = ChromeCastSessionManager.get();
    }

    private void setupAlternateMocks() {
        mAltObserver = mock(ChromeCastSessionObserver.class);
        mAltRequest = mock(CastSessionLaunchRequest.class);
        mAltSession = mock(CastSession.class);

        doReturn(mAltObserver).when(mAltRequest).getSessionObserver();
    }

    @After
    public void cleanup() {
        ChromeCastSessionManager.resetInstanceForTesting();
    }

    private void setupLaunch(
            CastSessionLaunchRequest request, CastSession session, boolean success) {
        doAnswer(new Answer() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                if (success)
                    mManager.onSessionCreated(session);
                else
                    mManager.onSessionLaunchError();

                return null;
            }
        })
                .when(request)
                .start(any());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testLaunchingRequest() {
        mManager.requestSessionLaunch(mRequest);
        verify(mRequest).start(any());
        verify(mObserver).onSessionLaunching(mRequest);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSessionRequestSucceeded() {
        // Setup a successful launch.
        setupLaunch(mRequest, mSession, true);

        mManager.requestSessionLaunch(mRequest);
        verify(mObserver).onSessionCreated(mSession);
    }

    @Test
    @Feature({"MediaRouter"})
    public void testSessionRequestFailed() {
        // Setup a failed launch.
        setupLaunch(mRequest, null, false);

        mManager.requestSessionLaunch(mRequest);
        verify(mObserver).onSessionLaunchError();
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNewRequestWaitOnSessionClose() {
        // Make sure |mManager| currently has a session.
        setupAlternateMocks();
        setupLaunch(mAltRequest, mAltSession, true);
        mManager.requestSessionLaunch(mAltRequest);

        // Simulate the session taking a while to stop.
        doNothing().when(mAltSession).stopApplication();

        // Launching a new request should stop the current session, and wait until it has has
        // stopped before launching the request.
        mManager.requestSessionLaunch(mRequest);
        verify(mAltSession).stopApplication();
        verify(mRequest, never()).start(any());
    }

    @Test
    @Feature({"MediaRouter"})
    public void testNewSessionStopsOldOnes() {
        // Make sure |mManager| currently has a session.
        setupAlternateMocks();
        setupLaunch(mAltRequest, mAltSession, true);
        mManager.requestSessionLaunch(mAltRequest);

        // Setup the current session to successfuly stop.
        doAnswer(new Answer() {
            @Override
            public Object answer(InvocationOnMock invocation) {
                mManager.onSessionClosed();
                return null;
            }
        })
                .when(mAltSession)
                .stopApplication();

        // The old observer should be notified of a session closing.
        // The new request should be launched
        mManager.requestSessionLaunch(mRequest);
        verify(mAltObserver).onSessionClosed();
        verify(mRequest).start(any());
    }

    // Makes sure we can override pending requests while waiting for an old
    // session to close.
    @Test
    @Feature({"MediaRouter"})
    public void testNewRequestOverwritePendingOnes() {
        // Make sure |mManager| currently has a session.
        setupAlternateMocks();
        setupLaunch(mAltRequest, mAltSession, true);
        mManager.requestSessionLaunch(mAltRequest);

        // Simulate the current session taking a while to stop.
        doNothing().when(mAltSession).stopApplication();

        // Make sure the queued request isn't launched immediately.
        CastSessionLaunchRequest middleRequest = mock(CastSessionLaunchRequest.class);
        mManager.requestSessionLaunch(middleRequest);

        // Queue up another pending request.
        mManager.requestSessionLaunch(mRequest);

        // Simulate |mAltSession| finally closing.
        mManager.onSessionClosed();

        // The first request shouldn't have started.
        verify(middleRequest, never()).start(any());
        // The last request should have been started.
        verify(mRequest).start(any());
    }

    // Makes sure new requests are dropped when we are waiting for a request to
    // complete its launch.
    @Test
    @Feature({"MediaRouter"})
    public void testRequestAreDroppedWhenLaunching() {
        setupAlternateMocks();

        // Simulate a slow to launch request.
        doNothing().when(mAltRequest).start(any());
        mManager.requestSessionLaunch(mAltRequest);

        // Queue up a new request whilst launching.
        // Make this new request isn't started.
        setupLaunch(mRequest, mSession, true);
        mManager.requestSessionLaunch(mRequest);
        verify(mRequest, never()).start(any());

        // Simulate the completion of the slow request.
        // Make sure the 2nd request was not launched.
        mManager.onSessionCreated(mAltSession);
        verify(mRequest, never()).start(any());
    }
}
