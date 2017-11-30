// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef MEDIA_GPU_D3D11_MOCKS_H_
#define MEDIA_GPU_D3D11_MOCKS_H_

#include <d3d11.h>
#include <d3d11_1.h>

#include "base/win/iunknown_impl.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {

// This template class is copied from chromium.
template <class Interface>
class MockCOMInterface : public Interface, public base::win::IUnknownImpl {
 public:
  // IUnknown interface
  ULONG STDMETHODCALLTYPE AddRef() override { return IUnknownImpl::AddRef(); }
  ULONG STDMETHODCALLTYPE Release() override { return IUnknownImpl::Release(); }

  STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
    if (riid == __uuidof(Interface)) {
      *ppv = static_cast<Interface*>(this);
      AddRef();
      return S_OK;
    }
    return IUnknownImpl::QueryInterface(riid, ppv);
  }

 protected:
  ~MockCOMInterface() override = default;
};

class D3D11Texture2DMock : public MockCOMInterface<ID3D11Texture2D> {
 public:
  D3D11Texture2DMock();
  ~D3D11Texture2DMock() override;
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDevice,
                             void(ID3D11Device**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(const GUID&, UINT*, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetType,
                             void(D3D11_RESOURCE_DIMENSION*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetEvictionPriority,
                             void(UINT));
  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, GetEvictionPriority, UINT());
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDesc,
                             void(D3D11_TEXTURE2D_DESC*));
};

