// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/SubresourceIntegrity.h"

#include "platform/Crypto.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/loader/fetch/Resource.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/ASCIICType.h"
#include "platform/wtf/Vector.h"
#include "platform/wtf/dtoa/utils.h"
#include "platform/wtf/text/Base64.h"
#include "platform/wtf/text/ParsingUtilities.h"
#include "platform/wtf/text/StringUTF8Adaptor.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebCrypto.h"
#include "public/platform/WebCryptoAlgorithm.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace blink {

// FIXME: This should probably use common functions with ContentSecurityPolicy.
static bool IsIntegrityCharacter(UChar c) {
  // Check if it's a base64 encoded value. We're pretty loose here, as there's
  // not much risk in it, and it'll make it simpler for developers.
  return IsASCIIAlphanumeric(c) || c == '_' || c == '-' || c == '+' ||
         c == '/' || c == '=';
}

static bool IsValueCharacter(UChar c) {
  // VCHAR per https://tools.ietf.org/html/rfc5234#appendix-B.1
  return c >= 0x21 && c <= 0x7e;
}

static bool DigestsEqual(const DigestValue& digest1,
                         const DigestValue& digest2) {
  if (digest1.size() != digest2.size())
    return false;

  for (size_t i = 0; i < digest1.size(); i++) {
    if (digest1[i] != digest2[i])
      return false;
  }

  return true;
}

static String DigestToString(const DigestValue& digest) {
  return Base64Encode(reinterpret_cast<const char*>(digest.data()),
                      digest.size(), kBase64DoNotInsertLFs);
}

void SubresourceIntegrity::ReportInfo::AddUseCount(UseCounterFeature feature) {
  use_counts_.push_back(feature);
}

void SubresourceIntegrity::ReportInfo::AddConsoleErrorMessage(
    const String& message) {
  console_error_messages_.push_back(message);
}

void SubresourceIntegrity::ReportInfo::Clear() {
  use_counts_.clear();
  console_error_messages_.clear();
}

bool SubresourceIntegrity::CheckSubresourceIntegrity(
    const String& integrity_attribute,
    const char* content,
    size_t size,
    const KURL& resource_url,
    const Resource& resource,
    ReportInfo& report_info) {
  if (integrity_attribute.IsEmpty())
    return true;

  IntegrityMetadataSet metadata_set;
  IntegrityParseResult integrity_parse_result =
      ParseIntegrityAttribute(integrity_attribute, metadata_set, &report_info);
  if (integrity_parse_result != kIntegrityParseValidResult)
    return true;

  return CheckSubresourceIntegrity(metadata_set, content, size, resource_url,
                                   resource, report_info);
}

bool SubresourceIntegrity::CheckSubresourceIntegrity(
    const IntegrityMetadataSet& metadata_set,
    const char* content,
    size_t size,
    const KURL& resource_url,
    const Resource& resource,
    ReportInfo& report_info) {
  if (!resource.IsSameOriginOrCORSSuccessful()) {
    report_info.AddConsoleErrorMessage(
        "Subresource Integrity: The resource '" + resource_url.ElidedString() +
        "' has an integrity attribute, but the resource "
        "requires the request to be CORS enabled to check "
        "the integrity, and it is not. The resource has been "
        "blocked because the integrity cannot be enforced.");
    report_info.AddUseCount(ReportInfo::UseCounterFeature::
                                kSRIElementIntegrityAttributeButIneligible);
    return false;
  }

  return CheckSubresourceIntegrity(
      metadata_set, content, size, resource_url,
      resource.GetResponse().HttpHeaderField("Integrity"), report_info);
}

bool SubresourceIntegrity::CheckSubresourceIntegrity(
    const String& integrity_metadata,
    const char* content,
    size_t size,
    const KURL& resource_url,
    const String integrity_header,
    ReportInfo& report_info) {
  IntegrityMetadataSet metadata_set;
  IntegrityParseResult integrity_parse_result =
      ParseIntegrityAttribute(integrity_metadata, metadata_set, &report_info);
  if (integrity_parse_result != kIntegrityParseValidResult)
    return true;

  return CheckSubresourceIntegrity(metadata_set, content, size, resource_url,
                                   integrity_header, report_info);
}

bool SubresourceIntegrity::CheckSubresourceIntegrity(
    const String& integrity_metadata,
    const char* content,
    size_t size,
    const KURL& resource_url,
    ReportInfo& report_info) {
  return CheckSubresourceIntegrity(integrity_metadata, content, size,
                                   resource_url, String() /* !!!! */,
                                   report_info);
}

