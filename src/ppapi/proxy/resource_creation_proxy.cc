// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/proxy/resource_creation_proxy.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/proxy/host_resource.h"
#include "ppapi/proxy/interface_id.h"
#include "ppapi/proxy/plugin_dispatcher.h"
#include "ppapi/c/trusted/ppb_image_data_trusted.h"
#include "ppapi/proxy/plugin_resource_tracker.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/proxy/ppb_audio_config_proxy.h"
#include "ppapi/proxy/ppb_audio_proxy.h"
#include "ppapi/proxy/ppb_buffer_proxy.h"
#include "ppapi/proxy/ppb_broker_proxy.h"
#include "ppapi/proxy/ppb_context_3d_proxy.h"
#include "ppapi/proxy/ppb_file_chooser_proxy.h"
#include "ppapi/proxy/ppb_file_ref_proxy.h"
#include "ppapi/proxy/ppb_file_system_proxy.h"
#include "ppapi/proxy/ppb_flash_menu_proxy.h"
#include "ppapi/proxy/ppb_flash_net_connector_proxy.h"
#include "ppapi/proxy/ppb_flash_tcp_socket_proxy.h"
#include "ppapi/proxy/ppb_font_proxy.h"
#include "ppapi/proxy/ppb_graphics_2d_proxy.h"
#include "ppapi/proxy/ppb_image_data_proxy.h"
#include "ppapi/proxy/ppb_surface_3d_proxy.h"
#include "ppapi/proxy/ppb_url_loader_proxy.h"
#include "ppapi/proxy/ppb_url_request_info_proxy.h"
#include "ppapi/shared_impl/font_impl.h"
#include "ppapi/shared_impl/function_group_base.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_image_data_api.h"

using ppapi::thunk::ResourceCreationAPI;

