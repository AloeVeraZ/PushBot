#pragma once

// The entire driver station is compiled into flash. No SPIFFS/LittleFS upload
// or internet connection is required.
static const char DRIVER_STATION_HTML[] PROGMEM = R"PUSHBOT_HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#10151c">
<title>Pushbot Driver Station</title>
<style>
@import url('data:text/css,');
:root{color-scheme:dark;--bg:#0b0f14;--panel:#141b24;--well:#0f151d;--line:#293442;--soft:#1f2a36;--text:#f3f6f9;--muted:#9aa9b8;--faint:#68798a;--cyan:#53d2e8;--green:#4ad295;--amber:#f6bd60;--red:#ff5d6c;--radius:13px;font-family:Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif}
*{box-sizing:border-box}body{margin:0;min-height:100vh;background:radial-gradient(circle at 50% -20%,#1a2a38 0,#0b0f14 42rem);color:var(--text)}button,input{font:inherit}button{color:inherit}.header{height:70px;padding:0 clamp(14px,3vw,34px);display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid var(--line);background:#0e141bcc;backdrop-filter:blur(12px);position:sticky;top:0;z-index:4}.brand{display:flex;align-items:center;gap:12px}.mark{display:grid;place-items:center;width:40px;height:40px;border:1px solid #3a4a5c;border-radius:10px;background:#182330;color:var(--cyan);font-size:21px;font-weight:900}.brand strong{display:block;letter-spacing:.02em}.brand small,.chip small{display:block;color:var(--muted);font-size:11px;text-transform:uppercase;letter-spacing:.12em;margin-top:3px}.chip{text-align:right}.chip strong{font:700 13px ui-monospace,SFMono-Regular,Consolas,monospace;color:var(--cyan)}
.signals{display:grid;grid-template-columns:repeat(4,1fr);border-bottom:1px solid var(--line);background:#0d131a}.signal{padding:10px 16px;border-right:1px solid var(--line);display:grid;grid-template-columns:8px 1fr;column-gap:9px;align-items:center}.signal:last-child{border:0}.signal i{grid-row:1/3;width:8px;height:8px;border-radius:50%;background:var(--faint);box-shadow:0 0 0 3px #68798a22}.signal small{color:var(--faint);font-size:9px;letter-spacing:.11em;text-transform:uppercase}.signal strong{font-size:12px}.signal.good i{background:var(--green);box-shadow:0 0 0 3px #4ad29522}.signal.warn i{background:var(--amber)}.signal.bad i{background:var(--red)}
.lost{display:none;padding:10px 18px;text-align:center;color:#ffdce0;background:#581d27;border-bottom:1px solid #a13847}.lost.show{display:block}.grid{max-width:1280px;margin:auto;padding:18px;display:grid;grid-template-columns:minmax(250px,320px) minmax(380px,1fr) minmax(240px,300px);gap:14px;align-items:start}.column{display:grid;gap:14px}.panel{border:1px solid var(--line);border-radius:var(--radius);background:linear-gradient(180deg,#171f29,#111821);box-shadow:0 12px 35px #0004;overflow:hidden}.panel-head{height:44px;padding:0 14px;display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid var(--line);background:#19232e}.panel-head h2{margin:0;font-size:12px;text-transform:uppercase;letter-spacing:.11em}.panel-head span{color:var(--muted);font:11px ui-monospace,SFMono-Regular,Consolas,monospace}.body{padding:14px}.stack{display:grid;gap:12px}.state{padding:18px;border:1px solid var(--line);border-radius:10px;background:var(--well);text-align:center}.state strong{display:block;font-size:24px;letter-spacing:.09em}.state small{color:var(--muted)}.state.armed{border-color:#277959;background:#123526}.state.armed strong{color:var(--green)}.check{display:flex;gap:9px;align-items:flex-start;color:var(--muted);font-size:12px;line-height:1.4}.check input{margin-top:2px;accent-color:var(--green)}.pair{display:grid;grid-template-columns:1fr 1fr;gap:8px}.command,.stop{min-height:44px;border:1px solid var(--line);border-radius:9px;background:#222d39;font-weight:800;letter-spacing:.06em;cursor:pointer}.command:disabled{opacity:.35;cursor:not-allowed}.enable:not(:disabled){border-color:#358865;background:#1c5c43}.disable{border-color:#77505a}.stop{width:100%;border-color:#bd3d4c;background:#7d2030;color:white}.range{display:grid;gap:7px;color:var(--muted);font-size:12px}.range span{display:flex;justify-content:space-between}.range output{color:var(--cyan);font-family:ui-monospace,monospace}.range input{width:100%;accent-color:var(--cyan)}
.controller{font-size:13px;font-weight:700;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}.axis{display:grid;grid-template-columns:55px 1fr 40px;gap:8px;align-items:center;color:var(--muted);font-size:11px}.axis-track{height:7px;border-radius:9px;background:#24313f;overflow:hidden}.axis-track i{display:block;height:100%;width:0;background:var(--cyan);transition:width 70ms}.axis output{text-align:right;font-family:ui-monospace,monospace}.keys{display:grid;grid-template-columns:repeat(3,48px);gap:6px;justify-content:center}.keys kbd{height:45px;display:grid;place-items:center;border:1px solid #3b4a5b;border-radius:8px;background:#202b37;color:var(--muted);font:800 16px ui-monospace,monospace;box-shadow:inset 0 -3px #111820}.keys kbd.active{border-color:var(--cyan);background:#194454;color:white;transform:translateY(2px);box-shadow:none}.hint{margin:0;color:var(--faint);font-size:11px;line-height:1.5}
.drive-stage{min-height:430px;display:grid;place-items:center;padding:28px;background:linear-gradient(#0e151d99,#0e151d99),repeating-linear-gradient(0deg,transparent 0 34px,#26324055 35px),repeating-linear-gradient(90deg,transparent 0 34px,#26324055 35px)}.robot{width:min(360px,86%);aspect-ratio:1.25;display:grid;grid-template-columns:64px 1fr 64px;gap:18px;filter:drop-shadow(0 20px 25px #0008)}.track{position:relative;border:2px solid #435264;border-radius:18px;background:repeating-linear-gradient(0deg,#17202a 0 17px,#2a3745 18px 25px);overflow:hidden}.track::after{content:"";position:absolute;inset:auto 9px 10px;height:var(--power,0%);max-height:calc(100% - 20px);border-radius:7px;background:linear-gradient(0deg,var(--cyan),#48df9b);box-shadow:0 0 22px #53d2e866;transition:height 80ms}.track.reverse::after{background:linear-gradient(0deg,var(--red),var(--amber))}.chassis{border:2px solid #405064;border-radius:27px;background:linear-gradient(145deg,#263341,#151e28);display:grid;place-items:center;position:relative}.chassis::before{content:"FRONT";position:absolute;top:19px;color:var(--faint);font:700 10px ui-monospace,monospace;letter-spacing:.15em}.arrow{font-size:66px;line-height:1;color:var(--cyan);transform:rotate(var(--angle,0deg));transition:transform 80ms,color 80ms}.readouts{position:absolute;bottom:20px;display:flex;gap:20px}.readouts div{text-align:center}.readouts strong{display:block;font:800 22px ui-monospace,monospace}.readouts small{color:var(--faint);font-size:9px;letter-spacing:.12em}.center-note{padding:12px 14px;border-top:1px solid var(--line);color:var(--muted);font-size:12px;text-align:center}
.metric{display:flex;justify-content:space-between;gap:12px;padding:10px 0;border-bottom:1px solid var(--soft);font-size:12px}.metric:last-child{border:0}.metric span{color:var(--muted)}.metric strong{font-family:ui-monospace,monospace}.safety{padding-left:18px;margin:0;color:var(--muted);font-size:11px;line-height:1.55}.safety li+li{margin-top:6px}.footer{max-width:1280px;margin:0 auto;padding:0 18px 22px;color:var(--faint);font-size:10px;text-align:center}
@media(max-width:980px){.grid{grid-template-columns:280px 1fr}.right{grid-column:1/-1;grid-template-columns:1fr 1fr}.drive-stage{min-height:390px}}@media(max-width:700px){.header{height:62px}.chip{display:none}.signals{grid-template-columns:1fr 1fr}.signal:nth-child(2){border-right:0}.signal:nth-child(-n+2){border-bottom:1px solid var(--line)}.grid{grid-template-columns:1fr;padding:10px}.center{grid-row:1}.right{grid-column:auto;grid-template-columns:1fr}.drive-stage{min-height:330px;padding:18px}.robot{grid-template-columns:48px 1fr 48px;gap:10px}.arrow{font-size:48px}}
</style>
</head>
<body>
<header class="header"><div class="brand"><div class="mark">P</div><div><strong>Pushbot Driver Station</strong><small>ESP32 tank drive</small></div></div><div class="chip"><small>Robot address</small><strong>192.168.4.1</strong></div></header>
<div class="signals">
  <div class="signal" id="sigComms"><i></i><small>Communications</small><strong>Connecting</strong></div>
  <div class="signal" id="sigRobot"><i></i><small>Robot</small><strong>Unknown</strong></div>
  <div class="signal" id="sigInput"><i></i><small>Driver input</small><strong>Keyboard</strong></div>
  <div class="signal" id="sigOutput"><i></i><small>Outputs</small><strong>Stopped</strong></div>
</div>
<div class="lost" id="lost"><strong>Robot connection lost.</strong> Motors have been commanded to stop. Reconnect and arm again.</div>
<main class="grid">
  <aside class="column">
    <section class="panel"><div class="panel-head"><h2>Robot control</h2><span>350 ms watchdog</span></div><div class="body stack">
      <div class="state" id="robotState"><strong>DISABLED</strong><small>Drive commands are blocked</small></div>
      <label class="check"><input id="safe" type="checkbox"><span>The robot is raised or the driving area is clear, and the motor-power cutoff is within reach.</span></label>
      <div class="pair"><button class="command enable" id="arm" disabled>ENABLE</button><button class="command disable" id="disarm">DISABLE</button></div>
      <label class="range"><span>Maximum drive output <output id="speedOut">40%</output></span><input id="speed" type="range" min="10" max="100" step="5" value="40"></label>
      <button class="stop" id="estop">STOP ALL OUTPUTS</button>
    </div></section>
    <section class="panel"><div class="panel-head"><h2>Driver input</h2><span id="inputType">Keyboard</span></div><div class="body stack">
      <div class="controller" id="controller">No game controller detected</div>
      <div class="axis"><span>Throttle</span><div class="axis-track"><i id="throttleBar"></i></div><output id="throttleOut">0.00</output></div>
      <div class="axis"><span>Steering</span><div class="axis-track"><i id="turnBar"></i></div><output id="turnOut">0.00</output></div>
      <div class="keys"><kbd></kbd><kbd data-key="w">W</kbd><kbd></kbd><kbd data-key="a">A</kbd><kbd data-key="s">S</kbd><kbd data-key="d">D</kbd></div>
      <p class="hint">W/S drive forward and backward. A/D turn. Space immediately disables the robot. Gamepad: left stick Y drives and right stick X turns.</p>
    </div></section>
  </aside>
  <div class="column center">
    <section class="panel"><div class="panel-head"><h2>Tank output</h2><span id="driveMode">NEUTRAL</span></div>
      <div class="drive-stage"><div class="robot"><div class="track" id="leftTrack"></div><div class="chassis"><div class="arrow" id="arrow">↑</div><div class="readouts"><div><strong id="leftOut">0</strong><small>LEFT %</small></div><div><strong id="rightOut">0</strong><small>RIGHT %</small></div></div></div><div class="track" id="rightTrack"></div></div></div>
      <div class="center-note">Commands are sent about 20 times per second. Missing commands force a stop.</div>
    </section>
  </div>
  <aside class="column right">
    <section class="panel"><div class="panel-head"><h2>System</h2><span>LIVE</span></div><div class="body">
      <div class="metric"><span>Wi-Fi clients</span><strong id="clients">—</strong></div><div class="metric"><span>Command age</span><strong id="age">—</strong></div><div class="metric"><span>ESP32 uptime</span><strong id="uptime">—</strong></div><div class="metric"><span>Last stop reason</span><strong id="reason">Boot</strong></div>
    </div></section>
    <section class="panel"><div class="panel-head"><h2>Safe operation</h2><span>READ FIRST</span></div><div class="body"><ul class="safety"><li>Test with the wheels raised before placing the robot on the floor.</li><li>Keep a physical motor-power switch or battery disconnect reachable.</li><li>Never power motors from the ESP32. Join only the logic grounds.</li><li>If a side runs backward, change its inversion constants in the sketch.</li></ul></div></section>
  </aside>
</main>
<div class="footer">Pushbot · local control only · no internet connection required</div>
<script>
const $=s=>document.querySelector(s), $$=s=>[...document.querySelectorAll(s)];
const cid=(globalThis.crypto?.randomUUID?.()||Math.random().toString(36).slice(2)+Date.now().toString(36)).slice(0,48);
const held=new Set(); let connected=false, armed=false, owner=false, inFlight=false, queued=false, padIndex=null, lastPadButtons=[];
let command={left:0,right:0,throttle:0,turn:0,source:'Keyboard'};
const clamp=(v,a=-1,b=1)=>Math.max(a,Math.min(b,v));
const dead=v=>Math.abs(v)<.12?0:Math.sign(v)*(Math.abs(v)-.12)/.88;
function signal(id,state,text){const el=$(id);el.className='signal '+state;el.querySelector('strong').textContent=text}
function form(data){return new URLSearchParams(data).toString()}
async function post(path,data={}){const r=await fetch(path,{method:'POST',cache:'no-store',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:form(data)});const out=await r.json().catch(()=>({}));if(!r.ok)throw Error(out.error||`HTTP ${r.status}`);return out}
function updateArmUi(){const live=armed&&owner;$('#robotState').classList.toggle('armed',live);$('#robotState').querySelector('strong').textContent=live?'ENABLED':'DISABLED';$('#robotState').querySelector('small').textContent=live?'Driver commands are live':armed?'Another driver owns control':'Drive commands are blocked';$('#arm').disabled=!connected||!$('#safe').checked||armed;signal('#sigRobot',live?'good':armed?'warn':'',live?'Enabled':armed?'In use':'Disabled')}
function paintCommand(){const l=command.left,r=command.right;$('#leftOut').textContent=Math.round(l*100);$('#rightOut').textContent=Math.round(r*100);for(const [el,v] of [[$('#leftTrack'),l],[$('#rightTrack'),r]]){el.style.setProperty('--power',`${Math.abs(v)*100}%`);el.classList.toggle('reverse',v<0)}$('#throttleBar').style.width=`${Math.abs(command.throttle)*100}%`;$('#turnBar').style.width=`${Math.abs(command.turn)*100}%`;$('#throttleOut').textContent=command.throttle.toFixed(2);$('#turnOut').textContent=command.turn.toFixed(2);$('#inputType').textContent=command.source;signal('#sigInput',command.throttle||command.turn?'good':'',command.source);const mode=Math.abs(l)<.01&&Math.abs(r)<.01?'NEUTRAL':l*r<0?'SPIN':Math.abs(l-r)>.08?'TURNING':l>0?'FORWARD':'REVERSE';$('#driveMode').textContent=mode;$('#arrow').style.setProperty('--angle',`${clamp(command.turn)*55}deg`);$('#arrow').style.color=command.throttle<0?'var(--amber)':'var(--cyan)';signal('#sigOutput',liveOutput()?'good':'',liveOutput()?mode:'Stopped')}
const liveOutput=()=>armed&&owner&&(Math.abs(command.left)>.01||Math.abs(command.right)>.01);
function mix(throttle,turn,source){let left=throttle+turn,right=throttle-turn;const scale=Math.max(1,Math.abs(left),Math.abs(right));const limit=Number($('#speed').value)/100;command={left:left/scale*limit,right:right/scale*limit,throttle,turn,source};paintCommand()}
function keyboardCommand(){const throttle=(held.has('w')?1:0)-(held.has('s')?1:0);const turn=(held.has('a')?1:0)-(held.has('d')?1:0);return {throttle,turn}}
function activePad(){if(typeof navigator.getGamepads!=='function')return null;const pads=[...navigator.getGamepads()].filter(Boolean);if(!pads.length){padIndex=null;return null}const pad=pads.find(p=>p.index===padIndex)||pads[0];padIndex=pad.index;return pad}
function readInput(){const keys=keyboardCommand();const pad=activePad();let throttle=keys.throttle,turn=keys.turn,source='Keyboard';if(!throttle&&!turn&&pad){throttle=-dead(pad.axes[1]||0);turn=-dead((pad.axes.length>2?pad.axes[2]:pad.axes[0])||0);source='Gamepad';$('#controller').textContent=(pad.id||'Game controller').split('(')[0].trim();const buttons=pad.buttons.map(b=>b.pressed);if(buttons[1]&&!lastPadButtons[1])emergencyStop();lastPadButtons=buttons}else if(!pad){$('#controller').textContent=typeof navigator.getGamepads==='function'?'No game controller detected':'Gamepad API unavailable; use WASD'}mix(throttle,turn,source)}
async function sendDrive(){if(!connected||!armed||!owner)return;if(inFlight){queued=true;return}inFlight=true;try{await post('/api/drive',{cid,left:Math.round(command.left*1000),right:Math.round(command.right*1000)})}catch(_){connected=false;armed=false;owner=false;showDisconnected()}finally{inFlight=false;if(queued){queued=false;sendDrive()}}}
async function armRobot(){try{const s=await post('/api/arm',{cid,safe:$('#safe').checked?'1':'0'});applyStatus(s)}catch(e){$('#reason').textContent=e.message;pollStatus()}}
async function stopRobot(){held.clear();paintKeys();mix(0,0,'Keyboard');try{applyStatus(await post('/api/stop',{cid}))}catch(_){showDisconnected()}}
function emergencyStop(){$('#safe').checked=false;stopRobot()}
function paintKeys(){$$('[data-key]').forEach(el=>el.classList.toggle('active',held.has(el.dataset.key)))}
function showDisconnected(){connected=false;armed=false;owner=false;$('#lost').classList.add('show');signal('#sigComms','bad','Disconnected');updateArmUi();paintCommand()}
function applyStatus(s){connected=true;armed=!!s.armed;owner=!!s.owner;$('#lost').classList.remove('show');signal('#sigComms','good','Connected');$('#clients').textContent=s.clients;$('#age').textContent=s.armed?`${s.commandAgeMs} ms`:'—';$('#uptime').textContent=`${Math.floor(s.uptimeMs/60000)}m ${Math.floor(s.uptimeMs/1000)%60}s`;$('#reason').textContent=s.reason||'—';updateArmUi();paintCommand()}
async function pollStatus(){try{const r=await fetch(`/api/status?cid=${encodeURIComponent(cid)}`,{cache:'no-store'});if(!r.ok)throw Error();applyStatus(await r.json())}catch(_){showDisconnected()}}
$('#safe').addEventListener('change',updateArmUi);$('#arm').addEventListener('click',armRobot);$('#disarm').addEventListener('click',stopRobot);$('#estop').addEventListener('click',emergencyStop);$('#speed').addEventListener('input',()=>{$('#speedOut').textContent=$('#speed').value+'%';readInput();sendDrive()});
addEventListener('keydown',e=>{const k=e.key.toLowerCase();if(k===' '){e.preventDefault();emergencyStop();return}if(!'wasd'.includes(k)||e.repeat||['INPUT','BUTTON'].includes(document.activeElement.tagName))return;e.preventDefault();held.add(k);paintKeys();readInput();sendDrive()});
addEventListener('keyup',e=>{const k=e.key.toLowerCase();if(!'wasd'.includes(k))return;held.delete(k);paintKeys();readInput();sendDrive()});
addEventListener('blur',emergencyStop);addEventListener('pagehide',()=>navigator.sendBeacon('/api/stop'));document.addEventListener('visibilitychange',()=>{if(document.hidden)emergencyStop()});addEventListener('gamepaddisconnected',()=>{padIndex=null;emergencyStop()});
setInterval(()=>{readInput();sendDrive()},50);setInterval(pollStatus,300);readInput();pollStatus();
</script>
</body>
</html>
)PUSHBOT_HTML";

