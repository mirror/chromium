// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/output/shader.h"

#include <stddef.h>

#include <algorithm>
#include <vector>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "cc/output/static_geometry_binding.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

template <size_t size>
std::string StripLambda(const char(&shader)[size]) {
  // Must contain at least "[]() {}" and trailing null (included in size).
  static_assert(size >= 8,
                "String passed to StripLambda must be at least 8 characters");
  DCHECK_EQ(strncmp("[]() {", shader, 6), 0);
  DCHECK_EQ(shader[size - 2], '}');
  return std::string(shader + 6, shader + size - 2);
}

// Shaders are passed in with lambda syntax, which tricks clang-format into
// handling them correctly. StipLambda removes this.
#define SHADER0(Src) StripLambda(#Src)

using gpu::gles2::GLES2Interface;

namespace cc {

namespace {

static void GetProgramUniformLocations(GLES2Interface* context,
                                       unsigned program,
                                       size_t count,
                                       const char** uniforms,
                                       int* locations,
                                       int* base_uniform_index) {
  for (size_t i = 0; i < count; i++) {
    locations[i] = (*base_uniform_index)++;
    context->BindUniformLocationCHROMIUM(program, locations[i], uniforms[i]);
  }
}

static std::string SetFragmentTexCoordPrecision(
    TexCoordPrecision requested_precision,
    std::string shader_string) {
  switch (requested_precision) {
    case TEX_COORD_PRECISION_HIGH:
      DCHECK_NE(shader_string.find("TexCoordPrecision"), std::string::npos);
      return "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
             "  #define TexCoordPrecision highp\n"
             "#else\n"
             "  #define TexCoordPrecision mediump\n"
             "#endif\n" +
             shader_string;
    case TEX_COORD_PRECISION_MEDIUM:
      DCHECK_NE(shader_string.find("TexCoordPrecision"), std::string::npos);
      return "#define TexCoordPrecision mediump\n" + shader_string;
    case TEX_COORD_PRECISION_NA:
      DCHECK_EQ(shader_string.find("TexCoordPrecision"), std::string::npos);
      DCHECK_EQ(shader_string.find("texture2D"), std::string::npos);
      DCHECK_EQ(shader_string.find("texture2DRect"), std::string::npos);
      return shader_string;
    default:
      NOTREACHED();
      break;
  }
  return shader_string;
}

TexCoordPrecision TexCoordPrecisionRequired(GLES2Interface* context,
                                            int* highp_threshold_cache,
                                            int highp_threshold_min,
                                            int x,
                                            int y) {
  if (*highp_threshold_cache == 0) {
    // Initialize range and precision with minimum spec values for when
    // GetShaderPrecisionFormat is a test stub.
    // TODO(brianderson): Implement better stubs of GetShaderPrecisionFormat
    // everywhere.
    GLint range[2] = {14, 14};
    GLint precision = 10;
    context->GetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT,
                                      range, &precision);
    *highp_threshold_cache = 1 << precision;
  }

  int highp_threshold = std::max(*highp_threshold_cache, highp_threshold_min);
  if (x > highp_threshold || y > highp_threshold)
    return TEX_COORD_PRECISION_HIGH;
  return TEX_COORD_PRECISION_MEDIUM;
}

static std::string SetFragmentSamplerType(SamplerType requested_type,
                                          std::string shader_string) {
  switch (requested_type) {
    case SAMPLER_TYPE_2D:
      DCHECK_NE(shader_string.find("SamplerType"), std::string::npos);
      DCHECK_NE(shader_string.find("TextureLookup"), std::string::npos);
      return "#define SamplerType sampler2D\n"
             "#define TextureLookup texture2D\n" +
             shader_string;
    case SAMPLER_TYPE_2D_RECT:
      DCHECK_NE(shader_string.find("SamplerType"), std::string::npos);
      DCHECK_NE(shader_string.find("TextureLookup"), std::string::npos);
      return "#extension GL_ARB_texture_rectangle : require\n"
             "#define SamplerType sampler2DRect\n"
             "#define TextureLookup texture2DRect\n" +
             shader_string;
    case SAMPLER_TYPE_EXTERNAL_OES:
      DCHECK_NE(shader_string.find("SamplerType"), std::string::npos);
      DCHECK_NE(shader_string.find("TextureLookup"), std::string::npos);
      return "#extension GL_OES_EGL_image_external : enable\n"
             "#extension GL_NV_EGL_stream_consumer_external : enable\n"
             "#define SamplerType samplerExternalOES\n"
             "#define TextureLookup texture2D\n" +
             shader_string;
    case SAMPLER_TYPE_NA:
      DCHECK_EQ(shader_string.find("SamplerType"), std::string::npos);
      DCHECK_EQ(shader_string.find("TextureLookup"), std::string::npos);
      return shader_string;
    default:
      NOTREACHED();
      break;
  }
  return shader_string;
}

}  // namespace

ShaderLocations::ShaderLocations() {
}

TexCoordPrecision TexCoordPrecisionRequired(GLES2Interface* context,
                                            int* highp_threshold_cache,
                                            int highp_threshold_min,
                                            const gfx::Point& max_coordinate) {
  return TexCoordPrecisionRequired(context,
                                   highp_threshold_cache,
                                   highp_threshold_min,
                                   max_coordinate.x(),
                                   max_coordinate.y());
}

TexCoordPrecision TexCoordPrecisionRequired(GLES2Interface* context,
                                            int* highp_threshold_cache,
                                            int highp_threshold_min,
                                            const gfx::Size& max_size) {
  return TexCoordPrecisionRequired(context,
                                   highp_threshold_cache,
                                   highp_threshold_min,
                                   max_size.width(),
                                   max_size.height());
}

VertexShaderBase::VertexShaderBase() {}

