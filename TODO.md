- Merging new DB with old: handle deletions with tombstones?

- (Maybe?) Write "diff" function for B+ tree. It will allow clients to request deltas between DB versions.

- Make some size comparison between RLE on keys enabled/disabled (with LZ4 compression on) and check if it's worth making RLE a compile time flag

- JSON file is probably not needed. On load, scan for files in the target path (and validate them). This is to get towards a "crash only" shutdown design.

- Add validation methods (verify lengths/pointers, sorted order of keys, metadata, etc.). Could be used as part of health check. Failing this could cause a "healing process".

- Make reader/iterator thread safe?

- Add a thread to writer to do the flushing so user can continue writing another buffer?
