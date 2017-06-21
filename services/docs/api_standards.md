# API Standards for Foundation Services

In creating and maintaining the public-facing structure of a foundation service,
you should hold yourself to several first-order goals:

* The purpose of the service should be readily apparent.
* The supported usage models of the service should be easy for a new 
  consumer to understand.
* The service should be consistent with other foundation services. 
* It should be feasible for a third party to build a new API-compliant 
  implementation of the service.

Below we outline concrete standards that aid in achieving the above goals.

## Naming

* Give your service a name that makes it immediately obvious what the service is
for ("network", "metrics"). If you cannot find such a name, is it possible that
your feature is not necessarily a foundation service?

* Avoid the usage of "Service" in interface names. While the term "Service" is
  overloaded in Chromium, in the context of //services it has a very specific
  meaning and should not be overloaded.

* Strive to avoid conceptual layering violations in naming -- e.g., references
  to Blink or //content.

* Use the names "FooClient" and "FooObserver" consistently in interfaces. If 
  there is an expected 1:1 correspondence between Foo and its counterpart, that
  counterpart should be called FooClient. If there is an expected 1:many
  correspondence between Foo and its counterparts, those counterparts should be
  called FooObservers.

## Documentation

* Every service should have a top-level README.md that explains the purpose and 
  supported usage models of the service.

* Every public interface should be documented at the interface (class) level and
  at the method level.

* Interface documentation should be complete enough to serve as test 
  specifications. If the method returns information of a user's accounts, what 
  happens if the user is not signed in? If the method makes a request for an
  access token, what happens if a client makes a second method call before a
  first one has completed?  If a method returns a nullable object, under which
  conditions will it be null?

* Strive to avoid your documentation being too specific to a given client.

## API Shape

* Strive to avoid molding your API shape too specifically to the needs of a
  given client.

* If a given interface Foo requires "construction parameters" (e.g., the client 
  must give it a FooClient before calling any methods), provide a FooFactory
  interface with a GetFoo() method that takes in the relevant construction
  parameters. This approach eliminates the possibility of a badly-written (or
  malevolent) client calling methods on a partially-constructed Foo.

* In the absence of specific guidance, strive for consistency with surrounding
  interfaces and with interfaces in other services.

## Testing

* Use service tests to test the public interfaces exposed by your service.

* Every public method should be covered by at least one service test. Strive
  to have your tests enforce your documentation (corollary: if you can enforce
  your documentation without any tests, improve your documentation :).

* Think of these tests as a form of "compliance tests": They should be written
  in such a way that a third party with an independent implementation of your
  APIs should trivially be able to run your tests against their implementation.
  Notably, try to avoid relying on implementation details of the service in its
  tests.

* Related to the above, aim for a high degree of coverage with these tests. If a
  third-party implementation passes your tests, you should have a high degree of
  confidence that it will be usable by your consumers.

## Appendix: Responsibility for Upholding These Standards

The responsibility for holding these standards is shared across
//services/OWNERS, individual service OWNERS, and services developers:

* //services/OWNERS own the standards themselves and are responsible for
  ensuring that quality and consistency are maintained across //services.
* Individual service OWNERS are responsible for ensuring that their service
  adheres to these standards.
* Service developers are responsible for developing as if they were OWNERS.

We expect that these services will evolve over time. If you encounter a tricky
situation not covered here, please send an email to services-dev@. Similarly, if
you see inconsistency or violations of the standards, please file a bug and CC
relevant OWNERS (i.e., of the service in question and/or //services/OWNERS).
