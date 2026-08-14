.mode list
CREATE TABLE kv(k TEXT PRIMARY KEY, v INTEGER);
INSERT INTO kv VALUES('a',10),('b',20),('c',30);
UPDATE kv SET v=v+1 WHERE k='b';
DELETE FROM kv WHERE k='c';
CREATE INDEX idx ON kv(v);
SELECT 'kv '||k||'='||v FROM kv ORDER BY k;
SELECT 'total='||SUM(v) FROM kv;
SELECT 'idxscan '||k FROM kv WHERE v>10 ORDER BY v;
PRAGMA integrity_check;
.exit