void VertexShaderBase::Init(GLES2Interface* context,
                            unsigned program,
                            int* base_uniform_index) {
  std::vector<const char*> uniforms;
  std::vector<int> locations;

  if (has_tex_transform_)
    uniforms.push_back("texTransform");
  if (has_vertex_tex_transform_)
    uniforms.push_back("vertexTexTransform");
  if (has_tex_matrix_)
    uniforms.push_back("texMatrix");
  if (has_ya_uv_tex_scale_offset_) {
    uniforms.push_back("yaTexScale");
    uniforms.push_back("yaTexOffset");
    uniforms.push_back("uvTexScale");
    uniforms.push_back("uvTexOffset");
  }
  if (has_matrix_)
    uniforms.push_back("matrix");
  if (has_vertex_opacity_)
    uniforms.push_back("opacity");
  if (has_aa_) {
    uniforms.push_back("viewport");
    uniforms.push_back("edge");
  }
  if (has_quad_)
    uniforms.push_back("quad");

  locations.resize(uniforms.size());

  GetProgramUniformLocations(context, program, uniforms.size(), uniforms.data(),
                             locations.data(), base_uniform_index);

  size_t index = 0;
  if (has_tex_transform_)
    tex_transform_location_ = locations[index++];
  if (has_vertex_tex_transform_)
    vertex_tex_transform_location_ = locations[index++];
  if (has_tex_matrix_)
    tex_matrix_location_ = locations[index++];
  if (has_ya_uv_tex_scale_offset_) {
    ya_tex_scale_location_ = locations[index++];
    ya_tex_offset_location_ = locations[index++];
    uv_tex_scale_location_ = locations[index++];
    uv_tex_offset_location_ = locations[index++];
  }
  if (has_matrix_)
    matrix_location_ = locations[index++];
  if (has_vertex_opacity_)
    vertex_opacity_location_ = locations[index++];
  if (has_aa_) {
    viewport_location_ = locations[index++];
    edge_location_ = locations[index++];
  }
  if (has_quad_)
    quad_location_ = locations[index++];
}

void VertexShaderBase::FillLocations(ShaderLocations* locations) const {
  locations->quad = quad_location();
  locations->edge = edge_location();
  locations->viewport = viewport_location();
  locations->matrix = matrix_location();
  locations->tex_transform = tex_transform_location();
}

std::string VertexShaderBase::GetShaderString() const {
  // We unconditionally use highp in the vertex shader since
  // we are unlikely to be vertex shader bound when drawing large quads.
  // Also, some vertex shaders mutate the texture coordinate in such a
  // way that the effective precision might be lower than expected.
  return base::StringPrintf(
             "#define TexCoordPrecision highp\n"
             "#define NUM_STATIC_QUADS %d\n",
             StaticGeometryBinding::NUM_QUADS) +
         GetShaderSource();
}

std::string VertexShaderPosTex::GetShaderSource() const {
  return SHADER0([]() {
    attribute vec4 a_position;
    attribute TexCoordPrecision vec2 a_texCoord;
    uniform mat4 matrix;
    varying TexCoordPrecision vec2 v_texCoord;
    void main() {
      gl_Position = matrix * a_position;
      v_texCoord = a_texCoord;
    }
  });
}

std::string VertexShaderPosTexYUVStretchOffset::GetShaderSource() const {
  return SHADER0([]() {
    precision mediump float;
    attribute vec4 a_position;
    attribute TexCoordPrecision vec2 a_texCoord;
    uniform mat4 matrix;
    varying TexCoordPrecision vec2 v_yaTexCoord;
    varying TexCoordPrecision vec2 v_uvTexCoord;
    uniform TexCoordPrecision vec2 yaTexScale;
    uniform TexCoordPrecision vec2 yaTexOffset;
    uniform TexCoordPrecision vec2 uvTexScale;
    uniform TexCoordPrecision vec2 uvTexOffset;
    void main() {
      gl_Position = matrix * a_position;
      v_yaTexCoord = a_texCoord * yaTexScale + yaTexOffset;
      v_uvTexCoord = a_texCoord * uvTexScale + uvTexOffset;
    }
  });
}

std::string VertexShaderPos::GetShaderSource() const {
  return SHADER0([]() {
    attribute vec4 a_position;
    uniform mat4 matrix;
    void main() { gl_Position = matrix * a_position; }
  });
}

std::string VertexShaderPosTexTransform::GetShaderSource() const {
  return SHADER0([]() {
    attribute vec4 a_position;
    attribute TexCoordPrecision vec2 a_texCoord;
    attribute float a_index;
    uniform mat4 matrix[NUM_STATIC_QUADS];
    uniform TexCoordPrecision vec4 texTransform[NUM_STATIC_QUADS];
    uniform float opacity[NUM_STATIC_QUADS * 4];
    varying TexCoordPrecision vec2 v_texCoord;
    varying float v_alpha;
    void main() {
      int quad_index = int(a_index * 0.25);  // NOLINT
      gl_Position = matrix[quad_index] * a_position;
      TexCoordPrecision vec4 texTrans = texTransform[quad_index];
      v_texCoord = a_texCoord * texTrans.zw + texTrans.xy;
      v_alpha = opacity[int(a_index)];  // NOLINT
    }
  });
}

std::string VertexShaderPosTexIdentity::GetShaderSource() const {
  return SHADER0([]() {
    attribute vec4 a_position;
    varying TexCoordPrecision vec2 v_texCoord;
    void main() {
      gl_Position = a_position;
      v_texCoord = (a_position.xy + vec2(1.0)) * 0.5;
    }
  });
}

