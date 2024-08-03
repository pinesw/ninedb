## Installation

npm page: https://www.npmjs.com/package/@pinesw/ninedb

### npm

```bash
$ npm install @pinesw/ninedb
```

## Usage

### Examples

```typescript
import { KvDatabase } from '@pinesw/ninedb';

const db = new KvDatabase('data/my-database');
db.add(Buffer.from('key_1'), Buffer.from('value_1'));
db.add(Buffer.from('key_2'), Buffer.from('value_2'));
db.add(Buffer.from('key_3'), Buffer.from('value_3'));
db.flush();

const itr = db.begin();
while (itr.hasNext()) {
    console.log(itr.getKey().toString(), itr.getValue().toString());
    itr.next();
}
```
