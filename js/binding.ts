const pjson = require('./package.json');
const binding = require(pjson.binary.target);

declare const __brand: unique symbol;
type Brand<B> = { [__brand]: B };
type Branded<T, B> = T & Brand<B>;
type KvDbContext = Branded<{}, 'KvDbContext'>;
type HrDbContext = Branded<{}, 'HrDbContext'>;
type KvDbIteratorContext = Branded<{}, 'KvDbIteratorContext'>;

export interface DbConfig {
    createIfMissing?: boolean;
    deleteIfExists?: boolean;
    errorIfExists?: boolean;
    maxBufferSize?: number;
    maxLevelCount?: number;
    internalNodeCacheSize?: number;
    leafNodeCacheSize?: number;
    enableCompression?: boolean;
    enablePrefixEncoding?: boolean;
    initialPbtSize?: number;
    maxNodeEntries?: number;
    reduce?: (values: Buffer) => Buffer;
}

export class KvDbIterator {
    private context: KvDbIteratorContext;

    public constructor(context: KvDbIteratorContext) {
        this.context = context;
    }

    public next() {
        binding.itr_next(this.context);
    }

    public hasNext(): boolean {
        return binding.itr_has_next(this.context);
    }

    public getKey(): Buffer {
        return binding.itr_get_key(this.context);
    }

    public getValue(): Buffer {
        return binding.itr_get_value(this.context);
    }
}

export class KvDatabase {
    private context: KvDbContext;

    public constructor(path: string, config: DbConfig = {}) {
        this.context = binding.kvdb_open(path, config);
    }

    public add(key: Buffer, value: Buffer) {
        binding.kvdb_add(this.context, key, value);
    }

    public get(key: Buffer): Buffer | null {
        return binding.kvdb_get(this.context, key);
    }

    public at(index: number): { key: Buffer, value: Buffer } | null {
        return binding.kvdb_at(this.context, index);
    }

    public traverse(predicate: (value: Buffer) => boolean): Buffer[] {
        return binding.kvdb_traverse(this.context, predicate);
    }

    public flush() {
        binding.kvdb_flush(this.context);
    }

    public compact() {
        binding.kvdb_compact(this.context);
    }

    public begin(): KvDbIterator {
        return new KvDbIterator(binding.kvdb_begin(this.context));
    }

    public seek_key(key: Buffer): KvDbIterator {
        return new KvDbIterator(binding.kvdb_seek_key(this.context, key));
    }

    public seek_index(index: number): KvDbIterator {
        return new KvDbIterator(binding.kvdb_seek_index(this.context, index));
    }
}

export class HrDatabase {
    private context: HrDbContext;

    public constructor(path: string, config: DbConfig = {}) {
        this.context = binding.hrdb_open(path, config);
    }

    public add(x0: number, y0: number, x1: number, y1: number, value: Buffer) {
        binding.hrdb_add(this.context, x0, y0, x1, y1, value);
    }

    public search(x0: number, y0: number, x1: number, y1: number): Buffer[] {
        return binding.hrdb_search(this.context, x0, y0, x1, y1);
    }

    public flush() {
        binding.hrdb_flush(this.context);
    }

    public compact() {
        binding.hrdb_compact(this.context);
    }
}