bool SubresourceIntegrity::CheckSubresourceIntegrity(
    const IntegrityMetadataSet& metadata_set,
    const char* content,
    size_t size,
    const KURL& resource_url,
    const String integrity_header,
    ReportInfo& report_info) {
  if (!metadata_set.size())
    return true;

  // Find the "strongest" algorithm in the set. (This relies on
  // IntegrityAlgorithm declaration order matching the "strongest" order, so
  // make the compiler check this assumption first.)
  static_assert(IntegrityAlgorithm::kSha256 < IntegrityAlgorithm::kSha384 &&
                    IntegrityAlgorithm::kSha384 < IntegrityAlgorithm::kSha512 &&
                    IntegrityAlgorithm::kSha512 < IntegrityAlgorithm::kEd25519,
                "IntegrityAlgorithm enum order should match the priority "
                "of the integrity algorithms.");

  IntegrityAlgorithm strongest_algorithm = IntegrityAlgorithm::kSha256;
  for (const IntegrityMetadata& metadata : metadata_set) {
    strongest_algorithm = std::max(metadata.Algorithm(), strongest_algorithm);
  }

  // Determine which function should be used for checking this type of integrity
  // constraint.
  bool (*check_fn)(const IntegrityMetadata&, const char*, size_t,
                   const String&) = nullptr;
  switch (strongest_algorithm) {
    case IntegrityAlgorithm::kSha256:
    case IntegrityAlgorithm::kSha384:
    case IntegrityAlgorithm::kSha512:
      check_fn = SubresourceIntegrity::CheckSubresourceIntegrityDigest;
      break;
    case IntegrityAlgorithm::kEd25519:
      check_fn = SubresourceIntegrity::CheckSubresourceIntegritySignature;
      break;
  }

  // Check any of the "strongest" integrity constraints.
  for (const IntegrityMetadata& metadata : metadata_set) {
    if (metadata.Algorithm() == strongest_algorithm &&
        (*check_fn)(metadata, content, size, integrity_header)) {
      report_info.AddUseCount(ReportInfo::UseCounterFeature::
                                  kSRIElementWithMatchingIntegrityAttribute);
      return true;
    }
  }

  // If we arrive here, none of the "strongest" constaints have validated
  // the data we received. Report this fact.
  DigestValue digest;
  if (ComputeDigest(kHashAlgorithmSha256, content, size, digest)) {
    // This message exposes the digest of the resource to the console.
    // Because this is only to the console, that's okay for now, but we
    // need to be very careful not to expose this in exceptions or
    // JavaScript, otherwise it risks exposing information about the
    // resource cross-origin.
    report_info.AddConsoleErrorMessage(
        "Failed to find a valid digest in the 'integrity' attribute for "
        "resource '" +
        resource_url.ElidedString() + "' with computed SHA-256 integrity '" +
        DigestToString(digest) + "'. The resource has been blocked.");
  } else {
    report_info.AddConsoleErrorMessage(
        "There was an error computing an integrity value for resource '" +
        resource_url.ElidedString() + "'. The resource has been blocked.");
  }
  report_info.AddUseCount(ReportInfo::UseCounterFeature::
                              kSRIElementWithNonMatchingIntegrityAttribute);
  return false;
}

bool SubresourceIntegrity::CheckSubresourceIntegrityDigest(
    const IntegrityMetadata& metadata,
    const char* content,
    size_t size,
    const String& integrity_header) {
  blink::HashAlgorithm hash_algo = kHashAlgorithmSha256;
  switch (metadata.Algorithm()) {
    case IntegrityAlgorithm::kSha256:
      hash_algo = kHashAlgorithmSha256;
      break;
    case IntegrityAlgorithm::kSha384:
      hash_algo = kHashAlgorithmSha384;
      break;
    case IntegrityAlgorithm::kSha512:
      hash_algo = kHashAlgorithmSha512;
      break;
    case IntegrityAlgorithm::kEd25519:
      NOTREACHED();
      break;
  }

  DigestValue digest;
  if (!ComputeDigest(hash_algo, content, size, digest))
    return false;

  Vector<char> hash_vector;
  Base64Decode(metadata.Digest(), hash_vector);
  DigestValue converted_hash_vector;
  converted_hash_vector.Append(reinterpret_cast<uint8_t*>(hash_vector.data()),
                               hash_vector.size());
  return DigestsEqual(digest, converted_hash_vector);
}

