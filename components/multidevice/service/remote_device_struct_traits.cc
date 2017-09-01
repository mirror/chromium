#include "components/multidevice/service/remote_device_struct_traits.h"

namespace mojo {
bool StructTraits<device_sync::mojom::RemoteDevice, cryptauth::RemoteDevice>::
    Read(device_sync::mojom::RemoteDevice data,
         cryptauth::RemoteDevice* out_remote_device) {
  return true;
}
}  // namespace mojo