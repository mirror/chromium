# Media Mojo Design

`media::mojom::InterfaceFactory` is the central point for requesting any
media-related interface and is used by the renderer and (indirectly) by a
standalone CDM to request services from the media service.

`media::mojom::MediaService` is the central master interface implemented by the
media service and the CDM service. Each service is initialized by calling
`CreateInterfaceFactory`, which initializes a `media::mojom::InterfaceFactory` in
the calling process, and also passes a
`service_manager::mojom::InterfaceProvider` that the media or CDM service can
use to request additional interfaces.

> Note: there are plans to split apart the responsibilities of
> `media::mojom::InterfaceFactory` to make it clear which interfaces are used
> where. Similarly, there are plans to split apart `media::mojom::MediaService`
> so it is clearer which functionality is needed by the CDM service and which
> functionality is needed by the media service.

# Renderer

`content::MediaInterfaceFactory` implements `media::mojom::InterfaceFactory` in
the renderer process. Calls to the factory methods are simply forwarded to the
browser process. This implementation of `media::mojom::InterfaceFactory` should
only ever be used to create `media::mojom::AudioDecoder`,
`media::mojom::VideoDecoder`, `media::mojom::Renderer`, and
`media::mojom::ContentDecryptionModule` interfaces.

# Browser

The browser process is responsible for creating the media and CDM services and
configuring the `service_manager::mojom::InterfaceProvider` that these services
use to request additional interfaces from the browser process. The exposed
interfaces are configured by `content::MediaInterfaceProxy::GetFrameServices()`.

When the renderer `content::MediaInterfaceFactory` class forwards a factory
method call, this request is handled in `content::MediaInterfaceProxy` in the
browser process. The implementation simply takes incoming method calls (from the
renderer, over Mojo) and forwards them to the media service or CDM service (over
Mojo).

If the standalone CDM service is enabled, `CreateCdm()` is forwarded to the CDM
service; otherwise, it is forwarded to the media service.

Note that the browser itself never directly makes these factory method calls; it
is always calling into the media or CDM service's
`media::mojom::InterfaceFactory` on behalf of another process.

# Media Service

The media service can live in either the GPU process (desktop) or the
browser process (Android and cast). Incoming factory method calls forwarded from
the browser process are handled by `media::InterfaceFactoryImpl`. The media
service is responsible for creating concrete implementations of the
`media::mojom::AudioRenderer`, `media::mojom::VideoDecoder`, and
`media::mojom::Renderer` interfaces.

If the standalone CDM service is disabled, the media service is also responsible
for creating a concrete implementation of the
`media::mojom::ContentDecryptionModule` interface.

> Note: there are plans to move the media service out of the browser process in
> the future.

# CDM Service

When the standalone CDM service is enabled, a separate CDM service is hosted in
the utility process.  It is also backed by `media::InterfaceFactoryImpl`, but in
this mode, it is only responsible for creating a concrete implementation of the
`media::mojom::ContentDecryptionModule` interface.

> TODO(xhwang): What happens if the CDM service is asked for an audio decoder,
> video decoder, or renderer?
