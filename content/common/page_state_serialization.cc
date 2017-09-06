// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/page_state_serialization.h"

#include <stddef.h>

#include <algorithm>
#include <limits>

#include "base/pickle.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/common/page_state.mojom.h"
#include "content/common/unique_name_helper.h"
#include "content/public/common/resource_request_body.h"
#include "mojo/common/common_custom_types_struct_traits.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace content {
namespace {

#if defined(OS_ANDROID)
float g_device_scale_factor_for_testing = 0.0;
#endif

//-----------------------------------------------------------------------------

void AppendDataToRequestBody(
    const scoped_refptr<ResourceRequestBody>& request_body,
    const char* data,
    int data_length) {
  request_body->AppendBytes(data, data_length);
}

void AppendFileRangeToRequestBody(
    const scoped_refptr<ResourceRequestBody>& request_body,
    const base::NullableString16& file_path,
    int file_start,
    int file_length,
    double file_modification_time) {
  request_body->AppendFileRange(
      base::FilePath::FromUTF16Unsafe(file_path.string()),
      static_cast<uint64_t>(file_start), static_cast<uint64_t>(file_length),
      base::Time::FromDoubleT(file_modification_time));
}

void AppendURLRangeToRequestBody(
    const scoped_refptr<ResourceRequestBody>& request_body,
    const GURL& url,
    int file_start,
    int file_length,
    double file_modification_time) {
  request_body->AppendFileSystemFileRange(
      url, static_cast<uint64_t>(file_start),
      static_cast<uint64_t>(file_length),
      base::Time::FromDoubleT(file_modification_time));
}

void AppendBlobToRequestBody(
    const scoped_refptr<ResourceRequestBody>& request_body,
    const std::string& uuid) {
  request_body->AppendBlob(uuid);
}

//----------------------------------------------------------------------------

void AppendReferencedFilesFromHttpBody(
    const std::vector<ResourceRequestBody::Element>& elements,
    std::vector<base::NullableString16>* referenced_files) {
  for (size_t i = 0; i < elements.size(); ++i) {
    if (elements[i].type() == ResourceRequestBody::Element::TYPE_FILE)
      referenced_files->push_back(
          base::NullableString16(elements[i].path().AsUTF16Unsafe(), false));
  }
}

bool AppendReferencedFilesFromDocumentState(
    const std::vector<base::NullableString16>& document_state,
    std::vector<base::NullableString16>* referenced_files) {
  if (document_state.empty())
    return true;

  // This algorithm is adapted from Blink's FormController code.
  // We only care about how that code worked when this code snapshot was taken
  // as this code is only needed for backwards compat.
  //
  // For reference, see FormController::formStatesFromStateVector in
  // third_party/WebKit/Source/core/html/forms/FormController.cpp.

  size_t index = 0;

  if (document_state.size() < 3)
    return false;

  index++;  // Skip over magic signature.
  index++;  // Skip over form key.

  size_t item_count;
  if (!base::StringToSizeT(document_state[index++].string(), &item_count))
    return false;

  while (item_count--) {
    if (index + 1 >= document_state.size())
      return false;

    index++;  // Skip over name.
    const base::NullableString16& type = document_state[index++];

    if (index >= document_state.size())
      return false;

    size_t value_size;
    if (!base::StringToSizeT(document_state[index++].string(), &value_size))
      return false;

    if (index + value_size > document_state.size() ||
        index + value_size < index)  // Check for overflow.
      return false;

    if (base::EqualsASCII(type.string(), "file")) {
      if (value_size != 2)
        return false;

      referenced_files->push_back(document_state[index++]);
      index++;  // Skip over display name.
    } else {
      index += value_size;
    }
  }

  return true;
}

bool RecursivelyAppendReferencedFiles(
    const ExplodedFrameState& frame_state,
    std::vector<base::NullableString16>* referenced_files) {
  if (frame_state.http_body.request_body != nullptr) {
    AppendReferencedFilesFromHttpBody(
        *frame_state.http_body.request_body->elements(), referenced_files);
  }

  if (!AppendReferencedFilesFromDocumentState(frame_state.document_state,
                                              referenced_files))
    return false;

  for (size_t i = 0; i < frame_state.children.size(); ++i) {
    if (!RecursivelyAppendReferencedFiles(frame_state.children[i],
                                          referenced_files))
      return false;
  }

  return true;
}

//----------------------------------------------------------------------------

struct SerializeObject {
  SerializeObject()
      : version(0),
        parse_error(false) {
  }

  SerializeObject(const char* data, int len)
      : pickle(data, len),
        version(0),
        parse_error(false) {
    iter = base::PickleIterator(pickle);
  }

  std::string GetAsString() {
    return std::string(static_cast<const char*>(pickle.data()), pickle.size());
  }

