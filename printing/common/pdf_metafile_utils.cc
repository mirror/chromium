// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/common/pdf_metafile_utils.h"

#include "base/time/time.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkPictureRecorder.h"
#include "third_party/skia/include/core/SkTime.h"

namespace {

SkTime::DateTime TimeToSkTime(base::Time time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);
  SkTime::DateTime skdate;
  skdate.fTimeZoneMinutes = 0;
  skdate.fYear = exploded.year;
  skdate.fMonth = exploded.month;
  skdate.fDayOfWeek = exploded.day_of_week;
  skdate.fDay = exploded.day_of_month;
  skdate.fHour = exploded.hour;
  skdate.fMinute = exploded.minute;
  skdate.fSecond = exploded.second;
  return skdate;
}

sk_sp<SkData> SerializeOopPicture(SkPicture* pic, void* ctx) {
  printing::SerializationContext* context =
      reinterpret_cast<printing::SerializationContext*>(ctx);
  uint32_t pic_id = pic->uniqueID();
  for (auto id : context->pic_ids) {
    if (id == pic_id) {
      uint64_t unique_id = OopPicUniqueId(context->process_id, id);
      return SkData::MakeWithCopy(&unique_id, sizeof(unique_id));
    }
  }
  return nullptr;
}

sk_sp<SkPicture> GetEmptyPicture() {
  SkPictureRecorder rec;
  SkCanvas* c = rec.beginRecording(100, 100);
  c->save();
  c->restore();
  return rec.finishRecordingAsPicture();
}

sk_sp<SkPicture> DeserializeOopPicture(const void* data,
                                       size_t length,
                                       void* ctx) {
  DCHECK(ctx);
  auto* pic_map = reinterpret_cast<printing::DeserializationContext*>(ctx);
  uint64_t id;
  sk_sp<SkPicture> empty_pic = GetEmptyPicture();
  if (length < sizeof(id)) {
    NOTREACHED();  // Should not happen if the content is as written.
    return empty_pic;
  }
  memcpy(&id, data, sizeof(id));
  auto iter = pic_map->find(id);
  if (iter == pic_map->end()) {
    // When we don't have the out-of-process picture available, we return
    // an empty picture. Returning a nullptr will cause the deserialization
    // crash.
    return empty_pic;
  }
  return iter->second;
}

}  // namespace

namespace printing {

sk_sp<SkDocument> MakePdfDocument(const std::string& creator,
                                  SkWStream* stream) {
  SkDocument::PDFMetadata metadata;
  SkTime::DateTime now = TimeToSkTime(base::Time::Now());
  metadata.fCreation.fEnabled = true;
  metadata.fCreation.fDateTime = now;
  metadata.fModified.fEnabled = true;
  metadata.fModified.fDateTime = now;
  metadata.fCreator = creator.empty()
                          ? SkString("Chromium")
                          : SkString(creator.c_str(), creator.size());
  return SkDocument::MakePDF(stream, metadata);
}

SkSerialProcs SerializationProcs(SerializationContext* ctx) {
  SkSerialProcs procs;
  procs.fPictureProc = SerializeOopPicture;
  procs.fPictureCtx = reinterpret_cast<void*>(ctx);
  return procs;
}

SkDeserialProcs DeserializationProcs(DeserializationContext* ctx) {
  SkDeserialProcs procs;
  procs.fPictureProc = DeserializeOopPicture;
  procs.fPictureCtx = reinterpret_cast<void*>(ctx);
  return procs;
}
}  // namespace printing
