# Service Manager Embedder API Public Library

This directory contains interfaces via which an embedder of the Service Manager
can communicate with its own embedders, e.g., providing hooks to allow those
embedders to register their own services. As an example, the Content API
provides hooks using these interfaces to allow e.g. Chrome to register embedded
services.