namespace pp {
namespace proxy {

ResourceCreationProxy::ResourceCreationProxy(Dispatcher* dispatcher)
    : dispatcher_(dispatcher) {
}

ResourceCreationProxy::~ResourceCreationProxy() {
}

ResourceCreationAPI* ResourceCreationProxy::AsResourceCreationAPI() {
  return this;
}

PP_Resource ResourceCreationProxy::CreateAudio(
    PP_Instance instance,
    PP_Resource config_id,
    PPB_Audio_Callback audio_callback,
    void* user_data) {
  return PPB_Audio_Proxy::CreateProxyResource(instance, config_id,
                                              audio_callback, user_data);
}

PP_Resource ResourceCreationProxy::CreateAudioConfig(
    PP_Instance instance,
    PP_AudioSampleRate sample_rate,
    uint32_t sample_frame_count) {
  return PPB_AudioConfig_Proxy::CreateProxyResource(
      instance, sample_rate, sample_frame_count);
}

PP_Resource ResourceCreationProxy::CreateAudioTrusted(PP_Instance instance) {
  // Proxied plugins can't created trusted audio devices.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateBroker(PP_Instance instance) {
  return PPB_Broker_Proxy::CreateProxyResource(instance);
}

PP_Resource ResourceCreationProxy::CreateBuffer(PP_Instance instance,
                                                uint32_t size) {
  return PPB_Buffer_Proxy::CreateProxyResource(instance, size);
}

PP_Resource ResourceCreationProxy::CreateContext3D(
    PP_Instance instance,
    PP_Config3D_Dev config,
    PP_Resource share_context,
    const int32_t* attrib_list) {
  return PPB_Context3D_Proxy::Create(instance, config, share_context,
                                     attrib_list);
}

PP_Resource ResourceCreationProxy::CreateContext3DRaw(
    PP_Instance instance,
    PP_Config3D_Dev config,
    PP_Resource share_context,
    const int32_t* attrib_list) {
  // Not proxied. The raw creation function is used only in the implementation
  // of the proxy on the host side.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateDirectoryReader(
    PP_Resource directory_ref) {
  NOTIMPLEMENTED();  // Not proxied yet.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateFileChooser(
    PP_Instance instance,
    const PP_FileChooserOptions_Dev* options) {
  return PPB_FileChooser_Proxy::CreateProxyResource(instance, options);
}

PP_Resource ResourceCreationProxy::CreateFileIO(PP_Instance instance) {
  NOTIMPLEMENTED();  // Not proxied yet.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateFileRef(PP_Resource file_system,
                                                 const char* path) {
  return PPB_FileRef_Proxy::CreateProxyResource(file_system, path);
}

PP_Resource ResourceCreationProxy::CreateFileSystem(
    PP_Instance instance,
    PP_FileSystemType type) {
  return PPB_FileSystem_Proxy::CreateProxyResource(instance, type);
}

PP_Resource ResourceCreationProxy::CreateFlashMenu(
    PP_Instance instance,
    const PP_Flash_Menu* menu_data) {
  return PPB_Flash_Menu_Proxy::CreateProxyResource(instance, menu_data);
}

PP_Resource ResourceCreationProxy::CreateFlashNetConnector(
    PP_Instance instance) {
  return PPB_Flash_NetConnector_Proxy::CreateProxyResource(instance);
}

PP_Resource ResourceCreationProxy::CreateFlashTCPSocket(
    PP_Instance instance) {
  return PPB_Flash_TCPSocket_Proxy::CreateProxyResource(instance);
}

PP_Resource ResourceCreationProxy::CreateFontObject(
    PP_Instance instance,
    const PP_FontDescription_Dev* description) {
  if (!ppapi::FontImpl::IsPPFontDescriptionValid(*description))
    return 0;

  linked_ptr<Font> object(new Font(HostResource::MakeInstanceOnly(instance),
                                   *description));
  return PluginResourceTracker::GetInstance()->AddResource(object);
}

PP_Resource ResourceCreationProxy::CreateGraphics2D(PP_Instance instance,
                                                    const PP_Size& size,
                                                    PP_Bool is_always_opaque) {
  return PPB_Graphics2D_Proxy::CreateProxyResource(instance, size,
                                                   is_always_opaque);
}

PP_Resource ResourceCreationProxy::CreateImageData(PP_Instance instance,
                                                   PP_ImageDataFormat format,
                                                   const PP_Size& size,
                                                   PP_Bool init_to_zero) {
  PluginDispatcher* dispatcher = PluginDispatcher::GetForInstance(instance);
  if (!dispatcher)
    return 0;

  HostResource result;
  std::string image_data_desc;
  ImageHandle image_handle = ImageData::NullHandle;
  dispatcher->Send(new PpapiHostMsg_ResourceCreation_ImageData(
      INTERFACE_ID_RESOURCE_CREATION, instance, format, size, init_to_zero,
      &result, &image_data_desc, &image_handle));

  if (result.is_null() || image_data_desc.size() != sizeof(PP_ImageDataDesc))
    return 0;

  // We serialize the PP_ImageDataDesc just by copying to a string.
  PP_ImageDataDesc desc;
  memcpy(&desc, image_data_desc.data(), sizeof(PP_ImageDataDesc));

  linked_ptr<ImageData> object(new ImageData(result, desc, image_handle));
  return PluginResourceTracker::GetInstance()->AddResource(object);
}

PP_Resource ResourceCreationProxy::CreateGraphics3D(
    PP_Instance instance,
    PP_Config3D_Dev config,
    PP_Resource share_context,
    const int32_t* attrib_list) {
  NOTIMPLEMENTED();  // Not proxied yet.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateScrollbar(PP_Instance instance,
                                                   PP_Bool vertical) {
  NOTIMPLEMENTED();  // Not proxied yet.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateSurface3D(
    PP_Instance instance,
    PP_Config3D_Dev config,
    const int32_t* attrib_list) {
  return PPB_Surface3D_Proxy::CreateProxyResource(instance, config,
                                                  attrib_list);
}

PP_Resource ResourceCreationProxy::CreateTransport(PP_Instance instance,
                                                   const char* name,
                                                   const char* proto) {
  NOTIMPLEMENTED();  // Not proxied yet.
  return 0;
}

PP_Resource ResourceCreationProxy::CreateURLLoader(PP_Instance instance) {
  return PPB_URLLoader_Proxy::CreateProxyResource(instance);
}

PP_Resource ResourceCreationProxy::CreateURLRequestInfo(PP_Instance instance) {
  return PPB_URLRequestInfo_Proxy::CreateProxyResource(instance);
}

PP_Resource ResourceCreationProxy::CreateVideoDecoder(PP_Instance instance) {
  NOTIMPLEMENTED();
  return 0;
}

PP_Resource ResourceCreationProxy::CreateVideoLayer(
    PP_Instance instance,
    PP_VideoLayerMode_Dev mode) {
  NOTIMPLEMENTED();
  return 0;
}

bool ResourceCreationProxy::Send(IPC::Message* msg) {
  return dispatcher_->Send(msg);
}

bool ResourceCreationProxy::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ResourceCreationProxy, msg)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_ResourceCreation_Graphics2D,
                        OnMsgCreateGraphics2D)
    IPC_MESSAGE_HANDLER(PpapiHostMsg_ResourceCreation_ImageData,
                        OnMsgCreateImageData)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ResourceCreationProxy::OnMsgCreateGraphics2D(PP_Instance instance,
                                                  const PP_Size& size,
                                                  PP_Bool is_always_opaque,
                                                  HostResource* result) {
  ppapi::thunk::EnterFunction<ResourceCreationAPI> enter(instance, false);
  if (enter.succeeded()) {
    result->SetHostResource(instance, enter.functions()->CreateGraphics2D(
        instance, size, is_always_opaque));
  }
}

