// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_JPEG_ENCODE_SOFTWARE_H_
#define MEDIA_GPU_JPEG_ENCODE_SOFTWARE_H_

// We must include cstdio before jpeglib.h. It is a requirement of libjpeg.
#include <cstdio>

extern "C" {
#include <jerror.h>
#include <jpeglib.h>
}

#include <vector>

namespace media {

/* Encapsulates a converter from YU12 to JPEG format. This class is not thread-safe. */
class JpegEncodeSoftware {
public:
    JpegEncodeSoftware();
    ~JpegEncodeSoftware();

    /*
     * Compresses YU12 image to JPEG format. After calling this method, call getCompressedImage()
     * to get the image. |quality| is the resulted jpeg image quality. It ranges from 1
     * (poorest quality) to 100 (highest quality). |app1Buffer| is the buffer of APP1 segment (exif)
     * which will be added to the compressed image.
     * Returns false if errors occur during compression.
     */
    bool compressImage(const void* image, int width, int height, int quality,
                       const void* app1Buffer, unsigned int app1Size);

    /*
     * Returns the compressed JPEG buffer pointer. This method must be called only after calling
     * compressImage().
     */
    const void* getCompressedImagePtr();

    /*
     * Returns the compressed JPEG buffer size. This method must be called only after calling
     * compressImage().
     */
    size_t getCompressedImageSize();

private:
    // initDestination(), emptyOutputBuffer() and emptyOutputBuffer() are callback functions to be
    // passed into jpeg library.
    static void initDestination(j_compress_ptr cinfo);
    static boolean emptyOutputBuffer(j_compress_ptr cinfo);
    static void terminateDestination(j_compress_ptr cinfo);
    static void outputErrorMessage(j_common_ptr cinfo);

    // Returns false if errors occur.
    bool encode(const void* inYuv, int width, int height, int jpegQuality,
                const void* app1Buffer, unsigned int app1Size);
    void setJpegDestination(jpeg_compress_struct* cinfo);
    void setJpegCompressStruct(int width, int height, int quality, jpeg_compress_struct* cinfo);
    // Returns false if errors occur.
    bool compress(jpeg_compress_struct* cinfo, const uint8_t* yuv);

    // The block size for encoded jpeg image buffer.
    static const int kBlockSize = 16384;
    // Process 16 lines of Y and 16 lines of U/V each time.
    // We must pass at least 16 scanlines according to libjpeg documentation.
    static const int kCompressBatchSize = 16;

    // The buffer that holds the compressed result.
    std::vector<JOCTET> mResultBuffer;
};

} // namespace media

#endif // MEDIA_GPU_JPEG_ENCODE_SOFTWARE_H_
