# ninedb PBT file specification

A specification for Packed B-Trees.

## Purpose

This file format is designed to achieve fast look-up for key-value pairs of variable length.
Aggregated values may be saved at intermediate nodes of the B-trees to support constant time look-up for a group of nodes, e.g. bounding boxes for spatial index queries.

## Versioning

The version schema follows the `major.minor` format.
The `major` version shall be incremented when backwards incompatible changes are made.
The `minor` version shall be incremented when backwards compatible changes are made.
Backwards compatibility is loosely defined as the situation where an implementation working with a specific format version can still work with the new changes.

Updates to a specification without any changes to the physical file format, such as fixing spelling mistakes or adding examples, shall be made without increasing any part of the version.

- [Version 0.1](0.1/README.md)
