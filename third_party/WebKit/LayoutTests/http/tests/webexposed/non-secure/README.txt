The default /webexposed/ LayoutTests execute in a secure context.

Files here reload themselves in a non-secure context so that
interfaces, attributes and methods marked [SecureContext] can be
verified to be absent.
