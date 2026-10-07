const {readFileSync}=require('node:fs');
const {join}=require('node:path');
const vm=require('node:vm');
const assert=require('node:assert/strict');
const source=readFileSync(join(__dirname,'firmware/Pushbot/driver_station.h'),'utf8').match(/<script>([\s\S]*?)<\/script>/)[1];
const settle=()=>new Promise(resolve=>setImmediate(resolve));

function harness(){
  const elements=new Map(),windowEvents={},documentEvents={},intervals=[],timers=new Map(),requests=[],delayed=[];
  let pads=[],delayPath=null,timerId=0;
  const state={armed:false,owner:false,clients:1,commandAgeMs:0,uptimeMs:1000,left:0,right:0,reason:'Boot'};
  function element(id){
    if(elements.has(id))return elements.get(id);
    const classes=new Set(),children=new Map();
    const el={id,value:id==='speed'?'40':'',checked:false,textContent:'',disabled:false,dataset:{},events:{},
      tagName:'DIV',style:{setProperty(k,v){this[k]=v}},
      classList:{add(c){classes.add(c)},remove(c){classes.delete(c)},contains(c){return classes.has(c)},toggle(c,on){if(on??!classes.has(c))classes.add(c);else classes.delete(c)}},
      addEventListener(name,fn){(this.events[name]??=[]).push(fn)},
      querySelector(selector){if(!children.has(selector))children.set(selector,element(id+selector));return children.get(selector)},
      setAttribute(name,value){this[name]=value},
      getBoundingClientRect(){
        const left=id.startsWith('right')?160:0;
        const width=id.endsWith('.stick-base')?(id.startsWith('left')?54:122):id.endsWith('.stick-knob')?42:134;
        const height=id.endsWith('.stick-base')?(id.startsWith('left')?156:54):id.endsWith('.stick-knob')?42:180;
        return {left,top:0,width,height,right:left+width,bottom:height};
      },
      setPointerCapture(pointer){this.captured=pointer},
      releasePointerCapture(pointer){if(this.captured!==pointer)return;this.captured=null;dispatch(id,'lostpointercapture',{pointerId:pointer})},
    };
    elements.set(id,el);return el;
  }
  function dispatch(id,type,data={}){return Promise.all((element(id).events[type]||[]).map(fn=>fn({button:0,preventDefault(){},...data})));}
  const document={hidden:false,activeElement:{tagName:'BODY'},querySelector:s=>element(s.slice(1)),querySelectorAll:()=>[],addEventListener(n,fn){documentEvents[n]=fn}};
  function response(snapshot){return {ok:true,status:200,json:async()=>({...snapshot})};}
  const fetch=async(path,options={})=>{
    const name=path.split('?')[0],data=Object.fromEntries(new URLSearchParams(options.body||''));
    requests.push({path:name,data,method:options.method||'GET'});
    if(name==='/api/arm')Object.assign(state,{armed:true,owner:true,left:0,right:0,reason:'Enabled by driver'});
    if(name==='/api/stop')Object.assign(state,{armed:false,owner:false,left:0,right:0,reason:'Stopped by driver'});
    if(name==='/api/drive'){assert.equal(state.armed,true);state.left=Number(data.left)/1000;state.right=Number(data.right)/1000;}
    const snapshot={...state};
    if(name!==delayPath)return response(snapshot);
    delayPath=null;
    return new Promise((resolve,reject)=>{
      delayed.push({finish:()=>resolve(response(snapshot)),signal:options.signal});
      options.signal.addEventListener('abort',()=>reject(Error('aborted')));
    });
  };
  const context=vm.createContext({document,navigator:{getGamepads:()=>pads,sendBeacon(path){requests.push({path,beacon:true});Object.assign(state,{armed:false,owner:false})}},
    addEventListener(n,fn){windowEvents[n]=fn},fetch,URLSearchParams,AbortController,console,crypto:{randomUUID:()=> 'test-client-123'},
    setInterval(fn,ms){intervals.push({fn,ms});return intervals.length},clearInterval(){},setTimeout(fn,ms){const id=++timerId;timers.set(id,{fn,ms});return id},clearTimeout(id){timers.delete(id)}});
  vm.runInContext(source,context);
  return {element,dispatch,state,requests,delayed,windowEvents,documentEvents,document,
    value:expression=>vm.runInContext(expression,context),
    delayNext:path=>{delayPath=path},pads:value=>{pads=value},
    async arm(){await settle();element('safe').checked=true;await dispatch('arm','click');await settle();assert.equal(this.value('canDrive()'),true)},
    async tick(){intervals.find(x=>x.ms===50).fn();await settle()},
    async poll(){await vm.runInContext('pollStatus()',context);await settle()},
    expireRequests(){[...timers.values()].forEach(x=>x.fn())},
    drive(){const req=requests.filter(x=>x.path==='/api/drive').at(-1);return req?[Number(req.data.left),Number(req.data.right)]:null},
  };
}

