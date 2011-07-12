// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/utf_string_conversions.h"
#include "ppapi/c/dev/ppb_find_dev.h"
#include "ppapi/c/dev/ppb_fullscreen_dev.h"
#include "ppapi/c/dev/ppb_zoom_dev.h"
#include "ppapi/c/dev/ppp_find_dev.h"
#include "ppapi/c/dev/ppp_selection_dev.h"
#include "ppapi/c/dev/ppp_zoom_dev.h"
#include "ppapi/c/pp_input_event.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"
#include "ppapi/c/private/ppb_instance_private.h"
#include "ppapi/c/private/ppp_instance_private.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_buffer_api.h"
#include "printing/units.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebBindings.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCursorInfo.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebElement.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebPluginContainer.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebRect.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebString.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebURL.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebURLRequest.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebView.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/skia_util.h"
#include "webkit/plugins/ppapi/common.h"
#include "webkit/plugins/ppapi/event_conversion.h"
#include "webkit/plugins/ppapi/fullscreen_container.h"
#include "webkit/plugins/ppapi/message_channel.h"
#include "webkit/plugins/ppapi/plugin_delegate.h"
#include "webkit/plugins/ppapi/plugin_module.h"
#include "webkit/plugins/ppapi/plugin_object.h"
#include "webkit/plugins/ppapi/ppb_buffer_impl.h"
#include "webkit/plugins/ppapi/ppb_graphics_2d_impl.h"
#include "webkit/plugins/ppapi/ppb_image_data_impl.h"
#include "webkit/plugins/ppapi/ppb_input_event_impl.h"
#include "webkit/plugins/ppapi/ppb_surface_3d_impl.h"
#include "webkit/plugins/ppapi/ppb_url_loader_impl.h"
#include "webkit/plugins/ppapi/ppb_url_request_info_impl.h"
#include "webkit/plugins/ppapi/ppp_pdf.h"
#include "webkit/plugins/ppapi/string.h"
#include "webkit/plugins/ppapi/var.h"
#include "webkit/plugins/sad_plugin.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "printing/metafile_impl.h"
#endif

#if defined(USE_SKIA)
#include "printing/metafile.h"
#include "printing/metafile_skia_wrapper.h"
#endif

#if defined(OS_WIN)
#include "skia/ext/vector_platform_device_emf_win.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/gdi_util.h"
#endif

#if defined(OS_MACOSX) && defined(USE_SKIA)
#include "skia/ext/skia_utils_mac.h"
#endif

using ppapi::thunk::EnterResourceNoLock;
using ppapi::thunk::PPB_Buffer_API;
using ppapi::thunk::PPB_Graphics2D_API;
using ppapi::thunk::PPB_ImageData_API;
using ppapi::thunk::PPB_Instance_FunctionAPI;
using ppapi::thunk::PPB_Surface3D_API;
using WebKit::WebBindings;
using WebKit::WebCanvas;
using WebKit::WebCursorInfo;
using WebKit::WebDocument;
using WebKit::WebFrame;
using WebKit::WebInputEvent;
using WebKit::WebPluginContainer;
using WebKit::WebString;
using WebKit::WebURLRequest;
using WebKit::WebView;

