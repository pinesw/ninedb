package io.woutervh.ninedb;

import java.util.function.Function;

public class DbConfig {
    public Boolean createIfMissing = null;
    public Boolean deleteIfExists = null;
    public Boolean errorIfExists = null;
    public Integer maxBufferSize = null;
    public Integer maxLevelCount = null;
    public Integer internalNodeCacheSize = null;
    public Integer leafNodeCacheSize = null;
    public Boolean enableCompression = null;
    public Boolean enablePrefixEncoding = null;
    public Integer initialPbtSize = null;
    public Integer maxNodeEntries = null;
    public Function<byte[][], byte[]> reduce = null;
}