std::string VertexShaderQuad::GetShaderSource() const {
#if defined(OS_ANDROID)
  // TODO(epenner): Find the cause of this 'quad' uniform
  // being missing if we don't add dummy variables.
  // http://crbug.com/240602
  return SHADER0([]() {
    attribute TexCoordPrecision vec4 a_position;
    attribute float a_index;
    uniform mat4 matrix;
    uniform TexCoordPrecision vec2 quad[4];
    uniform TexCoordPrecision vec2 dummy_uniform;
    varying TexCoordPrecision vec2 dummy_varying;
    void main() {
      vec2 pos = quad[int(a_index)];  // NOLINT
      gl_Position = matrix * vec4(pos, a_position.z, a_position.w);
      dummy_varying = dummy_uniform;
    }
  });
#else
  return SHADER0([]() {
    attribute TexCoordPrecision vec4 a_position;
    attribute float a_index;
    uniform mat4 matrix;
    uniform TexCoordPrecision vec2 quad[4];
    void main() {
      vec2 pos = quad[int(a_index)];  // NOLINT
      gl_Position = matrix * vec4(pos, a_position.z, a_position.w);
    }
  });
#endif
}

std::string VertexShaderQuadAA::GetShaderSource() const {
  return SHADER0([]() {
    attribute TexCoordPrecision vec4 a_position;
    attribute float a_index;
    uniform mat4 matrix;
    uniform vec4 viewport;
    uniform TexCoordPrecision vec2 quad[4];
    uniform TexCoordPrecision vec3 edge[8];
    varying TexCoordPrecision vec4 edge_dist[2];  // 8 edge distances.
    void main() {
      vec2 pos = quad[int(a_index)];  // NOLINT
      gl_Position = matrix * vec4(pos, a_position.z, a_position.w);
      vec2 ndc_pos = 0.5 * (1.0 + gl_Position.xy / gl_Position.w);
      vec3 screen_pos = vec3(viewport.xy + viewport.zw * ndc_pos, 1.0);
      edge_dist[0] = vec4(dot(edge[0], screen_pos), dot(edge[1], screen_pos),
                          dot(edge[2], screen_pos), dot(edge[3], screen_pos)) *
                     gl_Position.w;
      edge_dist[1] = vec4(dot(edge[4], screen_pos), dot(edge[5], screen_pos),
                          dot(edge[6], screen_pos), dot(edge[7], screen_pos)) *
                     gl_Position.w;
    }
  });
}

std::string VertexShaderQuadTexTransformAA::GetShaderSource() const {
  return SHADER0([]() {
    attribute TexCoordPrecision vec4 a_position;
    attribute float a_index;
    uniform mat4 matrix;
    uniform vec4 viewport;
    uniform TexCoordPrecision vec2 quad[4];
    uniform TexCoordPrecision vec3 edge[8];
    uniform TexCoordPrecision vec4 texTransform;
    varying TexCoordPrecision vec2 v_texCoord;
    varying TexCoordPrecision vec4 edge_dist[2];  // 8 edge distances.
    void main() {
      vec2 pos = quad[int(a_index)];  // NOLINT
      gl_Position = matrix * vec4(pos, a_position.z, a_position.w);
      vec2 ndc_pos = 0.5 * (1.0 + gl_Position.xy / gl_Position.w);
      vec3 screen_pos = vec3(viewport.xy + viewport.zw * ndc_pos, 1.0);
      edge_dist[0] = vec4(dot(edge[0], screen_pos), dot(edge[1], screen_pos),
                          dot(edge[2], screen_pos), dot(edge[3], screen_pos)) *
                     gl_Position.w;
      edge_dist[1] = vec4(dot(edge[4], screen_pos), dot(edge[5], screen_pos),
                          dot(edge[6], screen_pos), dot(edge[7], screen_pos)) *
                     gl_Position.w;
      v_texCoord = (pos.xy + vec2(0.5)) * texTransform.zw + texTransform.xy;
    }
  });
}

std::string VertexShaderTile::GetShaderSource() const {
  return SHADER0([]() {
    attribute TexCoordPrecision vec4 a_position;
    attribute TexCoordPrecision vec2 a_texCoord;
    attribute float a_index;
    uniform mat4 matrix;
    uniform TexCoordPrecision vec2 quad[4];
    uniform TexCoordPrecision vec4 vertexTexTransform;
    varying TexCoordPrecision vec2 v_texCoord;
    void main() {
      vec2 pos = quad[int(a_index)];  // NOLINT
      gl_Position = matrix * vec4(pos, a_position.z, a_position.w);
      v_texCoord = a_texCoord * vertexTexTransform.zw + vertexTexTransform.xy;
    }
  });
}

std::string VertexShaderTileAA::GetShaderSource() const {
  return SHADER0([]() {
    attribute TexCoordPrecision vec4 a_position;
    attribute float a_index;
    uniform mat4 matrix;
    uniform vec4 viewport;
    uniform TexCoordPrecision vec2 quad[4];
    uniform TexCoordPrecision vec3 edge[8];
    uniform TexCoordPrecision vec4 vertexTexTransform;
    varying TexCoordPrecision vec2 v_texCoord;
    varying TexCoordPrecision vec4 edge_dist[2];  // 8 edge distances.
    void main() {
      vec2 pos = quad[int(a_index)];  // NOLINT
      gl_Position = matrix * vec4(pos, a_position.z, a_position.w);
      vec2 ndc_pos = 0.5 * (1.0 + gl_Position.xy / gl_Position.w);
      vec3 screen_pos = vec3(viewport.xy + viewport.zw * ndc_pos, 1.0);
      edge_dist[0] = vec4(dot(edge[0], screen_pos), dot(edge[1], screen_pos),
                          dot(edge[2], screen_pos), dot(edge[3], screen_pos)) *
                     gl_Position.w;
      edge_dist[1] = vec4(dot(edge[4], screen_pos), dot(edge[5], screen_pos),
                          dot(edge[6], screen_pos), dot(edge[7], screen_pos)) *
                     gl_Position.w;
      v_texCoord = pos.xy * vertexTexTransform.zw + vertexTexTransform.xy;
    }
  });
}

std::string VertexShaderVideoTransform::GetShaderSource() const {
  return SHADER0([]() {
    attribute vec4 a_position;
    attribute TexCoordPrecision vec2 a_texCoord;
    uniform mat4 matrix;
    uniform TexCoordPrecision mat4 texMatrix;
    varying TexCoordPrecision vec2 v_texCoord;
    void main() {
      gl_Position = matrix * a_position;
      v_texCoord = (texMatrix * vec4(a_texCoord.xy, 0.0, 1.0)).xy;
    }
  });
}

