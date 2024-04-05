import { HrDatabase } from "../binding"

const db = new HrDatabase('data/test-hrdb', { deleteIfExists: true });

for (let i = 0; i < 1e3; i++) {
    const x = Math.random() * 450 + 25;
    const y = Math.random() * 450 + 25;
    db.add(x - 5, y - 5, x + 5, y + 5, Buffer.from(`value_${i}`));
}
db.flush();
db.compact();

console.log("done");
