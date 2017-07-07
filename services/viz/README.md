rectory Structure

The Viz (Visuals) service is a collection of subservices: compositing, gl, hit
testing, and media.

Viz has two types of clients: privileged and unprivileged.

Privileged clients are responsible for starting and restarting Viz after a crash
and for facilitating connections to Viz from unprivileged clients. Privileged
clients are trusted by all other clients, and are expected to be long-lived and
not prone to crashes. 

Unprivileged clients request connections to Viz through a privileged client such
as the browser process or the window server. Furthermore, unprivileged clients
may be malicious or may crash at any time. Unprivileged clients are expected to
be mutually distrusting of one another. Thus, an unprivileged client cannot be
provided interfaces by which it can impact the operation of another client.

For example, GL command buffers can only be dispensed by a privileged client,
but can be used by unprivileged clients. GL commands are exposed as a stable
public API to the command buffer by the client library whereas the underlying
IPC messages and their semantics are constantly changing and meaningless without
deep knowledge of implementation details.

We propose the following directory structure to accommodate Viz's needs.

//services/viz/{compositing, gl, hit_test, media}/public/interfaces
//services/viz/{compositing, gl, hit_test, media}/public/<language>

The interfaces directories contain mojoms that define the public, unprivileged
interface for the Viz subservices. Clients may directly use the mojo interfaces 
in these directories or choose to use the client library in a public/<language>
directory if one exists for a given mojom. private and privileged interfaces may
depend on public interfaces.
 
//services/viz/{compositing, gl, hit_test, media}/private/interfaces

These interfaces directories contain mojoms that may only be used by going
through a language-specific client library. They are meant for unprivileged use,
without direct access to the mojoms. As such, only the
//services/viz/public/<language> and //services/viz/privileged/<language>
directories may depend on private, while other directories including interface
directories must not. There is no private client library, as these are meant for
consumption by the public client library.

//services/viz/{compositing, gl, hit_test, media}/privileged/interfaces
//services/viz/{compositing, gl, hit_test, media}/privileged/<language>

The interfaces directories contains mojoms that may only be used by a privileged
client. These interfaces may be used directly or through a privileged/<language>
client library. The public and private interfaces must not depend on privileged
interfaces. Typically, the browser process or the window server serves as the
privileged client to Viz.

//services/viz

This is the glue code that implements the primordial VizMain interface (in
//services/viz/main/privileged/interfaces) that starts up the Viz process
through the service manager. VizMain is a factory interface that enables a
privileged client to instantiate the Viz subservices: compositing, gl, hit_test,
and media.

//services/viz/{compositing, gl, hit_test, media}/service

Service-side implementation code live in the various sub-service "service"
directories. Service code may depend on the common and interfaces
subdirectories, but cannot depend on any of the
//service/viz/.../public/<language> client libraries.

Short term: //components/viz and //gpu and //media

At this time, the Viz public client library for the compositing and hit_test
subservices live in //components/viz/client, and the privileged client library
lives in //components/viz/host.

Command buffer code will continue to live in //gpu and media code will continue
to live in //media.

Once the content module has been removed (or no longer depends on
components/viz/service), the code in //components/viz/client will move to
appropriate destinations in //services/viz/.../<language>.
//components/viz/service will move to the appropriate service directories in
//services/viz/.../service. //components/viz/host will move to
//services/viz/{compositing, gl, hit_test, media}/privileged.

Once the content module is gone, and //services/ui is the only privileged
client, then perhaps the privileged client library may move to //services/ui.

Common data types

Common data types such as CompositorFrame, and GpuMemoryBufferHandle can live
in:

//services/viz/{compositing, gl, hit_test, media}/common.

Their associated mojoms can live in:

//services/viz/{compositing, gl, hit_test, media}/public/interfaces.

Note: //services/viz/{compositing, gl, hit_test, media}/common holds C++ types
only and does not depend on
//services/viz/{compositing, gl, hit_test, media}/public/interfaces. Instead,
there are StructTraits with the interfaces that produce/consume types in the
common directory for mojo transport.


Acceptable Dependencies

Unprivileged client, can depend on
  services/viz/{compositing, gl, hit_test, media}/public/<language>,
  can depend on
    services/viz/{compositing, gl, hit_test, media}/public/interfaces,
    can depend on
      services/viz/{compositing, gl, hit_test, media}/common
  services/viz/{compositing, gl, hit_test, media}/private/interfaces,
  can depend on
    services/viz/{compositing, gl, hit_test, media}/common
  services/viz/public/interfaces, can depend on
    services/viz/{compositing, gl, hit_test, media}/common

Privileged client, can depend on
  services/viz/privileged/<language>, can depend on
    services/viz/{compositing, gl, hit_test, media}/privileged/interfaces, can
    depend on
      services/viz/{compositing, gl, hit_test, media}/common
  services/viz/{compositing, gl, hit_test, media}/public/interfaces,
  can depend on
    services/viz/{compositing, gl, hit_test, media}/common
  services/viz/{compositing, gl, hit test, media}/private/interfaces, can
  depend on
    services/viz/{compositing, gl, hit_test, media}/common

Services can depend on:
  services/viz/{compositing, gl, hit_test, media}/common
  services/viz/{compositing, gl, hit_test, media}/privileged/interfaces
  services/viz/{compositing, gl, hit test, media}/private/interfaces
  services/viz/{compositing, gl, hit_test, media}/public/interfaces