#define BLEND_MODE_UNIFORMS "s_backdropTexture", \
                            "s_originalBackdropTexture", \
                            "backdropRect"
#define UNUSED_BLEND_MODE_UNIFORMS (!has_blend_mode() ? 3 : 0)
#define BLEND_MODE_SET_LOCATIONS(X, POS)                   \
  if (has_blend_mode()) {                                  \
    DCHECK_LT(static_cast<size_t>(POS) + 2, arraysize(X)); \
    backdrop_location_ = locations[POS];                   \
    original_backdrop_location_ = locations[POS + 1];      \
    backdrop_rect_location_ = locations[POS + 2];          \
  }

FragmentShaderBase::FragmentShaderBase() {}

std::string FragmentShaderBase::GetShaderString(TexCoordPrecision precision,
                                                SamplerType sampler) const {
  // The AA shader values will use TexCoordPrecision.
  if (has_aa_ && precision == TEX_COORD_PRECISION_NA)
    precision = TEX_COORD_PRECISION_MEDIUM;
  return SetFragmentTexCoordPrecision(
      precision, SetFragmentSamplerType(
                     sampler, SetBlendModeFunctions(GetShaderSource())));
}

void FragmentShaderBase::Init(GLES2Interface* context,
                              unsigned program,
                              int* base_uniform_index) {
  std::vector<const char*> uniforms;
  std::vector<int> locations;
  if (has_blend_mode()) {
    uniforms.push_back("s_backdropTexture");
    uniforms.push_back("s_originalBackdropTexture");
    uniforms.push_back("backdropRect");
  }
  if (has_mask_sampler_) {
    uniforms.push_back("s_mask");
    uniforms.push_back("maskTexCoordScale");
    uniforms.push_back("maskTexCoordOffset");
  }
  if (has_color_matrix_) {
    uniforms.push_back("colorMatrix");
    uniforms.push_back("colorOffset");
  }
  if (has_uniform_alpha_)
    uniforms.push_back("alpha");
  if (has_background_color_)
    uniforms.push_back("background_color");
  switch (input_color_type_) {
    case INPUT_COLOR_SOURCE_RGBA_TEXTURE:
      uniforms.push_back("s_texture");
      if (has_rgba_fragment_tex_transform_)
        uniforms.push_back("fragmentTexTransform");
      break;
    case INPUT_COLOR_SOURCE_UNIFORM:
      uniforms.push_back("color");
      break;
  }

  locations.resize(uniforms.size());

  GetProgramUniformLocations(context, program, uniforms.size(), uniforms.data(),
                             locations.data(), base_uniform_index);

  size_t index = 0;
  if (has_blend_mode()) {
    backdrop_location_ = locations[index++];
    original_backdrop_location_ = locations[index++];
    backdrop_rect_location_ = locations[index++];
  }
  if (has_mask_sampler_) {
    mask_sampler_location_ = locations[index++];
    mask_tex_coord_scale_location_ = locations[index++];
    mask_tex_coord_offset_location_ = locations[index++];
  }
  if (has_color_matrix_) {
    color_matrix_location_ = locations[index++];
    color_offset_location_ = locations[index++];
  }
  if (has_uniform_alpha_)
    alpha_location_ = locations[index++];
  if (has_background_color_)
    background_color_location_ = locations[index++];
  switch (input_color_type_) {
    case INPUT_COLOR_SOURCE_RGBA_TEXTURE:
      sampler_location_ = locations[index++];
      if (has_rgba_fragment_tex_transform_)
        fragment_tex_transform_location_ = locations[index++];
      break;
    case INPUT_COLOR_SOURCE_UNIFORM:
      color_location_ = locations[index++];
      break;
  }
  DCHECK_EQ(index, locations.size());
}

void FragmentShaderBase::FillLocations(ShaderLocations* locations) const {
  if (has_blend_mode()) {
    locations->backdrop = backdrop_location_;
    locations->backdrop_rect = backdrop_rect_location_;
  }
  if (mask_for_background())
    locations->original_backdrop = original_backdrop_location_;
  if (has_mask_sampler_) {
    locations->mask_sampler = mask_sampler_location_;
    locations->mask_tex_coord_scale = mask_tex_coord_scale_location_;
    locations->mask_tex_coord_offset = mask_tex_coord_offset_location_;
  }
  if (has_color_matrix_) {
    locations->color_matrix = color_matrix_location_;
    locations->color_offset = color_offset_location_;
  }
  if (has_uniform_alpha_)
    locations->alpha = alpha_location_;
  switch (input_color_type_) {
    case INPUT_COLOR_SOURCE_RGBA_TEXTURE:
      locations->sampler = sampler_location_;
      break;
    case INPUT_COLOR_SOURCE_UNIFORM:
      break;
  }
}

std::string FragmentShaderBase::SetBlendModeFunctions(
    const std::string& shader_string) const {
  if (shader_string.find("ApplyBlendMode") == std::string::npos)
    return shader_string;

  if (!has_blend_mode()) {
    return "#define ApplyBlendMode(X, Y) (X)\n" + shader_string;
  }

  static const std::string kUniforms = SHADER0([]() {
    uniform sampler2D s_backdropTexture;
    uniform sampler2D s_originalBackdropTexture;
    uniform TexCoordPrecision vec4 backdropRect;
  });

  std::string mixFunction;
  if (mask_for_background()) {
    mixFunction = SHADER0([]() {
      vec4 MixBackdrop(TexCoordPrecision vec2 bgTexCoord, float mask) {
        vec4 backdrop = texture2D(s_backdropTexture, bgTexCoord);
        vec4 original_backdrop =
            texture2D(s_originalBackdropTexture, bgTexCoord);
        return mix(original_backdrop, backdrop, mask);
      }
    });
  } else {
    mixFunction = SHADER0([]() {
      vec4 MixBackdrop(TexCoordPrecision vec2 bgTexCoord, float mask) {
        return texture2D(s_backdropTexture, bgTexCoord);
      }
    });
  }

  static const std::string kFunctionApplyBlendMode = SHADER0([]() {
    vec4 GetBackdropColor(float mask) {
      TexCoordPrecision vec2 bgTexCoord = gl_FragCoord.xy - backdropRect.xy;
      bgTexCoord.x /= backdropRect.z;
      bgTexCoord.y /= backdropRect.w;
      return MixBackdrop(bgTexCoord, mask);
    }

    vec4 ApplyBlendMode(vec4 src, float mask) {
      vec4 dst = GetBackdropColor(mask);
      return Blend(src, dst);
    }
  });

  return "precision mediump float;" + GetHelperFunctions() +
         GetBlendFunction() + kUniforms + mixFunction +
         kFunctionApplyBlendMode + shader_string;
}