namespace webkit {
namespace ppapi {

#if defined(OS_WIN)
// Exported by pdf.dll
typedef bool (*RenderPDFPageToDCProc)(
    const unsigned char* pdf_buffer, int buffer_size, int page_number, HDC dc,
    int dpi_x, int dpi_y, int bounds_origin_x, int bounds_origin_y,
    int bounds_width, int bounds_height, bool fit_to_bounds,
    bool stretch_to_bounds, bool keep_aspect_ratio, bool center_in_bounds);
#endif  // defined(OS_WIN)

namespace {

#define COMPILE_ASSERT_MATCHING_ENUM(webkit_name, np_name) \
    COMPILE_ASSERT(static_cast<int>(WebCursorInfo::webkit_name) \
                       == static_cast<int>(np_name), \
                   mismatching_enums)

COMPILE_ASSERT_MATCHING_ENUM(TypePointer, PP_CURSORTYPE_POINTER);
COMPILE_ASSERT_MATCHING_ENUM(TypeCross, PP_CURSORTYPE_CROSS);
COMPILE_ASSERT_MATCHING_ENUM(TypeHand, PP_CURSORTYPE_HAND);
COMPILE_ASSERT_MATCHING_ENUM(TypeIBeam, PP_CURSORTYPE_IBEAM);
COMPILE_ASSERT_MATCHING_ENUM(TypeWait, PP_CURSORTYPE_WAIT);
COMPILE_ASSERT_MATCHING_ENUM(TypeHelp, PP_CURSORTYPE_HELP);
COMPILE_ASSERT_MATCHING_ENUM(TypeEastResize, PP_CURSORTYPE_EASTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthResize, PP_CURSORTYPE_NORTHRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthEastResize,
                             PP_CURSORTYPE_NORTHEASTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthWestResize,
                             PP_CURSORTYPE_NORTHWESTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeSouthResize, PP_CURSORTYPE_SOUTHRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeSouthEastResize,
                             PP_CURSORTYPE_SOUTHEASTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeSouthWestResize,
                             PP_CURSORTYPE_SOUTHWESTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeWestResize, PP_CURSORTYPE_WESTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthSouthResize,
                             PP_CURSORTYPE_NORTHSOUTHRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeEastWestResize, PP_CURSORTYPE_EASTWESTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthEastSouthWestResize,
                             PP_CURSORTYPE_NORTHEASTSOUTHWESTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthWestSouthEastResize,
                             PP_CURSORTYPE_NORTHWESTSOUTHEASTRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeColumnResize, PP_CURSORTYPE_COLUMNRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeRowResize, PP_CURSORTYPE_ROWRESIZE);
COMPILE_ASSERT_MATCHING_ENUM(TypeMiddlePanning, PP_CURSORTYPE_MIDDLEPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeEastPanning, PP_CURSORTYPE_EASTPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthPanning, PP_CURSORTYPE_NORTHPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthEastPanning,
                             PP_CURSORTYPE_NORTHEASTPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeNorthWestPanning,
                             PP_CURSORTYPE_NORTHWESTPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeSouthPanning, PP_CURSORTYPE_SOUTHPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeSouthEastPanning,
                             PP_CURSORTYPE_SOUTHEASTPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeSouthWestPanning,
                             PP_CURSORTYPE_SOUTHWESTPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeWestPanning, PP_CURSORTYPE_WESTPANNING);
COMPILE_ASSERT_MATCHING_ENUM(TypeMove, PP_CURSORTYPE_MOVE);
COMPILE_ASSERT_MATCHING_ENUM(TypeVerticalText, PP_CURSORTYPE_VERTICALTEXT);
COMPILE_ASSERT_MATCHING_ENUM(TypeCell, PP_CURSORTYPE_CELL);
COMPILE_ASSERT_MATCHING_ENUM(TypeContextMenu, PP_CURSORTYPE_CONTEXTMENU);
COMPILE_ASSERT_MATCHING_ENUM(TypeAlias, PP_CURSORTYPE_ALIAS);
COMPILE_ASSERT_MATCHING_ENUM(TypeProgress, PP_CURSORTYPE_PROGRESS);
COMPILE_ASSERT_MATCHING_ENUM(TypeNoDrop, PP_CURSORTYPE_NODROP);
COMPILE_ASSERT_MATCHING_ENUM(TypeCopy, PP_CURSORTYPE_COPY);
COMPILE_ASSERT_MATCHING_ENUM(TypeNone, PP_CURSORTYPE_NONE);
COMPILE_ASSERT_MATCHING_ENUM(TypeNotAllowed, PP_CURSORTYPE_NOTALLOWED);
COMPILE_ASSERT_MATCHING_ENUM(TypeZoomIn, PP_CURSORTYPE_ZOOMIN);
COMPILE_ASSERT_MATCHING_ENUM(TypeZoomOut, PP_CURSORTYPE_ZOOMOUT);
COMPILE_ASSERT_MATCHING_ENUM(TypeGrab, PP_CURSORTYPE_GRAB);
COMPILE_ASSERT_MATCHING_ENUM(TypeGrabbing, PP_CURSORTYPE_GRABBING);
// Do not assert WebCursorInfo::TypeCustom == PP_CURSORTYPE_CUSTOM;
// PP_CURSORTYPE_CUSTOM is pinned to allow new cursor types.

void RectToPPRect(const gfx::Rect& input, PP_Rect* output) {
  *output = PP_MakeRectFromXYWH(input.x(), input.y(),
                                input.width(), input.height());
}

}  // namespace

// static
PluginInstance* PluginInstance::Create0_5(PluginDelegate* delegate,
                                          PluginModule* module,
                                          const void* ppp_instance_if_0_5) {
  const PPP_Instance_0_5* interface =
      static_cast<const PPP_Instance_0_5*>(ppp_instance_if_0_5);
  return new PluginInstance(
      delegate,
      module,
      new ::ppapi::PPP_Instance_Combined(*interface));
}

// static
PluginInstance* PluginInstance::Create0_4(PluginDelegate* delegate,
                                          PluginModule* module,
                                          const void* ppp_instance_if_0_4) {
  const PPP_Instance_0_4* interface =
      static_cast<const PPP_Instance_0_4*>(ppp_instance_if_0_4);
  return new PluginInstance(
      delegate,
      module,
      new ::ppapi::PPP_Instance_Combined(*interface));
}

PluginInstance::PluginInstance(
    PluginDelegate* delegate,
    PluginModule* module,
    ::ppapi::PPP_Instance_Combined* instance_interface)
    : delegate_(delegate),
      module_(module),
      instance_interface_(instance_interface),
      pp_instance_(0),
      container_(NULL),
      full_frame_(false),
      has_webkit_focus_(false),
      has_content_area_focus_(false),
      find_identifier_(-1),
      plugin_find_interface_(NULL),
      plugin_messaging_interface_(NULL),
      plugin_input_event_interface_(NULL),
      plugin_private_interface_(NULL),
      plugin_pdf_interface_(NULL),
      plugin_selection_interface_(NULL),
      plugin_zoom_interface_(NULL),
      checked_for_plugin_input_event_interface_(false),
      checked_for_plugin_messaging_interface_(false),
      plugin_print_interface_(NULL),
      plugin_graphics_3d_interface_(NULL),
      always_on_top_(false),
      fullscreen_container_(NULL),
      fullscreen_(false),
      message_channel_(NULL),
      sad_plugin_(NULL),
      input_event_mask_(0),
      filtered_input_event_mask_(0) {
  pp_instance_ = ResourceTracker::Get()->AddInstance(this);

  memset(&current_print_settings_, 0, sizeof(current_print_settings_));
  DCHECK(delegate);
  module_->InstanceCreated(this);
  delegate_->InstanceCreated(this);
  message_channel_.reset(new MessageChannel(this));
}

PluginInstance::~PluginInstance() {
  // Free all the plugin objects. This will automatically clear the back-
  // pointer from the NPObject so WebKit can't call into the plugin any more.
  //
  // Swap out the set so we can delete from it (the objects will try to
  // unregister themselves inside the delete call).
  PluginObjectSet plugin_object_copy;
  live_plugin_objects_.swap(plugin_object_copy);
  for (PluginObjectSet::iterator i = plugin_object_copy.begin();
       i != plugin_object_copy.end(); ++i)
    delete *i;

  delegate_->InstanceDeleted(this);
  module_->InstanceDeleted(this);

  ResourceTracker::Get()->InstanceDeleted(pp_instance_);
}

// NOTE: Any of these methods that calls into the plugin needs to take into
// account that the plugin may use Var to remove the <embed> from the DOM, which
// will make the WebPluginImpl drop its reference, usually the last one. If a
// method needs to access a member of the instance after the call has returned,
// then it needs to keep its own reference on the stack.

void PluginInstance::Delete() {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  instance_interface_->DidDestroy(pp_instance());

  if (fullscreen_container_) {
    fullscreen_container_->Destroy();
    fullscreen_container_ = NULL;
  }
  container_ = NULL;
}

void PluginInstance::Paint(WebCanvas* canvas,
                           const gfx::Rect& plugin_rect,
                           const gfx::Rect& paint_rect) {
  if (module()->is_crashed()) {
    // Crashed plugin painting.
    if (!sad_plugin_)  // Lazily initialize bitmap.
      sad_plugin_ = delegate_->GetSadPluginBitmap();
    if (sad_plugin_)
      webkit::PaintSadPlugin(canvas, plugin_rect, *sad_plugin_);
    return;
  }

  if (bound_graphics_2d())
    bound_graphics_2d()->Paint(canvas, plugin_rect, paint_rect);
}

void PluginInstance::InvalidateRect(const gfx::Rect& rect) {
  if (fullscreen_container_) {
    if (rect.IsEmpty())
      fullscreen_container_->Invalidate();
    else
      fullscreen_container_->InvalidateRect(rect);
  } else {
    if (!container_ || position_.IsEmpty())
      return;  // Nothing to do.
    if (rect.IsEmpty())
      container_->invalidate();
    else
      container_->invalidateRect(rect);
  }
}

void PluginInstance::ScrollRect(int dx, int dy, const gfx::Rect& rect) {
  if (fullscreen_container_) {
    fullscreen_container_->ScrollRect(dx, dy, rect);
  } else {
    if (full_frame_) {
      container_->scrollRect(dx, dy, rect);
    } else {
      // Can't do optimized scrolling since there could be other elements on top
      // of us.
      InvalidateRect(rect);
    }
  }
}

unsigned PluginInstance::GetBackingTextureId() {
  if (!bound_graphics_3d())
    return 0;

  return bound_graphics_3d()->GetBackingTextureId();
}

void PluginInstance::CommitBackingTexture() {
  if (fullscreen_container_)
    fullscreen_container_->Invalidate();
  else
    container_->commitBackingTexture();
}

void PluginInstance::InstanceCrashed() {
  // Force free all resources and vars.
  ResourceTracker::Get()->InstanceCrashed(pp_instance());

  // Free any associated graphics.
  SetFullscreen(false, false);
  bound_graphics_ = NULL;
  InvalidateRect(gfx::Rect());

  delegate()->PluginCrashed(this);
}

bool PluginInstance::SetCursor(PP_CursorType_Dev type,
                               PP_Resource custom_image,
                               const PP_Point* hot_spot) {
  if (type != PP_CURSORTYPE_CUSTOM) {
    cursor_.reset(new WebCursorInfo(static_cast<WebCursorInfo::Type>(type)));
    return true;
  }

  if (!hot_spot)
    return false;

  EnterResourceNoLock<PPB_ImageData_API> enter(custom_image, true);
  if (enter.failed())
    return false;
  PPB_ImageData_Impl* image_data =
      static_cast<PPB_ImageData_Impl*>(enter.object());

  if (image_data->format() != PPB_ImageData_Impl::GetNativeImageDataFormat()) {
    // TODO(yzshen): Handle the case that the image format is different from the
    // native format.
    NOTIMPLEMENTED();
    return false;
  }

  ImageDataAutoMapper auto_mapper(image_data);
  if (!auto_mapper.is_valid())
    return false;

  scoped_ptr<WebCursorInfo> custom_cursor(
      new WebCursorInfo(WebCursorInfo::TypeCustom));
  custom_cursor->hotSpot.x = hot_spot->x;
  custom_cursor->hotSpot.y = hot_spot->y;

#if WEBKIT_USING_SKIA
  const SkBitmap* bitmap = image_data->GetMappedBitmap();
  // Make a deep copy, so that the cursor remains valid even after the original
  // image data gets freed.
  if (!bitmap->copyTo(&custom_cursor->customImage.getSkBitmap(),
                      bitmap->config())) {
    return false;
  }
#elif WEBKIT_USING_CG
  // TODO(yzshen): Implement it.
  NOTIMPLEMENTED();
  return false;
#endif

  cursor_.reset(custom_cursor.release());
  return true;
}

bool PluginInstance::Initialize(WebPluginContainer* container,
                                const std::vector<std::string>& arg_names,
                                const std::vector<std::string>& arg_values,
                                const GURL& plugin_url,
                                bool full_frame) {
  container_ = container;
  plugin_url_ = plugin_url;
  full_frame_ = full_frame;

  size_t argc = 0;
  scoped_array<const char*> argn(new const char*[arg_names.size()]);
  scoped_array<const char*> argv(new const char*[arg_names.size()]);
  for (size_t i = 0; i < arg_names.size(); ++i) {
    argn[argc] = arg_names[i].c_str();
    argv[argc] = arg_values[i].c_str();
    argc++;
  }

  return PPBoolToBool(instance_interface_->DidCreate(pp_instance(),
                                                     argc,
                                                     argn.get(),
                                                     argv.get()));
}

bool PluginInstance::HandleDocumentLoad(PPB_URLLoader_Impl* loader) {
  Resource::ScopedResourceId resource(loader);
  return PPBoolToBool(instance_interface_->HandleDocumentLoad(pp_instance(),
                                                              resource.id));
}

bool PluginInstance::HandleInputEvent(const WebKit::WebInputEvent& event,
                                      WebCursorInfo* cursor_info) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);

