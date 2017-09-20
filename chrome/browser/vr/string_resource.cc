#include "string_resource.h"

#include "ui/base/l10n/l10n_util.h"

#if defined(OS_LINUX)
#include "base/strings/utf_string_conversions.h"
#endif

namespace vr {

base::string16 GetStringResource(int string_id) {
#if defined(OS_LINUX)
  return l10n_util::GetStringUTF16(string_id);
// LOG(INFO) << "Fetch string resource id " << string_id;
// return base::UTF8ToUTF16("Test string");
// return base::UTF8ToUTF16( "Text to be shown below the URL bar to announce
// that this is under development.\n");
#else
  return l10n_util::GetStringUTF16(string_id);
#endif
}

}  // namespace vr