std::string FragmentShaderBase::GetHelperFunctions() const {
  static const std::string kFunctionHardLight = SHADER0([]() {
    vec3 hardLight(vec4 src, vec4 dst) {
      vec3 result;
      result.r =
          (2.0 * src.r <= src.a)
              ? (2.0 * src.r * dst.r)
              : (src.a * dst.a - 2.0 * (dst.a - dst.r) * (src.a - src.r));
      result.g =
          (2.0 * src.g <= src.a)
              ? (2.0 * src.g * dst.g)
              : (src.a * dst.a - 2.0 * (dst.a - dst.g) * (src.a - src.g));
      result.b =
          (2.0 * src.b <= src.a)
              ? (2.0 * src.b * dst.b)
              : (src.a * dst.a - 2.0 * (dst.a - dst.b) * (src.a - src.b));
      result.rgb += src.rgb * (1.0 - dst.a) + dst.rgb * (1.0 - src.a);
      return result;
    }
  });

  static const std::string kFunctionColorDodgeComponent = SHADER0([]() {
    float getColorDodgeComponent(float srcc, float srca, float dstc,
                                 float dsta) {
      if (0.0 == dstc)
        return srcc * (1.0 - dsta);
      float d = srca - srcc;
      if (0.0 == d)
        return srca * dsta + srcc * (1.0 - dsta) + dstc * (1.0 - srca);
      d = min(dsta, dstc * srca / d);
      return d * srca + srcc * (1.0 - dsta) + dstc * (1.0 - srca);
    }
  });

  static const std::string kFunctionColorBurnComponent = SHADER0([]() {
    float getColorBurnComponent(float srcc, float srca, float dstc,
                                float dsta) {
      if (dsta == dstc)
        return srca * dsta + srcc * (1.0 - dsta) + dstc * (1.0 - srca);
      if (0.0 == srcc)
        return dstc * (1.0 - srca);
      float d = max(0.0, dsta - (dsta - dstc) * srca / srcc);
      return srca * d + srcc * (1.0 - dsta) + dstc * (1.0 - srca);
    }
  });

  static const std::string kFunctionSoftLightComponentPosDstAlpha =
      SHADER0([]() {
        float getSoftLightComponent(float srcc, float srca, float dstc,
                                    float dsta) {
          if (2.0 * srcc <= srca) {
            return (dstc * dstc * (srca - 2.0 * srcc)) / dsta +
                   (1.0 - dsta) * srcc + dstc * (-srca + 2.0 * srcc + 1.0);
          } else if (4.0 * dstc <= dsta) {
            float DSqd = dstc * dstc;
            float DCub = DSqd * dstc;
            float DaSqd = dsta * dsta;
            float DaCub = DaSqd * dsta;
            return (-DaCub * srcc +
                    DaSqd * (srcc - dstc * (3.0 * srca - 6.0 * srcc - 1.0)) +
                    12.0 * dsta * DSqd * (srca - 2.0 * srcc) -
                    16.0 * DCub * (srca - 2.0 * srcc)) /
                   DaSqd;
          } else {
            return -sqrt(dsta * dstc) * (srca - 2.0 * srcc) - dsta * srcc +
                   dstc * (srca - 2.0 * srcc + 1.0) + srcc;
          }
        }
      });

  static const std::string kFunctionLum = SHADER0([]() {
    float luminance(vec3 color) { return dot(vec3(0.3, 0.59, 0.11), color); }

    vec3 set_luminance(vec3 hueSat, float alpha, vec3 lumColor) {
      float diff = luminance(lumColor - hueSat);
      vec3 outColor = hueSat + diff;
      float outLum = luminance(outColor);
      float minComp = min(min(outColor.r, outColor.g), outColor.b);
      float maxComp = max(max(outColor.r, outColor.g), outColor.b);
      if (minComp < 0.0 && outLum != minComp) {
        outColor = outLum +
                   ((outColor - vec3(outLum, outLum, outLum)) * outLum) /
                       (outLum - minComp);
      }
      if (maxComp > alpha && maxComp != outLum) {
        outColor =
            outLum +
            ((outColor - vec3(outLum, outLum, outLum)) * (alpha - outLum)) /
                (maxComp - outLum);
      }
      return outColor;
    }
  });

  static const std::string kFunctionSat = SHADER0([]() {
    float saturation(vec3 color) {
      return max(max(color.r, color.g), color.b) -
             min(min(color.r, color.g), color.b);
    }

    vec3 set_saturation_helper(float minComp, float midComp, float maxComp,
                               float sat) {
      if (minComp < maxComp) {
        vec3 result;
        result.r = 0.0;
        result.g = sat * (midComp - minComp) / (maxComp - minComp);
        result.b = sat;
        return result;
      } else {
        return vec3(0, 0, 0);
      }
    }

    vec3 set_saturation(vec3 hueLumColor, vec3 satColor) {
      float sat = saturation(satColor);
      if (hueLumColor.r <= hueLumColor.g) {
        if (hueLumColor.g <= hueLumColor.b) {
          hueLumColor.rgb = set_saturation_helper(hueLumColor.r, hueLumColor.g,
                                                  hueLumColor.b, sat);
        } else if (hueLumColor.r <= hueLumColor.b) {
          hueLumColor.rbg = set_saturation_helper(hueLumColor.r, hueLumColor.b,
                                                  hueLumColor.g, sat);
        } else {
          hueLumColor.brg = set_saturation_helper(hueLumColor.b, hueLumColor.r,
                                                  hueLumColor.g, sat);
        }
      } else if (hueLumColor.r <= hueLumColor.b) {
        hueLumColor.grb = set_saturation_helper(hueLumColor.g, hueLumColor.r,
                                                hueLumColor.b, sat);
      } else if (hueLumColor.g <= hueLumColor.b) {
        hueLumColor.gbr = set_saturation_helper(hueLumColor.g, hueLumColor.b,
                                                hueLumColor.r, sat);
      } else {
        hueLumColor.bgr = set_saturation_helper(hueLumColor.b, hueLumColor.g,
                                                hueLumColor.r, sat);
      }
      return hueLumColor;
    }
  });

  switch (blend_mode_) {
    case BLEND_MODE_OVERLAY:
    case BLEND_MODE_HARD_LIGHT:
      return kFunctionHardLight;
    case BLEND_MODE_COLOR_DODGE:
      return kFunctionColorDodgeComponent;
    case BLEND_MODE_COLOR_BURN:
      return kFunctionColorBurnComponent;
    case BLEND_MODE_SOFT_LIGHT:
      return kFunctionSoftLightComponentPosDstAlpha;
    case BLEND_MODE_HUE:
    case BLEND_MODE_SATURATION:
      return kFunctionLum + kFunctionSat;
    case BLEND_MODE_COLOR:
    case BLEND_MODE_LUMINOSITY:
      return kFunctionLum;
    default:
      return std::string();
  }
}