bool SubresourceIntegrity::CheckSubresourceIntegritySignature(
    const IntegrityMetadata& metadata,
    const char* content,
    size_t size,
    const String& integrity_header) {
  DCHECK_EQ(IntegrityAlgorithm::kEd25519, metadata.Algorithm());

  Vector<char> pubkey;
  Base64Decode(metadata.Digest(), pubkey);
  if (pubkey.size() != 32)
    return false;

  // Parse the Integrity:-header containing the signature(s).
  Vector<UChar> integrity_header_chars;
  integrity_header.AppendTo(integrity_header_chars);
  const UChar* position = integrity_header_chars.begin();
  const UChar* const end_position = integrity_header_chars.end();
  while (position < end_position) {
    SkipWhile<UChar, IsASCIISpace>(position, end_position);

    IntegrityAlgorithm algorithm;
    if (kAlgorithmValid !=
            ParseIntegrityHeaderAlgorithm(position, end_position, algorithm) ||
        algorithm != IntegrityAlgorithm::kEd25519) {
      // If we found no valid algorithm, or one not supported in this context,
      // we'll skip until the next token and continue with the loop.
      SkipUntil<UChar, IsASCIISpace>(position, end_position);
      continue;
    }

    String signature_raw;
    if (!ParseDigest(position, end_position, signature_raw))
      continue;

    Vector<char> signature;
    Base64Decode(signature_raw, signature);
    if (signature.size() != 64)
      continue;

    // BoringSSL/OpenSSL functions return 1 for success.
    if (1 ==
        ED25519_verify(reinterpret_cast<const uint8_t*>(content), size,
                       reinterpret_cast<const uint8_t*>(&*signature.begin()),
                       reinterpret_cast<const uint8_t*>(&*pubkey.begin()))) {
      return true;
    }
  }
  return false;
}

SubresourceIntegrity::AlgorithmParseResult
SubresourceIntegrity::ParseAttributeAlgorithm(const UChar*& begin,
                                              const UChar* end,
                                              IntegrityAlgorithm& algorithm) {
  static const char* kPrefixes[] = {"sha256", "sha-256", "sha384", "sha-384",
                                    "sha512", "sha-512", "ed25519"};
  static const IntegrityAlgorithm kAlgorithms[] = {
      IntegrityAlgorithm::kSha256, IntegrityAlgorithm::kSha256,
      IntegrityAlgorithm::kSha384, IntegrityAlgorithm::kSha384,
      IntegrityAlgorithm::kSha512, IntegrityAlgorithm::kSha512,
      IntegrityAlgorithm::kEd25519};
  static_assert(WTF_ARRAY_LENGTH(kPrefixes) == WTF_ARRAY_LENGTH(kAlgorithms),
                "Each prefix should a corresponding algorithms entry.");

  const char** last_prefix = kPrefixes + WTF_ARRAY_LENGTH(kPrefixes);
  if (!RuntimeEnabledFeatures::SignatureBasedIntegrityEnabled())
    last_prefix--;

  size_t prefix = 0;
  AlgorithmParseResult result =
      ParseAlgorithmPrefix(begin, end, kPrefixes, last_prefix, prefix);
  if (result == kAlgorithmValid)
    algorithm = kAlgorithms[prefix];
  return result;
}

SubresourceIntegrity::AlgorithmParseResult
SubresourceIntegrity::ParseIntegrityHeaderAlgorithm(
    const UChar*& begin,
    const UChar* end,
    IntegrityAlgorithm& algorithm) {
  static const char* kPrefixes[] = {"ed25519"};
  static const IntegrityAlgorithm kAlgorithms[] = {
      IntegrityAlgorithm::kEd25519};
  static_assert(WTF_ARRAY_LENGTH(kPrefixes) == WTF_ARRAY_LENGTH(kAlgorithms),
                "Each prefix should a corresponding algorithms entry.");

  size_t prefix = 0;
  AlgorithmParseResult result = ParseAlgorithmPrefix(
      begin, end, kPrefixes, kPrefixes + WTF_ARRAY_LENGTH(kPrefixes), prefix);
  if (result == kAlgorithmValid)
    algorithm = kAlgorithms[prefix];
  return result;
}

SubresourceIntegrity::AlgorithmParseResult
SubresourceIntegrity::ParseAlgorithmPrefix(const UChar*& string_position,
                                           const UChar* string_end,
                                           const char** prefix_begin,
                                           const char** prefix_end,
                                           size_t& prefix_position) {
  for (const char** iter = prefix_begin; iter != prefix_end; ++iter) {
    const UChar* pos = string_position;
    if (SkipToken<UChar>(pos, string_end, *iter) &&
        SkipExactly<UChar>(pos, string_end, '-')) {
      string_position = pos;
      prefix_position = iter - prefix_begin;
      return kAlgorithmValid;
    }
  }

  const UChar* dash_position = string_position;
  SkipUntil<UChar>(dash_position, string_end, '-');
  return dash_position < string_end ? kAlgorithmUnknown : kAlgorithmUnparsable;
}