  bool rv = false;
  if (LoadInputEventInterface()) {
    PP_InputEvent_Class event_class = ClassifyInputEvent(event.type);
    if (!event_class)
      return false;

    if ((filtered_input_event_mask_ & event_class) ||
        (input_event_mask_ & event_class)) {
      // Actually send the event.
      std::vector< ::ppapi::InputEventData > events;
      CreateInputEventData(event, &events);

      // Each input event may generate more than one PP_InputEvent.
      ResourceTracker* tracker = ResourceTracker::Get();
      for (size_t i = 0; i < events.size(); i++) {
        if (filtered_input_event_mask_ & event_class)
          events[i].is_filtered = true;
        scoped_refptr<PPB_InputEvent_Impl> event_resource(
            new PPB_InputEvent_Impl(this, events[i]));
        PP_Resource resource = event_resource->GetReference();

        rv |= PPBoolToBool(plugin_input_event_interface_->HandleInputEvent(
            pp_instance(), event_resource->GetReference()));

        // Release the reference we took above.
        tracker->UnrefResource(resource);
      }
    }
  }

  // For compatibility, also send all input events through the old interface.
  // TODO(brettw) remove this.
  std::vector<PP_InputEvent> pp_events;
  CreatePPEvent(event, &pp_events);

  // Each input event may generate more than one PP_InputEvent.
  for (size_t i = 0; i < pp_events.size(); i++) {
    rv |= PPBoolToBool(instance_interface_->HandleInputEvent(pp_instance(),
                                                             &pp_events[i]));
  }

  if (cursor_.get())
    *cursor_info = *cursor_;
  return rv;
}

void PluginInstance::HandleMessage(PP_Var message) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadMessagingInterface())
    return;
  plugin_messaging_interface_->HandleMessage(pp_instance(), message);
}

PP_Var PluginInstance::GetInstanceObject() {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);

  // Try the private interface first. If it is not supported, we fall back to
  // looking in the older version of the PPP_Instance interface for
  // GetInstanceObject. If all that fails, return an undefined Var.
  // TODO(dmichael): Remove support for PPP_Instance.GetInstanceObject
  if (LoadPrivateInterface())
    return plugin_private_interface_->GetInstanceObject(pp_instance());
  else if (instance_interface_->GetInstanceObject_0_4)
    return instance_interface_->GetInstanceObject_0_4(pp_instance());
  return PP_MakeUndefined();
}

void PluginInstance::ViewChanged(const gfx::Rect& position,
                                 const gfx::Rect& clip) {
  fullscreen_ = (fullscreen_container_ != NULL);
  position_ = position;

  if (clip.IsEmpty()) {
    // WebKit can give weird (x,y) positions for empty clip rects (since the
    // position technically doesn't matter). But we want to make these
    // consistent since this is given to the plugin, so force everything to 0
    // in the "everything is clipped" case.
    clip_ = gfx::Rect();
  } else {
    clip_ = clip;
  }

  PP_Rect pp_position, pp_clip;
  RectToPPRect(position_, &pp_position);
  RectToPPRect(clip_, &pp_clip);
  instance_interface_->DidChangeView(pp_instance(), &pp_position, &pp_clip);
}

void PluginInstance::SetWebKitFocus(bool has_focus) {
  if (has_webkit_focus_ == has_focus)
    return;

  bool old_plugin_focus = PluginHasFocus();
  has_webkit_focus_ = has_focus;
  if (PluginHasFocus() != old_plugin_focus) {
    delegate()->PluginFocusChanged(PluginHasFocus());
    instance_interface_->DidChangeFocus(pp_instance(),
                                        BoolToPPBool(PluginHasFocus()));
  }
}

void PluginInstance::SetContentAreaFocus(bool has_focus) {
  if (has_content_area_focus_ == has_focus)
    return;

  bool old_plugin_focus = PluginHasFocus();
  has_content_area_focus_ = has_focus;
  if (PluginHasFocus() != old_plugin_focus) {
    instance_interface_->DidChangeFocus(pp_instance(),
                                        BoolToPPBool(PluginHasFocus()));
  }
}

void PluginInstance::ViewInitiatedPaint() {
  if (bound_graphics_2d())
    bound_graphics_2d()->ViewInitiatedPaint();
  if (bound_graphics_3d())
    bound_graphics_3d()->ViewInitiatedPaint();
}

void PluginInstance::ViewFlushedPaint() {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (bound_graphics_2d())
    bound_graphics_2d()->ViewFlushedPaint();
  if (bound_graphics_3d())
    bound_graphics_3d()->ViewFlushedPaint();
}

bool PluginInstance::GetBitmapForOptimizedPluginPaint(
    const gfx::Rect& paint_bounds,
    TransportDIB** dib,
    gfx::Rect* location,
    gfx::Rect* clip) {
  if (!always_on_top_)
    return false;
  if (!bound_graphics_2d() || !bound_graphics_2d()->is_always_opaque())
    return false;

  // We specifically want to compare against the area covered by the backing
  // store when seeing if we cover the given paint bounds, since the backing
  // store could be smaller than the declared plugin area.
  PPB_ImageData_Impl* image_data = bound_graphics_2d()->image_data();
  gfx::Rect plugin_backing_store_rect(position_.origin(),
                                      gfx::Size(image_data->width(),
                                                image_data->height()));
  gfx::Rect clip_page(clip_);
  clip_page.Offset(position_.origin());
  gfx::Rect plugin_paint_rect = plugin_backing_store_rect.Intersect(clip_page);
  if (!plugin_paint_rect.Contains(paint_bounds))
    return false;

  *dib = image_data->platform_image()->GetTransportDIB();
  *location = plugin_backing_store_rect;
  *clip = clip_page;
  return true;
}