std::string FragmentShaderBase::GetBlendFunction() const {
  return "vec4 Blend(vec4 src, vec4 dst) {"
         "    vec4 result;"
         "    result.a = src.a + (1.0 - src.a) * dst.a;" +
         GetBlendFunctionBodyForRGB() +
         "    return result;"
         "}";
}

std::string FragmentShaderBase::GetBlendFunctionBodyForRGB() const {
  switch (blend_mode_) {
    case BLEND_MODE_NORMAL:
      return "result.rgb = src.rgb + dst.rgb * (1.0 - src.a);";
    case BLEND_MODE_SCREEN:
      return "result.rgb = src.rgb + (1.0 - src.rgb) * dst.rgb;";
    case BLEND_MODE_LIGHTEN:
      return "result.rgb = max((1.0 - src.a) * dst.rgb + src.rgb,"
             "                 (1.0 - dst.a) * src.rgb + dst.rgb);";
    case BLEND_MODE_OVERLAY:
      return "result.rgb = hardLight(dst, src);";
    case BLEND_MODE_DARKEN:
      return "result.rgb = min((1.0 - src.a) * dst.rgb + src.rgb,"
             "                 (1.0 - dst.a) * src.rgb + dst.rgb);";
    case BLEND_MODE_COLOR_DODGE:
      return "result.r = getColorDodgeComponent(src.r, src.a, dst.r, dst.a);"
             "result.g = getColorDodgeComponent(src.g, src.a, dst.g, dst.a);"
             "result.b = getColorDodgeComponent(src.b, src.a, dst.b, dst.a);";
    case BLEND_MODE_COLOR_BURN:
      return "result.r = getColorBurnComponent(src.r, src.a, dst.r, dst.a);"
             "result.g = getColorBurnComponent(src.g, src.a, dst.g, dst.a);"
             "result.b = getColorBurnComponent(src.b, src.a, dst.b, dst.a);";
    case BLEND_MODE_HARD_LIGHT:
      return "result.rgb = hardLight(src, dst);";
    case BLEND_MODE_SOFT_LIGHT:
      return "if (0.0 == dst.a) {"
             "  result.rgb = src.rgb;"
             "} else {"
             "  result.r = getSoftLightComponent(src.r, src.a, dst.r, dst.a);"
             "  result.g = getSoftLightComponent(src.g, src.a, dst.g, dst.a);"
             "  result.b = getSoftLightComponent(src.b, src.a, dst.b, dst.a);"
             "}";
    case BLEND_MODE_DIFFERENCE:
      return "result.rgb = src.rgb + dst.rgb -"
             "    2.0 * min(src.rgb * dst.a, dst.rgb * src.a);";
    case BLEND_MODE_EXCLUSION:
      return "result.rgb = dst.rgb + src.rgb - 2.0 * dst.rgb * src.rgb;";
    case BLEND_MODE_MULTIPLY:
      return "result.rgb = (1.0 - src.a) * dst.rgb +"
             "    (1.0 - dst.a) * src.rgb + src.rgb * dst.rgb;";
    case BLEND_MODE_HUE:
      return "vec4 dstSrcAlpha = dst * src.a;"
             "result.rgb ="
             "    set_luminance(set_saturation(src.rgb * dst.a,"
             "                                 dstSrcAlpha.rgb),"
             "                  dstSrcAlpha.a,"
             "                  dstSrcAlpha.rgb);"
             "result.rgb += (1.0 - src.a) * dst.rgb + (1.0 - dst.a) * src.rgb;";
    case BLEND_MODE_SATURATION:
      return "vec4 dstSrcAlpha = dst * src.a;"
             "result.rgb = set_luminance(set_saturation(dstSrcAlpha.rgb,"
             "                                          src.rgb * dst.a),"
             "                           dstSrcAlpha.a,"
             "                           dstSrcAlpha.rgb);"
             "result.rgb += (1.0 - src.a) * dst.rgb + (1.0 - dst.a) * src.rgb;";
    case BLEND_MODE_COLOR:
      return "vec4 srcDstAlpha = src * dst.a;"
             "result.rgb = set_luminance(srcDstAlpha.rgb,"
             "                           srcDstAlpha.a,"
             "                           dst.rgb * src.a);"
             "result.rgb += (1.0 - src.a) * dst.rgb + (1.0 - dst.a) * src.rgb;";
    case BLEND_MODE_LUMINOSITY:
      return "vec4 srcDstAlpha = src * dst.a;"
             "result.rgb = set_luminance(dst.rgb * src.a,"
             "                           srcDstAlpha.a,"
             "                           srcDstAlpha.rgb);"
             "result.rgb += (1.0 - src.a) * dst.rgb + (1.0 - dst.a) * src.rgb;";
    case BLEND_MODE_NONE:
      NOTREACHED();
  }
  return "result = vec4(1.0, 0.0, 0.0, 1.0);";
}

