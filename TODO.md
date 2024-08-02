- Merging new DB with old: handle deletions with tombstones?

- (Maybe?) Write "diff" function for B+ tree. It will allow clients to request deltas between DB versions.

- Make some size comparison between RLE on keys enabled/disabled (with LZ4 compression on) and check if it's worth making RLE a compile time flag

- JSON file is probably not needed. On load, scan for files in the target path (and validate them). This is to get towards a "crash only" shutdown design.

- Add validation methods (verify lengths/pointers, sorted order of keys, metadata, etc.). Could be used as part of health check. Failing this could cause a "healing process".

- Make reader/iterator thread safe?

- Reduce function/class templating?

- Add a thread to writer to do the flushing so user can continue writing another buffer?

- If config is such that there is no compression/caching/whatever, then drop the cache and read directly from mmap when using the PBT.

New:

- Try with file operations rather than mmap

- Add flags for compression vs. performance: varints/fixed ints, compression, key prefixes, read cache (RLU)

- Refactor write/read methods: return length of written bytes, no pointer faffing

- Use binary search within nodes instead of looping

- Consider storing max_node_children into the PBT and then use tree path calculations to determine number of entries, child, start, end, etc.