string16 PluginInstance::GetSelectedText(bool html) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadSelectionInterface())
    return string16();

  PP_Var rv = plugin_selection_interface_->GetSelectedText(pp_instance(),
                                                           BoolToPPBool(html));
  scoped_refptr<StringVar> string(StringVar::FromPPVar(rv));
  Var::PluginReleasePPVar(rv);  // Release the ref the plugin transfered to us.
  if (!string)
    return string16();
  return UTF8ToUTF16(string->value());
}

string16 PluginInstance::GetLinkAtPosition(const gfx::Point& point) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadPdfInterface())
    return string16();

  PP_Point p;
  p.x = point.x();
  p.y = point.y();
  PP_Var rv = plugin_pdf_interface_->GetLinkAtPosition(pp_instance(), p);
  scoped_refptr<StringVar> string(StringVar::FromPPVar(rv));
  Var::PluginReleasePPVar(rv);  // Release the ref the plugin transfered to us.
  if (!string)
    return string16();
  return UTF8ToUTF16(string->value());
}

void PluginInstance::Zoom(double factor, bool text_only) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadZoomInterface())
    return;
  plugin_zoom_interface_->Zoom(pp_instance(), factor, BoolToPPBool(text_only));
}

bool PluginInstance::StartFind(const string16& search_text,
                               bool case_sensitive,
                               int identifier) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadFindInterface())
    return false;
  find_identifier_ = identifier;
  return PPBoolToBool(
      plugin_find_interface_->StartFind(
          pp_instance(),
          UTF16ToUTF8(search_text.c_str()).c_str(),
          BoolToPPBool(case_sensitive)));
}

void PluginInstance::SelectFindResult(bool forward) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (LoadFindInterface())
    plugin_find_interface_->SelectFindResult(pp_instance(),
                                             BoolToPPBool(forward));
}

void PluginInstance::StopFind() {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadFindInterface())
    return;
  find_identifier_ = -1;
  plugin_find_interface_->StopFind(pp_instance());
}

bool PluginInstance::LoadFindInterface() {
  if (!plugin_find_interface_) {
    plugin_find_interface_ =
        static_cast<const PPP_Find_Dev*>(module_->GetPluginInterface(
            PPP_FIND_DEV_INTERFACE));
  }

  return !!plugin_find_interface_;
}

bool PluginInstance::LoadInputEventInterface() {
  if (!checked_for_plugin_input_event_interface_) {
    checked_for_plugin_input_event_interface_ = true;
    plugin_input_event_interface_ =
        static_cast<const PPP_InputEvent*>(module_->GetPluginInterface(
            PPP_INPUT_EVENT_INTERFACE));
  }
  return !!plugin_input_event_interface_;
}

bool PluginInstance::LoadMessagingInterface() {
  if (!checked_for_plugin_messaging_interface_) {
    checked_for_plugin_messaging_interface_ = true;
    plugin_messaging_interface_ =
        static_cast<const PPP_Messaging*>(module_->GetPluginInterface(
            PPP_MESSAGING_INTERFACE));
  }

  return !!plugin_messaging_interface_;
}

bool PluginInstance::LoadPdfInterface() {
  if (!plugin_pdf_interface_) {
    plugin_pdf_interface_ =
        static_cast<const PPP_Pdf*>(module_->GetPluginInterface(
            PPP_PDF_INTERFACE));
  }

  return !!plugin_pdf_interface_;
}

bool PluginInstance::LoadSelectionInterface() {
  if (!plugin_selection_interface_) {
    plugin_selection_interface_ =
        static_cast<const PPP_Selection_Dev*>(module_->GetPluginInterface(
            PPP_SELECTION_DEV_INTERFACE));
  }

  return !!plugin_selection_interface_;
}

bool PluginInstance::LoadPrintInterface() {
  if (!plugin_print_interface_.get()) {
    // Try to get the most recent version first.  Fall back to older supported
    // versions if necessary.
    const PPP_Printing_Dev_0_4* print_if =
        static_cast<const PPP_Printing_Dev_0_4*>(
            module_->GetPluginInterface(PPP_PRINTING_DEV_INTERFACE_0_4));
    if (print_if) {
      plugin_print_interface_.reset(new PPP_Printing_Dev_Combined(*print_if));
    } else {
      const PPP_Printing_Dev_0_3* print_if_0_3 =
          static_cast<const PPP_Printing_Dev_0_3*>(
              module_->GetPluginInterface(PPP_PRINTING_DEV_INTERFACE_0_3));
      if (print_if_0_3)
        plugin_print_interface_.reset(
            new PPP_Printing_Dev_Combined(*print_if_0_3));
    }
  }
  return !!plugin_print_interface_.get();
}

bool PluginInstance::LoadPrivateInterface() {
  if (!plugin_private_interface_) {
    plugin_private_interface_ = static_cast<const PPP_Instance_Private*>(
            module_->GetPluginInterface(PPP_INSTANCE_PRIVATE_INTERFACE));
  }

  return !!plugin_private_interface_;
}

bool PluginInstance::LoadZoomInterface() {
  if (!plugin_zoom_interface_) {
    plugin_zoom_interface_ =
        static_cast<const PPP_Zoom_Dev*>(module_->GetPluginInterface(
            PPP_ZOOM_DEV_INTERFACE));
  }

  return !!plugin_zoom_interface_;
}

bool PluginInstance::PluginHasFocus() const {
  return has_webkit_focus_ && has_content_area_focus_;
}

void PluginInstance::ReportGeometry() {
  // If this call was delayed, we may have transitioned back to fullscreen in
  // the mean time, so only report the geometry if we are actually in normal
  // mode.
  if (container_ && !fullscreen_container_)
    container_->reportGeometry();
}

bool PluginInstance::GetPreferredPrintOutputFormat(
    PP_PrintOutputFormat_Dev_0_4* format) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  if (!LoadPrintInterface())
    return false;
  if (plugin_print_interface_->QuerySupportedFormats) {
    // If the most recent version of the QuerySupportedFormats functions is
    // available, use it.
    uint32_t supported_formats =
        plugin_print_interface_->QuerySupportedFormats(pp_instance());
    if (supported_formats & PP_PRINTOUTPUTFORMAT_PDF_0_4) {
      *format = PP_PRINTOUTPUTFORMAT_PDF_0_4;
      return true;
    } else if (supported_formats & PP_PRINTOUTPUTFORMAT_RASTER_0_4) {
      *format = PP_PRINTOUTPUTFORMAT_RASTER_0_4;
      return true;
    }
    return false;
  } else if (plugin_print_interface_->QuerySupportedFormats_0_3) {
    // If we couldn't use the latest version of the function, but the 0.3
    // version exists, we can use it.
    uint32_t format_count = 0;
    PP_PrintOutputFormat_Dev_0_3* supported_formats =
        plugin_print_interface_->QuerySupportedFormats_0_3(pp_instance(),
                                                           &format_count);
    if (!supported_formats)
      return false;

    bool found_supported_format = false;
    for (uint32_t index = 0; index < format_count; index++) {
      if (supported_formats[index] == PP_PRINTOUTPUTFORMAT_PDF_0_3) {
        // If we found PDF, we are done.
        found_supported_format = true;
        *format = PP_PRINTOUTPUTFORMAT_PDF_0_4;
        break;
      } else if (supported_formats[index] ==
                 PP_PRINTOUTPUTFORMAT_RASTER_0_3) {
        // We found raster. Keep looking.
        found_supported_format = true;
        *format = PP_PRINTOUTPUTFORMAT_RASTER_0_4;
      }
    }
    PluginModule::GetCore()->MemFree(supported_formats);
    return found_supported_format;
  }
  return false;
}

