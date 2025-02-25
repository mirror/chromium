// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SubresourceIntegrity_h
#define SubresourceIntegrity_h

#include "base/gtest_prod_util.h"
#include "platform/PlatformExport.h"
#include "platform/loader/fetch/IntegrityMetadata.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class KURL;
class Resource;

class PLATFORM_EXPORT SubresourceIntegrity final {
  STATIC_ONLY(SubresourceIntegrity);

 public:
  class ReportInfo final {
   public:
    enum class UseCounterFeature {
      kSRIElementWithMatchingIntegrityAttribute,
      kSRIElementWithNonMatchingIntegrityAttribute,
      kSRIElementIntegrityAttributeButIneligible,
      kSRIElementWithUnparsableIntegrityAttribute,
      kSRISignatureCheck,
      kSRISignatureSuccess,
    };

    void AddUseCount(UseCounterFeature);
    void AddConsoleErrorMessage(const String&);
    void Clear();

    const Vector<UseCounterFeature>& UseCounts() const { return use_counts_; }
    const Vector<String>& ConsoleErrorMessages() const {
      return console_error_messages_;
    }

   private:
    Vector<UseCounterFeature> use_counts_;
    Vector<String> console_error_messages_;
  };

  enum IntegrityParseResult {
    kIntegrityParseValidResult,
    kIntegrityParseNoValidResult
  };

  // The versions with the IntegrityMetadataSet passed as the first argument
  // assume that the integrity attribute has already been parsed, and the
  // IntegrityMetadataSet represents the result of that parsing.
  static bool CheckSubresourceIntegrity(const String& integrity_attribute,
                                        const char* content,
                                        size_t content_size,
                                        const KURL& resource_url,
                                        const Resource&,
                                        ReportInfo&);
  static bool CheckSubresourceIntegrity(const IntegrityMetadataSet&,
                                        const char* content,
                                        size_t content_size,
                                        const KURL& resource_url,
                                        const Resource&,
                                        ReportInfo&);
  static bool CheckSubresourceIntegrity(const String&,
                                        const char* content,
                                        size_t content_size,
                                        const KURL& resource_url,
                                        ReportInfo&);

  // set of all valid, parsed metadata from |attribute|.
  static IntegrityParseResult ParseIntegrityAttribute(
      const WTF::String& attribute,
      IntegrityMetadataSet&);
  static IntegrityParseResult ParseIntegrityAttribute(
      const WTF::String& attribute,
      IntegrityMetadataSet&,
      ReportInfo*);

 private:
  friend class SubresourceIntegrityTest;
  FRIEND_TEST_ALL_PREFIXES(SubresourceIntegrityTest, Parsing);
  FRIEND_TEST_ALL_PREFIXES(SubresourceIntegrityTest, ParseAlgorithm);
  FRIEND_TEST_ALL_PREFIXES(SubresourceIntegrityTest, ParseHeader);
  FRIEND_TEST_ALL_PREFIXES(SubresourceIntegrityTest, Prioritization);
  FRIEND_TEST_ALL_PREFIXES(SubresourceIntegrityTest, FindBestAlgorithm);
  FRIEND_TEST_ALL_PREFIXES(SubresourceIntegrityTest,
                           GetCheckFunctionForAlgorithm);

  // The core implementation for all CheckSubresoureIntegrity functions.
  static bool CheckSubresourceIntegrityImpl(const IntegrityMetadataSet&,
                                            const char*,
                                            size_t,
                                            const KURL& resource_url,
                                            const String integrity_header,
                                            ReportInfo&);

  enum AlgorithmParseResult {
    kAlgorithmValid,
    kAlgorithmUnparsable,
    kAlgorithmUnknown
  };

  static IntegrityAlgorithm FindBestAlgorithm(const IntegrityMetadataSet&);

  typedef bool (*CheckFunction)(const IntegrityMetadata&,
                                const char*,
                                size_t,
                                const String&);
  static CheckFunction GetCheckFunctionForAlgorithm(IntegrityAlgorithm);

  static bool CheckSubresourceIntegrityDigest(const IntegrityMetadata&,
                                              const char*,
                                              size_t,
                                              const String& integrity_header);
  static bool CheckSubresourceIntegritySignature(
      const IntegrityMetadata&,
      const char*,
      size_t,
      const String& integrity_header);

  static AlgorithmParseResult ParseAttributeAlgorithm(const UChar*& begin,
                                                      const UChar* end,
                                                      IntegrityAlgorithm&);
  static AlgorithmParseResult ParseIntegrityHeaderAlgorithm(
      const UChar*& begin,
      const UChar* end,
      IntegrityAlgorithm&);
  typedef std::pair<const char*, IntegrityAlgorithm> AlgorithmPrefixPair;
  static AlgorithmParseResult ParseAlgorithmPrefix(
      const UChar*& string_position,
      const UChar* string_end,
      const AlgorithmPrefixPair* prefix_table,
      size_t prefix_table_size,
      IntegrityAlgorithm&);
  static bool ParseDigest(const UChar*& begin,
                          const UChar* end,
                          String& digest);
};

}  // namespace blink

#endif
