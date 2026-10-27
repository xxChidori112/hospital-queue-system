'use strict';
// node tests/queue_demo_check.js <hospital_web> <browser.exe>
const assert=require('assert');
const {spawn}=require('child_process');
const {chromium}=require('playwright');
const net=require('net');
const path=require('path');

async function main(){
  assert(process.argv[2]&&process.argv[3],'Cần đường dẫn máy chủ và trình duyệt.');
  const socket=net.createServer();await new Promise(resolve=>socket.listen(0,'127.0.0.1',resolve));
  const port=socket.address().port;await new Promise(resolve=>socket.close(resolve));
  const base='http://127.0.0.1:'+port;
  const server=spawn(process.argv[2],[],{env:{...process.env,HOST:'127.0.0.1',PORT:String(port),HOSPITAL_DB:':memory:'},stdio:'ignore'});
  const getState=async()=>(await fetch(base+'/api/state')).json();
  let browser;
  try{
    let ready=false;
    for(let i=0;i<50;i++){try{await getState();ready=true;break;}catch{await new Promise(resolve=>setTimeout(resolve,200));}}
    assert(ready,'Máy chủ chưa sẵn sàng.');
    const before=await getState();
    browser=await chromium.launch({headless:true,executablePath:process.argv[3]});
    const context=await browser.newContext({viewport:{width:1440,height:1000}});
    const page=await context.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));
    await page.goto(base,{waitUntil:'networkidle'});
    await page.fill('#input-name','Giữ nguyên biểu mẫu');
    await page.click('#btn-queue-demo');
    const frame=page.frameLocator('#waiting-frame');
    await frame.locator('#regular-cells .cell').first().waitFor();
    assert.equal(await frame.locator('#regular-cells .cell').count(),4);
    assert(await frame.locator('#call').isDisabled());
    assert.equal(await frame.locator('#return-link').isVisible(),false);
    const next=()=>frame.locator('#next-step').click();
    for(let i=0;i<3;i++)await next();
    assert((await frame.locator('#regular-order').textContent()).includes('MP001 → MP002 → MP003'));
    await next();assert.equal(await frame.locator('#last-ticket').textContent(),'MP001');
    await next();assert.equal(await frame.locator('#last-ticket').textContent(),'MP002');
    assert.equal(await frame.locator('#regular-front').textContent(),'2');
    await next();assert.equal(await frame.locator('#regular-rear').textContent(),'0');
    await next();assert((await frame.locator('#regular-cells .cell').nth(0).textContent()).includes('MP005'));
    await next();assert.equal(await frame.locator('#regular-count').textContent(),'4');
    assert.equal(await frame.locator('#regular-front').textContent(),await frame.locator('#regular-rear').textContent());
    await next();assert.equal(await frame.locator('#regular-capacity').textContent(),'8');
    assert.equal(await frame.locator('#regular-front').textContent(),'0');
    assert((await frame.locator('#regular-order').textContent()).includes('MP003 → MP004 → MP005 → MP006 → MP007'));
    for(let i=0;i<3;i++)await next();
    assert((await frame.locator('.heap-node.root').textContent()).includes('MP009'));
    await page.locator('#waiting-frame').screenshot({path:path.join(process.env.TEMP||process.cwd(),'carequeue-demo-desktop.png')});
    for(const ticket of ['MP009','MP010','MP008','MP003','MP004','MP005','MP006','MP007']){
      await next();assert.equal(await frame.locator('#last-ticket').textContent(),ticket);
    }
    assert(await frame.locator('#next-step').isDisabled());
    assert(await frame.locator('#call').isDisabled());
    assert.deepEqual(await frame.locator('.history-item').evaluateAll(items=>items.map(item=>item.dataset.ticket)),['MP007','MP006','MP005','MP004','MP003']);
    await frame.locator('#remove-newest').click();await frame.locator('#remove-oldest').click();
    assert.deepEqual(await frame.locator('.history-item').evaluateAll(items=>items.map(item=>item.dataset.ticket)),['MP006','MP005','MP004']);
    await frame.locator('#reset').click();
    assert.equal(await frame.locator('#regular-count').textContent(),'0');
    assert.equal(await frame.locator('#regular-capacity').textContent(),'4');
    assert(await frame.locator('#remove-newest').isDisabled());
    await frame.locator('#priority').selectOption('5');await frame.locator('#add').click();
    await frame.locator('#priority').selectOption('2');await frame.locator('#add').click();
    await frame.locator('#priority').selectOption('1');await frame.locator('#add').click();
    for(const ticket of ['MP003','MP002','MP001']){
      await frame.locator('#call').click();assert.equal(await frame.locator('#last-ticket').textContent(),ticket);
    }
    const embedded=page.frames().find(f=>f.url()===base+'/queue-demo');
    assert(await embedded.evaluate(()=>{
      const originalHeap=heap;heap=[];
      try{
        const patients=Array.from({length:100},(_,i)=>({ticket:'TEST'+i,order:i,level:i%3?2:1}));
        patients.forEach(pushEmergency);
        return patients.sort((a,b)=>a.level-b.level||a.order-b.order).every(p=>popEmergency().ticket===p.ticket)&&heap.length===0;
      }finally{heap=originalHeap;}
    }),'Heap phải giữ mức ưu tiên và FIFO qua nhiều lần siftUp/siftDown.');
    assert.deepEqual(await getState(),before,'Dữ liệu mẫu không được thay đổi dữ liệu điều phối.');
    await page.click('#close-waiting-screen');
    assert.equal(await page.inputValue('#input-name'),'Giữ nguyên biểu mẫu');
    assert.equal(await page.locator('#waiting-frame').getAttribute('src'),null);
    for(const width of [320,390]){
      await page.setViewportSize({width,height:844});await page.click('#btn-queue-demo');
      await frame.locator('#regular-cells .cell').first().waitFor();
      const embedded=page.frames().find(f=>f.url()===base+'/queue-demo');
      assert(await embedded.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
      await page.locator('#waiting-frame').screenshot({path:path.join(process.env.TEMP||process.cwd(),'carequeue-demo-'+width+'.png')});
      await page.keyboard.press('Escape');
      await page.waitForFunction(()=>!document.getElementById('waiting-dialog').open);
    }
    assert.equal(context.pages().length,1);assert.deepEqual(errors,[]);
    console.log('Minh họa Queue: FIFO, quay vòng, mở rộng, heap cấp cứu, Deque, đặt lại, cách ly dữ liệu và giao diện điện thoại đều đúng.');
  }finally{if(browser)await browser.close();server.kill();}
}
main().catch(error=>{console.error(error);process.exitCode=1;});
