// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_RENDER_VIEW_HOST_H_
#define CONTENT_TEST_TEST_RENDER_VIEW_HOST_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/public/test/test_renderer_host.h"
#include "ui/base/ime/dummy_text_input_client.h"
#include "ui/base/layout.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/vector2d_f.h"

// This file provides a testing framework for mocking out the RenderProcessHost
// layer. It allows you to test RenderViewHost, WebContentsImpl,
// NavigationController, and other layers above that without running an actual
// renderer process.
//
// To use, derive your test base class from RenderViewHostImplTestHarness.

struct FrameHostMsg_DidCommitProvisionalLoad_Params;
struct ViewHostMsg_TextInputState_Params;

namespace gfx {
class Rect;
}

namespace content {

class SiteInstance;
class TestRenderFrameHost;
class TestWebContents;

// Utility function to initialize ViewHostMsg_NavigateParams_Params
// with given |page_id|, |url| and |transition_type|.
void InitNavigateParams(FrameHostMsg_DidCommitProvisionalLoad_Params* params,
                        int page_id,
                        const GURL& url,
                        ui::PageTransition transition_type);

// TestRenderViewHostView ------------------------------------------------------

// Subclass the RenderViewHost's view so that we can call Show(), etc.,
// without having side-effects.
class TestRenderWidgetHostView : public RenderWidgetHostViewBase {
 public:
  explicit TestRenderWidgetHostView(RenderWidgetHost* rwh);
  virtual ~TestRenderWidgetHostView();

  // RenderWidgetHostView implementation.
  virtual void InitAsChild(gfx::NativeView parent_view) override {}
  virtual RenderWidgetHost* GetRenderWidgetHost() const override;
  virtual void SetSize(const gfx::Size& size) override {}
  virtual void SetBounds(const gfx::Rect& rect) override {}
  virtual gfx::Vector2dF GetLastScrollOffset() const override;
  virtual gfx::NativeView GetNativeView() const override;
  virtual gfx::NativeViewId GetNativeViewId() const override;
  virtual gfx::NativeViewAccessible GetNativeViewAccessible() override;
  virtual ui::TextInputClient* GetTextInputClient() override;
  virtual bool HasFocus() const override;
  virtual bool IsSurfaceAvailableForCopy() const override;
  virtual void Show() override;
  virtual void Hide() override;
  virtual bool IsShowing() override;
  virtual gfx::Rect GetViewBounds() const override;
#if defined(OS_MACOSX)
  virtual void SetActive(bool active) override;
  virtual void SetTakesFocusOnlyOnMouseDown(bool flag) override {}
  virtual void SetWindowVisibility(bool visible) override {}
  virtual void WindowFrameChanged() override {}
  virtual void ShowDefinitionForSelection() override {}
  virtual bool SupportsSpeech() const override;
  virtual void SpeakSelection() override;
  virtual bool IsSpeaking() const override;
  virtual void StopSpeaking() override;
#endif  // defined(OS_MACOSX)
  virtual void OnSwapCompositorFrame(
      uint32 output_surface_id,
      scoped_ptr<cc::CompositorFrame> frame) override;

  // RenderWidgetHostViewBase implementation.
  virtual void InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos) override {}
  virtual void InitAsFullscreen(
      RenderWidgetHostView* reference_host_view) override {}
  virtual void WasShown() override {}
  virtual void WasHidden() override {}
  virtual void MovePluginWindows(
      const std::vector<WebPluginGeometry>& moves) override {}
  virtual void Focus() override {}
  virtual void Blur() override {}
  virtual void SetIsLoading(bool is_loading) override {}
  virtual void UpdateCursor(const WebCursor& cursor) override {}
  virtual void TextInputStateChanged(
      const ViewHostMsg_TextInputState_Params& params) override {}
  virtual void ImeCancelComposition() override {}
#if defined(OS_MACOSX) || defined(USE_AURA)
  virtual void ImeCompositionRangeChanged(
      const gfx::Range& range,
      const std::vector<gfx::Rect>& character_bounds) override {}
#endif
  virtual void RenderProcessGone(base::TerminationStatus status,
                                 int error_code) override;
  virtual void Destroy() override;
  virtual void SetTooltipText(const base::string16& tooltip_text) override {}
  virtual void SelectionBoundsChanged(
      const ViewHostMsg_SelectionBounds_Params& params) override {}
  virtual void CopyFromCompositingSurface(
      const gfx::Rect& src_subrect,
      const gfx::Size& dst_size,
      CopyFromCompositingSurfaceCallback& callback,
      const SkColorType color_type) override;
  virtual void CopyFromCompositingSurfaceToVideoFrame(
      const gfx::Rect& src_subrect,
      const scoped_refptr<media::VideoFrame>& target,
      const base::Callback<void(bool)>& callback) override;
  virtual bool CanCopyToVideoFrame() const override;
  virtual bool HasAcceleratedSurface(const gfx::Size& desired_size) override;
#if defined(OS_MACOSX)
  virtual bool PostProcessEventForPluginIme(
      const NativeWebKeyboardEvent& event) override;
#endif
#if defined(OS_ANDROID)
  virtual void LockCompositingSurface() override {}
  virtual void UnlockCompositingSurface() override {}
#endif
  virtual void GetScreenInfo(blink::WebScreenInfo* results) override {}
  virtual gfx::Rect GetBoundsInRootWindow() override;
  virtual gfx::GLSurfaceHandle GetCompositingSurface() override;
  virtual bool LockMouse() override;
  virtual void UnlockMouse() override;
#if defined(OS_WIN)
  virtual void SetParentNativeViewAccessible(
      gfx::NativeViewAccessible accessible_parent) override;
  virtual gfx::NativeViewId GetParentForWindowlessPlugin() const override;
#endif

