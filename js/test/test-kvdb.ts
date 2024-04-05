import { KvDatabase } from "../binding"

const t1 = Date.now();
const db = new KvDatabase('data/test-kvdb', { deleteIfExists: true, reduce: () => Buffer.from('x') });
console.log('db1', db);
for (let i = 0; i < 1e4; i++) {
    db.add(Buffer.from(`key_${i}`), Buffer.from(`value_${i}`));
}
db.flush();
db.compact();
const t2 = Date.now();
console.log(`Insert time: ${t2 - t1}ms`);

let count;

const t3 = Date.now();
count = 0;
const itr1 = db.begin();
while (itr1.hasNext()) {
    count += 1;
    itr1.next();
}
const t4 = Date.now();
console.log(`Iterate time: ${t4 - t3}ms`);

console.log(db.get(Buffer.from("key_10")));
console.log(db.at(10));

console.log(count);