bool PluginInstance::SupportsPrintInterface() {
  PP_PrintOutputFormat_Dev_0_4 format;
  return GetPreferredPrintOutputFormat(&format);
}

int PluginInstance::PrintBegin(const gfx::Rect& printable_area,
                               int printer_dpi) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  PP_PrintOutputFormat_Dev_0_4 format;
  if (!GetPreferredPrintOutputFormat(&format)) {
    // PrintBegin should not have been called since SupportsPrintInterface
    // would have returned false;
    NOTREACHED();
    return 0;
  }

  int num_pages = 0;
  PP_PrintSettings_Dev_0_4 print_settings;
  RectToPPRect(printable_area, &print_settings.printable_area);
  print_settings.dpi = printer_dpi;
  print_settings.orientation = PP_PRINTORIENTATION_NORMAL;
  print_settings.grayscale = PP_FALSE;
  print_settings.format = format;
  if (plugin_print_interface_->Begin) {
    // If we have the most current version of Begin, use it.
    num_pages = plugin_print_interface_->Begin(pp_instance(),
                                               &print_settings);
  } else if (plugin_print_interface_->Begin_0_3) {
    // Fall back to the 0.3 version of Begin if necessary, and convert the
    // output format.
    PP_PrintSettings_Dev_0_3 print_settings_0_3;
    RectToPPRect(printable_area, &print_settings_0_3.printable_area);
    print_settings_0_3.dpi = printer_dpi;
    print_settings_0_3.orientation = PP_PRINTORIENTATION_NORMAL;
    print_settings_0_3.grayscale = PP_FALSE;
    switch (format) {
      case PP_PRINTOUTPUTFORMAT_RASTER_0_4:
        print_settings_0_3.format = PP_PRINTOUTPUTFORMAT_RASTER_0_3;
        break;
      case PP_PRINTOUTPUTFORMAT_PDF_0_4:
        print_settings_0_3.format = PP_PRINTOUTPUTFORMAT_PDF_0_3;
        break;
      case PP_PRINTOUTPUTFORMAT_POSTSCRIPT_0_4:
        print_settings_0_3.format = PP_PRINTOUTPUTFORMAT_POSTSCRIPT_0_3;
        break;
      default:
        return 0;
    }
    num_pages = plugin_print_interface_->Begin_0_3(pp_instance(),
                                                   &print_settings_0_3);
  }
  if (!num_pages)
    return 0;
  current_print_settings_ = print_settings;
#if defined(OS_LINUX) || defined(OS_WIN)
  canvas_ = NULL;
  ranges_.clear();
#endif  // OS_LINUX || OS_WIN
  return num_pages;
}

bool PluginInstance::PrintPage(int page_number, WebKit::WebCanvas* canvas) {
  DCHECK(plugin_print_interface_.get());
  PP_PrintPageNumberRange_Dev page_range;
  page_range.first_page_number = page_range.last_page_number = page_number;
#if defined(OS_LINUX) || defined(OS_WIN)
  // The canvas only has a metafile on it for print preview.
  if (printing::MetafileSkiaWrapper::GetMetafileFromCanvas(canvas)) {
    ranges_.push_back(page_range);
    canvas_ = canvas;
    return true;
  } else
#endif  // OS_LINUX || OS_WIN
  {
    return PrintPageHelper(&page_range, 1, canvas);
  }
}

bool PluginInstance::PrintPageHelper(PP_PrintPageNumberRange_Dev* page_ranges,
                                     int num_ranges,
                                     WebKit::WebCanvas* canvas) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
  PP_Resource print_output = plugin_print_interface_->PrintPages(
      pp_instance(), page_ranges, num_ranges);
  if (!print_output)
    return false;

  bool ret = false;

  if (current_print_settings_.format == PP_PRINTOUTPUTFORMAT_PDF_0_4)
    ret = PrintPDFOutput(print_output, canvas);
  else if (current_print_settings_.format == PP_PRINTOUTPUTFORMAT_RASTER_0_4)
    ret = PrintRasterOutput(print_output, canvas);

  // Now we need to release the print output resource.
  PluginModule::GetCore()->ReleaseResource(print_output);

  return ret;
}

void PluginInstance::PrintEnd() {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);
#if defined(OS_LINUX) || defined(OS_WIN)
  if (!ranges_.empty())
    PrintPageHelper(&(ranges_.front()), ranges_.size(), canvas_.get());
  canvas_ = NULL;
  ranges_.clear();
#endif  // OS_LINUX || OS_WIN

  DCHECK(plugin_print_interface_.get());
  if (plugin_print_interface_.get())
    plugin_print_interface_->End(pp_instance());

  memset(&current_print_settings_, 0, sizeof(current_print_settings_));
#if defined(OS_MACOSX)
  last_printed_page_ = NULL;
#endif  // defined(OS_MACOSX)
}

bool PluginInstance::IsFullscreenOrPending() {
  return fullscreen_container_ != NULL;
}

void PluginInstance::SetFullscreen(bool fullscreen, bool delay_report) {
  // Keep a reference on the stack. See NOTE above.
  scoped_refptr<PluginInstance> ref(this);

  // We check whether we are trying to switch to the state we're already going
  // to (i.e. if we're already switching to fullscreen but the fullscreen
  // container isn't ready yet, don't do anything more).
  if (fullscreen == IsFullscreenOrPending())
    return;

  BindGraphics(pp_instance(), 0);
  VLOG(1) << "Setting fullscreen to " << (fullscreen ? "on" : "off");
  if (fullscreen) {
    DCHECK(!fullscreen_container_);
    fullscreen_container_ = delegate_->CreateFullscreenContainer(this);
  } else {
    DCHECK(fullscreen_container_);
    fullscreen_container_->Destroy();
    fullscreen_container_ = NULL;
    fullscreen_ = false;
    if (!delay_report) {
      ReportGeometry();
    } else {
      MessageLoop::current()->PostTask(
          FROM_HERE, NewRunnableMethod(this, &PluginInstance::ReportGeometry));
    }
  }
}

int32_t PluginInstance::Navigate(PPB_URLRequestInfo_Impl* request,
                                 const char* target,
                                 bool from_user_action) {
  if (!container_)
    return PP_ERROR_FAILED;

  WebDocument document = container_->element().document();
  WebFrame* frame = document.frame();
  if (!frame)
    return PP_ERROR_FAILED;
  WebURLRequest web_request(request->ToWebURLRequest(frame));
  web_request.setFirstPartyForCookies(document.firstPartyForCookies());
  web_request.setHasUserGesture(from_user_action);

  GURL gurl(web_request.url());
  if (gurl.SchemeIs("javascript")) {
    // In imitation of the NPAPI implementation, only |target_frame == frame| is
    // allowed for security reasons.
    WebFrame* target_frame =
        frame->view()->findFrameByName(WebString::fromUTF8(target), frame);
    if (target_frame != frame)
      return PP_ERROR_NOACCESS;

    // TODO(viettrungluu): NPAPI sends the result back to the plugin -- do we
    // need that?
    WebString result = container_->executeScriptURL(gurl, from_user_action);
    return result.isNull() ? PP_ERROR_FAILED : PP_OK;
  }

  // Only GETs and POSTs are supported.
  if (web_request.httpMethod() != "GET" &&
      web_request.httpMethod() != "POST")
    return PP_ERROR_BADARGUMENT;

  WebString target_str = WebString::fromUTF8(target);
  container_->loadFrameRequest(web_request, target_str, false, NULL);
  return PP_OK;
}