  base::Pickle pickle;
  base::PickleIterator iter;
  int version;
  bool parse_error;
};

// Version ID of serialized format.
// 11: Min version
// 12: Adds support for contains_passwords in HTTP body
// 13: Adds support for URL (FileSystem URL)
// 14: Adds list of referenced files, version written only for first item.
// 15: Removes a bunch of values we defined but never used.
// 16: Switched from blob urls to blob uuids.
// 17: Add a target frame id number.
// 18: Add referrer policy.
// 19: Remove target frame id, which was a bad idea, and original url string,
//         which is no longer used.
// 20: Add visual viewport scroll offset, the offset of the pinched zoomed
//     viewport within the unzoomed main frame.
// 21: Add frame sequence number.
// 22: Add scroll restoration type.
// 23: Remove frame sequence number, there are easier ways.
// 24: Add did save scroll or scale state.
// 25: Limit the length of unique names: https://crbug.com/626202
// 26: Switch to mojo-based serialization.
//
// NOTE: If the version is -1, then the pickle contains only a URL string.
// See ReadPageState.
//
const int kMinVersion = 11;
// NOTE: When changing the version, please add a backwards compatibility test.
// See PageStateSerializationTest.DumpExpectedPageStateForBackwardsCompat for
// instructions on how to generate the new test case.
const int kCurrentVersion = 26;

// A bunch of convenience functions to write to/read from SerializeObjects.  The
// de-serializers assume the input data will be in the correct format and fall
// back to returning safe defaults when not. These are mostly used by
// legacy(pre-mojo) serialization methods. If you're making changes to the
// PageState serialization format You almost certainly want to add/remove fields
// in page_state.mojom rather than using these methods.

void WriteData(const void* data, int length, SerializeObject* obj) {
  obj->pickle.WriteData(static_cast<const char*>(data), length);
}

void ReadData(SerializeObject* obj, const void** data, int* length) {
  const char* tmp;
  if (obj->iter.ReadData(&tmp, length)) {
    *data = tmp;
  } else {
    obj->parse_error = true;
    *data = NULL;
    *length = 0;
  }
}

void WriteInteger(int data, SerializeObject* obj) {
  obj->pickle.WriteInt(data);
}

int ReadInteger(SerializeObject* obj) {
  int tmp;
  if (obj->iter.ReadInt(&tmp))
    return tmp;
  obj->parse_error = true;
  return 0;
}

void WriteInteger64(int64_t data, SerializeObject* obj) {
  obj->pickle.WriteInt64(data);
}

int64_t ReadInteger64(SerializeObject* obj) {
  int64_t tmp = 0;
  if (obj->iter.ReadInt64(&tmp))
    return tmp;
  obj->parse_error = true;
  return 0;
}

void WriteReal(double data, SerializeObject* obj) {
  WriteData(&data, sizeof(double), obj);
}

double ReadReal(SerializeObject* obj) {
  const void* tmp = NULL;
  int length = 0;
  double value = 0.0;
  ReadData(obj, &tmp, &length);
  if (length == static_cast<int>(sizeof(double))) {
    // Use memcpy, as tmp may not be correctly aligned.
    memcpy(&value, tmp, sizeof(double));
  } else {
    obj->parse_error = true;
  }
  return value;
}

void WriteBoolean(bool data, SerializeObject* obj) {
  obj->pickle.WriteInt(data ? 1 : 0);
}

bool ReadBoolean(SerializeObject* obj) {
  bool tmp;
  if (obj->iter.ReadBool(&tmp))
    return tmp;
  obj->parse_error = true;
  return false;
}

void WriteGURL(const GURL& url, SerializeObject* obj) {
  obj->pickle.WriteString(url.possibly_invalid_spec());
}

GURL ReadGURL(SerializeObject* obj) {
  std::string spec;
  if (obj->iter.ReadString(&spec))
    return GURL(spec);
  obj->parse_error = true;
  return GURL();
}

void WriteStdString(const std::string& s, SerializeObject* obj) {
  obj->pickle.WriteString(s);
}

std::string ReadStdString(SerializeObject* obj) {
  std::string s;
  if (obj->iter.ReadString(&s))
    return s;
  obj->parse_error = true;
  return std::string();
}

// WriteString pickles the NullableString16 as <int length><char16* data>.
// If length == -1, then the NullableString16 itself is null.  Otherwise the
// length is the number of char16 (not bytes) in the NullableString16.
void WriteString(const base::NullableString16& str, SerializeObject* obj) {
  if (str.is_null()) {
    obj->pickle.WriteInt(-1);
  } else {
    const base::char16* data = str.string().data();
    size_t length_in_bytes = str.string().length() * sizeof(base::char16);

    CHECK_LT(length_in_bytes,
             static_cast<size_t>(std::numeric_limits<int>::max()));
    obj->pickle.WriteInt(length_in_bytes);
    obj->pickle.WriteBytes(data, length_in_bytes);
  }
}

// This reads a serialized NullableString16 from obj. If a string can't be
// read, NULL is returned.
const base::char16* ReadStringNoCopy(SerializeObject* obj, int* num_chars) {
  int length_in_bytes;
  if (!obj->iter.ReadInt(&length_in_bytes)) {
    obj->parse_error = true;
    return NULL;
  }

  if (length_in_bytes < 0)
    return NULL;

  const char* data;
  if (!obj->iter.ReadBytes(&data, length_in_bytes)) {
    obj->parse_error = true;
    return NULL;
  }

  if (num_chars)
    *num_chars = length_in_bytes / sizeof(base::char16);
  return reinterpret_cast<const base::char16*>(data);
}

base::NullableString16 ReadString(SerializeObject* obj) {
  int num_chars;
  const base::char16* chars = ReadStringNoCopy(obj, &num_chars);
  return chars ?
      base::NullableString16(base::string16(chars, num_chars), false) :
      base::NullableString16();
}

template <typename T>
void WriteAndValidateVectorSize(const std::vector<T>& v, SerializeObject* obj) {
  CHECK_LT(v.size(), std::numeric_limits<int>::max() / sizeof(T));
  WriteInteger(static_cast<int>(v.size()), obj);
}

size_t ReadAndValidateVectorSize(SerializeObject* obj, size_t element_size) {
  size_t num_elements = static_cast<size_t>(ReadInteger(obj));

  // Ensure that resizing a vector to size num_elements makes sense.
  if (std::numeric_limits<int>::max() / element_size <= num_elements) {
    obj->parse_error = true;
    return 0;
  }

  // Ensure that it is plausible for the pickle to contain num_elements worth
  // of data.
  if (obj->pickle.payload_size() <= num_elements) {
    obj->parse_error = true;
    return 0;
  }

  return num_elements;
}

// Writes a Vector of strings into a SerializeObject for serialization.
void WriteStringVector(
    const std::vector<base::NullableString16>& data, SerializeObject* obj) {
  WriteAndValidateVectorSize(data, obj);
  for (size_t i = 0; i < data.size(); ++i) {
    WriteString(data[i], obj);
  }
}

void ReadStringVector(SerializeObject* obj,
                      std::vector<base::NullableString16>* result) {
  size_t num_elements =
      ReadAndValidateVectorSize(obj, sizeof(base::NullableString16));

  result->resize(num_elements);
  for (size_t i = 0; i < num_elements; ++i)
    (*result)[i] = ReadString(obj);
}

base::NullableString16 NS16(base::string16 s) {
  return s.empty() ? base::NullableString16()
                   : base::NullableString16(s, false);
}

void LegacyReadResourceRequestBody(
    SerializeObject* obj,
    const scoped_refptr<ResourceRequestBody>& request_body) {
  int num_elements = ReadInteger(obj);
  for (int i = 0; i < num_elements; ++i) {
    int type = ReadInteger(obj);
    if (type == blink::WebHTTPBody::Element::kTypeData) {
      const void* data;
      int length = -1;
      ReadData(obj, &data, &length);
      if (length >= 0) {
        AppendDataToRequestBody(request_body, static_cast<const char*>(data),
                                length);
      }
    } else if (type == blink::WebHTTPBody::Element::kTypeFile) {
      base::NullableString16 file_path = ReadString(obj);
      int64_t file_start = ReadInteger64(obj);
      int64_t file_length = ReadInteger64(obj);
      double file_modification_time = ReadReal(obj);
      AppendFileRangeToRequestBody(request_body, file_path, file_start,
                                   file_length, file_modification_time);
    } else if (type == blink::WebHTTPBody::Element::kTypeFileSystemURL) {
      GURL url = ReadGURL(obj);
      int64_t file_start = ReadInteger64(obj);
      int64_t file_length = ReadInteger64(obj);
      double file_modification_time = ReadReal(obj);
      AppendURLRangeToRequestBody(request_body, url, file_start, file_length,
                                  file_modification_time);
    } else if (type == blink::WebHTTPBody::Element::kTypeBlob) {
      if (obj->version >= 16) {
        std::string blob_uuid = ReadStdString(obj);
        AppendBlobToRequestBody(request_body, blob_uuid);
      } else {
        ReadGURL(obj); // Skip the obsolete blob url value.
      }
    }
  }
  request_body->set_identifier(ReadInteger64(obj));
}

void LegacyWriteResourceRequestBody(const ResourceRequestBody& request_body,
                                    SerializeObject* obj) {
  WriteAndValidateVectorSize(*request_body.elements(), obj);
  for (const auto& element : *request_body.elements()) {
    switch (element.type()) {
      case ResourceRequestBody::Element::TYPE_BYTES:
        WriteInteger(blink::WebHTTPBody::Element::kTypeData, obj);
        WriteData(element.bytes(), static_cast<int>(element.length()), obj);
        break;
      case ResourceRequestBody::Element::TYPE_FILE:
        WriteInteger(blink::WebHTTPBody::Element::kTypeFile, obj);
        WriteString(
            base::NullableString16(element.path().AsUTF16Unsafe(), false), obj);
        WriteInteger64(static_cast<int64_t>(element.offset()), obj);
        WriteInteger64(static_cast<int64_t>(element.length()), obj);
        WriteReal(element.expected_modification_time().ToDoubleT(), obj);
        break;
      case ResourceRequestBody::Element::TYPE_FILE_FILESYSTEM:
        WriteInteger(blink::WebHTTPBody::Element::kTypeFileSystemURL, obj);
        WriteGURL(element.filesystem_url(), obj);
        WriteInteger64(static_cast<int64_t>(element.offset()), obj);
        WriteInteger64(static_cast<int64_t>(element.length()), obj);
        WriteReal(element.expected_modification_time().ToDoubleT(), obj);
        break;
      case ResourceRequestBody::Element::TYPE_BLOB:
        WriteInteger(blink::WebHTTPBody::Element::kTypeBlob, obj);
        WriteStdString(element.blob_uuid(), obj);
        break;
      case ResourceRequestBody::Element::TYPE_BYTES_DESCRIPTION:
      case ResourceRequestBody::Element::TYPE_DISK_CACHE_ENTRY:
      default:
        NOTREACHED();
        continue;
    }
  }
  WriteInteger64(request_body.identifier(), obj);
}

void LegacyReadHttpBody(SerializeObject* obj, ExplodedHttpBody* http_body) {
  // An initial boolean indicates if we have an HTTP body.
  if (!ReadBoolean(obj))
    return;

  http_body->request_body = new ResourceRequestBody();
  LegacyReadResourceRequestBody(obj, http_body->request_body);

  if (obj->version >= 12)
    http_body->contains_passwords = ReadBoolean(obj);
}

void LegacyWriteHttpBody(const ExplodedHttpBody& http_body,
                         SerializeObject* obj) {
  bool is_null = http_body.request_body == nullptr;
  WriteBoolean(!is_null, obj);
  if (is_null)
    return;

  LegacyWriteResourceRequestBody(*http_body.request_body, obj);
  WriteBoolean(http_body.contains_passwords, obj);
}

void LegacyReadFrameState(
    SerializeObject* obj,
    bool is_top,
    std::vector<UniqueNameHelper::Replacement>* unique_name_replacements,
    ExplodedFrameState* state) {
  if (obj->version < 14 && !is_top)
    ReadInteger(obj);  // Skip over redundant version field.

  state->url_string = ReadString(obj);

  if (obj->version < 19)
    ReadString(obj);  // Skip obsolete original url string field.

  state->target = ReadString(obj);
  if (obj->version < 25 && !state->target.is_null()) {
    state->target = base::NullableString16(
        base::UTF8ToUTF16(UniqueNameHelper::UpdateLegacyNameFromV24(
            base::UTF16ToUTF8(state->target.string()),
            unique_name_replacements)),
        false);
  }
  if (obj->version < 15) {
    ReadString(obj);  // Skip obsolete parent field.
    ReadString(obj);  // Skip obsolete title field.
    ReadString(obj);  // Skip obsolete alternate title field.
    ReadReal(obj);    // Skip obsolete visited time field.
  }

  if (obj->version >= 24) {
    state->did_save_scroll_or_scale_state = ReadBoolean(obj);
  } else {
    state->did_save_scroll_or_scale_state = true;
  }

  if (state->did_save_scroll_or_scale_state) {
    int x = ReadInteger(obj);
    int y = ReadInteger(obj);
    state->scroll_offset = gfx::Point(x, y);
  }

  if (obj->version < 15) {
    ReadBoolean(obj);  // Skip obsolete target item flag.
    ReadInteger(obj);  // Skip obsolete visit count field.
  }
  state->referrer = ReadString(obj);

  ReadStringVector(obj, &state->document_state);

  if (state->did_save_scroll_or_scale_state)
    state->page_scale_factor = ReadReal(obj);

  state->item_sequence_number = ReadInteger64(obj);
  state->document_sequence_number = ReadInteger64(obj);
  if (obj->version >= 21 && obj->version < 23)
    ReadInteger64(obj); // Skip obsolete frame sequence number.

  if (obj->version >= 17 && obj->version < 19)
    ReadInteger64(obj); // Skip obsolete target frame id number.

  if (obj->version >= 18) {
    state->referrer_policy =
        static_cast<blink::WebReferrerPolicy>(ReadInteger(obj));
  }

  if (obj->version >= 20 && state->did_save_scroll_or_scale_state) {
    double x = ReadReal(obj);
    double y = ReadReal(obj);
    state->visual_viewport_scroll_offset = gfx::PointF(x, y);
  } else {
    state->visual_viewport_scroll_offset = gfx::PointF(-1, -1);
  }

  if (obj->version >= 22) {
    state->scroll_restoration_type =
        static_cast<blink::WebHistoryScrollRestorationType>(ReadInteger(obj));
  }

  bool has_state_object = ReadBoolean(obj);
  if (has_state_object)
    state->state_object = ReadString(obj);

  LegacyReadHttpBody(obj, &state->http_body);

  // NOTE: It is a quirk of the format that we still have to read the
  // http_content_type field when the HTTP body is null.  That's why this code
  // is here instead of inside ReadHttpBody.
  state->http_body.http_content_type = ReadString(obj);

  if (obj->version < 14)
    ReadString(obj);  // Skip unused referrer string.

#if defined(OS_ANDROID)
  if (obj->version == 11) {
    // Now-unused values that shipped in this version of Chrome for Android when
    // it was on a private branch.
    ReadReal(obj);
    ReadBoolean(obj);

    // In this version, page_scale_factor included device_scale_factor and
    // scroll offsets were premultiplied by pageScaleFactor.
    if (state->page_scale_factor) {
      float device_scale_factor = g_device_scale_factor_for_testing;
      if (!device_scale_factor) {
        device_scale_factor = display::Screen::GetScreen()
                                  ->GetPrimaryDisplay()
                                  .device_scale_factor();
      }
      state->scroll_offset =
          gfx::Point(state->scroll_offset.x() / state->page_scale_factor,
                     state->scroll_offset.y() / state->page_scale_factor);
      state->page_scale_factor /= device_scale_factor;
    }
  }
#endif

  // Subitems
  size_t num_children =
      ReadAndValidateVectorSize(obj, sizeof(ExplodedFrameState));
  state->children.resize(num_children);
  for (size_t i = 0; i < num_children; ++i)
    LegacyReadFrameState(obj, false, unique_name_replacements,
                         &state->children[i]);
}

// Writes the ExplodedFrameState data into the SerializeObject object for
// serialization. This uses the custom, legacy format, and its implementation 
// should remain frozen in order to preserve this format. 
void LegacyWriteFrameState(const ExplodedFrameState& state,
                           SerializeObject* obj,
                           bool is_top) {
  // WARNING: This data may be persisted for later use. As such, care must be
  // taken when changing the serialized format. If a new field needs to be
  // written, only adding at the end will make it easier to deal with loading
  // older versions. Similarly, this should NOT save fields with sensitive
  // data, such as password fields.

  WriteString(state.url_string, obj);
  WriteString(state.target, obj);
  WriteBoolean(state.did_save_scroll_or_scale_state, obj);

  if (state.did_save_scroll_or_scale_state) {
    WriteInteger(state.scroll_offset.x(), obj);
    WriteInteger(state.scroll_offset.y(), obj);
  }

  WriteString(state.referrer, obj);

  WriteStringVector(state.document_state, obj);

  if (state.did_save_scroll_or_scale_state)
    WriteReal(state.page_scale_factor, obj);

  WriteInteger64(state.item_sequence_number, obj);
  WriteInteger64(state.document_sequence_number, obj);
  WriteInteger(static_cast<int>(state.referrer_policy), obj);

  if (state.did_save_scroll_or_scale_state) {
    WriteReal(state.visual_viewport_scroll_offset.x(), obj);
    WriteReal(state.visual_viewport_scroll_offset.y(), obj);
  }

  WriteInteger(state.scroll_restoration_type, obj);

  bool has_state_object = !state.state_object.is_null();
  WriteBoolean(has_state_object, obj);
  if (has_state_object)
    WriteString(state.state_object, obj);

  LegacyWriteHttpBody(state.http_body, obj);

  // NOTE: It is a quirk of the format that we still have to write the
  // http_content_type field when the HTTP body is null.  That's why this code
  // is here instead of inside WriteHttpBody.
  WriteString(state.http_body.http_content_type, obj);

  // Subitems
  const std::vector<ExplodedFrameState>& children = state.children;
  WriteAndValidateVectorSize(children, obj);
  for (size_t i = 0; i < children.size(); ++i)
    LegacyWriteFrameState(children[i], obj, false);
}

void LegacyWritePageState(const ExplodedPageState& state,
                          SerializeObject* obj) {
  WriteInteger(obj->version, obj);
  WriteStringVector(state.referenced_files, obj);
  LegacyWriteFrameState(state.top, obj, true);
}

void WriteResourceRequestBody(const ResourceRequestBody& request_body,
                              mojom::RequestBody* mojo_body) {
  for (const auto& element : *request_body.elements()) {
    mojom::ElementPtr data_element = mojom::Element::New();
    switch (element.type()) {
      case ResourceRequestBody::Element::TYPE_BYTES: {
        data_element->set_bytes(std::vector<unsigned char>(
            reinterpret_cast<const char*>(element.bytes()),
            element.bytes() + element.length()));
        break;
      }
      case ResourceRequestBody::Element::TYPE_FILE: {
        mojom::FilePtr file = mojom::File::New(
            element.path().AsUTF16Unsafe(), element.offset(), element.length(),
            element.expected_modification_time().ToDoubleT());
        data_element->set_file(std::move(file));
        break;
      }
      case ResourceRequestBody::Element::TYPE_FILE_FILESYSTEM: {
        mojom::FileSystemPtr file_system = mojom::FileSystem::New(
            element.path().AsUTF16Unsafe(), element.offset(), element.length(),
            element.expected_modification_time().ToDoubleT());
        data_element->set_file_system(std::move(file_system));
        break;
      }
      case ResourceRequestBody::Element::TYPE_BLOB:
        data_element->set_blob(element.blob_uuid());
        break;
      case ResourceRequestBody::Element::TYPE_BYTES_DESCRIPTION:
      case ResourceRequestBody::Element::TYPE_DISK_CACHE_ENTRY:
      default:
        NOTREACHED();
        continue;
    }
    mojo_body->elements.push_back(std::move(data_element));
  }
  mojo_body->identifier = request_body.identifier();
}

void ReadResourceRequestBody(
    mojom::RequestBody* mojo_body,
    const scoped_refptr<ResourceRequestBody>& request_body) {
  for (const auto& element : mojo_body->elements) {
    mojom::Element::Tag tag = element->which();
    switch (tag) {
      case mojom::Element::Tag::BYTES:
        AppendDataToRequestBody(
            request_body,
            reinterpret_cast<const char*>(element->get_bytes().data()),
            element->get_bytes().size());
        break;
      case mojom::Element::Tag::FILE: {
        mojom::File* file = element->get_file().get();
        AppendFileRangeToRequestBody(request_body, NS16(file->path),
                                     file->offset, file->length,
                                     file->modification_time);
        break;
      }
      case mojom::Element::Tag::FILE_SYSTEM: {
        mojom::FileSystem* file_system = element->get_file_system().get();
        AppendURLRangeToRequestBody(request_body,
                                    GURL(file_system->filesystem_url),
                                    file_system->offset, file_system->length,
                                    file_system->modification_time);
        break;
      }
      case mojom::Element::Tag::BLOB:
        AppendBlobToRequestBody(request_body, element->get_blob());
        break;
      default:
        NOTREACHED();
    }
  }
  request_body->set_identifier(mojo_body->identifier);
}

void WriteHttpBody(const ExplodedHttpBody& http_body,
                   mojom::HttpBody* mojo_body) {
  if (http_body.request_body != nullptr) {
    mojo_body->request_body = mojom::RequestBody::New();
    mojo_body->contains_passwords = http_body.contains_passwords;
    WriteResourceRequestBody(*http_body.request_body,
                             mojo_body->request_body.get());
  }
}

void ReadHttpBody(mojom::HttpBody* mojo_body, ExplodedHttpBody* http_body) {
  http_body->contains_passwords = mojo_body->contains_passwords;
  if (mojo_body->request_body) {
    http_body->request_body = new ResourceRequestBody();
    ReadResourceRequestBody(mojo_body->request_body.get(),
                            http_body->request_body);
  }
}

void WriteFrameState(const ExplodedFrameState& state,
                     mojom::FrameState* frame) {
  frame->url_string = state.url_string.string();
  frame->target = state.target.string();
  frame->did_save_scroll_or_scale_state = state.did_save_scroll_or_scale_state;

  frame->scroll_offset = mojom::Point::New();
  frame->visual_viewport_scroll_offset = mojom::PointF::New();
  if (state.did_save_scroll_or_scale_state) {
    frame->scroll_offset->x = state.scroll_offset.x();
    frame->scroll_offset->y = state.scroll_offset.y();

    frame->visual_viewport_scroll_offset->x =
        state.visual_viewport_scroll_offset.x();
    frame->visual_viewport_scroll_offset->y =
        state.visual_viewport_scroll_offset.y();
  }

  frame->scroll_restoration_type =
      static_cast<mojom::ScrollRestorationType>(state.scroll_restoration_type);

  frame->referrer = state.referrer.string();

  for (const auto& s : state.document_state) {
    frame->document_state.push_back(s.string());
  }

  if (state.did_save_scroll_or_scale_state)
    frame->page_scale_factor = state.page_scale_factor;

  frame->item_sequence_number = state.item_sequence_number;
  frame->document_sequence_number = state.document_sequence_number;
  frame->referrer_policy =
      static_cast<mojom::WebReferrerPolicy>(state.referrer_policy);

  bool has_state_object = !state.state_object.is_null();
  if (has_state_object)
    frame->state_object = state.state_object.string();

  frame->http_body = mojom::HttpBody::New();
  WriteHttpBody(state.http_body, frame->http_body.get());
  frame->http_body->http_content_type =
      state.http_body.http_content_type.string();

  // Subitems
  const std::vector<ExplodedFrameState>& children = state.children;
  for (const auto& child : children) {
    mojom::FrameStatePtr child_frame = mojom::FrameState::New();
    WriteFrameState(child, child_frame.get());
    frame->children.push_back(std::move(child_frame));
  }
}

void ReadFrameState(mojom::FrameState* frame, ExplodedFrameState* state) {
  state->url_string = NS16(frame->url_string);
  state->referrer = NS16(frame->referrer);
  state->target = NS16(frame->target);
  state->state_object = NS16(frame->state_object);

  for (auto s : frame->document_state) {
    state->document_state.push_back(base::NullableString16(s));
  }

  state->scroll_restoration_type =
      static_cast<blink::WebHistoryScrollRestorationType>(
          frame->scroll_restoration_type);
  state->did_save_scroll_or_scale_state = frame->did_save_scroll_or_scale_state;
  state->scroll_offset =
      gfx::Point(frame->scroll_offset->x, frame->scroll_offset->y);
  state->visual_viewport_scroll_offset =
      gfx::PointF(frame->visual_viewport_scroll_offset->x,
                  frame->visual_viewport_scroll_offset->y);

  state->item_sequence_number = frame->item_sequence_number;
  state->document_sequence_number = frame->document_sequence_number;

  state->page_scale_factor = frame->page_scale_factor;

  state->referrer_policy =
      static_cast<blink::WebReferrerPolicy>(frame->referrer_policy);
  if (frame->http_body) {
    ReadHttpBody(frame->http_body.get(), &state->http_body);
    state->http_body.http_content_type =
        NS16(frame->http_body->http_content_type);
  } else {
    state->http_body.request_body = nullptr;
  }

  state->children.resize(frame->children.size());
  int i = 0;
  for (const auto& child : frame->children)
    ReadFrameState(child.get(), &state->children[i++]);
}

void ReadMojoPageState(SerializeObject* obj, ExplodedPageState* state) {
  const void* tmp = nullptr;
  int length = 0;
  ReadData(obj, &tmp, &length);
  if (obj->parse_error || length <= 0)
    return;

  mojom::PageStatePtr page;
  obj->parse_error = !(mojom::PageState::Deserialize(tmp, length, &page));
  if (obj->parse_error)
    return;

  for (const auto& referenced_file : page->referenced_files) {
    state->referenced_files.push_back(NS16(referenced_file));
  }

  ReadFrameState(page->top.get(), &state->top);

  state->referenced_files.erase(
      std::unique(state->referenced_files.begin(),
                  state->referenced_files.end()),
      state->referenced_files.end());
}

void WritePageState(const ExplodedPageState& state, SerializeObject* obj) {
  WriteInteger(obj->version, obj);

  mojom::PageStatePtr page = mojom::PageState::New();
  for (const auto& referenced_file : state.referenced_files) {
    page->referenced_files.push_back(referenced_file.string());
  }

  page->top = mojom::FrameState::New();
  WriteFrameState(state.top, page->top.get());

  std::vector<uint8_t> page_bytes = mojom::PageState::Serialize(&page);
  obj->pickle.WriteData(reinterpret_cast<char*>(page_bytes.data()),
                        page_bytes.size());
}

void ReadPageState(SerializeObject* obj, ExplodedPageState* state) {
  obj->version = ReadInteger(obj);

  if (obj->version == -1) {
    GURL url = ReadGURL(obj);
    // NOTE: GURL::possibly_invalid_spec() always returns valid UTF-8.
    state->top.url_string =
        base::NullableString16(
            base::UTF8ToUTF16(url.possibly_invalid_spec()), false);
    return;
  }

  if (obj->version > kCurrentVersion || obj->version < kMinVersion) {
    obj->parse_error = true;
    return;
  }

  if (obj->version >= 26) {
    ReadMojoPageState(obj, state);
    return;
  }

  if (obj->version >= 14)
    ReadStringVector(obj, &state->referenced_files);

  std::vector<UniqueNameHelper::Replacement> unique_name_replacements;
  LegacyReadFrameState(obj, true, &unique_name_replacements, &state->top);

  if (obj->version < 14)
    RecursivelyAppendReferencedFiles(state->top, &state->referenced_files);

  // De-dupe
  state->referenced_files.erase(
      std::unique(state->referenced_files.begin(),
                  state->referenced_files.end()),
      state->referenced_files.end());
}

}  // namespace