  bool is_showing() const { return is_showing_; }
  bool did_swap_compositor_frame() const { return did_swap_compositor_frame_; }

 protected:
  RenderWidgetHostImpl* rwh_;

 private:
  bool is_showing_;
  bool did_swap_compositor_frame_;
  ui::DummyTextInputClient text_input_client_;
};

#if defined(COMPILER_MSVC)
// See comment for same warning on RenderViewHostImpl.
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

// TestRenderViewHost ----------------------------------------------------------

// TODO(brettw) this should use a TestWebContents which should be generalized
// from the WebContentsImpl test. We will probably also need that class' version
// of CreateRenderViewForRenderManager when more complicated tests start using
// this.
//
// Note that users outside of content must use this class by getting
// the separate RenderViewHostTester interface via
// RenderViewHostTester::For(rvh) on the RenderViewHost they want to
// drive tests on.
//
// Users within content may directly static_cast from a
// RenderViewHost* to a TestRenderViewHost*.
//
// The reasons we do it this way rather than extending the parallel
// inheritance hierarchy we have for RenderWidgetHost/RenderViewHost
// vs. RenderWidgetHostImpl/RenderViewHostImpl are:
//
// a) Extending the parallel class hierarchy further would require
// more classes to use virtual inheritance.  This is a complexity that
// is better to avoid, especially when it would be introduced in the
// production code solely to facilitate testing code.
//
// b) While users outside of content only need to drive tests on a
// RenderViewHost, content needs a test version of the full
// RenderViewHostImpl so that it can test all methods on that concrete
// class (e.g. overriding a method such as
// RenderViewHostImpl::CreateRenderView).  This would have complicated
// the dual class hierarchy even further.
//
// The reason we do it this way instead of using composition is
// similar to (b) above, essentially it gets very tricky.  By using
// the split interface we avoid complexity within content and maintain
// reasonable utility for embedders.
class TestRenderViewHost
    : public RenderViewHostImpl,
      public RenderViewHostTester {
 public:
  TestRenderViewHost(SiteInstance* instance,
                     RenderViewHostDelegate* delegate,
                     RenderWidgetHostDelegate* widget_delegate,
                     int routing_id,
                     int main_frame_routing_id,
                     bool swapped_out);
  virtual ~TestRenderViewHost();

  // RenderViewHostTester implementation.  Note that CreateRenderView
  // is not specified since it is synonymous with the one from
  // RenderViewHostImpl, see below.
  virtual void SetContentsMimeType(const std::string& mime_type) override;
  virtual void SimulateWasHidden() override;
  virtual void SimulateWasShown() override;

  // NOTE: These methods are deprecated and the equivalents in
  // TestRenderFrameHost should be used.
  virtual void SendNavigate(int page_id, const GURL& url) override;
  virtual void SendFailedNavigate(int page_id, const GURL& url) override;
  virtual void SendNavigateWithTransition(
      int page_id,
      const GURL& url,
      ui::PageTransition transition) override;

  // Calls OnNavigate on the RenderViewHost with the given information,
  // including a custom original request URL.  Sets the rest of the
  // parameters in the message to the "typical" values.  This is a helper
  // function for simulating the most common types of loads.
  void SendNavigateWithOriginalRequestURL(
      int page_id, const GURL& url, const GURL& original_request_url);

  void SendNavigateWithFile(
      int page_id, const GURL& url, const base::FilePath& file_path);

  void SendNavigateWithParams(
      FrameHostMsg_DidCommitProvisionalLoad_Params* params);

  void TestOnUpdateStateWithFile(
      int page_id, const base::FilePath& file_path);

  void TestOnStartDragging(const DropData& drop_data);

  // If set, *delete_counter is incremented when this object destructs.
  void set_delete_counter(int* delete_counter) {
    delete_counter_ = delete_counter;
  }

  // Sets whether the RenderView currently exists or not. This controls the
  // return value from IsRenderViewLive, which the rest of the system uses to
  // check whether the RenderView has crashed or not.
  void set_render_view_created(bool created) {
    render_view_created_ = created;
  }

  // If set, navigations will appear to have loaded through a proxy
  // (ViewHostMsg_FrameNavigte_Params::was_fetched_via_proxy).
  // False by default.
  void set_simulate_fetch_via_proxy(bool proxy);

  // If set, navigations will appear to have cleared the history list in the
  // RenderView
  // (FrameHostMsg_DidCommitProvisionalLoad_Params::history_list_was_cleared).
  // False by default.
  void set_simulate_history_list_was_cleared(bool cleared);

  // The opener route id passed to CreateRenderView().
  int opener_route_id() const { return opener_route_id_; }

  // TODO(creis): Remove the need for these methods.
  TestRenderFrameHost* main_render_frame_host() const {
    return main_render_frame_host_;
  }
  void set_main_render_frame_host(TestRenderFrameHost* rfh) {
    main_render_frame_host_ = rfh;
  }

  // RenderViewHost overrides --------------------------------------------------

  virtual bool CreateRenderView(const base::string16& frame_name,
                                int opener_route_id,
                                int proxy_route_id,
                                int32 max_page_id,
                                bool window_was_created_with_opener) override;
  virtual bool IsRenderViewLive() const override;
  virtual bool IsFullscreen() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(RenderViewHostTest, FilterNavigate);

  void SendNavigateWithTransitionAndResponseCode(int page_id,
                                                 const GURL& url,
                                                 ui::PageTransition transition,
                                                 int response_code);

  // Calls OnNavigate on the RenderViewHost with the given information.
  // Sets the rest of the parameters in the message to the "typical" values.
  // This is a helper function for simulating the most common types of loads.
  void SendNavigateWithParameters(
      int page_id,
      const GURL& url,
      ui::PageTransition transition,
      const GURL& original_request_url,
      int response_code,
      const base::FilePath* file_path_for_history_item);

  // Tracks if the caller thinks if it created the RenderView. This is so we can
  // respond to IsRenderViewLive appropriately.
  bool render_view_created_;

  // See set_delete_counter() above. May be NULL.
  int* delete_counter_;

  // See set_simulate_fetch_via_proxy() above.
  bool simulate_fetch_via_proxy_;

  // See set_simulate_history_list_was_cleared() above.
  bool simulate_history_list_was_cleared_;

  // See SetContentsMimeType() above.
  std::string contents_mime_type_;

  // See opener_route_id() above.
  int opener_route_id_;

  TestRenderFrameHost* main_render_frame_host_;

  DISALLOW_COPY_AND_ASSIGN(TestRenderViewHost);
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

// Adds methods to get straight at the impl classes.
class RenderViewHostImplTestHarness : public RenderViewHostTestHarness {
 public:
  RenderViewHostImplTestHarness();
  virtual ~RenderViewHostImplTestHarness();

  // contents() is equivalent to static_cast<TestWebContents*>(web_contents())
  TestWebContents* contents();

  // RVH/RFH getters are shorthand for oft-used bits of web_contents().

  // test_rvh() is equivalent to any of the following:
  //   contents()->GetMainFrame()->GetRenderViewHost()
  //   contents()->GetRenderViewHost()
  //   static_cast<TestRenderViewHost*>(rvh())
  //
  // Since most functionality will eventually shift from RVH to RFH, you may
  // prefer to use the GetMainFrame() method in tests.
  TestRenderViewHost* test_rvh();

  // pending_test_rvh() is equivalent to all of the following:
  //   contents()->GetPendingMainFrame()->GetRenderViewHost() [if frame exists]
  //   contents()->GetPendingRenderViewHost()
  //   static_cast<TestRenderViewHost*>(pending_rvh())
  //
  // Since most functionality will eventually shift from RVH to RFH, you may
  // prefer to use the GetPendingMainFrame() method in tests.
  TestRenderViewHost* pending_test_rvh();

  // active_test_rvh() is equivalent to:
  //   contents()->GetPendingRenderViewHost() ?
  //        contents()->GetPendingRenderViewHost() :
  //        contents()->GetRenderViewHost();
  TestRenderViewHost* active_test_rvh();

  // main_test_rfh() is equivalent to contents()->GetMainFrame()
  // TODO(nick): Replace all uses with contents()->GetMainFrame()
  TestRenderFrameHost* main_test_rfh();

 private:
  typedef scoped_ptr<ui::test::ScopedSetSupportedScaleFactors>
      ScopedSetSupportedScaleFactors;
  ScopedSetSupportedScaleFactors scoped_set_supported_scale_factors_;
  DISALLOW_COPY_AND_ASSIGN(RenderViewHostImplTestHarness);
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_RENDER_VIEW_HOST_H_