PluginDelegate::PlatformContext3D* PluginInstance::CreateContext3D() {
  if (fullscreen_container_)
    return fullscreen_container_->CreateContext3D();
  else
    return delegate_->CreateContext3D();
}

bool PluginInstance::PrintPDFOutput(PP_Resource print_output,
                                    WebKit::WebCanvas* canvas) {
  ::ppapi::thunk::EnterResourceNoLock<PPB_Buffer_API> enter(print_output, true);
  if (enter.failed())
    return false;

  BufferAutoMapper mapper(enter.object());
  if (!mapper.data() || !mapper.size()) {
    NOTREACHED();
    return false;
  }
#if defined(OS_WIN)
  // For Windows, we need the PDF DLL to render the output PDF to a DC.
  HMODULE pdf_module = GetModuleHandle(L"pdf.dll");
  if (!pdf_module)
    return false;
  RenderPDFPageToDCProc render_proc =
      reinterpret_cast<RenderPDFPageToDCProc>(
          GetProcAddress(pdf_module, "RenderPDFPageToDC"));
  if (!render_proc)
    return false;
#endif  // defined(OS_WIN)

  bool ret = false;
#if defined(OS_LINUX) || (defined(OS_MACOSX) && defined(USE_SKIA))
  // On Linux we just set the final bits in the native metafile
  // (NativeMetafile and PreviewMetafile must have compatible formats,
  // i.e. both PDF for this to work).
  printing::Metafile* metafile =
      printing::MetafileSkiaWrapper::GetMetafileFromCanvas(canvas);
  DCHECK(metafile != NULL);
  if (metafile)
    ret = metafile->InitFromData(mapper.data(), mapper.size());
#elif defined(OS_MACOSX)
  printing::NativeMetafile metafile;
  // Create a PDF metafile and render from there into the passed in context.
  if (metafile.InitFromData(mapper.data(), mapper.size())) {
    // Flip the transform.
    CGContextRef cgContext = canvas;
    CGContextSaveGState(cgContext);
    CGContextTranslateCTM(cgContext, 0,
                          current_print_settings_.printable_area.size.height);
    CGContextScaleCTM(cgContext, 1.0, -1.0);
    CGRect page_rect;
    page_rect.origin.x = current_print_settings_.printable_area.point.x;
    page_rect.origin.y = current_print_settings_.printable_area.point.y;
    page_rect.size.width = current_print_settings_.printable_area.size.width;
    page_rect.size.height = current_print_settings_.printable_area.size.height;

    ret = metafile.RenderPage(1, cgContext, page_rect, true, false, true, true);
    CGContextRestoreGState(cgContext);
  }
#elif defined(OS_WIN)
  printing::Metafile* metafile =
    printing::MetafileSkiaWrapper::GetMetafileFromCanvas(canvas);
  if (metafile) {
    // We only have a metafile when doing print preview, so we just want to
    // pass the PDF off to preview.
    ret = metafile->InitFromData(mapper.data(), mapper.size());
  } else {
    // On Windows, we now need to render the PDF to the DC that backs the
    // supplied canvas.
    HDC dc = skia::BeginPlatformPaint(canvas);
    gfx::Size size_in_pixels;
    size_in_pixels.set_width(printing::ConvertUnit(
        current_print_settings_.printable_area.size.width,
        static_cast<int>(printing::kPointsPerInch),
        current_print_settings_.dpi));
    size_in_pixels.set_height(printing::ConvertUnit(
        current_print_settings_.printable_area.size.height,
        static_cast<int>(printing::kPointsPerInch),
        current_print_settings_.dpi));
    // We need to render using the actual printer DPI (rendering to a smaller
    // set of pixels leads to a blurry output). However, we need to counter the
    // scaling up that will happen in the browser.
    XFORM xform = {0};
    xform.eM11 = xform.eM22 = static_cast<float>(printing::kPointsPerInch) /
        static_cast<float>(current_print_settings_.dpi);
    ModifyWorldTransform(dc, &xform, MWT_LEFTMULTIPLY);

    ret = render_proc(static_cast<unsigned char*>(mapper.data()), mapper.size(),
                      0, dc, current_print_settings_.dpi,
                      current_print_settings_.dpi, 0, 0, size_in_pixels.width(),
                      size_in_pixels.height(), true, false, true, true);
    skia::EndPlatformPaint(canvas);
  }
#endif  // defined(OS_WIN)

  return ret;
}

bool PluginInstance::PrintRasterOutput(PP_Resource print_output,
                                       WebKit::WebCanvas* canvas) {
  EnterResourceNoLock<PPB_ImageData_API> enter(print_output, true);
  if (enter.failed())
    return false;
  PPB_ImageData_Impl* image =
      static_cast<PPB_ImageData_Impl*>(enter.object());

  // TODO(brettw) this should not require the image to be mapped. It should
  // instead map on demand. The DCHECK here is to remind you if you see the
  // assert fire, fix the bug rather than mapping the data.
  DCHECK(image->is_mapped());
  if (!image->is_mapped())
    return false;

  const SkBitmap* bitmap = image->GetMappedBitmap();
  if (!bitmap)
    return false;

  // Draw the printed image into the supplied canvas.
  SkIRect src_rect;
  src_rect.set(0, 0, bitmap->width(), bitmap->height());
  SkRect dest_rect;
  dest_rect.set(
      SkIntToScalar(current_print_settings_.printable_area.point.x),
      SkIntToScalar(current_print_settings_.printable_area.point.y),
      SkIntToScalar(current_print_settings_.printable_area.point.x +
                    current_print_settings_.printable_area.size.width),
      SkIntToScalar(current_print_settings_.printable_area.point.y +
                    current_print_settings_.printable_area.size.height));
  bool draw_to_canvas = true;
  gfx::Rect dest_rect_gfx;
  dest_rect_gfx.set_x(current_print_settings_.printable_area.point.x);
  dest_rect_gfx.set_y(current_print_settings_.printable_area.point.y);
  dest_rect_gfx.set_width(current_print_settings_.printable_area.size.width);
  dest_rect_gfx.set_height(current_print_settings_.printable_area.size.height);

#if defined(OS_WIN)
  // Since this is a raster output, the size of the bitmap can be
  // huge (especially at high printer DPIs). On Windows, this can
  // result in a HUGE EMF (on Mac and Linux the output goes to PDF
  // which appears to Flate compress the bitmap). So, if this bitmap
  // is larger than 20 MB, we save the bitmap as a JPEG into the EMF
  // DC. Note: We chose JPEG over PNG because JPEG compression seems
  // way faster (about 4 times faster).
  static const int kCompressionThreshold = 20 * 1024 * 1024;
  if (bitmap->getSize() > kCompressionThreshold) {
    DrawJPEGToPlatformDC(*bitmap, dest_rect_gfx, canvas);
    draw_to_canvas = false;
  }
#endif  // defined(OS_WIN)
#if defined(OS_MACOSX) && !defined(USE_SKIA)
  draw_to_canvas = false;
  DrawSkBitmapToCanvas(*bitmap, canvas, dest_rect_gfx,
                       current_print_settings_.printable_area.size.height);
  // See comments in the header file.
  last_printed_page_ = image;
#else  // defined(OS_MACOSX) && !defined(USE_SKIA)
  if (draw_to_canvas)
    canvas->drawBitmapRect(*bitmap, &src_rect, dest_rect);
#endif  // defined(OS_MACOSX) && !defined(USE_SKIA)
  return true;
}

