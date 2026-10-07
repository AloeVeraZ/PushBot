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
.touch-drive{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.stick-zone{display:grid;gap:8px;min-width:0}.stick-pad{position:relative;display:grid;place-items:center;min-height:180px;overflow:hidden;border:1px solid var(--line);border-radius:10px;background:var(--well);cursor:grab;touch-action:none;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none;-webkit-tap-highlight-color:transparent}.stick-pad.held{cursor:grabbing}.stick-base{display:grid;place-items:center;width:min(120px,calc(100% - 12px));aspect-ratio:1;border:1px solid #435264;border-radius:50%;background:linear-gradient(#435264,#435264) center/1px 100% no-repeat,linear-gradient(#435264,#435264) center/100% 1px no-repeat,#182330;box-shadow:inset 0 2px 12px #0005;transition:transform .15s}.stick-pad[data-axes="y"] .stick-base{width:54px;height:156px;aspect-ratio:auto;border-radius:999px}.stick-pad[data-axes="x"] .stick-base{width:calc(100% - 12px);height:54px;aspect-ratio:auto;border-radius:999px}.stick-knob{width:42px;height:42px;border:1px solid #6a829b;border-radius:50%;background:#35475b;box-shadow:0 6px 14px #0007;transition:transform .12s,background-color .12s;pointer-events:none}.stick-pad.held .stick-base,.stick-pad.held .stick-knob{transition:none}.stick-pad.held .stick-knob{background:var(--cyan);border-color:transparent;box-shadow:0 0 18px #53d2e855}.touch-drive.locked .stick-base{opacity:.45}.touch-drive.locked .stick-pad::after{content:"Enable to drive";position:absolute;bottom:10px;color:var(--faint);font-size:10px;pointer-events:none}.stick-caption{display:grid;gap:3px;text-align:center}.stick-caption strong{font-size:11px;text-transform:uppercase;letter-spacing:.08em}.stick-caption output{color:var(--muted);font:11px ui-monospace,monospace}.stick-pad:focus-visible{outline:2px solid var(--cyan);outline-offset:2px}
@media(max-width:700px){.controls{grid-row:1}.center{grid-row:2}.stick-pad{min-height:186px}.keys{display:none}.drive-stage{min-height:260px}}
@media(hover:none) and (pointer:coarse){.keys{display:none}}
.tabs{display:flex;gap:4px;padding:3px;border:1px solid var(--line);border-radius:10px;background:var(--well)}.tab{min-height:36px;padding:0 14px;border:0;border-radius:7px;background:none;color:var(--muted);font-weight:800;letter-spacing:.05em;cursor:pointer}.tab.active{background:#222d39;color:var(--text);box-shadow:inset 0 0 0 1px #3a4a5c}.tab:focus-visible{outline:2px solid var(--cyan);outline-offset:1px}
[hidden]{display:none!important}.wifi-page{max-width:720px;margin:auto;padding:18px;display:grid;gap:14px}.wifi-result{margin:0;padding:10px 12px;border:1px solid var(--line);border-radius:9px;background:var(--well);color:var(--muted);font-size:12px;line-height:1.45}.net-list{display:grid;gap:6px;max-height:280px;overflow:auto}.net-list:empty{display:none}.net{display:flex;align-items:center;justify-content:space-between;gap:10px;min-height:44px;padding:0 12px;border:1px solid var(--line);border-radius:9px;background:#1a2430;text-align:left;cursor:pointer}.net.selected{border-color:var(--cyan);background:#173442}.net span{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-weight:700}.net small{flex:none;color:var(--muted);font:11px ui-monospace,monospace}.field{display:grid;gap:6px;color:var(--muted);font-size:12px}.field input{min-height:44px;padding:0 12px;border:1px solid var(--line);border-radius:9px;background:var(--well);color:var(--text);font-size:16px}.field input:focus{outline:2px solid var(--cyan);outline-offset:1px}.wifi-page .metric strong{overflow-wrap:anywhere;text-align:right}
@media(max-width:700px){.brand small{display:none}.brand strong{font-size:14px}.tab{padding:0 10px}.wifi-page{padding:10px}}@media(max-width:400px){.brand strong{display:none}}
/* Phones: unwrap the control panels so Stop and both sticks share one screen. */
@media(max-width:700px),(orientation:landscape) and (max-height:540px){.controls,.controls .panel,.controls .body,.touch-drive{display:contents}.controls .panel-head,.controller,.axis{display:none}.grid>.center,.grid>.right,.hint{grid-column:1/-1;grid-row:auto}}
@media(max-width:700px){.grid{grid-template-columns:1fr 1fr;gap:10px}.state,.check,.pair,.range,.stop{grid-column:1/-1}}
@media(orientation:landscape) and (max-height:540px){.header{height:46px}.mark{width:32px;height:32px;font-size:17px}.brand small,.chip,.signals{display:none}.grid{grid-template-columns:minmax(0,1fr) minmax(220px,280px) minmax(0,1fr);gap:8px;padding:8px max(10px,env(safe-area-inset-right)) 8px max(10px,env(safe-area-inset-left))}.state,.check,.pair,.range,.stop{grid-column:2}.state{padding:8px}.state strong{font-size:18px}.keys{display:none}.stick-zone{grid-row:1/span 5;grid-template-rows:1fr auto;align-self:stretch}.stick-zone:first-child{grid-column:1}.stick-zone:last-child{grid-column:3}.stick-pad{min-height:170px}}
</style>
</head>
<body>
<header class="header"><div class="brand"><div class="mark">P</div><div><strong>Pushbot Driver Station</strong><small>ESP32 tank drive</small></div></div><nav class="tabs" aria-label="Pages"><button class="tab active" id="driveTab" aria-pressed="true">Drive</button><button class="tab" id="wifiTab" aria-pressed="false">Wi-Fi</button></nav><div class="chip"><small>Robot address</small><strong id="robotAddress">192.168.4.1</strong></div></header>
<div class="signals">
  <div class="signal" id="sigComms"><i></i><small>Communications</small><strong>Connecting</strong></div>
  <div class="signal" id="sigRobot"><i></i><small>Robot</small><strong>Unknown</strong></div>
  <div class="signal" id="sigInput"><i></i><small>Driver input</small><strong>Keyboard</strong></div>
  <div class="signal" id="sigOutput"><i></i><small>Outputs</small><strong>Stopped</strong></div>
</div>
<div class="lost" id="lost"><strong>Robot connection lost.</strong> Motors have been commanded to stop. Reconnect and arm again.</div>
<main class="grid" id="drivePage">
  <aside class="column controls">
    <section class="panel"><div class="panel-head"><h2>Robot control</h2><span>500 ms watchdog</span></div><div class="body stack">
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
      <div class="touch-drive locked" id="touchDrive">
        <div class="stick-zone"><div class="stick-pad" id="leftStickPad" data-axes="y" role="slider" tabindex="0" aria-label="Forward and reverse drive" aria-valuemin="-100" aria-valuemax="100" aria-valuenow="0" aria-orientation="vertical"><div class="stick-base"><i class="stick-knob"></i></div></div><div class="stick-caption"><strong>Drive</strong><output id="leftStickOut">Forward / reverse</output></div></div>
        <div class="stick-zone"><div class="stick-pad" id="rightStickPad" data-axes="x" role="slider" tabindex="0" aria-label="Turn left and right" aria-valuemin="-100" aria-valuemax="100" aria-valuenow="0" aria-orientation="horizontal"><div class="stick-base"><i class="stick-knob"></i></div></div><div class="stick-caption"><strong>Turn</strong><output id="rightStickOut">Left / right</output></div></div>
      </div>
      <p class="hint">Left stick drives forward/backward; right stick turns. Use both for curves, or turn alone to rotate in place. Touch down, then drag; release to center. Keyboard: W/S drive, A/D turn, Space disables. Gamepad: left stick Y drives, right stick X turns.</p>
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
      <div class="metric"><span>Wi-Fi clients</span><strong id="clients">—</strong></div><div class="metric"><span>Command age</span><strong id="age">—</strong></div><div class="metric"><span>ESP32 uptime</span><strong id="uptime">—</strong></div><div class="metric"><span>Last stop reason</span><strong id="reason">Boot</strong></div><div class="metric"><span>Last reset</span><strong id="reset">—</strong></div>
    </div></section>
    <section class="panel"><div class="panel-head"><h2>Safe operation</h2><span>READ FIRST</span></div><div class="body"><ul class="safety"><li>Test with the wheels raised before placing the robot on the floor.</li><li>Keep a physical motor-power switch or battery disconnect reachable.</li><li>Never power motors from the ESP32. Join only the logic grounds.</li><li>If a side runs backward, change its inversion constants in the sketch.</li></ul></div></section>
  </aside>
</main>
<main class="wifi-page" id="wifiPage" hidden>
  <section class="panel"><div class="panel-head"><h2>Connection</h2><span id="wifiState">—</span></div><div class="body stack">
    <div class="metric"><span>Connected through</span><strong id="wifiVia">—</strong></div>
    <div class="metric"><span>Open the driver station at</span><strong id="wifiAddress">—</strong></div>
    <div class="metric"><span>Saved home network</span><strong id="wifiSaved">None</strong></div>
    <p class="wifi-result" id="wifiResult" role="status">—</p>
  </div></section>
  <section class="panel"><div class="panel-head"><h2>Join a network</h2><span>2.4 GHz only</span></div><div class="body stack">
    <button class="command" id="wifiScan">SCAN FOR NETWORKS</button>
    <div class="net-list" id="wifiList" role="list" aria-label="Networks Pushbot can see"></div>
    <label class="field">Network name<input id="wifiSsid" type="text" maxlength="32" autocomplete="off" autocapitalize="none" spellcheck="false"></label>
    <label class="field">Password<input id="wifiPassword" type="password" maxlength="63" autocomplete="off"></label>
    <label class="check"><input id="wifiShow" type="checkbox"><span>Show password</span></label>
    <div class="pair"><button class="command enable" id="wifiSave">SAVE &amp; CONNECT</button><button class="command disable" id="wifiForget">FORGET</button></div>
    <p class="hint" id="wifiMessage" role="status"></p>
    <p class="hint">Pushbot saves one home network. It joins it at power-on when it is in range, and looks for it again every minute while no one is using its own hotspot. If it can't join, it starts the <strong>Pushbot</strong> hotspot (password pushbot-drive) at http://192.168.4.1/. On the home network, open http://pushbot.local/ or the address above. Opening this page disables the robot.</p>
  </div></section>
</main>
<div class="footer">Pushbot · local control only · no internet connection required</div>
<script>
// Adapted from MotionModule: core/motion_module/static/touch-sticks.js.
/* On-screen thumbsticks for the Driver Station. No network or robot state
   lives here: a stick reports how far it is pushed, from -1 to 1 with up and
   right positive, and the page decides what each direction does.

   The stick centres itself wherever the thumb lands, so touching down never
   moves the robot; only dragging does. Each stick follows one finger, so two
   thumbs work two sticks. A finger that touched down while driving was not
   allowed, or was holding the stick when clear() let it go, does nothing
   until it lifts and touches again. */
function createTouchStick(pad, { enabled, change }) {
  const base = pad.querySelector('.stick-base');
  const knob = pad.querySelector('.stick-knob');
  const clamp = (value, low, high) => Math.min(Math.max(value, low), high);
  const round = value => Math.round(value * 1000) / 1000 || 0;
  let axes = { x: true, y: true };
  let pointer = null;
  let origin = null;
  let travel = null;
  let value = { x: 0, y: 0 };

  const report = next => {
    if (next.x === value.x && next.y === value.y) return;
    value = next;
    change({ ...value });
  };

  function letGo() {
    const released = pointer;
    pointer = null;
    origin = null;
    pad.classList.remove('held');
    if (released !== null) {
      try { pad.releasePointerCapture?.(released); } catch (_) { /* already released */ }
    }
    base.style.transform = '';
    knob.style.transform = '';
    report({ x: 0, y: 0 });
  }

  pad.addEventListener('pointerdown', event => {
    if (event.button > 0 || pointer !== null) return;
    event.preventDefault();
    if (!enabled() || !(axes.x || axes.y)) return;
    const box = pad.getBoundingClientRect();
    const baseBox = base.getBoundingClientRect();
    const knobBox = knob.getBoundingClientRect();
    travel = {
      x: (baseBox.width - knobBox.width) / 2,
      y: (baseBox.height - knobBox.height) / 2,
    };
    if ((axes.x && !(travel.x > 0)) || (axes.y && !(travel.y > 0))) return;
    pointer = event.pointerId;
    origin = { x: event.clientX, y: event.clientY };
    try { pad.setPointerCapture?.(pointer); } catch (_) { /* moves outside the pad are lost */ }
    pad.classList.add('held');
    // The ring moves under the thumb, staying inside the pad.
    const centre = {
      x: clamp(event.clientX, box.left + baseBox.width / 2, box.right - baseBox.width / 2),
      y: clamp(event.clientY, box.top + baseBox.height / 2, box.bottom - baseBox.height / 2),
    };
    base.style.transform =
      `translate(${centre.x - (box.left + box.width / 2)}px, ${centre.y - (box.top + box.height / 2)}px)`;
  });

  pad.addEventListener('pointermove', event => {
    if (event.pointerId !== pointer) return;
    event.preventDefault();
    let dx = axes.x ? event.clientX - origin.x : 0;
    let dy = axes.y ? event.clientY - origin.y : 0;
    let x;
    let y;
    if (axes.x && axes.y) {
      const reach = Math.min(travel.x, travel.y);
      const length = Math.hypot(dx, dy);
      if (length > reach) {
        dx *= reach / length;
        dy *= reach / length;
      }
      x = dx / reach;
      y = -dy / reach;
    } else {
      dx = clamp(dx, -travel.x, travel.x);
      dy = clamp(dy, -travel.y, travel.y);
      x = axes.x ? dx / travel.x : 0;
      y = axes.y ? -dy / travel.y : 0;
    }
    knob.style.transform = `translate(${dx}px, ${dy}px)`;
    report({ x: round(x), y: round(y) });
  });

  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
    pad.addEventListener(type, event => {
      if (event.pointerId === pointer) letGo();
    });
  }
  pad.addEventListener('contextmenu', event => event.preventDefault());

  return {
    get value() { return { ...value }; },
    // Which ways this stick moves: { x, y }. Changing it lets go.
    configure(next) {
      axes = { x: Boolean(next.x), y: Boolean(next.y) };
      pad.dataset.axes = `${axes.x ? 'x' : ''}${axes.y ? 'y' : ''}`;
      letGo();
    },
    clear: letGo,
  };
}

const $=s=>document.querySelector(s), $$=s=>[...document.querySelectorAll(s)];
const cid=(globalThis.crypto?.randomUUID?.()||Math.random().toString(36).slice(2)+Date.now().toString(36)).slice(0,48);
const held=new Set(), sticks={}, stickMoving={left:false,right:false};
let connected=false, armed=false, owner=false, inFlight=false, queued=false, padIndex=null, lastPadButtons=[];
let pendingStop=false, arming=false, driveAuthorized=false, epoch=0, awaitingNeutral=true, touchSelected=false, suppressInput=false, wifiPageOpen=false, wifiTimer=null;
let command={left:0,right:0,throttle:0,turn:0,source:'Ready'};
const clamp=(v,a=-1,b=1)=>Math.max(a,Math.min(b,v));
const dead=v=>!Number.isFinite(v)||Math.abs(v)<.12?0:Math.sign(v)*(Math.abs(v)-.12)/.88;
const canDrive=()=>connected&&armed&&owner&&driveAuthorized&&!pendingStop&&!arming&&!wifiPageOpen;
const typingInField=e=>{const t=e.target;return !!t&&(t.tagName==='TEXTAREA'||(t.tagName==='INPUT'&&!['checkbox','range','button'].includes(t.type)))};
function signal(id,state,text){const el=$(id);el.className='signal '+state;el.querySelector('strong').textContent=text}
function form(data){return new URLSearchParams(data).toString()}
async function request(path,options={}){
  const abort=new AbortController(), timer=setTimeout(()=>abort.abort(),750);
  try{const r=await fetch(path,{...options,cache:'no-store',signal:abort.signal});const out=await r.json();if(!r.ok)throw Error(out.error||`HTTP ${r.status}`);return out}
  finally{clearTimeout(timer)}
}
const post=(path,data={})=>request(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:form(data)});
function updateArmUi(){
  const live=canDrive();$('#robotState').classList.toggle('armed',live);
  $('#robotState').querySelector('strong').textContent=live?'ENABLED':'DISABLED';
  $('#robotState').querySelector('small').textContent=live?'Driver commands are live':arming?'Enabling at zero output':armed?'Waiting for robot to stop':'Drive commands are blocked';
  $('#arm').disabled=!connected||!$('#safe').checked||armed||pendingStop||arming;
  $('#touchDrive').classList.toggle('locked',!live);
  for(const side of ['left','right'])$(`#${side}StickPad`).setAttribute('aria-disabled',String(!live));
  signal('#sigRobot',live?'good':armed?'warn':'',live?'Enabled':armed?'In use':'Disabled');
}
function paintCommand(){const l=command.left,r=command.right;$('#leftOut').textContent=Math.round(l*100);$('#rightOut').textContent=Math.round(r*100);for(const [el,v] of [[$('#leftTrack'),l],[$('#rightTrack'),r]]){el.style.setProperty('--power',`${Math.abs(v)*100}%`);el.classList.toggle('reverse',v<0)}$('#throttleBar').style.width=`${Math.abs(command.throttle)*100}%`;$('#turnBar').style.width=`${Math.abs(command.turn)*100}%`;$('#throttleOut').textContent=command.throttle.toFixed(2);$('#turnOut').textContent=command.turn.toFixed(2);$('#inputType').textContent=command.source;signal('#sigInput',command.throttle||command.turn?'good':'',command.source);const mode=Math.abs(l)<.01&&Math.abs(r)<.01?'NEUTRAL':l*r<0?'SPIN':Math.abs(l-r)>.08?'TURNING':l>0?'FORWARD':'REVERSE';$('#driveMode').textContent=mode;$('#arrow').style.setProperty('--angle',`${clamp(command.turn)*55}deg`);$('#arrow').style.color=command.throttle<0?'var(--amber)':'var(--cyan)';signal('#sigOutput',canDrive()&&(l||r)?'good':'',canDrive()&&(l||r)?mode:'Stopped')}
function mix(throttle,turn,source){let left=throttle+turn,right=throttle-turn;const scale=Math.max(1,Math.abs(left),Math.abs(right)),limit=Number($('#speed').value)/100;command={left:left/scale*limit,right:right/scale*limit,throttle,turn,source};paintCommand()}
function paintKeys(){$$('[data-key]').forEach(el=>el.classList.toggle('active',held.has(el.dataset.key)))}
function clearTouchInputs(){suppressInput=true;try{Object.values(sticks).forEach(stick=>stick.clear())}finally{suppressInput=false}}
function clearInputs(){held.clear();paintKeys();clearTouchInputs();queued=false;awaitingNeutral=true;mix(0,0,'Ready')}
function keyboardCommand(){return {throttle:(held.has('w')?1:0)-(held.has('s')?1:0),turn:(held.has('d')?1:0)-(held.has('a')?1:0)}}
function activePad(){
  if(typeof navigator.getGamepads!=='function')return null;
  let pads;try{pads=[...navigator.getGamepads()].filter(Boolean)}catch(_){return null}
  if(!pads.length){padIndex=null;return null}const pad=pads.find(p=>p.index===padIndex)||pads[0];padIndex=pad.index;return pad;
}
function readInput(){
  const pad=activePad(),buttons=pad?pad.buttons.map(b=>b.pressed):[];
  const stopPressed=buttons[1]&&!lastPadButtons[1];lastPadButtons=buttons;
  $('#controller').textContent=pad?(pad.id||'Game controller').split('(')[0].trim():'Touch joysticks or WASD';
  if(stopPressed){emergencyStop();return}
  if(!canDrive()){mix(0,0,'Ready');return}
  const keys=keyboardCommand();let throttle=keys.throttle,turn=keys.turn,source='Keyboard';
  if(!throttle&&!turn){
    if(touchSelected){throttle=dead(sticks.left.value.y);turn=dead(sticks.right.value.x);source='Touch sticks'}
    else if(pad){throttle=-dead(pad.axes[1]||0);turn=dead((pad.axes.length>2?pad.axes[2]:pad.axes[0])||0);source='Gamepad'}
  }
  // Deflected gamepads never inherit an Enable/reconnect. Center before driving.
  if(awaitingNeutral){if(!throttle&&!turn)awaitingNeutral=false;mix(0,0,source);return}
  mix(throttle,turn,source);
}
async function sendDrive(){
  if(!canDrive())return;if(inFlight){queued=true;return}inFlight=true;const token=epoch;
  try{await post('/api/drive',{cid,left:Math.round(command.left*1000),right:Math.round(command.right*1000)})}
  catch(_){if(token===epoch)showDisconnected()}
  finally{inFlight=false;if(queued){queued=false;sendDrive()}}
}
async function armRobot(){
  if(!connected||!$('#safe').checked||armed||pendingStop||arming)return;
  const token=++epoch;arming=true;driveAuthorized=false;clearInputs();updateArmUi();
  try{const s=await post('/api/arm',{cid,safe:'1'});if(token!==epoch)return;arming=false;driveAuthorized=!!s.armed&&!!s.owner;applyStatus(s);readInput();sendDrive()}
  catch(e){if(token!==epoch)return;arming=false;pendingStop=true;clearInputs();updateArmUi();$('#reason').textContent=e.message;pollStatus()}
}
async function stopRobot(){
  const token=++epoch;pendingStop=true;arming=false;driveAuthorized=false;armed=false;owner=false;clearInputs();updateArmUi();
  try{const s=await post('/api/stop',{cid});if(token===epoch)applyStatus(s)}catch(_){if(token===epoch)showDisconnected()}
}
function emergencyStop(){$('#safe').checked=false;stopRobot()}
function showDisconnected(){++epoch;connected=false;armed=false;owner=false;driveAuthorized=false;arming=false;clearInputs();$('#lost').classList.add('show');signal('#sigComms','bad','Disconnected');updateArmUi()}
function applyStatus(s){
  connected=true;armed=!!s.armed;owner=!!s.owner;
  if(!armed){pendingStop=false;driveAuthorized=false}
  if(!owner)driveAuthorized=false;
  $('#lost').classList.remove('show');signal('#sigComms','good','Connected');$('#clients').textContent=s.clients;$('#age').textContent=s.armed?`${s.commandAgeMs} ms`:'—';$('#uptime').textContent=`${Math.floor(s.uptimeMs/60000)}m ${Math.floor(s.uptimeMs/1000)%60}s`;$('#reason').textContent=s.reason||'—';$('#reset').textContent=s.reset||'—';
  if(!canDrive())clearInputs();updateArmUi();paintCommand();
}
async function pollStatus(){const token=epoch;try{const s=await request(`/api/status?cid=${encodeURIComponent(cid)}`);if(token===epoch&&!arming)applyStatus(s)}catch(_){if(token===epoch)showDisconnected()}}
for(const side of ['left','right']){
  const pad=$(`#${side}StickPad`);
  pad.addEventListener('pointerdown',event=>{if(canDrive()&&event.button<=0){touchSelected=true;held.clear();paintKeys()}});
  pad.addEventListener('pointercancel',()=>{if(canDrive())emergencyStop()});
  pad.addEventListener('lostpointercapture',()=>{if(canDrive()&&pad.classList.contains('held'))emergencyStop()});
  sticks[side]=createTouchStick(pad,{
    enabled:canDrive,
    change:next=>{
      const moving=!!(next.x||next.y),edge=moving!==stickMoving[side];stickMoving[side]=moving;
      const value=side==='left'?next.y:next.x;
      $(`#${side}StickOut`).textContent=value.toFixed(2);pad.setAttribute('aria-valuenow',String(Math.round(value*100)));
      if(!suppressInput){readInput();if(edge)sendDrive()}
    },
  });
  sticks[side].configure({x:side==='right',y:side==='left'});
  const keyMap=side==='left'?{ArrowUp:'w',ArrowDown:'s'}:{ArrowLeft:'a',ArrowRight:'d'};
  pad.addEventListener('keydown',event=>{const key=keyMap[event.key];if(!key||event.repeat||!canDrive())return;event.preventDefault();clearTouchInputs();touchSelected=false;held.add(key);paintKeys();readInput();sendDrive()});
  pad.addEventListener('keyup',event=>{const key=keyMap[event.key];if(!key)return;event.preventDefault();held.delete(key);paintKeys();readInput();sendDrive()});
}
$('#safe').addEventListener('change',updateArmUi);$('#arm').addEventListener('click',armRobot);$('#disarm').addEventListener('click',stopRobot);$('#estop').addEventListener('click',emergencyStop);$('#speed').addEventListener('input',()=>{$('#speedOut').textContent=$('#speed').value+'%';readInput();sendDrive()});
addEventListener('keydown',e=>{if(typingInField(e))return;const k=e.key.toLowerCase();if(k===' '){e.preventDefault();emergencyStop();return}if(!['w','a','s','d'].includes(k)||e.repeat||!canDrive()||['INPUT','BUTTON'].includes(document.activeElement.tagName))return;e.preventDefault();clearTouchInputs();touchSelected=false;held.add(k);paintKeys();readInput();sendDrive()});
addEventListener('keyup',e=>{const k=e.key.toLowerCase();if(!['w','a','s','d'].includes(k))return;held.delete(k);paintKeys();readInput();sendDrive()});
addEventListener('blur',emergencyStop);addEventListener('pagehide',()=>{++epoch;driveAuthorized=false;armed=false;owner=false;clearInputs();navigator.sendBeacon('/api/stop')});document.addEventListener('visibilitychange',()=>{if(document.hidden)emergencyStop()});addEventListener('gamepaddisconnected',()=>{padIndex=null;emergencyStop()});addEventListener('gamepadconnected',()=>{if(!$('#leftStickPad').classList.contains('held')&&!$('#rightStickPad').classList.contains('held'))touchSelected=false});
addEventListener('resize',()=>{if(canDrive())emergencyStop();else clearInputs()});
// Wi-Fi page: pick and save the home network. Opening it disables the robot.
function wifiMessage(text){$('#wifiMessage').textContent=text}
function paintWifi(w){
  const states={home:'HOME WI-FI',hotspot:'HOTSPOT',joining:'JOINING',failed:'WI-FI FAILED',starting:'STARTING'};
  $('#wifiState').textContent=states[w.state]||String(w.state||'—').toUpperCase();
  $('#wifiVia').textContent=w.state==='home'?`${w.saved} (${w.rssi} dBm)`:w.hotspot?`${w.hotspotName} hotspot`:'—';
  const addresses=[];if(w.ip)addresses.push(`http://${w.ip}/`,`http://${w.hostname}/`);if(w.hotspot)addresses.push(`http://${w.hotspotIp}/ on ${w.hotspotName}`);
  $('#wifiAddress').textContent=addresses.join('  ·  ')||'—';
  $('#wifiSaved').textContent=w.saved||'None';$('#wifiResult').textContent=w.result||'—';
  if(!$('#wifiSsid').value&&w.saved&&document.activeElement!==$('#wifiSsid'))$('#wifiSsid').value=w.saved;
}
async function loadWifi(){try{paintWifi(await request('/api/wifi'))}catch(_){$('#wifiResult').textContent='Pushbot is not reachable right now. If it just changed networks, join that network and open its address.'}}
function showPage(page){
  wifiPageOpen=page==='wifi';
  if(wifiPageOpen&&(armed||arming))stopRobot();
  clearInputs();updateArmUi();
  $('#drivePage').hidden=wifiPageOpen;$('#wifiPage').hidden=!wifiPageOpen;
  for(const [id,on] of [['#driveTab',!wifiPageOpen],['#wifiTab',wifiPageOpen]]){$(id).classList.toggle('active',on);$(id).setAttribute('aria-pressed',String(on))}
  clearInterval(wifiTimer);wifiTimer=null;
  if(wifiPageOpen){loadWifi();wifiTimer=setInterval(loadWifi,2000)}
}
const signalBars=rssi=>rssi>=-55?'▂▄▆█':rssi>=-67?'▂▄▆':rssi>=-78?'▂▄':'▂';
function renderNetworks(list){
  const box=$('#wifiList');box.replaceChildren();
  for(const n of list){
    const b=document.createElement('button'),name=document.createElement('span'),info=document.createElement('small');
    b.type='button';b.className='net';b.setAttribute('role','listitem');name.textContent=n.ssid;info.textContent=`${n.secure?'🔒 ':''}${signalBars(n.rssi)} ${n.rssi} dBm`;b.append(name,info);
    b.addEventListener('click',()=>{box.querySelectorAll('.net').forEach(x=>x.classList.remove('selected'));b.classList.add('selected');$('#wifiSsid').value=n.ssid;$('#wifiPassword').value='';if(n.secure){$('#wifiPassword').focus();wifiMessage(`Enter the password for ${n.ssid}.`)}else wifiMessage('Open network: no password needed.')});
    box.append(b);
  }
}
async function scanWifi(){
  const button=$('#wifiScan');if(button.disabled)return;button.disabled=true;wifiMessage('Scanning… the Pushbot hotspot may pause for a few seconds.');
  try{
    await post('/api/wifi/scan');
    for(let i=0;i<30;i++){
      await new Promise(resolve=>setTimeout(resolve,700));
      let out;try{out=await request('/api/wifi/networks')}catch(_){continue}
      if(!out.scanning){const list=out.networks||[];renderNetworks(list);wifiMessage(list.length?'Tap your network, then enter its password.':'No networks found. Pushbot can only see 2.4 GHz networks.');return}
    }
    wifiMessage('The scan took too long. Try again.');
  }catch(e){wifiMessage(e.message)}
  finally{button.disabled=false}
}
async function saveWifi(){
  const ssid=$('#wifiSsid').value,password=$('#wifiPassword').value;
  if(!ssid.trim()){wifiMessage('Choose or type a network name.');return}
  if(password&&(password.length<8||password.length>63)){wifiMessage('Wi-Fi passwords are 8 to 63 characters.');return}
  $('#wifiSave').disabled=true;
  try{const w=await post('/api/wifi/save',{ssid,password});$('#wifiPassword').value='';paintWifi(w);wifiMessage(`Saved. Pushbot is joining ${ssid}. If it connects, its address appears above and the Pushbot hotspot stays on for one more minute. Then join ${ssid} on this device and open that address.`)}
  catch(e){wifiMessage(e.message)}
  finally{$('#wifiSave').disabled=false}
}
async function forgetWifi(){
  if(!confirm('Forget the saved network? Pushbot will use only its own hotspot.'))return;
  try{paintWifi(await post('/api/wifi/forget'));$('#wifiSsid').value='';wifiMessage('Forgotten. Pushbot now uses its own hotspot. Join Pushbot to reconnect.')}catch(e){wifiMessage(e.message)}
}
$('#driveTab').addEventListener('click',()=>showPage('drive'));$('#wifiTab').addEventListener('click',()=>showPage('wifi'));
$('#wifiScan').addEventListener('click',scanWifi);$('#wifiSave').addEventListener('click',saveWifi);$('#wifiForget').addEventListener('click',forgetWifi);
$('#wifiShow').addEventListener('change',()=>{$('#wifiPassword').type=$('#wifiShow').checked?'text':'password'});
if(globalThis.location?.host)$('#robotAddress').textContent=location.host;
setInterval(()=>{readInput();sendDrive()},50);setInterval(pollStatus,300);updateArmUi();readInput();pollStatus();
</script>
</body>
</html>
)PUSHBOT_HTML";

