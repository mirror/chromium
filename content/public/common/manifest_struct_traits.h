// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_MANIFEST_STRUCT_TRAITS_H_
#define CONTENT_PUBLIC_COMMON_MANIFEST_STRUCT_TRAITS_H_

#include "content/public/common/manifest.h"

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest.mojom-shared.h"

namespace mojo {

template <>
struct StructTraits<blink::mojom::ManifestDataView, content::Manifest> {
  static const base::Optional<base::string16>& name(
      const content::Manifest& m) {
    DCHECK(!m.name.is_null() ||
           m.name.string().size() <= content::Manifest::kMaxIPCStringLength);
    return m.name.as_optional_string16();
  }

  static const base::Optional<base::string16>& short_name(
      const content::Manifest& m) {
    DCHECK(m.short_name.is_null() ||
           m.short_name.string().size() <
               content::Manifest::kMaxIPCStringLength);
    return m.short_name.as_optional_string16();
  }

  static const base::Optional<base::string16>& gcm_sender_id(
      const content::Manifest& m) {
    DCHECK(m.gcm_sender_id.is_null() ||
           m.gcm_sender_id.string().size() <
               content::Manifest::kMaxIPCStringLength);
    return m.gcm_sender_id.as_optional_string16();
  }

  static const GURL& start_url(const content::Manifest& m) {
    return m.start_url;
  }

  static const GURL& scope(const content::Manifest& m) { return m.scope; }

  static blink::WebDisplayMode display(const content::Manifest& m) {
    return m.display;
  }

  static blink::WebScreenOrientationLockType orientation(
      const content::Manifest& m) {
    return m.orientation;
  }

  static int64_t theme_color(const content::Manifest& m) {
    return m.theme_color;
  }

  static int64_t background_color(const content::Manifest& m) {
    return m.background_color;
  }

  static const std::vector<content::Manifest::Icon>& icons(
      const content::Manifest& m) {
    return m.icons;
  }

  static const base::Optional<content::Manifest::ShareTarget> share_target(
      const content::Manifest& m) {
    return m.share_target;
  }

  static const std::vector<content::Manifest::RelatedApplication>&
  related_applications(const content::Manifest& m) {
    return m.related_applications;
  }

  static bool prefer_related_applications(const content::Manifest& m) {
    return m.prefer_related_applications;
  }

  static bool Read(blink::mojom::ManifestDataView data, content::Manifest* out);
};

template <>
struct StructTraits<blink::mojom::ManifestIconDataView,
                    content::Manifest::Icon> {
  static const GURL& src(const content::Manifest::Icon& m) { return m.src; }

  static const base::string16& type(const content::Manifest::Icon& m) {
    DCHECK(m.type.size() <= content::Manifest::kMaxIPCStringLength);
    return m.type;
  }
  static const std::vector<gfx::Size>& sizes(const content::Manifest::Icon& m) {
    return m.sizes;
  }

  static const std::vector<content::Manifest::Icon::IconPurpose>& purpose(
      const content::Manifest::Icon& m) {
    return m.purpose;
  }

  static bool Read(blink::mojom::ManifestIconDataView data,
                   content::Manifest::Icon* out);
};

template <>
struct StructTraits<blink::mojom::ManifestRelatedApplicationDataView,
                    content::Manifest::RelatedApplication> {
  static const base::Optional<base::string16>& platform(
      const content::Manifest::RelatedApplication& m) {
    DCHECK(m.platform.is_null() || m.platform.string().size() <=
                                       content::Manifest::kMaxIPCStringLength);
    return m.platform.as_optional_string16();
  }

  static const GURL& url(const content::Manifest::RelatedApplication& m) {
    return m.url;
  }

  static const base::Optional<base::string16>& id(
      const content::Manifest::RelatedApplication& m) {
    DCHECK(m.id.is_null() ||
           m.id.string().size() <= content::Manifest::kMaxIPCStringLength);
    return m.id.as_optional_string16();
  }

  static bool Read(blink::mojom::ManifestRelatedApplicationDataView data,
                   content::Manifest::RelatedApplication* out);
};

template <>
struct StructTraits<blink::mojom::ManifestShareTargetDataView,
                    content::Manifest::ShareTarget> {
  static const base::Optional<base::string16>& url_template(
      const content::Manifest::ShareTarget& m) {
    return m.url_template.as_optional_string16();
  }
  static bool Read(blink::mojom::ManifestShareTargetDataView data,
                   content::Manifest::ShareTarget* out);
};

template <>
struct EnumTraits<blink::mojom::ManifestIcon_Purpose,
                  content::Manifest::Icon::IconPurpose> {
  static blink::mojom::ManifestIcon_Purpose ToMojom(
      content::Manifest::Icon::IconPurpose purpose) {
    switch (purpose) {
      case content::Manifest::Icon::ANY:
        return blink::mojom::ManifestIcon_Purpose::ANY;
      case content::Manifest::Icon::BADGE:
        return blink::mojom::ManifestIcon_Purpose::BADGE;
    }
    NOTREACHED();
    return blink::mojom::ManifestIcon_Purpose::ANY;
  }
  static bool FromMojom(blink::mojom::ManifestIcon_Purpose input,
                        content::Manifest::Icon::IconPurpose* out) {
    switch (input) {
      case blink::mojom::ManifestIcon_Purpose::ANY:
        *out = content::Manifest::Icon::ANY;
        return true;
      case blink::mojom::ManifestIcon_Purpose::BADGE:
        *out = content::Manifest::Icon::BADGE;
        return true;
    }

    return false;
  }
};

}  // namespace mojo

#endif  // CONTENT_PUBLIC_COMMON_MANIFEST_STRUCT_TRAITS_H_