std::string FragmentShaderBase::GetShaderSource() const {
  std::string header = "precision mediump float;\n";
  std::string source = "void main() {\n";

#define HDR(x)                       \
  do {                               \
    header += x + std::string("\n"); \
  } while (0)
#define SRC(x)                                           \
  do {                                                   \
    source += std::string("  ") + x + std::string("\n"); \
  } while (0)

  // Read the input into vec4 texColor.
  switch (input_color_type_) {
    case INPUT_COLOR_SOURCE_RGBA_TEXTURE:
      if (ignore_sampler_type_)
        HDR("uniform sampler2D s_texture;");
      else
        HDR("uniform SamplerType s_texture;");
      HDR("varying TexCoordPrecision vec2 v_texCoord;");
      if (has_rgba_fragment_tex_transform_) {
        HDR("uniform TexCoordPrecision vec4 fragmentTexTransform;");
        SRC("// Transformed texture lookup");
        SRC("TexCoordPrecision vec2 texCoord =");
        SRC("    clamp(v_texCoord, 0.0, 1.0) * fragmentTexTransform.zw +");
        SRC("   fragmentTexTransform.xy;");
        SRC("vec4 texColor = TextureLookup(s_texture, texCoord);");
        DCHECK(!ignore_sampler_type_);
      } else {
        SRC("// Texture lookup");
        if (ignore_sampler_type_)
          SRC("vec4 texColor = texture2D(s_texture, v_texCoord);");
        else
          SRC("vec4 texColor = TextureLookup(s_texture, v_texCoord);");
      }
      break;
    case INPUT_COLOR_SOURCE_UNIFORM:
      DCHECK(!ignore_sampler_type_);
      DCHECK(!has_rgba_fragment_tex_transform_);
      HDR("uniform vec4 color;");
      SRC("// Uniform color");
      SRC("vec4 texColor = color;");
      break;
  }
  // Apply the color matrix to texColor.
  if (has_color_matrix_) {
    HDR("uniform mat4 colorMatrix;");
    HDR("uniform vec4 colorOffset;");
    SRC("// Apply color matrix");
    SRC("float nonZeroAlpha = max(texColor.a, 0.00001);");
    SRC("texColor = vec4(texColor.rgb / nonZeroAlpha, nonZeroAlpha);");
    SRC("texColor = colorMatrix * texColor + colorOffset;");
    SRC("texColor.rgb *= texColor.a;");
    SRC("texColor = clamp(texColor, 0.0, 1.0);");
  }

  // Read the mask texture.
  if (has_mask_sampler_) {
    HDR("uniform SamplerType s_mask;");
    HDR("uniform vec2 maskTexCoordScale;");
    HDR("uniform vec2 maskTexCoordOffset;");
    SRC("// Read the mask");
    SRC("TexCoordPrecision vec2 maskTexCoord =");
    SRC("    vec2(maskTexCoordOffset.x + v_texCoord.x * maskTexCoordScale.x,");
    SRC("         maskTexCoordOffset.y + v_texCoord.y * maskTexCoordScale.y);");
    SRC("vec4 maskColor = TextureLookup(s_mask, maskTexCoord);");
  }

  // Compute AA.
  if (has_aa_) {
    HDR("varying TexCoordPrecision vec4 edge_dist[2];  // 8 edge distances.");
    SRC("// Compute AA");
    SRC("vec4 d4 = min(edge_dist[0], edge_dist[1]);");
    SRC("vec2 d2 = min(d4.xz, d4.yw);");
    SRC("float aa = clamp(gl_FragCoord.w * min(d2.x, d2.y), 0.0, 1.0);");
  }

  // Premultiply by alpha.
  if (has_premultiply_alpha_) {
    SRC("// Premultiply alpha");
    SRC("texColor.rgb *= texColor.a;");
  }

  // Apply background texture.
  if (has_background_color_) {
    HDR("uniform vec4 background_color;");
    SRC("// Apply uniform background color blending");
    SRC("texColor += background_color * (1.0 - texColor.a);");
  }

  // Apply swizzle.
  if (has_swizzle_) {
    SRC("// Apply swizzle");
    SRC("texColor = texColor.bgra;\n");
  }

  // Include header text for alpha.
  if (has_uniform_alpha_) {
    HDR("uniform float alpha;");
  }
  if (has_varying_alpha_) {
    HDR("varying float v_alpha;");
  }

  // Apply uniform alpha, aa, varying alpha, and the mask.
  if (has_varying_alpha_ || has_aa_ || has_uniform_alpha_ ||
      has_mask_sampler_) {
    SRC("// Apply alpha from uniform, varying, aa, and mask.");
    std::string line = "  texColor = texColor";
    if (has_varying_alpha_)
      line += " * v_alpha";
    if (has_uniform_alpha_)
      line += " * alpha";
    if (has_aa_)
      line += " * aa";
    if (has_mask_sampler_)
      line += " * maskColor.a";
    line += ";\n";
    source += line;
  }

  // Write the fragment color.
  SRC("// Write the fragment color");
  switch (frag_color_mode_) {
    case FRAG_COLOR_MODE_DEFAULT:
      DCHECK_EQ(blend_mode_, BLEND_MODE_NONE);
      SRC("gl_FragColor = texColor;");
      break;
    case FRAG_COLOR_MODE_OPAQUE:
      DCHECK_EQ(blend_mode_, BLEND_MODE_NONE);
      SRC("gl_FragColor = vec4(texColor.rgb, 1.0);");
      break;
    case FRAG_COLOR_MODE_APPLY_BLEND_MODE:
      if (has_mask_sampler_)
        SRC("gl_FragColor = ApplyBlendMode(texColor, maskColor.w);");
      else
        SRC("gl_FragColor = ApplyBlendMode(texColor, 0.0);");
      break;
  }
  source += "}\n";

#undef HDR
#undef SRC

  return header + source;
}