ExplodedHttpBody::ExplodedHttpBody() : contains_passwords(false) {}

ExplodedHttpBody::~ExplodedHttpBody() {
}

ExplodedFrameState::ExplodedFrameState()
    : scroll_restoration_type(blink::kWebHistoryScrollRestorationAuto),
      did_save_scroll_or_scale_state(true),
      item_sequence_number(0),
      document_sequence_number(0),
      page_scale_factor(0.0),
      referrer_policy(blink::kWebReferrerPolicyDefault) {}

ExplodedFrameState::ExplodedFrameState(const ExplodedFrameState& other) {
  assign(other);
}

ExplodedFrameState::~ExplodedFrameState() {
}

void ExplodedFrameState::operator=(const ExplodedFrameState& other) {
  if (&other != this)
    assign(other);
}

void ExplodedFrameState::assign(const ExplodedFrameState& other) {
  url_string = other.url_string;
  referrer = other.referrer;
  target = other.target;
  state_object = other.state_object;
  document_state = other.document_state;
  scroll_restoration_type = other.scroll_restoration_type;
  did_save_scroll_or_scale_state = other.did_save_scroll_or_scale_state;
  visual_viewport_scroll_offset = other.visual_viewport_scroll_offset;
  scroll_offset = other.scroll_offset;
  item_sequence_number = other.item_sequence_number;
  document_sequence_number = other.document_sequence_number;
  page_scale_factor = other.page_scale_factor;
  referrer_policy = other.referrer_policy;
  http_body = other.http_body;
  children = other.children;
}

