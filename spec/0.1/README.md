# PBT specification 0.1

This document specifies the encoding of a packed B-tree in a binary file.

## File extension

The filename extension for PBT files should be `.pbt`.

## File format

A PBT file consists of leaf nodes and intermediate nodes which form a tree structure.
Leaf nodes consist of key-value pairs.
Intermediate nodes contain references to leaf nodes by byte-offset.
There is exactly one root node, which may be a leaf node or an intermediate node.
Finally, at the end of the file, there is a footer with meta information about the data in the file.

### Overview

All of the key-value pairs added to the database are stored in PBT files.
Within each PBT file, the key-value pairs appear in sorted order.
By "sorted order" we mean a lexicographical ordering by the bytes of the keys.

Globally, the following table defines the contents of a file.
The following shorthands are used:
- `L(n)`: the byte offset into the file where the `n`<sup>th</sup> leaf node resides
- `I(m)`: the byte offset into the file where the `m`<sup>th</sup> intermediate node resides
- `N` the total number of leaf nodes
- `M` the total number of intermediate nodes (note: can be 0)
- `S` the size (number of bytes) of the file

| Byte offset | Byte length | Encoding | Description |
|---|---|---|---|
| `0` | Variable | [Leaf node](#leaf-node) | The first leaf node. |
| `L(n)` | Variable | [Leaf node](#leaf-node) | The `n`<sup>th</sup> leaf node. |
| `I(0) = L(N)` | Variable | [Intermediate node](#intermediate-node) | The first intermediate node, if there is any at all. |
| `I(m)` | Variable | [Intermediate node](#intermediate-node) | The `m`<sup>th</sup> intermediate node, if there are any at all. |
| `S - 42` | 42 | [Footer](#footer) | The footer containing the metadata. |

### Leaf node

Leaf nodes store the key-value pairs that have been added to the database.
The offsets and lengths to each key-value pair is stored in the first part of a leaf node.
This is to facilitate binary searching through the node for fast look-up.

For a leaf node, the structure is defined by the following table.
The following shorthands are used:
- `K`: the number of key-value pairs in the leaf node
- `k`: the `k`<sup>th</sup> key-value pair in the leaf node
- `O(k)`: the offset where the `k`<sup>th</sup> key-value pair is located, counted from `L(n)`

| Byte offset | Byte length | Encoding | Description |
|---|---|---|---|
| `L(n) + 0` | 2 | Uint 16 LE | Number `K` of key-value pairs in this node. |
| `L(n) + 2 + 24 * k` | 8 | Uint 64 LE | Offset `O(k)` where the data of key-value pair `k` are stored. |
| `L(n) + 2 + 24 * k + 8` | 8 | Uint 64 LE | The length of the `k`<sup>th</sup> key. |
| `L(n) + 2 + 24 * k + 16` | 8 | Uint 64 LE | The length of the `k`<sup>th</sup> value. |
| `L(n) + 2 + 24 * K` | Variable | Bytes | Sequence of `K` key-value pairs. Each key-value pair can be found at `L(n) + O(k)` |

### Intermediate node

Intermediate nodes store references to child nodes by byte-offsets into the file.
Child nodes can be leaf nodes as well as intermediate nodes.
For each child node, the right-most (largest) key is kept in an entry in the intermediate node.
For the first child node, the left-most (smallest) key is also kept in the intermediate node.
These keys (left-most and right-most) are kept to facilitate binary searching within the intermediate node itself, as well as for selecting the child node to search further in.

For an intermediate node, the structure is defined by the following table.
The following shorthands are used:
- `K`: the number of child nodes referenced by the intermediate nodes.
- `k`: the `k`<sup>th</sup> child node of the intermediate node.
- `O(k)`: the offset where the `k`<sup>th</sup> child's right-most key and reduced value are located, counted from `I(m)`.

| Byte offset | Byte length | Encoding | Description |
|---|---|---|---|
| `I(m) + 0` | 2 | Uint 16 LE | Number `K` of child nodes referenced by this node. |
| `I(m) + 2` | 8 | Uint 64 LE | Offset where the first child node's left-most key is stored. |
| `I(m) + 10` | 8 | Uint 64 LE | The length of the first child node's left-msot key.
| `I(m) + 18 + 48 * k` | 8 | Uint 64 LE | Offset `O(k)` where the data of child node `k` are stored. |
| `I(m) + 18 + 48 * k + 8` | 8 | Uint 64 LE | The length of the `k`<sup>th</sup> child node's right-most key. |
| `I(m) + 18 + 48 * k + 16` | 8 | Uint 64 LE | The length of the `k`<sup>th</sup> child node's reduced value. |
| `I(m) + 18 + 48 * k + 24` | 8 | Uint 64 LE | The index of the `k`<sup>th</sup> child node's left-most key-value pair. |
| `I(m) + 18 + 48 * k + 32` | 8 | Uint 64 LE | The byte offset of the `k`<sup>th</sup> child node. |
| `I(m) + 18 + 48 * k + 40` | 8 | Uint 64 LE | The byte length of the `k`<sup>th</sup> child node. |
| `I(m) + 18 + 48 * K` | Variable | Bytes | The first child node's left-most key, followed by a sequence of `K` key-value pairs. Each key-value pair holds the right-most key and the reduced value of the respective child node. |

### Footer

For the footer, the structure is defined by the following table.

| Byte offset | Byte length | Encoding | Description |
|---|---|---|---|
| `S - 42` | 8 | Uint 64 LE | Root node offset. The byte-offset into the file where the root node can be found. |
| `S - 34` | 8 | Uint 64 LE | Root node byte length. |
| `S - 26` | 2 | Uint 16 LE | Tree height. The number of levels in the tree structure. |
| `S - 24` | 8 | Uint 64 LE | Global start. The index of the first key-value pair as counted in the entire database, across multiple PBT files. |
| `S - 16` | 8 | Uint 64 LE | Global end. The index (exclusive) of the last key-value pair as counted in the entire database, across multiple PBT files. |
| `S - 8` | 2 | Uint 16 LE | Version major. The major version of the format that the PBT file was written in. For this specification version, it is `0`. |
| `S - 6` | 2 | Uint 16 LE | Version minor. The minor version of the format that the PBT file was written in. For this specification version, it is `1`. |
| `S - 4` | 4 | Uint 32 LE | Magic number `0x1EAF1111`. |
