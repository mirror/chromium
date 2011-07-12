// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <iostream>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "ppapi/c/dev/ppb_opengles_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/dev/context_3d_dev.h"
#include "ppapi/cpp/dev/graphics_3d_client_dev.h"
#include "ppapi/cpp/dev/graphics_3d_dev.h"
#include "ppapi/cpp/dev/surface_3d_dev.h"
#include "ppapi/cpp/dev/video_decoder_client_dev.h"
#include "ppapi/cpp/dev/video_decoder_dev.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/rect.h"
#include "ppapi/examples/gles2/testdata.h"
#include "ppapi/lib/gl/include/GLES2/gl2.h"

// Use assert as a poor-man's CHECK, even in non-debug mode.
// Since <assert.h> redefines assert on every inclusion (it doesn't use
// include-guards), make sure this is the last file #include'd in this file.
#undef NDEBUG
#include <assert.h>

// Assert |context_| isn't holding any GL Errors.  Done as a macro instead of a
// function to preserve line number information in the failure message.
#define assertNoGLError() \
  assert(!gles2_if_->GetError(context_->pp_resource()));

namespace {

class GLES2DemoInstance : public pp::Instance, public pp::Graphics3DClient_Dev,
                          public pp::VideoDecoderClient_Dev {
 public:
  GLES2DemoInstance(PP_Instance instance, pp::Module* module);
  virtual ~GLES2DemoInstance();

  // pp::Instance implementation (see PPP_Instance).
  virtual void DidChangeView(const pp::Rect& position,
                             const pp::Rect& clip_ignored);

  // pp::Graphics3DClient_Dev implementation.
  virtual void Graphics3DContextLost() {
    // TODO(vrk/fischman): Properly reset after a lost graphics context.  In
    // particular need to delete context_ & surface_ and re-create textures.
    // Probably have to recreate the decoder from scratch, because old textures
    // can still be outstanding in the decoder!
    assert(!"Unexpectedly lost graphics context");
  }

  // pp::VideoDecoderClient_Dev implementation.
  virtual void ProvidePictureBuffers(
      pp::VideoDecoder_Dev decoder, uint32_t req_num_of_bufs,
      PP_Size dimensions, PP_PictureBufferType_Dev type);
  virtual void DismissPictureBuffer(
      pp::VideoDecoder_Dev decoder, int32_t picture_buffer_id);
  virtual void PictureReady(
      pp::VideoDecoder_Dev decoder, const PP_Picture_Dev& picture);
  virtual void EndOfStream(pp::VideoDecoder_Dev decoder);
  virtual void NotifyError(
      pp::VideoDecoder_Dev decoder, PP_VideoDecodeError_Dev error);

 private:
  enum { kNumConcurrentDecodes = 7 };

  // Initialize Video Decoder.
  void InitializeDecoder();

  // Callbacks passed into pp:VideoDecoder_Dev functions.
  void DecoderInitDone(int32_t result);
  void DecoderBitstreamDone(int32_t result, int bitstream_buffer_id);
  void DecoderFlushDone(int32_t result);
  void DecoderAbortDone(int32_t result);

  // Decode helpers.
  void DecodeNextNALUs();
  void DecodeNextNALU();
  void GetNextNALUBoundary(size_t start_pos, size_t* end_pos);
  void Render(const PP_GLESBuffer_Dev& buffer);

  // GL-related functions.
  void InitGL();
  GLuint CreateTexture(int32_t width, int32_t height);
  void CreateGLObjects();
  void CreateShader(GLuint program, GLenum type, const char* source, int size);
  void DeleteTexture(GLuint id);
  void PaintFinished(int32_t result, int picture_buffer_id);

  pp::Size position_size_;
  int next_picture_buffer_id_;
  int next_bitstream_buffer_id_;
  bool is_painting_;
  pp::CompletionCallbackFactory<GLES2DemoInstance> callback_factory_;
  size_t encoded_data_next_pos_to_decode_;
  std::set<int> bitstream_ids_at_decoder_;
  // When decode outpaces render, we queue up decoded pictures for later
  // painting.
  std::list<PP_Picture_Dev> pictures_pending_paint_;
  int num_frames_rendered_;

  // Map of texture buffers indexed by buffer id.
  typedef std::map<int, PP_GLESBuffer_Dev> PictureBufferMap;
  PictureBufferMap buffers_by_id_;
  // Map of bitstream buffers indexed by id.
  typedef std::map<int, pp::Buffer_Dev*> BitstreamBufferMap;
  BitstreamBufferMap bitstream_buffers_by_id_;
  PP_TimeTicks first_frame_delivered_ticks_;
  PP_TimeTicks last_swap_request_ticks_;
  PP_TimeTicks swap_ticks_;

  // Unowned pointers.
  const struct PPB_Core* core_if_;
  const struct PPB_OpenGLES2_Dev* gles2_if_;

  // Owned data.
  pp::Context3D_Dev* context_;
  pp::Surface3D_Dev* surface_;
  pp::VideoDecoder_Dev* video_decoder_;
};

GLES2DemoInstance::GLES2DemoInstance(PP_Instance instance, pp::Module* module)
    : pp::Instance(instance), pp::Graphics3DClient_Dev(this),
      pp::VideoDecoderClient_Dev(this),
      next_picture_buffer_id_(0),
      next_bitstream_buffer_id_(0),
      callback_factory_(this),
      encoded_data_next_pos_to_decode_(0),
      num_frames_rendered_(0),
      first_frame_delivered_ticks_(-1),
      swap_ticks_(0),
      context_(NULL),
      surface_(NULL),
      video_decoder_(NULL) {
  assert((core_if_ = static_cast<const struct PPB_Core*>(
      module->GetBrowserInterface(PPB_CORE_INTERFACE))));
  assert((gles2_if_ = static_cast<const struct PPB_OpenGLES2_Dev*>(
      module->GetBrowserInterface(PPB_OPENGLES2_DEV_INTERFACE))));
}

GLES2DemoInstance::~GLES2DemoInstance() {
  delete video_decoder_;
  delete surface_;
  delete context_;
}

void GLES2DemoInstance::DidChangeView(
    const pp::Rect& position, const pp::Rect& clip_ignored) {
  if (position.width() == 0 || position.height() == 0)
    return;
  if (position_size_.width()) {
    assert(position.size() == position_size_);
    return;
  }
  position_size_ = position.size();

  // Initialize graphics.
  InitGL();
  InitializeDecoder();
}

void GLES2DemoInstance::InitializeDecoder() {
  assert(!video_decoder_);
  video_decoder_ = new pp::VideoDecoder_Dev(*this);

  PP_VideoConfigElement configs = PP_VIDEOATTR_DICTIONARY_TERMINATOR;
  pp::CompletionCallback cb =
      callback_factory_.NewCallback(&GLES2DemoInstance::DecoderInitDone);
  video_decoder_->Initialize(&configs, *context_, cb);
}

void GLES2DemoInstance::DecoderInitDone(int32_t result) {
  DecodeNextNALUs();
}

void GLES2DemoInstance::DecoderBitstreamDone(
    int32_t result, int bitstream_buffer_id) {
  assert(bitstream_ids_at_decoder_.erase(bitstream_buffer_id) == 1);
  BitstreamBufferMap::iterator it =
      bitstream_buffers_by_id_.find(bitstream_buffer_id);
  assert(it != bitstream_buffers_by_id_.end());
  delete it->second;
  DecodeNextNALUs();
}

void GLES2DemoInstance::DecoderFlushDone(int32_t result) {
  // Check that each bitstream buffer ID we handed to the decoder got handed
  // back to us.
  assert(bitstream_ids_at_decoder_.empty());
}

void GLES2DemoInstance::DecoderAbortDone(int32_t result) {
}

static bool LookingAtNAL(const unsigned char* encoded, size_t pos) {
  return pos + 3 < kDataLen &&
      encoded[pos] == 0 && encoded[pos + 1] == 0 &&
      encoded[pos + 2] == 0 && encoded[pos + 3] == 1;
}

void GLES2DemoInstance::GetNextNALUBoundary(
    size_t start_pos, size_t* end_pos) {
  assert(LookingAtNAL(kData, start_pos));
  *end_pos = start_pos;
  *end_pos += 4;
  while (*end_pos + 3 < kDataLen &&
         !LookingAtNAL(kData, *end_pos)) {
    ++*end_pos;
  }
  if (*end_pos + 3 >= kDataLen) {
    *end_pos = kDataLen;
    return;
  }
}

void GLES2DemoInstance::DecodeNextNALUs() {
  while (encoded_data_next_pos_to_decode_ <= kDataLen &&
         bitstream_ids_at_decoder_.size() < kNumConcurrentDecodes) {
    DecodeNextNALU();
  }
}

void GLES2DemoInstance::DecodeNextNALU() {
  if (encoded_data_next_pos_to_decode_ == kDataLen) {
    ++encoded_data_next_pos_to_decode_;
    pp::CompletionCallback cb =
        callback_factory_.NewCallback(&GLES2DemoInstance::DecoderFlushDone);
    video_decoder_->Flush(cb);
    return;
  }
  size_t start_pos = encoded_data_next_pos_to_decode_;
  size_t end_pos;
  GetNextNALUBoundary(start_pos, &end_pos);
  pp::Buffer_Dev* buffer = new pp::Buffer_Dev (this, end_pos - start_pos);
  PP_VideoBitstreamBuffer_Dev bitstream_buffer;
  int id = ++next_bitstream_buffer_id_;
  bitstream_buffer.id = id;
  bitstream_buffer.size = end_pos - start_pos;
  bitstream_buffer.data = buffer->pp_resource();
  memcpy(buffer->data(), kData + start_pos, end_pos - start_pos);
  assert(bitstream_buffers_by_id_.insert(std::make_pair(id, buffer)).second);

  pp::CompletionCallback cb =
      callback_factory_.NewCallback(
          &GLES2DemoInstance::DecoderBitstreamDone, id);
  assert(bitstream_ids_at_decoder_.insert(id).second);
  video_decoder_->Decode(bitstream_buffer, cb);
  encoded_data_next_pos_to_decode_ = end_pos;
}

void GLES2DemoInstance::ProvidePictureBuffers(
    pp::VideoDecoder_Dev decoder, uint32_t req_num_of_bufs, PP_Size dimensions,
    PP_PictureBufferType_Dev type) {
  std::vector<PP_GLESBuffer_Dev> buffers;
  for (uint32_t i = 0; i < req_num_of_bufs; i++) {
    PP_GLESBuffer_Dev buffer;
    buffer.texture_id = CreateTexture(dimensions.width, dimensions.height);
    int id = ++next_picture_buffer_id_;
    buffer.info.id= id;
    buffers.push_back(buffer);
    assert(buffers_by_id_.insert(std::make_pair(id, buffer)).second);
  }
  video_decoder_->AssignGLESBuffers(buffers);
}

void GLES2DemoInstance::DismissPictureBuffer(
    pp::VideoDecoder_Dev decoder, int32_t picture_buffer_id) {
  PictureBufferMap::iterator it = buffers_by_id_.find(picture_buffer_id);
  assert(it != buffers_by_id_.end());
  DeleteTexture(it->second.texture_id);
  buffers_by_id_.erase(it);
}

void GLES2DemoInstance::PictureReady(
    pp::VideoDecoder_Dev decoder, const PP_Picture_Dev& picture) {
  if (first_frame_delivered_ticks_ == -1)
    assert((first_frame_delivered_ticks_ = core_if_->GetTimeTicks()) != -1);
  if (is_painting_) {
    pictures_pending_paint_.push_back(picture);
    return;
  }
  PictureBufferMap::iterator it =
      buffers_by_id_.find(picture.picture_buffer_id);
  assert(it != buffers_by_id_.end());
  Render(it->second);
}

void GLES2DemoInstance::EndOfStream(pp::VideoDecoder_Dev decoder) {
}

void GLES2DemoInstance::NotifyError(
    pp::VideoDecoder_Dev decoder, PP_VideoDecodeError_Dev error) {
}

// This object is the global object representing this plugin library as long
// as it is loaded.
class GLES2DemoModule : public pp::Module {
 public:
  GLES2DemoModule() : pp::Module() {}
  virtual ~GLES2DemoModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new GLES2DemoInstance(instance, this);
  }
};