ExplodedPageState::ExplodedPageState() {
}

ExplodedPageState::~ExplodedPageState() {
}

int DecodePageStateInternal(const std::string& encoded,
                            ExplodedPageState* exploded) {
  *exploded = ExplodedPageState();

  if (encoded.empty())
    return true;

  SerializeObject obj(encoded.data(), static_cast<int>(encoded.size()));
  ReadPageState(&obj, exploded);
  return obj.parse_error ? -1 : obj.version;
}

bool DecodePageState(const std::string& encoded, ExplodedPageState* exploded) {
  return DecodePageStateInternal(encoded, exploded) != -1;
}

int DecodePageStateForTesting(const std::string& encoded,
                              ExplodedPageState* exploded) {
  return DecodePageStateInternal(encoded, exploded);
}

void EncodePageState(const ExplodedPageState& exploded, std::string* encoded) {
  SerializeObject obj;
  obj.version = kCurrentVersion;
  WritePageState(exploded, &obj);
  *encoded = obj.GetAsString();
}

void LegacyEncodePageStateForTesting(const ExplodedPageState& exploded,
                                     int version,
                                     std::string* encoded) {
  SerializeObject obj;
  obj.version = version;
  LegacyWritePageState(exploded, &obj);
  *encoded = obj.GetAsString();
}

