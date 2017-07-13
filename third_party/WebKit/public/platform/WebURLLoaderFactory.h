#ifndef WebURLLoaderFactory_h
#define WebURLLoaderFactory_h

#include "WebCommon.h"

#include <memory>

namespace blink {

class WebTaskRunner;
class WebURLLoader;
class WebURLRequest;

// WebURLLoaderFactory is a factory of WebURLLoader.
//
// This will ultimately be replaced by mojom::URLLoaderFactory, but currently
// there is not direct correspondence between them as similar to the interface
// concept difference between WebURLLoader and mojom::URLLoader.
class WebURLLoaderFactory {
  virtual ~WebURLLoaderFactory() = default;

  // Creates a loader.
  // This function takes a request. It should be the same request as the
  // request given to WebURLLoader::LoadAsynchronously and
  // WebURLLoader::LoadSynchronously.
  virtual std::unique_ptr<WebURLLoader> CreateLoader(const WebURLRequest&,
                                                     WebTaskRunner*) = 0;
};

}  // namespace blink

#endif  // WebURLLoaderFactory_h