void GLES2DemoInstance::InitGL() {
  assert(position_size_.width() && position_size_.height());
  is_painting_ = false;

  assert(!context_ && !surface_);
  context_ = new pp::Context3D_Dev(*this, 0, pp::Context3D_Dev(), NULL);
  assert(!context_->is_null());

  int32_t surface_attributes[] = {
    PP_GRAPHICS3DATTRIB_WIDTH, position_size_.width(),
    PP_GRAPHICS3DATTRIB_HEIGHT, position_size_.height(),
    PP_GRAPHICS3DATTRIB_NONE
  };
  surface_ = new pp::Surface3D_Dev(*this, 0, surface_attributes);
  assert(!surface_->is_null());

  assert(!context_->BindSurfaces(*surface_, *surface_));

  // Set viewport window size and clear color bit.
  gles2_if_->Clear(context_->pp_resource(), GL_COLOR_BUFFER_BIT);
  gles2_if_->Viewport(context_->pp_resource(), 0, 0,
                      position_size_.width(), position_size_.height());

  assert(BindGraphics(*surface_));
  assertNoGLError();

  CreateGLObjects();
}

void GLES2DemoInstance::Render(const PP_GLESBuffer_Dev& buffer) {
  assert(!is_painting_);
  is_painting_ = true;
  gles2_if_->ActiveTexture(context_->pp_resource(), GL_TEXTURE0);
  gles2_if_->BindTexture(
      context_->pp_resource(), GL_TEXTURE_2D, buffer.texture_id);
  gles2_if_->DrawArrays(context_->pp_resource(), GL_TRIANGLE_STRIP, 0, 4);
  pp::CompletionCallback cb =
      callback_factory_.NewCallback(
          &GLES2DemoInstance::PaintFinished, buffer.info.id);
  last_swap_request_ticks_ = core_if_->GetTimeTicks();
  assert(surface_->SwapBuffers(cb) == PP_OK_COMPLETIONPENDING);
}

