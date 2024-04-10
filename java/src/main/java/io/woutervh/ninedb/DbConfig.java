package io.woutervh.ninedb;

import java.util.function.Function;

public class DbConfig {
    Boolean createIfMissing = null;
    Boolean deleteIfExists = null;
    Boolean errorIfExists = null;
    Integer maxBufferSize = null;
    Integer maxLevelCount = null;
    Integer internalNodeCacheSize = null;
    Integer leafNodeCacheSize = null;
    Boolean enableCompression = null;
    Boolean enablePrefixEncoding = null;
    Integer initialPbtSize = null;
    Integer maxNodeEntries = null;
    Function<byte[][], byte[]> reduce = null;
}
