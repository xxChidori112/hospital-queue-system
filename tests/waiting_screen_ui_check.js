'use strict';
// Cần Playwright có sẵn: node tests/waiting_screen_ui_check.js <hospital_web> <browser.exe>
const assert = require('assert');
const {spawn} = require('child_process');
const {chromium} = require('playwright');
const net = require('net');

async function main() {
  assert(process.argv[2] && process.argv[3], 'Cần đường dẫn máy chủ và trình duyệt.');
  const socket = net.createServer();
  await new Promise(resolve => socket.listen(0, '127.0.0.1', resolve));
  const port = socket.address().port;
  await new Promise(resolve => socket.close(resolve));
  const base = 'http://127.0.0.1:' + port;
  const server = spawn(process.argv[2], [], {env:{...process.env, HOST:'127.0.0.1', PORT:String(port), HOSPITAL_DB:':memory:'}, stdio:'ignore'});
  let browser;
  try {
    for (let i=0;i<50;i++) {
      try { await fetch(base+'/api/waiting-display'); break; }
      catch (_) { await new Promise(resolve=>setTimeout(resolve,200)); }
    }
    browser = await chromium.launch({headless:true, executablePath:process.argv[3]});
    const context = await browser.newContext({viewport:{width:1440,height:900}, reducedMotion:'reduce'});
    await context.route('https://media.giphy.com/**', route=>route.fulfill({contentType:'image/gif',body:Buffer.from('R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7','base64')}));
    const page = await context.newPage();
    await page.goto(base, {waitUntil:'networkidle'});
    assert((await page.title()).startsWith('CareQueue UTC2'));
    assert(!(await page.locator('body').innerText()).includes('MedCare'));
    assert(await page.evaluate(()=>['input-priority','filter-priority'].every(id=>
      [...document.getElementById(id).options].filter(o=>Number(o.value)>=1&&Number(o.value)<=5)
        .every(o=>o.textContent===priorityLabel(Number(o.value))))));
    await page.fill('#input-name','Đang nhập');
    await page.click('#btn-waiting-screen');
    await page.waitForFunction(()=>document.getElementById('waiting-dialog').open);
    const frame = page.frameLocator('#waiting-frame');
    await frame.locator('#connection').filter({hasText:'Đã kết nối'}).waitFor();
    assert.equal(await frame.locator('.room').count(),7);
    await frame.locator('#meme-image').waitFor({state:'visible'});
    const firstMeme = await frame.locator('#meme-image').getAttribute('src');
    assert(firstMeme.startsWith('https://media.giphy.com/media/'));
    assert.equal(await frame.locator('#meme-toggle').textContent(),'Tạm dừng', 'GIF tự phát ngay khi mở phòng chờ.');
    await frame.locator('#meme-toggle').click();
    assert.equal(await frame.locator('#meme-image').getAttribute('src'),null);
    await frame.locator('#meme-next').click();
    await frame.locator('#meme-toggle').click();
    await frame.locator('#meme-image').waitFor({state:'visible'});
    assert.notEqual(await frame.locator('#meme-image').getAttribute('src'),firstMeme);
    const secondMeme = await frame.locator('#meme-image').getAttribute('src');
    await frame.locator('#meme-next').click();
    await frame.locator('#meme-image').waitFor({state:'visible'});
    assert.notEqual(await frame.locator('#meme-image').getAttribute('src'),secondMeme);
    const currentMeme = await frame.locator('#meme-image').getAttribute('src');
    const embedded = page.frames().find(f=>f.url()===base+'/waiting');
    await embedded.waitForFunction(current=>document.getElementById('meme-image').getAttribute('src')!==current,currentMeme,{timeout:8000});
    const catalog = await embedded.evaluate(()=>({count:memes.length,unique:new Set(memes.map(m=>m.url)).size,
      giphyOnly:memes.every(m=>m.url.startsWith('https://media.giphy.com/media/') && m.source.startsWith('https://giphy.com/gifs/'))}));
    assert(catalog.count>=100, 'Cần ít nhất 100 GIF.');
    assert.equal(catalog.unique,catalog.count, 'Mỗi GIF phải có đường dẫn riêng.');
    assert(catalog.giphyOnly);
    const randomChoices = await embedded.evaluate(()=>{
      const originalRandom=Math.random, originalIndex=memeIndex;
      try {
        return [0,0.5,0.999999].map(value=>{
          memeIndex=0;Math.random=()=>value;nextMeme();return memeIndex;
        });
      } finally {Math.random=originalRandom;memeIndex=originalIndex;showMeme();}
    });
    assert.deepEqual(randomChoices,[1,1+Math.floor((catalog.count-1)/2),catalog.count-1], 'Chọn ngẫu nhiên phải tới được đầu, giữa và cuối danh sách.');
    await context.route('https://media.giphy.com/**',route=>route.abort());
    await frame.locator('#meme-next').click();
    await frame.locator('#meme-status').filter({hasText:'Không tải được GIF'}).waitFor();
    assert.equal(context.pages().length,1);
    assert.equal(await frame.locator('#return-link').isVisible(),false);
    await page.click('#close-waiting-screen');
    await page.waitForFunction(()=>!document.getElementById('waiting-frame').hasAttribute('src'));
    assert.equal(await page.inputValue('#input-name'),'Đang nhập');
    assert.equal(await page.locator('#waiting-frame').getAttribute('src'),null);
    await page.setViewportSize({width:390,height:844});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
    const brandBox=await page.locator('.utc-header-brand').boundingBox();
    const departmentBox=await page.locator('#header-department').boundingBox();
    assert(brandBox.width>200 && brandBox.y+brandBox.height<=departmentBox.y, 'Tên đơn vị cần đủ rộng và nằm trên bộ chọn khoa trên điện thoại.');
    await page.click('#btn-waiting-screen');
    await frame.locator('.room').first().waitFor();
    assert((await page.locator('#waiting-dialog').boundingBox()).width<=390);
    await page.keyboard.press('Escape');
    assert.equal(await page.locator('#waiting-dialog').evaluate(e=>e.open),false);
    console.log('Màn hình chờ: mở/đóng, GIF tự đổi/tạm dừng/lỗi tải, giữ biểu mẫu, điện thoại và Escape đều đúng.');
  } finally {if(browser)await browser.close();server.kill();}
}
main().catch(error=>{console.error(error);process.exitCode=1;});