// Before:
//
// [algorithm]-[hash]      OR     [algorithm]-[hash]?[options]
//             ^     ^                        ^               ^
//      position   end                 position             end
//
// After (if successful: if the method returns false, we make no promises and
// the caller should exit early):
//
// [algorithm]-[hash]      OR     [algorithm]-[hash]?[options]
//                   ^                              ^         ^
//        position/end                       position       end
bool SubresourceIntegrity::ParseDigest(const UChar*& position,
                                       const UChar* end,
                                       String& digest) {
  const UChar* begin = position;
  SkipWhile<UChar, IsIntegrityCharacter>(position, end);

  if (position == begin || (position != end && *position != '?')) {
    digest = g_empty_string;
    return false;
  }

  // We accept base64url encoding, but normalize to "normal" base64 internally:
  digest = NormalizeToBase64(String(begin, position - begin));
  return true;
}

SubresourceIntegrity::IntegrityParseResult
SubresourceIntegrity::ParseIntegrityAttribute(
    const WTF::String& attribute,
    IntegrityMetadataSet& metadata_set) {
  return ParseIntegrityAttribute(attribute, metadata_set, nullptr);
}

SubresourceIntegrity::IntegrityParseResult
SubresourceIntegrity::ParseIntegrityAttribute(
    const WTF::String& attribute,
    IntegrityMetadataSet& metadata_set,
    ReportInfo* report_info) {
  Vector<UChar> characters;
  attribute.StripWhiteSpace().AppendTo(characters);
  const UChar* position = characters.data();
  const UChar* end = characters.end();
  const UChar* current_integrity_end;

  metadata_set.clear();
  bool error = false;

  // The integrity attribute takes the form:
  //    *WSP hash-with-options *( 1*WSP hash-with-options ) *WSP / *WSP
  // To parse this, break on whitespace, parsing each algorithm/digest/option
  // in order.
  while (position < end) {
    WTF::String digest;
    IntegrityAlgorithm algorithm;

    SkipWhile<UChar, IsASCIISpace>(position, end);
    current_integrity_end = position;
    SkipUntil<UChar, IsASCIISpace>(current_integrity_end, end);

    // Algorithm parsing errors are non-fatal (the subresource should
    // still be loaded) because strong hash algorithms should be used
    // without fear of breaking older user agents that don't support
    // them.
    AlgorithmParseResult parse_result =
        ParseAttributeAlgorithm(position, current_integrity_end, algorithm);
    if (parse_result == kAlgorithmUnknown) {
      // Unknown hash algorithms are treated as if they're not present,
      // and thus are not marked as an error, they're just skipped.
      SkipUntil<UChar, IsASCIISpace>(position, end);
      if (report_info) {
        report_info->AddConsoleErrorMessage(
            "Error parsing 'integrity' attribute ('" + attribute +
            "'). The specified hash algorithm must be one of "
            "'sha256', 'sha384', or 'sha512'.");
        report_info->AddUseCount(
            ReportInfo::UseCounterFeature::
                kSRIElementWithUnparsableIntegrityAttribute);
      }
      continue;
    }

    if (parse_result == kAlgorithmUnparsable) {
      error = true;
      SkipUntil<UChar, IsASCIISpace>(position, end);
      if (report_info) {
        report_info->AddConsoleErrorMessage(
            "Error parsing 'integrity' attribute ('" + attribute +
            "'). The hash algorithm must be one of 'sha256', "
            "'sha384', or 'sha512', followed by a '-' "
            "character.");
        report_info->AddUseCount(
            ReportInfo::UseCounterFeature::
                kSRIElementWithUnparsableIntegrityAttribute);
      }
      continue;
    }

    DCHECK_EQ(parse_result, kAlgorithmValid);

    if (!ParseDigest(position, current_integrity_end, digest)) {
      error = true;
      SkipUntil<UChar, IsASCIISpace>(position, end);
      if (report_info) {
        report_info->AddConsoleErrorMessage(
            "Error parsing 'integrity' attribute ('" + attribute +
            "'). The digest must be a valid, base64-encoded value.");
        report_info->AddUseCount(
            ReportInfo::UseCounterFeature::
                kSRIElementWithUnparsableIntegrityAttribute);
      }
      continue;
    }

    // The spec defines a space in the syntax for options, separated by a
    // '?' character followed by unbounded VCHARs, but no actual options
    // have been defined yet. Thus, for forward compatibility, ignore any
    // options specified.
    if (SkipExactly<UChar>(position, end, '?')) {
      const UChar* begin = position;
      SkipWhile<UChar, IsValueCharacter>(position, end);
      if (begin != position && report_info) {
        report_info->AddConsoleErrorMessage(
            "Ignoring unrecogized 'integrity' attribute option '" +
            String(begin, position - begin) + "'.");
      }
    }

    IntegrityMetadata integrity_metadata(digest, algorithm);
    metadata_set.insert(integrity_metadata.ToPair());
  }

  if (metadata_set.size() == 0 && error)
    return kIntegrityParseNoValidResult;

  return kIntegrityParseValidResult;
}

}  // namespace blink