(async()=>{
  const h=harness();await settle();
  assert.equal(h.element('touchDrive')['classList'].contains('locked'),true);
  // A finger put down while disabled is ignored even after Enable.
  await h.dispatch('leftStickPad','pointerdown',{pointerId:1,clientX:67,clientY:90});
  await h.arm();await h.dispatch('leftStickPad','pointermove',{pointerId:1,clientX:67,clientY:33});await h.tick();assert.deepEqual(h.drive(),[0,0]);
  // Touch-down itself stays neutral; dragging commands proportional movement.
  await h.dispatch('leftStickPad','pointerdown',{pointerId:2,clientX:67,clientY:90});await h.tick();assert.deepEqual(h.drive(),[0,0]);
  await h.dispatch('leftStickPad','pointermove',{pointerId:2,clientX:67,clientY:33});await settle();assert.deepEqual(h.drive(),[400,400]);
  await h.dispatch('rightStickPad','pointerdown',{pointerId:3,clientX:227,clientY:90});
  await h.dispatch('rightStickPad','pointermove',{pointerId:3,clientX:267,clientY:90});await settle();assert.deepEqual(h.drive(),[400,0]);
  // Third fingers cannot steal either active stick.
  await h.dispatch('leftStickPad','pointerdown',{pointerId:4,clientX:67,clientY:90});
  await h.dispatch('leftStickPad','pointermove',{pointerId:4,clientX:67,clientY:147});await h.tick();assert.deepEqual(h.drive(),[400,0]);
  const stops=h.requests.filter(x=>x.path==='/api/stop').length;
  await h.dispatch('leftStickPad','pointerup',{pointerId:2});await settle();assert.deepEqual(h.drive(),[400,-400]);
  await h.dispatch('rightStickPad','pointerup',{pointerId:3});await settle();assert.deepEqual(h.drive(),[0,0]);
  assert.equal(h.requests.filter(x=>x.path==='/api/stop').length,stops,'normal capture release must not disable');
  assert.equal(h.element('leftStickPad')['aria-valuenow'],'0');
  // Touch remains the selected source on release, without inheriting a gamepad.
  h.pads([{index:0,id:'test pad',axes:[0,-1,0],buttons:[]}]);await h.tick();assert.deepEqual(h.drive(),[0,0]);h.pads([]);
  await h.dispatch('leftStickPad','pointerdown',{pointerId:5,clientX:67,clientY:90});
  await h.dispatch('leftStickPad','pointermove',{pointerId:5,clientX:67,clientY:147});await settle();assert.deepEqual(h.drive(),[-400,-400]);
  await h.dispatch('leftStickPad','pointercancel',{pointerId:5});await settle();assert.equal(h.value('canDrive()'),false);assert.equal(h.state.armed,false);
  await h.arm();await h.dispatch('rightStickPad','pointerdown',{pointerId:6,clientX:227,clientY:90});
  await h.dispatch('rightStickPad','pointermove',{pointerId:6,clientX:187,clientY:90});await settle();assert.deepEqual(h.drive(),[-400,400]);
  await h.dispatch('rightStickPad','lostpointercapture',{pointerId:6});await settle();assert.equal(h.state.armed,false);

  // Stale armed polling must never reverse a stop latch.
  await h.arm();h.delayNext('/api/status');const oldPoll=h.poll();await settle();
  await h.dispatch('estop','click');await settle();h.delayed.at(-1).finish();await oldPoll;
  assert.equal(h.value('canDrive()'),false);assert.equal(h.element('safe').checked,false);
  // A delayed Enable acknowledgement after Stop cannot authorize the UI.
  h.element('safe').checked=true;h.delayNext('/api/arm');const oldArm=h.dispatch('arm','click');await settle();
  await h.dispatch('estop','click');h.delayed.at(-1).finish();await oldArm;await settle();
  Object.assign(h.state,{armed:true,owner:true});await h.poll();assert.equal(h.value('canDrive()'),false);
  Object.assign(h.state,{armed:false,owner:false});await h.poll();
  await h.arm();h.document.hidden=true;h.documentEvents.visibilitychange();await settle();assert.equal(h.state.armed,false);h.document.hidden=false;
  await h.arm();h.windowEvents.blur();await settle();assert.equal(h.state.armed,false);
  await h.arm();h.windowEvents.resize();await settle();assert.equal(h.state.armed,false);
  // Opening the Wi-Fi page disables the robot and locks driving until Drive is reopened.
  await h.arm();await h.dispatch('wifiTab','click');await settle();assert.equal(h.state.armed,false);assert.equal(h.value('canDrive()'),false);
  assert.equal(h.element('drivePage').hidden,true);assert.equal(h.element('wifiPage').hidden,false);
  await h.dispatch('driveTab','click');await settle();assert.equal(h.element('drivePage').hidden,false);
  // A space typed into a text field (a Wi-Fi password) is not the Stop shortcut.
  await h.arm();h.windowEvents.keydown({key:' ',target:{tagName:'INPUT',type:'password'},preventDefault(){}});await settle();assert.equal(h.state.armed,true);
  h.windowEvents.keydown({key:' ',preventDefault(){}});await settle();assert.equal(h.state.armed,false);

  // Deflected gamepad waits for neutral after Enable, then right means right.
  const g=harness();await settle();g.pads([{index:0,id:'test pad',axes:[0,-1,0],buttons:[]}]);await g.arm();await g.tick();assert.deepEqual(g.drive(),[0,0]);
  g.pads([{index:0,id:'test pad',axes:[0,0,0],buttons:[]}]);await g.tick();
  g.pads([{index:0,id:'test pad',axes:[0,0,1],buttons:[]}]);await g.tick();assert.deepEqual(g.drive(),[400,-400]);
  g.windowEvents.keydown({key:'a',repeat:false,preventDefault(){}});await settle();assert.deepEqual(g.drive(),[-400,400]);
  g.windowEvents.keydown({key:' ',preventDefault(){}});await settle();assert.equal(g.state.armed,false);
  // A stalled HTTP command clears controls and requires a new explicit Enable.
  g.pads([]);await g.arm();g.delayNext('/api/drive');const pending=g.tick();await settle();g.expireRequests();await pending;await settle();assert.equal(g.value('canDrive()'),false);
  Object.assign(g.state,{armed:true,owner:true});await g.poll();assert.equal(g.value('canDrive()'),false);
  g.windowEvents.pagehide();assert.equal(g.requests.at(-1).beacon,true);assert.equal(g.value('canDrive()'),false);
  console.log('PASS: neutral arming, two-thumb arcs/pivots/reverse, release, multi-touch ownership, cancellation/capture loss, stop latch, stale replies, visibility/blur, gamepad neutral gate, steering signs, timeout and reconnect, Wi-Fi page lockout, typing in fields');
})().catch(error=>{console.error(error);process.exitCode=1});