void GLES2DemoInstance::PaintFinished(int32_t result, int picture_buffer_id) {
  swap_ticks_ += core_if_->GetTimeTicks() - last_swap_request_ticks_;
  is_painting_ = false;
  ++num_frames_rendered_;
  if (num_frames_rendered_ % 50 == 0) {
    double elapsed = core_if_->GetTimeTicks() - first_frame_delivered_ticks_;
    double fps = (elapsed > 0) ? num_frames_rendered_ / elapsed : 1000;
    double ms_per_swap = (swap_ticks_ * 1e3) / num_frames_rendered_;
    std::cerr << "Rendered frames: " << num_frames_rendered_ << ", fps: "
              << fps << ", with average ms/swap of: " << ms_per_swap
              << std::endl;
  }
  video_decoder_->ReusePictureBuffer(picture_buffer_id);
  while (!pictures_pending_paint_.empty() && !is_painting_) {
    PP_Picture_Dev picture = pictures_pending_paint_.front();
    pictures_pending_paint_.pop_front();
    PictureReady(*video_decoder_, picture);
  }
}

GLuint GLES2DemoInstance::CreateTexture(int32_t width, int32_t height) {
  GLuint texture_id;
  gles2_if_->GenTextures(context_->pp_resource(), 1, &texture_id);
  assertNoGLError();
  // Assign parameters.
  gles2_if_->ActiveTexture(context_->pp_resource(), GL_TEXTURE0);
  gles2_if_->BindTexture(
      context_->pp_resource(), GL_TEXTURE_2D, texture_id);
  gles2_if_->TexParameteri(
      context_->pp_resource(), GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
      GL_NEAREST);
  gles2_if_->TexParameteri(
      context_->pp_resource(), GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
      GL_NEAREST);
  gles2_if_->TexParameterf(
      context_->pp_resource(), GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
      GL_CLAMP_TO_EDGE);
  gles2_if_->TexParameterf(
      context_->pp_resource(), GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
      GL_CLAMP_TO_EDGE);

  // Allocate texture.
  gles2_if_->TexImage2D(
      context_->pp_resource(), GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  assertNoGLError();
  return texture_id;
}

void GLES2DemoInstance::DeleteTexture(GLuint id) {
  gles2_if_->DeleteTextures(context_->pp_resource(), 1, &id);
}

void GLES2DemoInstance::CreateGLObjects() {
  // Code and constants for shader.
  static const char kVertexShader[] =
      "varying vec2 v_texCoord;            \n"
      "attribute vec4 a_position;          \n"
      "attribute vec2 a_texCoord;          \n"
      "void main()                         \n"
      "{                                   \n"
      "    v_texCoord = a_texCoord;        \n"
      "    gl_Position = a_position;       \n"
      "}";

  static const char kFragmentShader[] =
      "precision mediump float;            \n"
      "varying vec2 v_texCoord;            \n"
      "uniform sampler2D s_texture;        \n"
      "void main()                         \n"
      "{"
      "    gl_FragColor = texture2D(s_texture, v_texCoord); \n"
      "}";

  // Create shader program.
  GLuint program = gles2_if_->CreateProgram(context_->pp_resource());
  CreateShader(program, GL_VERTEX_SHADER, kVertexShader, sizeof(kVertexShader));
  CreateShader(
      program, GL_FRAGMENT_SHADER, kFragmentShader, sizeof(kFragmentShader));
  gles2_if_->LinkProgram(context_->pp_resource(), program);
  gles2_if_->UseProgram(context_->pp_resource(), program);
  gles2_if_->DeleteProgram(context_->pp_resource(), program);
  gles2_if_->Uniform1i(
      context_->pp_resource(),
      gles2_if_->GetAttribLocation(
          context_->pp_resource(), program, "s_texture"), 0);
  assertNoGLError();

  // Assign vertex positions and texture coordinates to buffers for use in
  // shader program.
  static const float kVertices[] = {
    -1, 1, -1, -1, 1, 1, 1, -1,  // Position coordinates.
    0, 1, 0, 0, 1, 1, 1, 0,  // Texture coordinates.
  };

  GLuint buffer;
  gles2_if_->GenBuffers(context_->pp_resource(), 1, &buffer);
  gles2_if_->BindBuffer(context_->pp_resource(), GL_ARRAY_BUFFER, buffer);
  gles2_if_->BufferData(context_->pp_resource(), GL_ARRAY_BUFFER,
                        sizeof(kVertices), kVertices, GL_STATIC_DRAW);
  assertNoGLError();
  GLint pos_location = gles2_if_->GetAttribLocation(
      context_->pp_resource(), program, "a_position");
  GLint tc_location = gles2_if_->GetAttribLocation(
      context_->pp_resource(), program, "a_texCoord");
  assertNoGLError();
  gles2_if_->EnableVertexAttribArray(context_->pp_resource(), pos_location);
  gles2_if_->VertexAttribPointer(context_->pp_resource(), pos_location, 2,
                                 GL_FLOAT, GL_FALSE, 0, 0);
  gles2_if_->EnableVertexAttribArray(context_->pp_resource(), tc_location);
  gles2_if_->VertexAttribPointer(
      context_->pp_resource(), tc_location, 2, GL_FLOAT, GL_FALSE, 0,
      static_cast<float*>(0) + 8);  // Skip position coordinates.
  assertNoGLError();
}

void GLES2DemoInstance::CreateShader(
    GLuint program, GLenum type, const char* source, int size) {
  GLuint shader = gles2_if_->CreateShader(context_->pp_resource(), type);
  gles2_if_->ShaderSource(context_->pp_resource(), shader, 1, &source, &size);
  gles2_if_->CompileShader(context_->pp_resource(), shader);
  gles2_if_->AttachShader(context_->pp_resource(), program, shader);
  gles2_if_->DeleteShader(context_->pp_resource(), shader);
}
}  // anonymous namespace

namespace pp {
// Factory function for your specialization of the Module object.
Module* CreateModule() {
  return new GLES2DemoModule();
}
}  // namespace pp
