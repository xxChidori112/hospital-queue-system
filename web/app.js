'use strict';
const byId = id => document.getElementById(id);
const labels = {'Kham tong quat':'Khám tổng quát','Tim mach':'Tim mạch','Noi tiet':'Nội tiết','Nhi':'Nhi','Chan thuong':'Chấn thương'};
const statuses = {'Dang cho':'Đang chờ','Dang kham':'Đang khám','Da kham xong':'Đã hoàn tất','Vang mat':'Vắng mặt','Da huy':'Đã hủy'};
let state = {patients:[], waiting:[], history:[], departments:[]};
let activeTicket = '';
let soundEnabled = true;
const escapeText = value => String(value ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

function message(text, error = false) {
  const container = byId('toast-container');
  container.replaceChildren();
  const toast = document.createElement('div');
  toast.className = 'pointer-events-auto p-4 rounded-2xl shadow-lg text-sm font-semibold ' + (error ? 'bg-rose-700' : 'bg-sky-700') + ' text-white';
  toast.textContent = text;
  container.appendChild(toast);
  setTimeout(() => toast.remove(), 4500);
}
async function request(url, data) {
  const options = data ? {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'}, body:new URLSearchParams(data)} : {};
  const response = await fetch(url, options);
  const result = await response.json();
  if (!response.ok || result.ok === false) throw new Error(result.message || result.error || 'Không thể thực hiện thao tác.');
  return result;
}
function sound() {
  if (!soundEnabled) return;
  try {
    const Audio = window.AudioContext || window.webkitAudioContext;
    const context = new Audio(), oscillator = context.createOscillator(), volume = context.createGain();
    oscillator.connect(volume); volume.connect(context.destination);
    oscillator.frequency.value = 660; volume.gain.setValueAtTime(.1, context.currentTime);
    volume.gain.exponentialRampToValueAtTime(.001, context.currentTime + .45);
    oscillator.start(); oscillator.stop(context.currentTime + .45);
    oscillator.onended = () => context.close();
  } catch (_) { /* Âm báo phụ thuộc trình duyệt. */ }
}
function fillDepartments(id, allLabel) {
  const select = byId(id), previous = select.value;
  select.innerHTML = '<option value="">'+allLabel+'</option>' + state.departments.map(d => '<option value="'+escapeText(d)+'">'+escapeText(labels[d] || d)+'</option>').join('');
  if ([...select.options].some(option => option.value === previous)) select.value = previous;
}
function selectedPatient() {
  return state.patients.find(p => p.ticket === activeTicket && p.status === 'Dang kham');
}
function render() {
  const patients = state.patients || [], waiting = state.waiting || [];
  byId('kpi-waiting').textContent = waiting.length;
  byId('nav-queue-badge').textContent = waiting.length;
  byId('kpi-emergency').textContent = waiting.filter(p => p.emergency).length;
  byId('kpi-examining').textContent = patients.filter(p => p.status === 'Dang kham').length;
  byId('kpi-total').textContent = patients.length;
  byId('kpi-completed-sub').textContent = 'Đã hoàn tất: '+patients.filter(p => p.status === 'Da kham xong').length+' ca';
  const waits = waiting.map(p => p.waitMinutes).filter(n => typeof n === 'number');
  byId('kpi-wait-avg').textContent = waits.length ? 'Chờ dự kiến trung bình: '+Math.round(waits.reduce((a,b)=>a+b,0)/waits.length)+' phút' : 'Chưa có bệnh nhân chờ';
  fillDepartments('dispatch-dept','Tất cả khoa'); fillDepartments('filter-dept','Tất cả khoa');
  fillDepartments('input-dept','Tự động phân luồng'); fillDepartments('transfer-dept','Chọn khoa');
  fillDepartments('header-department','Tất cả các khoa / phòng khám');
  byId('header-department').value = byId('filter-dept').value;
  const q = byId('filter-search').value.toLocaleLowerCase('vi').trim(), dept = byId('filter-dept').value;
  const priority = byId('filter-priority').value, status = byId('filter-status').value;
  const statusKey = {'Dang cho':'waiting','Dang kham':'examining','Da kham xong':'completed','Vang mat':'absent','Da huy':'cancelled'};
  const shown = patients.filter(p => (!dept || p.department === dept) &&
    (priority === 'ALL' || (priority === 'urgent' ? p.emergency : String(p.triageLevel) === priority)) &&
    (status === 'ALL' || statusKey[p.status] === status) &&
    (p.ticket+' '+p.name+' '+p.symptom+' '+(labels[p.department] || p.department)).toLocaleLowerCase('vi').includes(q));
  byId('queue-count-pill').textContent = shown.length+' / '+patients.length+' hồ sơ';
  byId('queue-empty-state').classList.toggle('hidden', shown.length > 0);
  byId('queue-table-body').innerHTML = shown.map(p => {
    const ticket=escapeText(p.ticket);
    let actions = p.status === 'Dang cho' ? '<button data-action="call" data-ticket="'+ticket+'">Gọi</button> <button data-action="emergency" data-ticket="'+ticket+'">Cấp cứu</button> <button data-action="cancel" data-ticket="'+ticket+'">Hủy phiếu</button>' :
      p.status === 'Dang kham' ? '<button data-action="select" data-ticket="'+ticket+'">Chọn</button> <button data-action="complete" data-ticket="'+ticket+'">Hoàn tất</button>' : '';
    return '<tr><td class="py-3 pl-6 font-bold text-sky-700">'+ticket+'</td><td class="p-3">'+escapeText(p.name)+'<div class="text-xs text-slate-500">'+escapeText(p.symptom)+'</div></td><td class="p-3">'+escapeText(labels[p.department]||p.department)+'</td><td class="p-3">Mức '+p.triageLevel+'</td><td class="p-3">'+escapeText(statuses[p.status]||p.status)+'</td><td class="p-3">'+(p.waitMinutes == null ? '—' : p.waitMinutes+' phút')+'</td><td class="p-3 text-right text-sky-700 font-semibold">'+actions+'</td></tr>';
  }).join('');
  byId('history-list').innerHTML = (state.history || []).map(h => '<div class="px-6 py-3 text-sm"><b>'+escapeText(h.ticket)+' · '+escapeText(h.name)+'</b><span class="text-slate-500"> — '+escapeText(labels[h.department]||h.department)+' · '+escapeText(h.calledAt)+'</span></div>').join('') || '<p class="p-6 text-sm text-slate-500">Chưa có lượt gọi nào.</p>';
  if (!selectedPatient()) {
    const last = (state.history || []).find(h => patients.some(p => p.ticket === h.ticket && p.status === 'Dang kham'));
    activeTicket = last ? last.ticket : '';
  }
  const current = selectedPatient();
  byId('hero-active-code').textContent = current ? current.ticket : '—';
  byId('hero-active-name').textContent = current ? current.name+' ('+current.age+' tuổi)' : 'Chưa có bệnh nhân được chọn';
  byId('hero-active-dept').textContent = current ? 'Khoa '+(labels[current.department]||current.department) : 'Chưa có lượt khám';
  byId('hero-active-room').textContent = '';
  byId('hero-active-status').textContent = current ? 'Đang khám' : 'Chưa có lượt gọi';
  byId('hero-active-priority').textContent = current ? 'Mức '+current.triageLevel : '';
  byId('last-called-text').textContent = current ? current.ticket+' · '+current.name : 'Chưa có lượt gọi';
  byId('last-called-time').textContent = '';
  for (const id of ['hero-active-box','last-called-text','next-preview-name']) byId(id).style.visibility='visible';
  const next=waiting.find(p => !byId('dispatch-dept').value || p.department === byId('dispatch-dept').value);
  byId('next-preview-name').textContent = next ? next.name : 'Không có bệnh nhân đang chờ';
  byId('next-preview-priority').textContent = next ? 'Mức '+next.triageLevel : '—';
  byId('next-preview-meta').textContent = next ? next.ticket+' · '+(labels[next.department]||next.department) : '—';
  byId('next-ticket-preview').textContent = 'Mã phiếu được cấp tự động';
}
async function refresh() {
  try { state = await request('/api/state'); byId('connection-status').textContent='Đã kết nối'; byId('header-connection').textContent='Đã kết nối'; render(); }
  catch (_) { byId('connection-status').textContent='Mất kết nối'; byId('header-connection').textContent='Mất kết nối'; }
}
async function action(url, data, success) {
  try { const result = await request(url, data); if (result.ticket) activeTicket=result.ticket; await refresh(); message(success || result.message); return true; }
  catch (error) { message(error.message,true); return false; }
}
function scrollToId(id){byId(id).scrollIntoView({behavior:'smooth',block:'start'});}
function currentOrWarn() { const p=selectedPatient(); if(!p) message('Chọn bệnh nhân đang khám trong bảng trước.',true); return p; }
async function callAfterUpdate(department) {
  try { const result=await request('/api/call-next',{department}); activeTicket=result.ticket; await refresh(); sound(); message('Đã gọi bệnh nhân tiếp theo.'); }
  catch (error) { await refresh(); if (!error.message.includes('Khong co benh nhan dang cho')) message('Không gọi được lượt tiếp theo: '+error.message, true); }
}

for (const id of ['btn-seed-demo','btn-card-doctors','btn-card-locations']) byId(id).remove();
for (const el of document.querySelectorAll('.symptom-chip')) el.remove();
byId('input-age').max='120';
byId('filter-priority').insertAdjacentHTML('beforeend','<option value="urgent">Cấp cứu (mức 1–2)</option>');
byId('filter-status').insertAdjacentHTML('beforeend','<option value="absent">Vắng mặt</option><option value="cancelled">Đã hủy</option>');
byId('btn-call-next').insertAdjacentHTML('afterend', '<div class="grid grid-cols-2 gap-2 mt-3"><button type="button" id="btn-recall" class="bg-white/20 rounded-xl py-2 text-xs font-bold">Gọi lại</button><button type="button" id="btn-complete" class="bg-white/20 rounded-xl py-2 text-xs font-bold">Hoàn tất khám</button><button type="button" id="btn-absent" class="bg-white/20 rounded-xl py-2 text-xs font-bold">Đánh dấu vắng mặt</button><button type="button" id="btn-emergency" class="bg-white/20 rounded-xl py-2 text-xs font-bold">Ưu tiên cấp cứu</button></div><div class="flex gap-2 mt-3"><select id="transfer-dept" aria-label="Khoa chuyển đến" class="w-full px-3 py-2 rounded-xl text-slate-800 text-xs"><option value="">Chọn khoa</option></select><button type="button" id="btn-transfer-dept" class="bg-white/20 rounded-xl px-3 text-xs font-bold whitespace-nowrap">Chuyển khoa</button></div><p class="text-xs text-sky-100 mt-2">Chọn bệnh nhân đang khám trong bảng để cập nhật lượt.</p>');
byId('btn-emergency').remove();
byId('btn-call-next').addEventListener('click', async () => { if(await action('/api/call-next',{department:byId('dispatch-dept').value},'Đã gọi bệnh nhân tiếp theo.')) sound(); });
byId('btn-recall').addEventListener('click',()=>{const p=currentOrWarn();if(p){sound();message('Mời '+p.name+' ('+p.ticket+') vào khám.');}});
byId('btn-hero-recall').addEventListener('click',()=>byId('btn-recall').click());
byId('btn-complete').addEventListener('click',async()=>{const p=currentOrWarn();if(p && await action('/api/complete',{ticket:p.ticket},'Đã hoàn tất lượt khám.')){activeTicket='';await callAfterUpdate(p.department);}});
byId('btn-absent').addEventListener('click',async()=>{const p=currentOrWarn();if(p && await action('/api/absent',{ticket:p.ticket},'Đã đánh dấu vắng mặt.')){activeTicket='';await callAfterUpdate(p.department);}});
byId('btn-transfer-dept').addEventListener('click',async()=>{const p=currentOrWarn(),department=byId('transfer-dept').value;if(!p)return;if(!department){message('Chọn khoa chuyển đến.',true);return;}if(await action('/api/transfer-department',{ticket:p.ticket,department},'Đã chuyển khoa và đưa vào hàng chờ.')){activeTicket='';await callAfterUpdate(p.department);}});
byId('queue-table-body').addEventListener('click',async e=>{
  const b=e.target.closest('button[data-action]');if(!b)return;
  const ticket=b.dataset.ticket;
  if(b.dataset.action==='select'){activeTicket=ticket;render();message('Đã chọn bệnh nhân '+ticket);return;}
  if(b.dataset.action==='cancel' && !confirm('Hủy phiếu '+ticket+'?'))return;
  const routes={call:'/api/call-specific',emergency:'/api/transfer',cancel:'/api/cancel',complete:'/api/complete'};
  if(await action(routes[b.dataset.action],{ticket,triage:'1'},'Đã cập nhật phiếu '+ticket) && b.dataset.action==='call'){activeTicket=ticket;render();sound();}
});
byId('intake-form').addEventListener('submit',async e=>{
  e.preventDefault();const age=Number(byId('input-age').value);
  if(!Number.isInteger(age)||age<0||age>120){message('Tuổi phải từ 0 đến 120.',true);return;}
  if(await action('/api/register',{name:byId('input-name').value.trim(),age,triage:byId('input-priority').value,symptom:byId('input-symptoms').value.trim(),department:byId('input-dept').value},'Đã tiếp nhận bệnh nhân.')) e.target.reset();
});
for(const id of ['filter-search','filter-dept','filter-priority','filter-status','dispatch-dept']) byId(id).addEventListener('input',render);
byId('header-department').addEventListener('change',()=>{byId('filter-dept').value=byId('header-department').value;byId('dispatch-dept').value=byId('header-department').value;render();});
byId('btn-reset-filters').addEventListener('click',()=>{byId('filter-search').value='';byId('filter-dept').value='';byId('filter-priority').value='ALL';byId('filter-status').value='ALL';render();});
byId('btn-card-filter-urgent').addEventListener('click',()=>{byId('filter-priority').value='urgent';byId('filter-status').value='waiting';render();scrollToId('panel-queue');});
byId('btn-sound-toggle').addEventListener('click',()=>{soundEnabled=!soundEnabled;byId('label-sound').textContent='Âm báo: '+(soundEnabled?'Bật':'Tắt');if(soundEnabled)sound();});
for(const button of document.querySelectorAll('.nav-tab-btn'))button.addEventListener('click',()=>scrollToId(({overview:'hero-section',queue:'panel-queue',intake:'panel-intake',history:'panel-history'})[button.dataset.tab]));
for(const id of ['btn-top-appointment','btn-hero-intake'])byId(id).addEventListener('click',()=>scrollToId('panel-intake'));
byId('btn-hero-queue').addEventListener('click',()=>scrollToId('panel-queue'));
byId('dispatch-dept').addEventListener('change',render);
function clock(){const now=new Date();byId('live-clock').textContent=now.toLocaleTimeString('vi-VN');byId('header-clock').textContent=now.toLocaleString('vi-VN');}
clock();setInterval(clock,1000);refresh();setInterval(refresh,5000);