#if defined(OS_WIN)
bool PluginInstance::DrawJPEGToPlatformDC(
    const SkBitmap& bitmap,
    const gfx::Rect& printable_area,
    WebKit::WebCanvas* canvas) {
  // Ideally we should add JPEG compression to the VectorPlatformDevice class
  // However, Skia currently has no JPEG compression code and we cannot
  // depend on gfx/jpeg_codec.h in Skia. So we do the compression here.
  SkAutoLockPixels lock(bitmap);
  DCHECK(bitmap.getConfig() == SkBitmap::kARGB_8888_Config);
  const uint32_t* pixels =
      static_cast<const uint32_t*>(bitmap.getPixels());
  std::vector<unsigned char> compressed_image;
  base::TimeTicks start_time = base::TimeTicks::Now();
  bool encoded = gfx::JPEGCodec::Encode(
      reinterpret_cast<const unsigned char*>(pixels),
      gfx::JPEGCodec::FORMAT_BGRA, bitmap.width(), bitmap.height(),
      static_cast<int>(bitmap.rowBytes()), 100, &compressed_image);
  UMA_HISTOGRAM_TIMES("PepperPluginPrint.RasterBitmapCompressTime",
                      base::TimeTicks::Now() - start_time);
  if (!encoded) {
    NOTREACHED();
    return false;
  }

  skia::ScopedPlatformPaint scoped_platform_paint(canvas);
  HDC dc = scoped_platform_paint.GetPlatformSurface();
  // TODO(sanjeevr): This is a temporary hack. If we output a JPEG
  // to the EMF, the EnumEnhMetaFile call fails in the browser
  // process. The failure also happens if we output nothing here.
  // We need to investigate the reason for this failure and fix it.
  // In the meantime this temporary hack of drawing an empty
  // rectangle in the DC gets us by.
  Rectangle(dc, 0, 0, 0, 0);
  BITMAPINFOHEADER bmi = {0};
  gfx::CreateBitmapHeader(bitmap.width(), bitmap.height(), &bmi);
  bmi.biCompression = BI_JPEG;
  bmi.biSizeImage = compressed_image.size();
  bmi.biHeight = -bmi.biHeight;
  StretchDIBits(dc, printable_area.x(), printable_area.y(),
                printable_area.width(), printable_area.height(),
                0, 0, bitmap.width(), bitmap.height(),
                &compressed_image.front(),
                reinterpret_cast<const BITMAPINFO*>(&bmi),
                DIB_RGB_COLORS, SRCCOPY);
  return true;
}
#endif  // OS_WIN

#if defined(OS_MACOSX) && !defined(USE_SKIA)
void PluginInstance::DrawSkBitmapToCanvas(
    const SkBitmap& bitmap, WebKit::WebCanvas* canvas,
    const gfx::Rect& dest_rect,
    int canvas_height) {
  SkAutoLockPixels lock(bitmap);
  DCHECK(bitmap.getConfig() == SkBitmap::kARGB_8888_Config);
  base::mac::ScopedCFTypeRef<CGDataProviderRef> data_provider(
      CGDataProviderCreateWithData(
          NULL, bitmap.getAddr32(0, 0),
          bitmap.rowBytes() * bitmap.height(), NULL));
  base::mac::ScopedCFTypeRef<CGImageRef> image(
      CGImageCreate(
          bitmap.width(), bitmap.height(),
          8, 32, bitmap.rowBytes(),
          base::mac::GetSystemColorSpace(),
          kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host,
          data_provider, NULL, false, kCGRenderingIntentDefault));

  // Flip the transform
  CGContextSaveGState(canvas);
  CGContextTranslateCTM(canvas, 0, canvas_height);
  CGContextScaleCTM(canvas, 1.0, -1.0);

  CGRect bounds;
  bounds.origin.x = dest_rect.x();
  bounds.origin.y = canvas_height - dest_rect.y() - dest_rect.height();
  bounds.size.width = dest_rect.width();
  bounds.size.height = dest_rect.height();

  CGContextDrawImage(canvas, bounds, image);
  CGContextRestoreGState(canvas);
}
#endif  // defined(OS_MACOSX) && !defined(USE_SKIA)

PPB_Graphics2D_Impl* PluginInstance::bound_graphics_2d() const {
  if (bound_graphics_.get() == NULL)
    return NULL;

  if (bound_graphics_->AsPPB_Graphics2D_API())
    return static_cast<PPB_Graphics2D_Impl*>(bound_graphics_.get());
  return NULL;
}

PPB_Surface3D_Impl* PluginInstance::bound_graphics_3d() const {
  if (bound_graphics_.get() == NULL)
    return NULL;

  if (bound_graphics_->AsPPB_Surface3D_API())
    return static_cast<PPB_Surface3D_Impl*>(bound_graphics_.get());
  return NULL;
}

void PluginInstance::setBackingTextureId(unsigned int id) {
  // If we have a full-screen container_ then the plugin is fullscreen,
  // and the parent context is not the one for the browser page, but for the
  // full-screen window, and so the parent texture ID doesn't correspond to
  // anything in the page's context.
  //
  // TODO(alokp): It would be better at some point to have the equivalent
  // in the FullscreenContainer so that we don't need to poll
  if (fullscreen_container_)
    return;

  if (container_)
    container_->setBackingTextureId(id);
}

void PluginInstance::AddPluginObject(PluginObject* plugin_object) {
  DCHECK(live_plugin_objects_.find(plugin_object) ==
         live_plugin_objects_.end());
  live_plugin_objects_.insert(plugin_object);
}

void PluginInstance::RemovePluginObject(PluginObject* plugin_object) {
  // Don't actually verify that the object is in the set since during module
  // deletion we'll be in the process of freeing them.
  live_plugin_objects_.erase(plugin_object);
}

void PluginInstance::AddNPObjectVar(ObjectVar* object_var) {
  DCHECK(np_object_to_object_var_.find(object_var->np_object()) ==
         np_object_to_object_var_.end()) << "ObjectVar already in map";
  np_object_to_object_var_[object_var->np_object()] = object_var;
}

void PluginInstance::RemoveNPObjectVar(ObjectVar* object_var) {
  NPObjectToObjectVarMap::iterator found =
      np_object_to_object_var_.find(object_var->np_object());
  if (found == np_object_to_object_var_.end()) {
    NOTREACHED() << "ObjectVar not registered.";
    return;
  }
  if (found->second != object_var) {
    NOTREACHED() << "ObjectVar doesn't match.";
    return;
  }
  np_object_to_object_var_.erase(found);
}

ObjectVar* PluginInstance::ObjectVarForNPObject(NPObject* np_object) const {
  NPObjectToObjectVarMap::const_iterator found =
      np_object_to_object_var_.find(np_object);
  if (found == np_object_to_object_var_.end())
    return NULL;
  return found->second;
}

bool PluginInstance::IsFullPagePlugin() const {
  WebFrame* frame = container()->element().document().frame();
  return frame->view()->mainFrame()->document().isPluginDocument();
}

PPB_Instance_FunctionAPI* PluginInstance::AsPPB_Instance_FunctionAPI() {
  return this;
}