// This classs must mock QueryInterface, since a lot of things are
// QueryInterfac()ed thru this class.
class D3D11DeviceMock : public MockCOMInterface<ID3D11Device> {
 public:
  D3D11DeviceMock();
  ~D3D11DeviceMock() override;

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             QueryInterface,
                             HRESULT(REFIID riid, void** ppv));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateBuffer,
                             HRESULT(const D3D11_BUFFER_DESC*,
                                     const D3D11_SUBRESOURCE_DATA*,
                                     ID3D11Buffer**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateTexture1D,
                             HRESULT(const D3D11_TEXTURE1D_DESC*,
                                     const D3D11_SUBRESOURCE_DATA*,
                                     ID3D11Texture1D**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateTexture2D,
                             HRESULT(const D3D11_TEXTURE2D_DESC*,
                                     const D3D11_SUBRESOURCE_DATA*,
                                     ID3D11Texture2D**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateTexture3D,
                             HRESULT(const D3D11_TEXTURE3D_DESC*,
                                     const D3D11_SUBRESOURCE_DATA*,
                                     ID3D11Texture3D**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateShaderResourceView,
                             HRESULT(ID3D11Resource*,
                                     const D3D11_SHADER_RESOURCE_VIEW_DESC*,
                                     ID3D11ShaderResourceView**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateUnorderedAccessView,
                             HRESULT(ID3D11Resource*,
                                     const D3D11_UNORDERED_ACCESS_VIEW_DESC*,
                                     ID3D11UnorderedAccessView**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateRenderTargetView,
                             HRESULT(ID3D11Resource*,
                                     const D3D11_RENDER_TARGET_VIEW_DESC*,
                                     ID3D11RenderTargetView**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateDepthStencilView,
                             HRESULT(ID3D11Resource*,
                                     const D3D11_DEPTH_STENCIL_VIEW_DESC*,
                                     ID3D11DepthStencilView**));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateInputLayout,
                             HRESULT(const D3D11_INPUT_ELEMENT_DESC*,
                                     UINT,
                                     const void*,
                                     SIZE_T,
                                     ID3D11InputLayout**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVertexShader,
      HRESULT(const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11VertexShader**));

  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateGeometryShader,
                             HRESULT(const void*,
                                     SIZE_T,
                                     ID3D11ClassLinkage*,
                                     ID3D11GeometryShader**));

  MOCK_METHOD9_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateGeometryShaderWithStreamOutput,
                             HRESULT(const void*,
                                     SIZE_T,
                                     const D3D11_SO_DECLARATION_ENTRY*,
                                     UINT,
                                     const UINT*,
                                     UINT,
                                     UINT,
                                     ID3D11ClassLinkage*,
                                     ID3D11GeometryShader**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreatePixelShader,
      HRESULT(const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11PixelShader**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateHullShader,
      HRESULT(const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11HullShader**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateDomainShader,
      HRESULT(const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11DomainShader**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateComputeShader,
      HRESULT(const void*, SIZE_T, ID3D11ClassLinkage*, ID3D11ComputeShader**));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateClassLinkage,
                             HRESULT(ID3D11ClassLinkage**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateBlendState,
                             HRESULT(const D3D11_BLEND_DESC*,
                                     ID3D11BlendState**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateDepthStencilState,
                             HRESULT(const D3D11_DEPTH_STENCIL_DESC*,
                                     ID3D11DepthStencilState**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateRasterizerState,
                             HRESULT(const D3D11_RASTERIZER_DESC*,
                                     ID3D11RasterizerState**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateSamplerState,
                             HRESULT(const D3D11_SAMPLER_DESC*,
                                     ID3D11SamplerState**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateQuery,
                             HRESULT(const D3D11_QUERY_DESC*, ID3D11Query**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreatePredicate,
                             HRESULT(const D3D11_QUERY_DESC*,
                                     ID3D11Predicate**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateCounter,
                             HRESULT(const D3D11_COUNTER_DESC*,
                                     ID3D11Counter**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateDeferredContext,
                             HRESULT(UINT, ID3D11DeviceContext**));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             OpenSharedResource,
                             HRESULT(HANDLE, const IID&, void**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckFormatSupport,
                             HRESULT(DXGI_FORMAT, UINT*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckMultisampleQualityLevels,
                             HRESULT(DXGI_FORMAT, UINT, UINT*));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckCounterInfo,
                             void(D3D11_COUNTER_INFO*));

  MOCK_METHOD9_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckCounter,
                             HRESULT(const D3D11_COUNTER_DESC*,
                                     D3D11_COUNTER_TYPE*,
                                     UINT*,
                                     LPSTR,
                                     UINT*,
                                     LPSTR,
                                     UINT*,
                                     LPSTR,
                                     UINT*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckFeatureSupport,
                             HRESULT(D3D11_FEATURE, void*, UINT));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(const GUID&, UINT*, void*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetFeatureLevel,
                             D3D_FEATURE_LEVEL());

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, GetCreationFlags, UINT());

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDeviceRemovedReason,
                             HRESULT());

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetImmediateContext,
                             void(ID3D11DeviceContext**));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetExceptionMode,
                             HRESULT(UINT));

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, GetExceptionMode, UINT());
};

// TODO(crbug.com/788880): This may not be necessary. Tyr out and see if
// D3D11VideoDevice1Mock is sufficient. and if so, remove this.
class D3D11VideoDeviceMock : public MockCOMInterface<ID3D11VideoDevice> {
 public:
  D3D11VideoDeviceMock();
  ~D3D11VideoDeviceMock() override;
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateVideoDecoder,
                             HRESULT(const D3D11_VIDEO_DECODER_DESC*,
                                     const D3D11_VIDEO_DECODER_CONFIG*,
                                     ID3D11VideoDecoder**));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateVideoProcessor,
                             HRESULT(ID3D11VideoProcessorEnumerator*,
                                     UINT,
                                     ID3D11VideoProcessor**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateAuthenticatedChannel,
                             HRESULT(D3D11_AUTHENTICATED_CHANNEL_TYPE,
                                     ID3D11AuthenticatedChannel**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateCryptoSession,
      HRESULT(const GUID*, const GUID*, const GUID*, ID3D11CryptoSession**));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVideoDecoderOutputView,
      HRESULT(ID3D11Resource*,
              const D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC*,
              ID3D11VideoDecoderOutputView**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVideoProcessorInputView,
      HRESULT(ID3D11Resource*,
              ID3D11VideoProcessorEnumerator*,
              const D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC*,
              ID3D11VideoProcessorInputView**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVideoProcessorOutputView,
      HRESULT(ID3D11Resource*,
              ID3D11VideoProcessorEnumerator*,
              const D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC*,
              ID3D11VideoProcessorOutputView**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateVideoProcessorEnumerator,
                             HRESULT(const D3D11_VIDEO_PROCESSOR_CONTENT_DESC*,
                                     ID3D11VideoProcessorEnumerator**));

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderProfileCount,
                             UINT());

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderProfile,
                             HRESULT(UINT, GUID*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckVideoDecoderFormat,
                             HRESULT(const GUID*, DXGI_FORMAT, BOOL*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderConfigCount,
                             HRESULT(const D3D11_VIDEO_DECODER_DESC*, UINT*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderConfig,
                             HRESULT(const D3D11_VIDEO_DECODER_DESC*,
                                     UINT,
                                     D3D11_VIDEO_DECODER_CONFIG*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetContentProtectionCaps,
                             HRESULT(const GUID*,
                                     const GUID*,
                                     D3D11_VIDEO_CONTENT_PROTECTION_CAPS*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckCryptoKeyExchange,
                             HRESULT(const GUID*, const GUID*, UINT, GUID*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
};

class D3D11VideoDevice1Mock : public MockCOMInterface<ID3D11VideoDevice1> {
 public:
  D3D11VideoDevice1Mock();
  ~D3D11VideoDevice1Mock() override;

  MOCK_METHOD7_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CheckVideoDecoderDownsampling,
      HRESULT(const D3D11_VIDEO_DECODER_DESC* pInputDesc,
              DXGI_COLOR_SPACE_TYPE InputColorSpace,
              const D3D11_VIDEO_DECODER_CONFIG* pInputConfig,
              const DXGI_RATIONAL* pFrameRate,
              const D3D11_VIDEO_SAMPLE_DESC* pOutputDesc,
              BOOL* pSupported,
              BOOL* pRealTimeHint));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetCryptoSessionPrivateDataSize,
                             HRESULT(const GUID* pCryptoType,
                                     const GUID* pDecoderProfile,
                                     const GUID* pKeyExchangeType,
                                     UINT* pPrivateInputSize,
                                     UINT* pPrivateOutputSize));
  MOCK_METHOD7_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderCaps,
                             HRESULT(const GUID* pDecoderProfile,
                                     UINT SampleWidth,
                                     UINT SampleHeight,
                                     const DXGI_RATIONAL* pFrameRate,
                                     UINT BitRate,
                                     const GUID* pCryptoType,
                                     UINT* pDecoderCaps));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      RecommendVideoDecoderDownsampleParameters,
      HRESULT(const D3D11_VIDEO_DECODER_DESC* pInputDesc,
              DXGI_COLOR_SPACE_TYPE InputColorSpace,
              const D3D11_VIDEO_DECODER_CONFIG* pInputConfig,
              const DXGI_RATIONAL* pFrameRate,
              D3D11_VIDEO_SAMPLE_DESC* pRecommendedOutputDesc));

  // ID3D11VideoDevice methods.
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateVideoDecoder,
                             HRESULT(const D3D11_VIDEO_DECODER_DESC*,
                                     const D3D11_VIDEO_DECODER_CONFIG*,
                                     ID3D11VideoDecoder**));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateVideoProcessor,
                             HRESULT(ID3D11VideoProcessorEnumerator*,
                                     UINT,
                                     ID3D11VideoProcessor**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateAuthenticatedChannel,
                             HRESULT(D3D11_AUTHENTICATED_CHANNEL_TYPE,
                                     ID3D11AuthenticatedChannel**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateCryptoSession,
      HRESULT(const GUID*, const GUID*, const GUID*, ID3D11CryptoSession**));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVideoDecoderOutputView,
      HRESULT(ID3D11Resource*,
              const D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC*,
              ID3D11VideoDecoderOutputView**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVideoProcessorInputView,
      HRESULT(ID3D11Resource*,
              ID3D11VideoProcessorEnumerator*,
              const D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC*,
              ID3D11VideoProcessorInputView**));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CreateVideoProcessorOutputView,
      HRESULT(ID3D11Resource*,
              ID3D11VideoProcessorEnumerator*,
              const D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC*,
              ID3D11VideoProcessorOutputView**));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CreateVideoProcessorEnumerator,
                             HRESULT(const D3D11_VIDEO_PROCESSOR_CONTENT_DESC*,
                                     ID3D11VideoProcessorEnumerator**));

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderProfileCount,
                             UINT());

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderProfile,
                             HRESULT(UINT, GUID*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckVideoDecoderFormat,
                             HRESULT(const GUID*, DXGI_FORMAT, BOOL*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderConfigCount,
                             HRESULT(const D3D11_VIDEO_DECODER_DESC*, UINT*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetVideoDecoderConfig,
                             HRESULT(const D3D11_VIDEO_DECODER_DESC*,
                                     UINT,
                                     D3D11_VIDEO_DECODER_CONFIG*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetContentProtectionCaps,
                             HRESULT(const GUID*,
                                     const GUID*,
                                     D3D11_VIDEO_CONTENT_PROTECTION_CAPS*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckCryptoKeyExchange,
                             HRESULT(const GUID*, const GUID*, UINT, GUID*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
};

class D3D11VideoContextMock : public MockCOMInterface<ID3D11VideoContext> {
 public:
  D3D11VideoContextMock();
  ~D3D11VideoContextMock() override;
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDevice,
                             void(ID3D11Device**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(const GUID&, UINT*, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDecoderBuffer,
                             HRESULT(ID3D11VideoDecoder*,
                                     D3D11_VIDEO_DECODER_BUFFER_TYPE,
                                     UINT*,
                                     void**));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ReleaseDecoderBuffer,
                             HRESULT(ID3D11VideoDecoder*,
                                     D3D11_VIDEO_DECODER_BUFFER_TYPE));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecoderBeginFrame,
                             HRESULT(ID3D11VideoDecoder*,
                                     ID3D11VideoDecoderOutputView*,
                                     UINT,
                                     const void*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecoderEndFrame,
                             HRESULT(ID3D11VideoDecoder*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SubmitDecoderBuffers,
                             HRESULT(ID3D11VideoDecoder*,
                                     UINT,
                                     const D3D11_VIDEO_DECODER_BUFFER_DESC*));
  MOCK_METHOD2_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      DecoderExtension,
      APP_DEPRECATED_HRESULT(ID3D11VideoDecoder*,
                             const D3D11_VIDEO_DECODER_EXTENSION*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputTargetRect,
                             void(ID3D11VideoProcessor*, BOOL, const RECT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputBackgroundColor,
                             void(ID3D11VideoProcessor*,
                                  BOOL,
                                  const D3D11_VIDEO_COLOR*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputColorSpace,
                             void(ID3D11VideoProcessor*,
                                  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputAlphaFillMode,
                             void(ID3D11VideoProcessor*,
                                  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE,
                                  UINT));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputConstriction,
                             void(ID3D11VideoProcessor*, BOOL, SIZE));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputStereoMode,
                             void(ID3D11VideoProcessor*, BOOL));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetOutputExtension,
      APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*, const GUID*, UINT, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputTargetRect,
                             void(ID3D11VideoProcessor*, BOOL*, RECT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputBackgroundColor,
                             void(ID3D11VideoProcessor*,
                                  BOOL*,
                                  D3D11_VIDEO_COLOR*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputColorSpace,
                             void(ID3D11VideoProcessor*,
                                  D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputAlphaFillMode,
                             void(ID3D11VideoProcessor*,
                                  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE*,
                                  UINT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputConstriction,
                             void(ID3D11VideoProcessor*, BOOL*, SIZE*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputStereoMode,
                             void(ID3D11VideoProcessor*, BOOL*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetOutputExtension,
      APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*, const GUID*, UINT, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamFrameFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_FRAME_FORMAT));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamColorSpace,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamOutputRate,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_OUTPUT_RATE,
                                  BOOL,
                                  const DXGI_RATIONAL*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamSourceRect,
      void(ID3D11VideoProcessor*, UINT, BOOL, const RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamDestRect,
      void(ID3D11VideoProcessor*, UINT, BOOL, const RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamAlpha,
                             void(ID3D11VideoProcessor*, UINT, BOOL, FLOAT));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamPalette,
      void(ID3D11VideoProcessor*, UINT, UINT, const UINT*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamPixelAspectRatio,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL,
                                  const DXGI_RATIONAL*,
                                  const DXGI_RATIONAL*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamLumaKey,
      void(ID3D11VideoProcessor*, UINT, BOOL, FLOAT, FLOAT));
  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamStereoFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FORMAT,
                                  BOOL,
                                  BOOL,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE,
                                  int));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamAutoProcessingMode,
                             void(ID3D11VideoProcessor*, UINT, BOOL));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamFilter,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_FILTER,
                                  BOOL,
                                  int));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamExtension,
                             APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*,
                                                    UINT,
                                                    const GUID*,
                                                    UINT,
                                                    void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamFrameFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_FRAME_FORMAT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamColorSpace,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamOutputRate,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_OUTPUT_RATE*,
                                  BOOL*,
                                  DXGI_RATIONAL*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamSourceRect,
                             void(ID3D11VideoProcessor*, UINT, BOOL*, RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamDestRect,
                             void(ID3D11VideoProcessor*, UINT, BOOL*, RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamAlpha,
                             void(ID3D11VideoProcessor*, UINT, BOOL*, FLOAT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamPalette,
                             void(ID3D11VideoProcessor*, UINT, UINT, UINT*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetStreamPixelAspectRatio,
      void(ID3D11VideoProcessor*, UINT, BOOL*, DXGI_RATIONAL*, DXGI_RATIONAL*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetStreamLumaKey,
      void(ID3D11VideoProcessor*, UINT, BOOL*, FLOAT*, FLOAT*));
  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamStereoFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL*,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FORMAT*,
                                  BOOL*,
                                  BOOL*,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE*,
                                  int*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamAutoProcessingMode,
                             void(ID3D11VideoProcessor*, UINT, BOOL*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamFilter,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_FILTER,
                                  BOOL*,
                                  int*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamExtension,
                             APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*,
                                                    UINT,
                                                    const GUID*,
                                                    UINT,
                                                    void*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorBlt,
                             HRESULT(ID3D11VideoProcessor*,
                                     ID3D11VideoProcessorOutputView*,
                                     UINT,
                                     UINT,
                                     const D3D11_VIDEO_PROCESSOR_STREAM*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             NegotiateCryptoSessionKeyExchange,
                             HRESULT(ID3D11CryptoSession*, UINT, void*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             EncryptionBlt,
                             void(ID3D11CryptoSession*,
                                  ID3D11Texture2D*,
                                  ID3D11Texture2D*,
                                  UINT,
                                  void*));
  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecryptionBlt,
                             void(ID3D11CryptoSession*,
                                  ID3D11Texture2D*,
                                  ID3D11Texture2D*,
                                  D3D11_ENCRYPTED_BLOCK_INFO*,
                                  UINT,
                                  const void*,
                                  UINT,
                                  void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             StartSessionKeyRefresh,
                             void(ID3D11CryptoSession*, UINT, void*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             FinishSessionKeyRefresh,
                             void(ID3D11CryptoSession*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetEncryptionBltKey,
                             HRESULT(ID3D11CryptoSession*, UINT, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             NegotiateAuthenticatedChannelKeyExchange,
                             HRESULT(ID3D11AuthenticatedChannel*, UINT, void*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      QueryAuthenticatedChannel,
      HRESULT(ID3D11AuthenticatedChannel*, UINT, const void*, UINT, void*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ConfigureAuthenticatedChannel,
                             HRESULT(ID3D11AuthenticatedChannel*,
                                     UINT,
                                     const void*,
                                     D3D11_AUTHENTICATED_CONFIGURE_OUTPUT*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamRotation,
      void(ID3D11VideoProcessor*, UINT, BOOL, D3D11_VIDEO_PROCESSOR_ROTATION));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamRotation,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL*,
                                  D3D11_VIDEO_PROCESSOR_ROTATION*));
};

class D3D11VideoContext1Mock : public MockCOMInterface<ID3D11VideoContext1> {
 public:
  D3D11VideoContext1Mock();
  ~D3D11VideoContext1Mock() override;
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDevice,
                             void(ID3D11Device**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(const GUID&, UINT*, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDecoderBuffer,
                             HRESULT(ID3D11VideoDecoder*,
                                     D3D11_VIDEO_DECODER_BUFFER_TYPE,
                                     UINT*,
                                     void**));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ReleaseDecoderBuffer,
                             HRESULT(ID3D11VideoDecoder*,
                                     D3D11_VIDEO_DECODER_BUFFER_TYPE));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecoderBeginFrame,
                             HRESULT(ID3D11VideoDecoder*,
                                     ID3D11VideoDecoderOutputView*,
                                     UINT,
                                     const void*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecoderEndFrame,
                             HRESULT(ID3D11VideoDecoder*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SubmitDecoderBuffers,
                             HRESULT(ID3D11VideoDecoder*,
                                     UINT,
                                     const D3D11_VIDEO_DECODER_BUFFER_DESC*));
  MOCK_METHOD2_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      DecoderExtension,
      APP_DEPRECATED_HRESULT(ID3D11VideoDecoder*,
                             const D3D11_VIDEO_DECODER_EXTENSION*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputTargetRect,
                             void(ID3D11VideoProcessor*, BOOL, const RECT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputBackgroundColor,
                             void(ID3D11VideoProcessor*,
                                  BOOL,
                                  const D3D11_VIDEO_COLOR*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputColorSpace,
                             void(ID3D11VideoProcessor*,
                                  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputAlphaFillMode,
                             void(ID3D11VideoProcessor*,
                                  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE,
                                  UINT));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputConstriction,
                             void(ID3D11VideoProcessor*, BOOL, SIZE));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputStereoMode,
                             void(ID3D11VideoProcessor*, BOOL));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetOutputExtension,
      APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*, const GUID*, UINT, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputTargetRect,
                             void(ID3D11VideoProcessor*, BOOL*, RECT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputBackgroundColor,
                             void(ID3D11VideoProcessor*,
                                  BOOL*,
                                  D3D11_VIDEO_COLOR*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputColorSpace,
                             void(ID3D11VideoProcessor*,
                                  D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputAlphaFillMode,
                             void(ID3D11VideoProcessor*,
                                  D3D11_VIDEO_PROCESSOR_ALPHA_FILL_MODE*,
                                  UINT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputConstriction,
                             void(ID3D11VideoProcessor*, BOOL*, SIZE*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputStereoMode,
                             void(ID3D11VideoProcessor*, BOOL*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetOutputExtension,
      APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*, const GUID*, UINT, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamFrameFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_FRAME_FORMAT));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamColorSpace,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  const D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamOutputRate,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_OUTPUT_RATE,
                                  BOOL,
                                  const DXGI_RATIONAL*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamSourceRect,
      void(ID3D11VideoProcessor*, UINT, BOOL, const RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamDestRect,
      void(ID3D11VideoProcessor*, UINT, BOOL, const RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamAlpha,
                             void(ID3D11VideoProcessor*, UINT, BOOL, FLOAT));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamPalette,
      void(ID3D11VideoProcessor*, UINT, UINT, const UINT*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamPixelAspectRatio,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL,
                                  const DXGI_RATIONAL*,
                                  const DXGI_RATIONAL*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamLumaKey,
      void(ID3D11VideoProcessor*, UINT, BOOL, FLOAT, FLOAT));
  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamStereoFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FORMAT,
                                  BOOL,
                                  BOOL,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE,
                                  int));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamAutoProcessingMode,
                             void(ID3D11VideoProcessor*, UINT, BOOL));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamFilter,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_FILTER,
                                  BOOL,
                                  int));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamExtension,
                             APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*,
                                                    UINT,
                                                    const GUID*,
                                                    UINT,
                                                    void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamFrameFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_FRAME_FORMAT*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamColorSpace,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_COLOR_SPACE*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamOutputRate,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_OUTPUT_RATE*,
                                  BOOL*,
                                  DXGI_RATIONAL*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamSourceRect,
                             void(ID3D11VideoProcessor*, UINT, BOOL*, RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamDestRect,
                             void(ID3D11VideoProcessor*, UINT, BOOL*, RECT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamAlpha,
                             void(ID3D11VideoProcessor*, UINT, BOOL*, FLOAT*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamPalette,
                             void(ID3D11VideoProcessor*, UINT, UINT, UINT*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetStreamPixelAspectRatio,
      void(ID3D11VideoProcessor*, UINT, BOOL*, DXGI_RATIONAL*, DXGI_RATIONAL*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetStreamLumaKey,
      void(ID3D11VideoProcessor*, UINT, BOOL*, FLOAT*, FLOAT*));
  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamStereoFormat,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL*,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FORMAT*,
                                  BOOL*,
                                  BOOL*,
                                  D3D11_VIDEO_PROCESSOR_STEREO_FLIP_MODE*,
                                  int*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamAutoProcessingMode,
                             void(ID3D11VideoProcessor*, UINT, BOOL*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamFilter,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  D3D11_VIDEO_PROCESSOR_FILTER,
                                  BOOL*,
                                  int*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamExtension,
                             APP_DEPRECATED_HRESULT(ID3D11VideoProcessor*,
                                                    UINT,
                                                    const GUID*,
                                                    UINT,
                                                    void*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorBlt,
                             HRESULT(ID3D11VideoProcessor*,
                                     ID3D11VideoProcessorOutputView*,
                                     UINT,
                                     UINT,
                                     const D3D11_VIDEO_PROCESSOR_STREAM*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             NegotiateCryptoSessionKeyExchange,
                             HRESULT(ID3D11CryptoSession*, UINT, void*));
  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             EncryptionBlt,
                             void(ID3D11CryptoSession*,
                                  ID3D11Texture2D*,
                                  ID3D11Texture2D*,
                                  UINT,
                                  void*));
  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecryptionBlt,
                             void(ID3D11CryptoSession*,
                                  ID3D11Texture2D*,
                                  ID3D11Texture2D*,
                                  D3D11_ENCRYPTED_BLOCK_INFO*,
                                  UINT,
                                  const void*,
                                  UINT,
                                  void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             StartSessionKeyRefresh,
                             void(ID3D11CryptoSession*, UINT, void*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             FinishSessionKeyRefresh,
                             void(ID3D11CryptoSession*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetEncryptionBltKey,
                             HRESULT(ID3D11CryptoSession*, UINT, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             NegotiateAuthenticatedChannelKeyExchange,
                             HRESULT(ID3D11AuthenticatedChannel*, UINT, void*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      QueryAuthenticatedChannel,
      HRESULT(ID3D11AuthenticatedChannel*, UINT, const void*, UINT, void*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ConfigureAuthenticatedChannel,
                             HRESULT(ID3D11AuthenticatedChannel*,
                                     UINT,
                                     const void*,
                                     D3D11_AUTHENTICATED_CONFIGURE_OUTPUT*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamRotation,
      void(ID3D11VideoProcessor*, UINT, BOOL, D3D11_VIDEO_PROCESSOR_ROTATION));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamRotation,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  BOOL*,
                                  D3D11_VIDEO_PROCESSOR_ROTATION*));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SubmitDecoderBuffers1,
                             HRESULT(ID3D11VideoDecoder*,
                                     UINT,
                                     const D3D11_VIDEO_DECODER_BUFFER_DESC1*));
  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      GetDataForNewHardwareKey,
      HRESULT(ID3D11CryptoSession*, UINT, const void*, UINT64*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CheckCryptoSessionStatus,
                             HRESULT(ID3D11CryptoSession*,
                                     D3D11_CRYPTO_SESSION_STATUS*));
  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecoderEnableDownsampling,
                             HRESULT(ID3D11VideoDecoder*,
                                     DXGI_COLOR_SPACE_TYPE,
                                     const D3D11_VIDEO_SAMPLE_DESC*,
                                     UINT));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DecoderUpdateDownsampling,
                             HRESULT(ID3D11VideoDecoder*,
                                     const D3D11_VIDEO_SAMPLE_DESC*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputColorSpace1,
                             void(ID3D11VideoProcessor*,
                                  DXGI_COLOR_SPACE_TYPE));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetOutputShaderUsage,
                             void(ID3D11VideoProcessor*, BOOL));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputColorSpace1,
                             void(ID3D11VideoProcessor*,
                                  DXGI_COLOR_SPACE_TYPE*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetOutputShaderUsage,
                             void(ID3D11VideoProcessor*, BOOL*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorSetStreamColorSpace1,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  DXGI_COLOR_SPACE_TYPE));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorSetStreamMirror,
      void(ID3D11VideoProcessor*, UINT, BOOL, BOOL, BOOL));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VideoProcessorGetStreamColorSpace1,
                             void(ID3D11VideoProcessor*,
                                  UINT,
                                  DXGI_COLOR_SPACE_TYPE*));
  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetStreamMirror,
      void(ID3D11VideoProcessor*, UINT, BOOL*, BOOL*, BOOL*));
  MOCK_METHOD7_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VideoProcessorGetBehaviorHints,
      HRESULT(ID3D11VideoProcessor*,
              UINT,
              UINT,
              DXGI_FORMAT,
              UINT,
              const D3D11_VIDEO_PROCESSOR_STREAM_BEHAVIOR_HINT*,
              UINT*));
};

class D3D11VideoDecoderMock : public MockCOMInterface<ID3D11VideoDecoder> {
 public:
  D3D11VideoDecoderMock();
  ~D3D11VideoDecoderMock() override;
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDevice,
                             void(ID3D11Device**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(const GUID&, UINT*, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetCreationParameters,
                             HRESULT(D3D11_VIDEO_DECODER_DESC*,
                                     D3D11_VIDEO_DECODER_CONFIG*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDriverHandle,
                             HRESULT(HANDLE*));
};

class D3D11CryptoSessionMock : public MockCOMInterface<ID3D11CryptoSession> {
 public:
  D3D11CryptoSessionMock();
  ~D3D11CryptoSessionMock() override;
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDevice,
                             void(ID3D11Device**));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(const GUID&, UINT*, void*));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(const GUID&, UINT, const void*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(const GUID&, const IUnknown*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE, GetCryptoType, void(GUID*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE, GetDecoderProfile, void(GUID*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetCertificateSize,
                             HRESULT(UINT*));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetCertificate,
                             HRESULT(UINT, BYTE*));
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetCryptoSessionHandle,
                             void(HANDLE*));
};

// This classs must mock QueryInterface, since a lot of things are
// QueryInterfac()ed thru this class.
class D3D11DeviceContextMock : public MockCOMInterface<ID3D11DeviceContext> {
 public:
  D3D11DeviceContextMock();
  ~D3D11DeviceContextMock() override;

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             QueryInterface,
                             HRESULT(REFIID riid, void** ppv));

  // ID3D11DevieChild methods.
  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetDevice,
                             void(ID3D11Device** ppDevice));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPrivateData,
                             HRESULT(REFGUID guid,
                                     UINT* pDataSize,
                                     void* pData));
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateData,
                             HRESULT(REFGUID guid,
                                     UINT DataSize,
                                     const void* pData));
  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPrivateDataInterface,
                             HRESULT(REFGUID guid, const IUnknown* pData));

  // ID3D11DeviceContext methods.
  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VSSetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      PSSetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView* const* ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             PSSetShader,
                             void(ID3D11PixelShader* pPixelShader,
                                  ID3D11ClassInstance* const* ppClassInstances,
                                  UINT NumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             PSSetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState* const* ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VSSetShader,
                             void(ID3D11VertexShader* pVertexShader,
                                  ID3D11ClassInstance* const* ppClassInstances,
                                  UINT NumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DrawIndexed,
                             void(UINT IndexCount,
                                  UINT StartIndexLocation,
                                  INT BaseVertexLocation));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             Draw,
                             void(UINT VertexCount, UINT StartVertexLocation));

  MOCK_METHOD5_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      Map,
      HRESULT(ID3D11Resource* pResource,
              UINT Subresource,
              D3D11_MAP MapType,
              UINT MapFlags,
              D3D11_MAPPED_SUBRESOURCE* pMappedResource));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             Unmap,
                             void(ID3D11Resource* pResource, UINT Subresource));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             PSSetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppConstantBuffers));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IASetInputLayout,
                             void(ID3D11InputLayout* pInputLayout));

  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IASetVertexBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppVertexBuffers,
                                  const UINT* pStrides,
                                  const UINT* pOffsets));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IASetIndexBuffer,
                             void(ID3D11Buffer* pIndexBuffer,
                                  DXGI_FORMAT Format,
                                  UINT Offset));

  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DrawIndexedInstanced,
                             void(UINT IndexCountPerInstance,
                                  UINT InstanceCount,
                                  UINT StartIndexLocation,
                                  INT BaseVertexLocation,
                                  UINT StartInstanceLocation));

  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DrawInstanced,
                             void(UINT VertexCountPerInstance,
                                  UINT InstanceCount,
                                  UINT StartVertexLocation,
                                  UINT StartInstanceLocation));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GSSetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GSSetShader,
                             void(ID3D11GeometryShader* pShader,
                                  ID3D11ClassInstance* const* ppClassInstances,
                                  UINT NumClassInstances));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IASetPrimitiveTopology,
                             void(D3D11_PRIMITIVE_TOPOLOGY Topology));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VSSetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView* const* ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VSSetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState* const* ppSamplers));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             Begin,
                             void(ID3D11Asynchronous* pAsync));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             End,
                             void(ID3D11Asynchronous* pAsync));

  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetData,
                             HRESULT(ID3D11Asynchronous* pAsync,
                                     void* pData,
                                     UINT DataSize,
                                     UINT GetDataFlags));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetPredication,
                             void(ID3D11Predicate* pPredicate,
                                  BOOL PredicateValue));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      GSSetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView* const* ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GSSetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState* const* ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      OMSetRenderTargets,
      void(UINT NumViews,
           ID3D11RenderTargetView* const* ppRenderTargetViews,
           ID3D11DepthStencilView* pDepthStencilView));

  MOCK_METHOD7_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      OMSetRenderTargetsAndUnorderedAccessViews,
      void(UINT NumRTVs,
           ID3D11RenderTargetView* const* ppRenderTargetViews,
           ID3D11DepthStencilView* pDepthStencilView,
           UINT UAVStartSlot,
           UINT NumUAVs,
           ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
           const UINT* pUAVInitialCounts));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             OMSetBlendState,
                             void(ID3D11BlendState* pBlendState,
                                  const FLOAT BlendFactor[4],
                                  UINT SampleMask));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             OMSetDepthStencilState,
                             void(ID3D11DepthStencilState* pDepthStencilState,
                                  UINT StencilRef));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SOSetTargets,
                             void(UINT NumBuffers,
                                  ID3D11Buffer* const* ppSOTargets,
                                  const UINT* pOffsets));

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, DrawAuto, void());

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DrawIndexedInstancedIndirect,
                             void(ID3D11Buffer* pBufferForArgs,
                                  UINT AlignedByteOffsetForArgs));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DrawInstancedIndirect,
                             void(ID3D11Buffer* pBufferForArgs,
                                  UINT AlignedByteOffsetForArgs));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             Dispatch,
                             void(UINT ThreadGroupCountX,
                                  UINT ThreadGroupCountY,
                                  UINT ThreadGroupCountZ));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DispatchIndirect,
                             void(ID3D11Buffer* pBufferForArgs,
                                  UINT AlignedByteOffsetForArgs));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             RSSetState,
                             void(ID3D11RasterizerState* pRasterizerState));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             RSSetViewports,
                             void(UINT NumViewports,
                                  const D3D11_VIEWPORT* pViewports));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             RSSetScissorRects,
                             void(UINT NumRects, const D3D11_RECT* pRects));

  MOCK_METHOD8_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CopySubresourceRegion,
                             void(ID3D11Resource* pDstResource,
                                  UINT DstSubresource,
                                  UINT DstX,
                                  UINT DstY,
                                  UINT DstZ,
                                  ID3D11Resource* pSrcResource,
                                  UINT SrcSubresource,
                                  const D3D11_BOX* pSrcBox));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CopyResource,
                             void(ID3D11Resource* pDstResource,
                                  ID3D11Resource* pSrcResource));

  MOCK_METHOD6_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             UpdateSubresource,
                             void(ID3D11Resource* pDstResource,
                                  UINT DstSubresource,
                                  const D3D11_BOX* pDstBox,
                                  const void* pSrcData,
                                  UINT SrcRowPitch,
                                  UINT SrcDepthPitch));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CopyStructureCount,
                             void(ID3D11Buffer* pDstBuffer,
                                  UINT DstAlignedByteOffset,
                                  ID3D11UnorderedAccessView* pSrcView));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ClearRenderTargetView,
                             void(ID3D11RenderTargetView* pRenderTargetView,
                                  const FLOAT ColorRGBA[4]));

  MOCK_METHOD2_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      ClearUnorderedAccessViewUint,
      void(ID3D11UnorderedAccessView* pUnorderedAccessView,
           const UINT Values[4]));

  MOCK_METHOD2_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      ClearUnorderedAccessViewFloat,
      void(ID3D11UnorderedAccessView* pUnorderedAccessView,
           const FLOAT Values[4]));

  MOCK_METHOD4_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ClearDepthStencilView,
                             void(ID3D11DepthStencilView* pDepthStencilView,
                                  UINT ClearFlags,
                                  FLOAT Depth,
                                  UINT8 Stencil));

  MOCK_METHOD1_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      GenerateMips,
      void(ID3D11ShaderResourceView* pShaderResourceView));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SetResourceMinLOD,
                             void(ID3D11Resource* pResource, FLOAT MinLOD));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetResourceMinLOD,
                             FLOAT(ID3D11Resource* pResource));

  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ResolveSubresource,
                             void(ID3D11Resource* pDstResource,
                                  UINT DstSubresource,
                                  ID3D11Resource* pSrcResource,
                                  UINT SrcSubresource,
                                  DXGI_FORMAT Format));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             ExecuteCommandList,
                             void(ID3D11CommandList* pCommandList,
                                  BOOL RestoreContextState));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      HSSetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView* const* ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             HSSetShader,
                             void(ID3D11HullShader* pHullShader,
                                  ID3D11ClassInstance* const* ppClassInstances,
                                  UINT NumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             HSSetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState* const* ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             HSSetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      DSSetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView* const* ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DSSetShader,
                             void(ID3D11DomainShader* pDomainShader,
                                  ID3D11ClassInstance* const* ppClassInstances,
                                  UINT NumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DSSetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState* const* ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DSSetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CSSetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView* const* ppShaderResourceViews));

  MOCK_METHOD4_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CSSetUnorderedAccessViews,
      void(UINT StartSlot,
           UINT NumUAVs,
           ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
           const UINT* pUAVInitialCounts));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CSSetShader,
                             void(ID3D11ComputeShader* pComputeShader,
                                  ID3D11ClassInstance* const* ppClassInstances,
                                  UINT NumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CSSetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState* const* ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CSSetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer* const* ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VSGetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      PSGetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView** ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             PSGetShader,
                             void(ID3D11PixelShader** ppPixelShader,
                                  ID3D11ClassInstance** ppClassInstances,
                                  UINT* pNumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             PSGetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState** ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VSGetShader,
                             void(ID3D11VertexShader** ppVertexShader,
                                  ID3D11ClassInstance** ppClassInstances,
                                  UINT* pNumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             PSGetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppConstantBuffers));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IAGetInputLayout,
                             void(ID3D11InputLayout** ppInputLayout));

  MOCK_METHOD5_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IAGetVertexBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppVertexBuffers,
                                  UINT* pStrides,
                                  UINT* pOffsets));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IAGetIndexBuffer,
                             void(ID3D11Buffer** pIndexBuffer,
                                  DXGI_FORMAT* Format,
                                  UINT* Offset));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GSGetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GSGetShader,
                             void(ID3D11GeometryShader** ppGeometryShader,
                                  ID3D11ClassInstance** ppClassInstances,
                                  UINT* pNumClassInstances));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             IAGetPrimitiveTopology,
                             void(D3D11_PRIMITIVE_TOPOLOGY* pTopology));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      VSGetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView** ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             VSGetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState** ppSamplers));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetPredication,
                             void(ID3D11Predicate** ppPredicate,
                                  BOOL* pPredicateValue));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      GSGetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView** ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GSGetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState** ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             OMGetRenderTargets,
                             void(UINT NumViews,
                                  ID3D11RenderTargetView** ppRenderTargetViews,
                                  ID3D11DepthStencilView** ppDepthStencilView));

  MOCK_METHOD6_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      OMGetRenderTargetsAndUnorderedAccessViews,
      void(UINT NumRTVs,
           ID3D11RenderTargetView** ppRenderTargetViews,
           ID3D11DepthStencilView** ppDepthStencilView,
           UINT UAVStartSlot,
           UINT NumUAVs,
           ID3D11UnorderedAccessView** ppUnorderedAccessViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             OMGetBlendState,
                             void(ID3D11BlendState** ppBlendState,
                                  FLOAT BlendFactor[4],
                                  UINT* pSampleMask));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             OMGetDepthStencilState,
                             void(ID3D11DepthStencilState** ppDepthStencilState,
                                  UINT* pStencilRef));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             SOGetTargets,
                             void(UINT NumBuffers, ID3D11Buffer** ppSOTargets));

  MOCK_METHOD1_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             RSGetState,
                             void(ID3D11RasterizerState** ppRasterizerState));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             RSGetViewports,
                             void(UINT* pNumViewports,
                                  D3D11_VIEWPORT* pViewports));

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             RSGetScissorRects,
                             void(UINT* pNumRects, D3D11_RECT* pRects));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      HSGetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView** ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             HSGetShader,
                             void(ID3D11HullShader** ppHullShader,
                                  ID3D11ClassInstance** ppClassInstances,
                                  UINT* pNumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             HSGetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState** ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             HSGetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      DSGetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView** ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DSGetShader,
                             void(ID3D11DomainShader** ppDomainShader,
                                  ID3D11ClassInstance** ppClassInstances,
                                  UINT* pNumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DSGetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState** ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             DSGetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppConstantBuffers));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CSGetShaderResources,
      void(UINT StartSlot,
           UINT NumViews,
           ID3D11ShaderResourceView** ppShaderResourceViews));

  MOCK_METHOD3_WITH_CALLTYPE(
      STDMETHODCALLTYPE,
      CSGetUnorderedAccessViews,
      void(UINT StartSlot,
           UINT NumUAVs,
           ID3D11UnorderedAccessView** ppUnorderedAccessViews));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CSGetShader,
                             void(ID3D11ComputeShader** ppComputeShader,
                                  ID3D11ClassInstance** ppClassInstances,
                                  UINT* pNumClassInstances));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CSGetSamplers,
                             void(UINT StartSlot,
                                  UINT NumSamplers,
                                  ID3D11SamplerState** ppSamplers));

  MOCK_METHOD3_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             CSGetConstantBuffers,
                             void(UINT StartSlot,
                                  UINT NumBuffers,
                                  ID3D11Buffer** ppConstantBuffers));

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, ClearState, void());

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, Flush, void());

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             GetType,
                             D3D11_DEVICE_CONTEXT_TYPE());

  MOCK_METHOD0_WITH_CALLTYPE(STDMETHODCALLTYPE, GetContextFlags, UINT());

  MOCK_METHOD2_WITH_CALLTYPE(STDMETHODCALLTYPE,
                             FinishCommandList,
                             HRESULT(BOOL RestoreDeferredContextState,
                                     ID3D11CommandList** ppCommandList));
};
}  // namespace media

#endif  // MEDIA_GPU_D3D11_MOCKS_H_