void ResourceCreationProxy::OnMsgCreateImageData(
    PP_Instance instance,
    int32_t format,
    const PP_Size& size,
    PP_Bool init_to_zero,
    HostResource* result,
    std::string* image_data_desc,
    ImageHandle* result_image_handle) {
  *result_image_handle = ImageData::NullHandle;

  ppapi::thunk::EnterFunction<ResourceCreationAPI> enter(instance, false);
  if (enter.failed())
    return;

  PP_Resource resource = enter.functions()->CreateImageData(
      instance, static_cast<PP_ImageDataFormat>(format), size, init_to_zero);
  if (!resource)
    return;
  result->SetHostResource(instance, resource);

  // Get the description, it's just serialized as a string.
  ppapi::thunk::EnterResourceNoLock<ppapi::thunk::PPB_ImageData_API>
      enter_resource(resource, false);
  PP_ImageDataDesc desc;
  if (enter_resource.object()->Describe(&desc) == PP_TRUE) {
    image_data_desc->resize(sizeof(PP_ImageDataDesc));
    memcpy(&(*image_data_desc)[0], &desc, sizeof(PP_ImageDataDesc));
  }

  // Get the shared memory handle.
  const PPB_ImageDataTrusted* trusted =
      reinterpret_cast<const PPB_ImageDataTrusted*>(
          dispatcher_->GetLocalInterface(PPB_IMAGEDATA_TRUSTED_INTERFACE));
  uint32_t byte_count = 0;
  if (trusted) {
    int32_t handle;
    if (trusted->GetSharedMemory(resource, &handle, &byte_count) == PP_OK) {
#if defined(OS_WIN)
      pp::proxy::ImageHandle ih = ImageData::HandleFromInt(handle);
      *result_image_handle = dispatcher_->ShareHandleWithRemote(ih, false);
#else
      *result_image_handle = ImageData::HandleFromInt(handle);
#endif
    }
  }
}

}  // namespace proxy
}  // namespace pp