PP_Bool PluginInstance::BindGraphics(PP_Instance instance,
                                     PP_Resource device) {
  if (!device) {
    // Special-case clearing the current device.
    if (bound_graphics_.get()) {
      if (bound_graphics_2d()) {
        bound_graphics_2d()->BindToInstance(NULL);
      } else if (bound_graphics_.get()) {
        bound_graphics_3d()->BindToInstance(false);
      }
      setBackingTextureId(0);
      InvalidateRect(gfx::Rect());
    }
    bound_graphics_ = NULL;
    return PP_TRUE;
  }

  EnterResourceNoLock<PPB_Graphics2D_API> enter_2d(device, false);
  PPB_Graphics2D_Impl* graphics_2d = enter_2d.succeeded() ?
      static_cast<PPB_Graphics2D_Impl*>(enter_2d.object()) : NULL;
  EnterResourceNoLock<PPB_Surface3D_API> enter_3d(device, false);
  PPB_Surface3D_Impl* graphics_3d = enter_3d.succeeded() ?
      static_cast<PPB_Surface3D_Impl*>(enter_3d.object()) : NULL;

  if (graphics_2d) {
    // Refuse to bind if we're transitioning to fullscreen.
    if (fullscreen_container_ && !fullscreen_)
      return PP_FALSE;
    if (graphics_2d->instance() != this)
      return PP_FALSE;  // Can't bind other instance's contexts.
    if (!graphics_2d->BindToInstance(this))
      return PP_FALSE;  // Can't bind to more than one instance.

    bound_graphics_ = graphics_2d;
    setBackingTextureId(0);
    // BindToInstance will have invalidated the plugin if necessary.
  } else if (graphics_3d) {
    // Refuse to bind if we're transitioning to fullscreen.
    if (fullscreen_container_ && !fullscreen_)
      return PP_FALSE;
    // Make sure graphics can only be bound to the instance it is
    // associated with.
    if (graphics_3d->instance() != this)
      return PP_FALSE;
    if (!graphics_3d->BindToInstance(true))
      return PP_FALSE;

    setBackingTextureId(graphics_3d->GetBackingTextureId());
    bound_graphics_ = graphics_3d;
  } else {
    // The device is not a valid resource type.
    return PP_FALSE;
  }

  return PP_TRUE;
}

PP_Bool PluginInstance::IsFullFrame(PP_Instance instance) {
  return PP_FromBool(full_frame());
}

PP_Var PluginInstance::GetWindowObject(PP_Instance instance) {
  if (!container_)
    return PP_MakeUndefined();

  WebFrame* frame = container_->element().document().frame();
  if (!frame)
    return PP_MakeUndefined();

  return ObjectVar::NPObjectToPPVar(this, frame->windowObject());
}

PP_Var PluginInstance::GetOwnerElementObject(PP_Instance instance) {
  if (!container_)
    return PP_MakeUndefined();
  return ObjectVar::NPObjectToPPVar(this,
                                    container_->scriptableObjectForElement());
}

PP_Var PluginInstance::ExecuteScript(PP_Instance instance,
                                     PP_Var script,
                                     PP_Var* exception) {
  TryCatch try_catch(module(), exception);
  if (try_catch.has_exception())
    return PP_MakeUndefined();

  // Convert the script into an inconvenient NPString object.
  scoped_refptr<StringVar> script_string(StringVar::FromPPVar(script));
  if (!script_string) {
    try_catch.SetException("Script param to ExecuteScript must be a string.");
    return PP_MakeUndefined();
  }
  NPString np_script;
  np_script.UTF8Characters = script_string->value().c_str();
  np_script.UTF8Length = script_string->value().length();

  // Get the current frame to pass to the evaluate function.
  WebFrame* frame = container_->element().document().frame();
  if (!frame) {
    try_catch.SetException("No frame to execute script in.");
    return PP_MakeUndefined();
  }

  NPVariant result;
  bool ok = WebBindings::evaluate(NULL, frame->windowObject(), &np_script,
                                  &result);
  // DANGER! |this| could be deleted at this point if the script removed the
  // plugin from the DOM.
  if (!ok) {
    // TODO(brettw) bug 54011: The TryCatch isn't working properly and
    // doesn't actually catch this exception.
    try_catch.SetException("Exception caught");
    WebBindings::releaseVariantValue(&result);
    return PP_MakeUndefined();
  }

  PP_Var ret = Var::NPVariantToPPVar(this, &result);
  WebBindings::releaseVariantValue(&result);
  return ret;
}

PP_Bool PluginInstance::IsFullscreen(PP_Instance instance) {
  return PP_FromBool(fullscreen_);
}

PP_Bool PluginInstance::SetFullscreen(PP_Instance instance,
                                      PP_Bool fullscreen) {
  // TODO(yzshen): Re-enable it once fullscreen mode is supported on Windows.
#if !defined(OS_WIN)
  SetFullscreen(PPBoolToBool(fullscreen), true);
  return PP_TRUE;
#else
  return PP_FALSE;
#endif
}

PP_Bool PluginInstance::GetScreenSize(PP_Instance instance, PP_Size* size) {
  gfx::Size screen_size = delegate()->GetScreenSize();
  *size = PP_MakeSize(screen_size.width(), screen_size.height());
  return PP_TRUE;
}

int32_t PluginInstance::RequestInputEvents(PP_Instance instance,
                                           uint32_t event_classes) {
  // Don't allow unfiltered keyboard access. The error return value will be
  // generated by ValidateRequestInputEvents.
  uint32_t fixed_classes =
      event_classes & ~uint32_t(PP_INPUTEVENT_CLASS_KEYBOARD);

  input_event_mask_ |= fixed_classes;
  filtered_input_event_mask_ &= ~(fixed_classes);

  // Validate based on the original input to catch errors.
  return ValidateRequestInputEvents(false, event_classes);
}

int32_t PluginInstance::RequestFilteringInputEvents(PP_Instance instance,
                                                    uint32_t event_classes) {
  filtered_input_event_mask_ |= event_classes;
  input_event_mask_ &= ~(event_classes);
  return ValidateRequestInputEvents(true, event_classes);
}

void PluginInstance::ClearInputEventRequest(PP_Instance instance,
                                            uint32_t event_classes) {
  input_event_mask_ &= ~(event_classes);
  filtered_input_event_mask_ &= ~(event_classes);
}

void PluginInstance::ZoomChanged(PP_Instance instance, double factor) {
  // We only want to tell the page to change its zoom if the whole page is the
  // plugin.  If we're in an iframe, then don't do anything.
  if (!IsFullPagePlugin())
    return;

  double zoom_level = WebView::zoomFactorToZoomLevel(factor);
  // The conversino from zoom level to factor, and back, can introduce rounding
  // errors.  i.e. WebKit originally tells us 3.0, but by the time we tell the
  // plugin and it tells us back, the level becomes 3.000000000004.  Need to
  // round or else otherwise if the user zooms out, it will go to 3.0 instead of
  // 2.0.
  int rounded =
      static_cast<int>(zoom_level + (zoom_level > 0 ? 0.001 : -0.001));
  if (abs(rounded - zoom_level) < 0.001)
    zoom_level = rounded;
  container()->zoomLevelChanged(zoom_level);
}

void PluginInstance::ZoomLimitsChanged(PP_Instance instance,
                                       double minimum_factor,
                                       double maximium_factor) {
  if (minimum_factor > maximium_factor) {
    NOTREACHED();
    return;
  }
  delegate()->ZoomLimitsChanged(minimum_factor, maximium_factor);
}

void PluginInstance::PostMessage(PP_Instance instance, PP_Var message) {
  message_channel_->PostMessageToJavaScript(message);
}

}  // namespace ppapi
}  // namespace webkit
