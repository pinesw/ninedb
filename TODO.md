- Merging new DB with old: handle deletions with tombstones?

- (Maybe?) Write "diff" function for B+ tree. It will allow clients to request deltas between DB versions.

- Make some size comparison between RLE on keys enabled/disabled (with LZ4 compression on) and check if it's worth making RLE a compile time flag

- JSON file is probably not needed. On load, scan for files in the target path (and validate them). This is to get towards a "crash only" shutdown design.