FragmentShaderYUVVideo::FragmentShaderYUVVideo() {}

void FragmentShaderYUVVideo::SetFeatures(bool use_alpha_texture,
                                         bool use_nv12,
                                         bool use_color_lut) {
  use_alpha_texture_ = use_alpha_texture;
  use_nv12_ = use_nv12;
  use_color_lut_ = use_color_lut;
}

void FragmentShaderYUVVideo::Init(GLES2Interface* context,
                                  unsigned program,
                                  int* base_uniform_index) {
  static const char* uniforms[] = {
      "y_texture",
      "u_texture",
      "v_texture",
      "uv_texture",
      "a_texture",
      "lut_texture",
      "resource_multiplier",
      "resource_offset",
      "yuv_matrix",
      "yuv_adj",
      "alpha",
      "ya_clamp_rect",
      "uv_clamp_rect",
  };
  int locations[arraysize(uniforms)];

  GetProgramUniformLocations(context,
                             program,
                             arraysize(uniforms),
                             uniforms,
                             locations,
                             base_uniform_index);
  y_texture_location_ = locations[0];
  if (!use_nv12_) {
    u_texture_location_ = locations[1];
    v_texture_location_ = locations[2];
  } else {
    uv_texture_location_ = locations[3];
  }
  if (use_alpha_texture_) {
    a_texture_location_ = locations[4];
  }
  if (use_color_lut_) {
    lut_texture_location_ = locations[5];
    resource_multiplier_location_ = locations[6];
    resource_offset_location_ = locations[7];
  } else {
    yuv_matrix_location_ = locations[8];
    yuv_adj_location_ = locations[9];
  }
  alpha_location_ = locations[10];
  ya_clamp_rect_location_ = locations[11];
  uv_clamp_rect_location_ = locations[12];
}

std::string FragmentShaderYUVVideo::GetShaderSource() const {
  std::string head = SHADER0([]() {
    precision mediump float;
    precision mediump int;
    varying TexCoordPrecision vec2 v_yaTexCoord;
    varying TexCoordPrecision vec2 v_uvTexCoord;
    uniform SamplerType y_texture;
    uniform float alpha;
    uniform vec4 ya_clamp_rect;
    uniform vec4 uv_clamp_rect;
  });

  std::string functions = "";
  if (use_nv12_) {
    head += "  uniform SamplerType uv_texture;\n";
    functions += SHADER0([]() {
      vec2 GetUV(vec2 uv_clamped) {
        return TextureLookup(uv_texture, uv_clamped).xy;
      }
    });
  } else {
    head += "  uniform SamplerType u_texture;\n";
    head += "  uniform SamplerType v_texture;\n";
    functions += SHADER0([]() {
      vec2 GetUV(vec2 uv_clamped) {
        return vec2(TextureLookup(u_texture, uv_clamped).x,
                    TextureLookup(v_texture, uv_clamped).x);
      }
    });
  }

  if (use_alpha_texture_) {
    head += "  uniform SamplerType a_texture;\n";
    functions += SHADER0([]() {
      float GetAlpha(vec2 ya_clamped) {
        return alpha * TextureLookup(a_texture, ya_clamped).x;
      }
    });
  } else {
    functions += SHADER0([]() {
      float GetAlpha(vec2 ya_clamped) { return alpha; }
    });
  }

  if (use_color_lut_) {
    head += "  uniform sampler2D lut_texture;\n";
    head += "  uniform float resource_multiplier;\n";
    head += "  uniform float resource_offset;\n";
    functions += SHADER0([]() {
      vec4 LUT(sampler2D sampler, vec3 pos, float size) {
        pos *= size - 1.0;
        // Select layer
        float layer = min(floor(pos.z), size - 2.0);
        // Compress the xy coordinates so they stay within
        // [0.5 .. 31.5] / 17 (assuming a LUT size of 17^3)
        pos.xy = (pos.xy + vec2(0.5)) / size;
        pos.y = (pos.y + layer) / size;
        return mix(texture2D(sampler, pos.xy),
                   texture2D(sampler, pos.xy + vec2(0, 1.0 / size)),
                   pos.z - layer);
      }

      vec3 yuv2rgb(vec3 yuv) {
        yuv = (yuv - vec3(resource_offset)) * resource_multiplier;
        return LUT(lut_texture, yuv, 17.0).xyz;
      }
    });
  } else {
    head += "  uniform mat3 yuv_matrix;\n";
    head += "  uniform vec3 yuv_adj;\n";
    functions += SHADER0([]() {
      vec3 yuv2rgb(vec3 yuv) { return yuv_matrix * (yuv + yuv_adj); }
    });
  }

  functions += SHADER0([]() {
    void main() {
      vec2 ya_clamped =
          max(ya_clamp_rect.xy, min(ya_clamp_rect.zw, v_yaTexCoord));
      float y_raw = TextureLookup(y_texture, ya_clamped).x;
      vec2 uv_clamped =
          max(uv_clamp_rect.xy, min(uv_clamp_rect.zw, v_uvTexCoord));
      vec3 yuv = vec3(y_raw, GetUV(uv_clamped));
      gl_FragColor = vec4(yuv2rgb(yuv), 1.0) * GetAlpha(ya_clamped);
    }
  });

  return head + functions;
}

}  // namespace cc
