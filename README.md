# ninedb

Latest version: `1.1.1`

## Introduction

This is an embedded key-value data store with the following key features:
- Append only. You can add new key-value pairs, but not delete or edit them.
- Persisted to disk. You can re-open a saved database.
- Really fast.
- Implemented using a packed B-tree structure.
- Provides a 2D spatial index as well.
- Support for value aggregation at the nodes of the B-tree.
- "Crash-only" shutdown design.

## Usage

- [Java README](java/README.md)
- [JavaScript README](js/README.md)

## PBT file format specification

Specification for the file format used by the database ([link](spec/README.md)).

## Database file format

TODO
