const {readFileSync}=require('node:fs');
const {join}=require('node:path');
const vm=require('node:vm');
const assert=require('node:assert/strict');
const html=readFileSync(join(__dirname,'index.html'),'utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];
let now=0;
const elements=new Map(),intervals=[],timeouts=[],windowEvents={},documentEvents={};
function element(id){
  if(!elements.has(id))elements.set(id,{value:id==='speed'?'35':'',textContent:'',style:{},disabled:false,events:{},
    classList:{toggle(){},add(){},remove(){}},setAttribute(k,v){this[k]=v;},
    getBoundingClientRect(){return {top:0,height:200};},setPointerCapture(){},
    addEventListener(name,fn){(this.events[name]??=[]).push(fn);}});
  return elements.get(id);
}
class Socket{
  static OPEN=1;static all=[];
  constructor(){this.readyState=1;this.bufferedAmount=0;this.sent=[];Socket.all.push(this);}
  send(x){this.sent.push(x);}
  close(){this.readyState=3;this.onclose?.();}
  receive(x){this.onmessage({data:JSON.stringify(x)});}
}
const document={getElementById:element,body:element('body'),hidden:false,
  addEventListener(n,fn){documentEvents[n]=fn;}};
vm.runInNewContext(script,{document,window:{addEventListener(n,fn){windowEvents[n]=fn;}},
  WebSocket:Socket,location:{hostname:'192.168.4.1'},performance:{now:()=>now},
  setInterval:fn=>intervals.push(fn),setTimeout:fn=>timeouts.push(fn),clearTimeout(){},console});
const event=(id,name,args={})=>element(id).events[name].forEach(fn=>fn({preventDefault(){},...args}));
const state=(socket,armed=false,ack=0)=>socket.receive({type:'state',armed,ack,left:0,right:0,uptime:12});
const tick=()=>intervals.forEach(fn=>fn());
let socket=Socket.all.at(-1);
assert.equal(element('enable').disabled,true);
socket.receive({type:'hello',session:123,maxDuty:180});state(socket);
assert.equal(element('enable').disabled,false);
event('enable','click');assert.equal(socket.sent.at(-1),'A,123,1,0,0');
// An older status cannot enable motion before the arm acknowledgement.
state(socket,true,0);event('leftPad','pointerdown',{pointerId:1,clientY:0});tick();
assert.equal(socket.sent.at(-1),'A,123,1,0,0');
state(socket,true,1);
event('leftPad','pointerdown',{pointerId:1,clientY:0});
event('rightPad','pointerdown',{pointerId:2,clientY:200});tick();
assert.equal(socket.sent.at(-1),'D,123,2,35,-35');
event('leftPad','pointerup',{pointerId:1});
assert.equal(socket.sent.at(-1),'D,123,3,0,-35');
event('rightPad','pointercancel',{pointerId:2});assert.equal(socket.sent.at(-1),'D,123,4,0,0');
event('stop','click');assert.match(socket.sent.at(-1),/^S,/);
state(socket,true,4);tick();assert.match(socket.sent.at(-1),/^S,/); // Late armed status cannot undo stop.
event('enable','click');state(socket,true,6);
event('leftPad','keydown',{key:'ArrowUp'});tick();assert.match(socket.sent.at(-1),/,35,0$/);
document.hidden=true;documentEvents.visibilitychange();assert.match(socket.sent.at(-1),/^S,/);
document.hidden=false;
event('enable','click');state(socket,true,9);
now=501;tick();assert.equal(socket.readyState,3);assert.equal(element('enable').disabled,true);
timeouts.at(-1)();socket=Socket.all.at(-1);socket.receive({type:'hello',session:456});state(socket);
tick();assert.equal(socket.sent.length,0); // Reconnect stays disabled.
event('enable','click');state(socket,true,1);
windowEvents.blur();assert.match(socket.sent.at(-1),/^S,/);
event('enable','click');state(socket,false,3);tick();
assert.equal(element('state').textContent,'Robot disabled');
console.log('PASS: neutral arming, two-thumb drive, release/cancel, stop latch, hidden tab, timeout, reconnect and blur');
