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
    const context = await browser.newContext({viewport:{width:1440,height:900}});
    const page = await context.newPage();
    await page.goto(base, {waitUntil:'networkidle'});
    await page.fill('#input-name','Đang nhập');
    await page.click('#btn-waiting-screen');
    await page.waitForFunction(()=>document.getElementById('waiting-dialog').open);
    const frame = page.frameLocator('#waiting-frame');
    await frame.locator('#connection').filter({hasText:'Đã kết nối'}).waitFor();
    assert.equal(await frame.locator('.room').count(),7);
    assert.equal(context.pages().length,1);
    assert.equal(await frame.locator('#return-link').isVisible(),false);
    await page.click('#close-waiting-screen');
    await page.waitForFunction(()=>!document.getElementById('waiting-frame').hasAttribute('src'));
    assert.equal(await page.inputValue('#input-name'),'Đang nhập');
    assert.equal(await page.locator('#waiting-frame').getAttribute('src'),null);
    await page.setViewportSize({width:390,height:844});
    await page.click('#btn-waiting-screen');
    await frame.locator('.room').first().waitFor();
    assert((await page.locator('#waiting-dialog').boundingBox()).width<=390);
    await page.keyboard.press('Escape');
    assert.equal(await page.locator('#waiting-dialog').evaluate(e=>e.open),false);
    console.log('Màn hình chờ cùng trang: mở, đóng, giữ biểu mẫu, điện thoại và Escape đều đúng.');
  } finally {if(browser)await browser.close();server.kill();}
}
main().catch(error=>{console.error(error);process.exitCode=1;});