#if defined(OS_ANDROID)
bool DecodePageStateWithDeviceScaleFactorForTesting(
    const std::string& encoded,
    float device_scale_factor,
    ExplodedPageState* exploded) {
  g_device_scale_factor_for_testing = device_scale_factor;
  bool rv = DecodePageState(encoded, exploded);
  g_device_scale_factor_for_testing = 0.0;
  return rv;
}

scoped_refptr<ResourceRequestBody> DecodeResourceRequestBody(const char* data,
                                                             size_t size) {
  scoped_refptr<ResourceRequestBody> result = new ResourceRequestBody();
  SerializeObject obj(data, static_cast<int>(size));
  LegacyReadResourceRequestBody(&obj, result);
  // Please see the EncodeResourceRequestBody() function below for information
  // about why the contains_sensitive_info() field is being explicitly
  // deserialized.
  result->set_contains_sensitive_info(ReadBoolean(&obj));
  return obj.parse_error ? nullptr : result;
}

std::string EncodeResourceRequestBody(
    const ResourceRequestBody& resource_request_body) {
  SerializeObject obj;
  obj.version = 25;
  LegacyWriteResourceRequestBody(resource_request_body, &obj);
  // EncodeResourceRequestBody() is different from WriteResourceRequestBody()
  // because it covers additional data (e.g.|contains_sensitive_info|) which
  // is marshaled between native code and java. WriteResourceRequestBody()
  // serializes data which needs to be saved out to disk.
  WriteBoolean(resource_request_body.contains_sensitive_info(), &obj);
  return obj.GetAsString();
}

#endif

}  // namespace content
