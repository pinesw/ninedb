- Handle deletions with tombstones?

- (Maybe?) Write "diff" function for B-tree. It will allow clients to request deltas between DB versions.

- Add validation methods (verify lengths/pointers, sorted order of keys, metadata, etc.). Could be used as part of health check. Failing this could trigger a "healing process".

- Make reader/iterator thread safe? - DONE (NB: if a cache is added in the future for other features, this would need to be revisited)

- Reduce function/class templating - DONE

- Add a thread to writer to do the flushing so user can continue writing another buffer?

- If config is such that there is no compression/caching/whatever, then drop the cache and read directly from mmap when using the PBT. - IRRELEVANT at the moment

- Try with file operations rather than mmap
  - Tried, was not fast. Needs a large refactor to read/write larger nodes in blocks and add a cache for those blocks.

- Add flags for compression vs. performance: varints/fixed ints, compression, key prefixes (RLE), read cache (RLU)
  - Added LZ4 compression on node level in a branch. Needs a read cache for the reader for decoded nodes. Compresses toy databases by ~60% (100kb -> 40kb) but at a performance loss of ~1.35x.

- Refactor write/read methods: return length of written bytes, no pointer faffing - DONE

- Use binary search within nodes instead of looping - DONE

- Consider storing max_node_children into the PBT and then use tree path calculations to determine number of entries, child, start, end, etc.
