package io.pinesw.ninedb;

import java.util.function.Function;

public class DbConfig {
    public Boolean createIfMissing = null;
    public Boolean deleteIfExists = null;
    public Boolean errorIfExists = null;
    public Integer maxBufferSize = null;
    public Integer maxLevelCount = null;
    public Integer initialPbtSize = null;
    public Integer maxNodeChildren = null;
    public Function<byte[][], byte[]> reduce = null;
}
