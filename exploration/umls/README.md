µMLS
====

[Messaging Layer Security] (MLS) is a protocol for a group of participants to
authenticate each other and establish shared secrets.  MLS is very flexible,
allowing things like groups of arbitrary size, protocol messages that carry an
arbitrary number of changes to the group, and extensions that can change the
protocol arbitrarily.  To accommodate all this flexibility, MLS libraries
typically rely heavily on things like dynamic allocation that make them
difficult to deploy in embedded environments.

µMLS ("micro MLS") is a library that implements a limited profile of MLS with
minimal resource requirements.

* `#![no_std]`
* No dynamic allocations
* Non-owning message parsing
* Crypto provider interface to allow the use of platform crypto libraries

To make this possible, we impose a few constraints at run time:

* Maximum number of members in a group
* Maximum number of proposals per commit
* No cipher suite negotiation
* Only Basic credentials are supported
* No extensions are supported

# TODO

* Quickstart / Programming overview


[Messaging Layer Security]: https://messaginglayersecurity.rocks/
