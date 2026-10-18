// Executes the production fetch probe generated from EM_JS. It must read
// headers then abort before media, even if a server ignores Range.
const fs = require('node:fs');
const assert = require('node:assert/strict');
const source = fs.readFileSync('src/rede.c', 'utf8');
const iniciar = source.match(/EM_JS\(int, nv_http_iniciar_assinc,[\s\S]*?, \{([\s\S]*?)\n\}\);/)[1].replace(/\\\\/g,'\\');
const colher = source.match(/EM_JS\(char \*, nv_http_colher_assinc,[\s\S]*?, \{([\s\S]*?)\n\}\);/)[1];

function make(resposta) {
  const ints = new Int32Array(32), bytes = new Uint8Array(256);
  const Module = {};
  let headers, reads = 0;
  const start = new Function('Module','UTF8ToString','HEAP32','HEAPU8','setTimeout','clearTimeout','Date','AbortController','fetch','Uint8Array',
    'metodo','url','cabs','corpo','prazo','teto','modo',iniciar);
  const harvest = new Function('Module','UTF8ToString','HEAP32','HEAPU8','stringToUTF8','_malloc',
    'id','tam','status','urlFinal','urlFinalTam','etag','etagTam','retryAfter','limitado','mime','mimeTam','tamanho','modo',colher);
  const result = {final:'',mime:''};
  const id = start(Module,x=>x,ints,bytes,setTimeout,clearTimeout,Date,AbortController,
    async(url,init)=>{headers=init.headers;return {status:resposta.status,url:resposta.url,
      headers:{get:k=>(resposta.h||{})[k]||null},body:{getReader(){reads++;throw Error('must not read media');}}}},
    Uint8Array,'GET','https://fixture.invalid/',resposta.cabs||'Range: bytes=0-63',0,1000,64,1);
  return new Promise(resolve=>setTimeout(()=>resolve({id,headers,reads,
    value:harvest(Module,x=>x,ints,bytes,(x,p)=>{if(p===8)result.final=x;else if(p===12)result.mime=x;},()=>128,
      id,0,4,8,256,0,0,0,0,12,64,20,1),ints,result}),10));
}

(async()=>{
  const {value,headers,reads,ints,result} = await make({status:200,url:'https://media.example/final.mp4',h:{'content-length':'9','content-type':'video/mp4; charset=x'}});
  assert.equal(value,1);
  assert.equal(result.final,'https://media.example/final.mp4');
  assert.equal(result.mime,'video/mp4');
  assert.equal(ints[1],200); assert.equal(ints[5],9); assert.equal(headers.Range,'bytes=0-63');
  assert.equal(reads,0);
  console.log('rede_sonda_wgt: PASS (Range, bounded deadline, content length/type, no media body)');
})().catch(e=>{console.error(e);process.exitCode=1});
