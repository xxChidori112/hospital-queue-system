'use strict';
// Chạy: node tests/waiting_display_check.js <đường-dẫn-tới-hospital_web>
const assert = require('assert');
const { spawn } = require('child_process');
const net = require('net');

async function main() {
  if (!process.argv[2]) throw new Error('Cần cung cấp đường dẫn tới hospital_web đã biên dịch.');
  const socket = net.createServer();
  await new Promise(resolve => socket.listen(0, '127.0.0.1', resolve));
  const port = socket.address().port;
  await new Promise(resolve => socket.close(resolve));
  const base = 'http://127.0.0.1:' + port;
  const server = spawn(process.argv[2], [], {env: {...process.env, HOST:'127.0.0.1', PORT:String(port), HOSPITAL_DB:':memory:'}, stdio:'ignore'});
  const post = async (url, fields) => {
    const response = await fetch(base + url, {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams(fields)});
    const data = await response.json();
    assert(response.ok && data.ok, JSON.stringify(data));
    return data;
  };
  try {
    let ready = false;
    for (let i=0; i<50; i++) {
      try { await fetch(base+'/api/waiting-display'); ready=true; break; }
      catch (_) { await new Promise(resolve => setTimeout(resolve,200)); }
    }
    assert(ready, 'Máy chủ chưa sẵn sàng.');
    await post('/api/register',{name:'TEST_PRIVATE_NAME',age:30,symptom:'TEST_PRIVATE_SYMPTOM',department:'Mat',triage:5});
    await post('/api/register',{name:'TEST_OTHER',age:25,symptom:'TEST_OTHER_SYMPTOM',department:'Mat',triage:1});
    let response = await fetch(base+'/api/waiting-display');
    assert.equal(response.headers.get('cache-control'), 'no-store');
    let data = await response.json();
    assert.equal(data.waiting[0].ticket,'BN0002');
    assert.equal(data.waiting[1].ticket,'BN0001');
    assert(!JSON.stringify(data).includes('TEST_PRIVATE'));
    assert(!JSON.stringify(data).includes('TEST_OTHER'));
    await post('/api/call-next',{department:'Mat'});
    data = await (await fetch(base+'/api/waiting-display')).json();
    assert.equal(data.active[0].ticket,'BN0002');
    assert.equal(data.waiting[0].ticket,'BN0001');
    assert((await fetch(base+'/waiting')).ok);
    console.log('Màn hình chờ: ưu tiên, gọi lượt và dữ liệu hiển thị đều đúng.');
  } finally { server.kill(); }
}
main().catch(error => { console.error(error); process.exitCode=1; });