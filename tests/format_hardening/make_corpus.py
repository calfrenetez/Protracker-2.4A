"""Deterministic synthetic file corpus, version 1; no downloaded audio.

Usage: python3 -B tests/format_hardening/make_corpus.py NEW_DIRECTORY
Limits can be lowered by callers; all cases have specified validity oracles.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import struct
import zlib

ROOT = Path(__file__).resolve().parents[2]
SEED = 0x24A019
VERSION = 1

def project(bits=8, stereo=False, enhanced=False, channels=4, midi=False, empty=False):
    def chunk(tag, body, flags=1):
        return tag + struct.pack('>HHI', 1, flags, len(body)) + body + b'\0' * (-len(body) % 4)
    count = 0 if empty else 1
    head = b'Synthetic hardening\0'.ljust(32,b'\0') + bytes([channels,0,6,0]) + struct.pack('>HHHH',125,1,1,count)
    chan = b''.join(bytes([4 if midi and i==0 else 1 if i<4 else 2,0 if i%4 in (0,3) else 255,0,0,0,i+1])+bytes(16) for i in range(channels))
    patt = bytes(64*channels*12)
    caps = (1|2 if channels>4 else 0) | (4 if midi else 0)
    sample = b''
    if count:
        nch = 2 if stereo else 1
        values = [-(1 << (bits-1)), -1, 0, (1 << (bits-1))-1] * (4*nch)
        slices = [0,4,8] if enhanced else []
        caps |= {8:0,16:8,24:16}[bits] | (32 if stereo else 0) | (64|128 if enhanced else 0)
        sample = b'Master\0'.ljust(32,b'\0') + struct.pack('>II',8287,16) + bytes([bits,nch,64,7,2 if enhanced else 0,0])
        sample += struct.pack('>HIIII',len(slices),2 if enhanced else 0,12 if enhanced else 0,0,0)
        sample += b''.join(struct.pack('>I',s) for s in slices)
        sample += b''.join((v & ((1<<bits)-1)).to_bytes(bits//8,'big') for v in values)
        sample += bytes(-len(sample)%4)
    body = b''.join([chunk(b'HEAD',head),chunk(b'CHAN',chan),chunk(b'ORDR',b'\0\0'),chunk(b'PATT',patt),chunk(b'SAMP',sample),chunk(b'MIDI',bytes(64+64*channels+4))])
    header = b'PT24G\r\n\x1a' + struct.pack('>HHIIIII',1,0,32+len(body),caps,0,6,0)
    data = bytearray(header+body)
    struct.pack_into('>I',data,20,zlib.crc32(data))
    return bytes(data)

def repaired(data):
    b=bytearray(data);b[20:24]=bytes(4);struct.pack_into('>I',b,20,zlib.crc32(b));return bytes(b)

def mod(frames=16, patterns=1, extended=False, finetune=15):
    b=bytearray(1084+patterns*1024+frames);b[:20]=b'PT24G synthetic\0'.ljust(20,b'\0')
    for i in range(31):b[20+i*30+28:20+i*30+30]=b'\0\1'
    b[42:44]=struct.pack('>H',frames//2);b[44]=finetune;b[45]=64
    if frames>=8:b[46:50]=struct.pack('>HH',1,3)
    b[950]=1;b[951]=127;b[1079]=patterns-1;b[1080:1084]=b'M!K!' if extended else b'M.K.'
    b[1084:1088]=bytes([1,172,0x10,0])
    b[1084+patterns*1024:]=bytes((i*37+SEED)&255 for i in range(frames))
    return bytes(b)

def wav(bits,channels,frames=17):
    values=[-(1<<(bits-1)),-1,0,(1<<(bits-1))-1]
    values=[values[i%4] for i in range(frames*channels)]
    data=b''.join((v+128 if bits==8 else v&((1<<bits)-1)).to_bytes(bits//8,'little') for v in values)
    align=channels*(bits//8)
    fmt=struct.pack('<HHIIHH',1,channels,44100,44100*align,align,bits)
    def ck(tag,b):return tag+struct.pack('<I',len(b))+b+bytes(len(b)%2)
    body=b'WAVE'+ck(b'JUNK',b'x')+ck(b'fmt ',fmt)+ck(b'data',data)+ck(b'JUNK',b'xyz')
    return b'RIFF'+struct.pack('<I',len(body))+body,values

def svx(compressed=False):
    def ck(tag,b):return tag+struct.pack('>I',len(b))+b+bytes(len(b)%2)
    values=[-128,-1,0,127]*4+[-128]
    body=bytes(v&255 for v in values)
    if compressed:body=b'\0\x7f'+b'\x88'*9;values=[127]*18
    vh=struct.pack('>IIIHBBI',2,8,0,22050,1,int(compressed),65536)
    data=b'8SVX'+ck(b'ANNO',b'x')+ck(b'VHDR',vh)+ck(b'NAME',b'test')+ck(b'CHAN',struct.pack('>I',2))+ck(b'BODY',body)
    return b'FORM'+struct.pack('>I',len(data))+data,values

def generate(out, max_cases=512, max_fixture=1<<20, max_total=64<<20):
    out=Path(out);out.mkdir(parents=True,exist_ok=False)
    rows=[];files=[];total=0
    def add(name,fmt,data,valid=True,bits=0,ch=0,rate=0,values=None,params=None):
        nonlocal total
        assert len(rows)<max_cases and len(data)<=max_fixture
        path=name+'.'+fmt.lower();(out/path).write_bytes(data)
        total+=len(data);assert total<=max_total
        row={'id':name,'format':fmt,'valid':valid,'path':path,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'parameters':params or {},'expected':'accepted' if valid else 'refused'}
        frames=len(values)//ch if values is not None else 0
        ref='-'
        if values is not None:
            ref=name+'.pcm32be';expected=b''.join(struct.pack('>i',v) for v in values);(out/ref).write_bytes(expected)
            total+=len(expected);assert total<=max_total
            files.append({'path':ref,'bytes':len(expected),'sha256':hashlib.sha256(expected).hexdigest(),'purpose':'independent signed PCM oracle'})
        rows.append(row)
        row['declaration']={'bits':bits,'channels':ch,'rate':rate,'frames':frames,'pcmOracle':ref}
        return data
    def prefixes(name,fmt,data,cuts):
        for n in sorted(set(cuts)):
            if 0<=n<len(data):add(name+'-prefix-'+str(n),fmt,data[:n],False,params={'prefix':n})
    for n,p,e in [(0,1,False),(2,1,False),(16,2,False),(512,64,False),(254,100,True)]:
        data=add('mod-'+str(n)+'-'+str(p),'MOD',mod(n,p,e),params={'frames':n,'patterns':p,'extended':e})
        if p==2:prefixes('mod','MOD',data,[0,1,20,949,950,1080,1083,1084,2107,len(data)-1])
    base=mod()
    for name,offset,value in [('sig',1080,88),('orders',950,0),('order-limit',952,64),('volume',45,65),('finetune',44,16),('instrument',1084,0xf1),('loop',49,127)]:
        b=bytearray(base);b[offset]=value;add('mod-bad-'+name,'MOD',b,False)
    add('mod-trailing','MOD',base+b'\0',False)
    for name,kw in [('empty',{'empty':True}),('classic',{}),('16',{'bits':16}),('24',{'bits':24}),('rich',{'bits':24,'stereo':True,'enhanced':True}),('five',{'channels':5}),('midi',{'midi':True})]:
        add('ptg-'+name,'PTG',project(**kw),params=kw)
    p=project(bits=24,stereo=True,enhanced=True)
    cuts=[0,1,7,8,12,20,31,32];pos=32
    while pos<len(p):
        n=int.from_bytes(p[pos+8:pos+12],'big');cuts.extend([pos+1,pos+11,pos+12,pos+12+n-1]);pos+=12+(n+3)//4*4
    prefixes('ptg','PTG',p,cuts)
    for name,offset,value in [('crc',20,1),('version',9,2),('huge-size',12,255),('huge-chunks',24,255),('reserved',31,1),('channel-zero',76,0),('huge-chunk',40,255)]:
        b=bytearray(p);b[offset]=value
        add('ptg-bad-'+name,'PTG',bytes(b) if name=='crc' else repaired(b),False,params={'offset':offset,'crcRepaired':name!='crc'})
    spec=importlib.util.spec_from_file_location('pp',ROOT/'tools/make_pp20_fixture.py');pp=importlib.util.module_from_spec(spec);spec.loader.exec_module(pp)
    for skip in [0,1,7,8,31,32]:add('pp20-skip-'+str(skip),'PP20',pp.literal(base,skip),params={'skip':skip,'decodedSha256':hashlib.sha256(base).hexdigest()})
    packed=pp.literal(base)
    prefixes('pp20','PP20',packed,[0,1,4,7,8,9,len(packed)-5,len(packed)-1])
    for name,offset,value in [('table',4,0),('skip',len(packed)-1,255),('size',len(packed)-4,255)]:
        b=bytearray(packed);b[offset]=value;add('pp20-bad-'+name,'PP20',b,False)
    add('pp20-invalid-mod','PP20',pp.literal(b'not a module'*10),False)
    for bits in (8,16,24):
        for ch in (1,2):
            w,v=wav(bits,ch);add('wav-'+str(bits)+'-'+str(ch),'WAV',w,bits=bits,ch=ch,rate=44100,values=v)
    w,_=wav(24,2)
    prefixes('wav','WAV',w,[0,1,11,12,19,21,22,29,45,53,len(w)-1])
    # fmt begins at 22, payload at 30; data begins at 46.
    for name,off,value in [('size',4,255),('fmt-code',30,3),('channels',32,3),('bits',44,32),('align',42,1),('rate',34,0),('chunk-huge',26,255),('partial-frame',50,101)]:
        b=bytearray(w);b[off]=value;add('wav-bad-'+name,'WAV',b,False)
    for comp in (False,True):
        s,v=svx(comp);add('svx-'+str(int(comp)),'SVX',s,bits=8,ch=1,rate=22050,values=v,params={'compression':int(comp),'loopStart':2,'loopEnd':10})
    s,_=svx()
    prefixes('svx','SVX',s,[0,1,11,12,19,21,22,29,49,50,61,len(s)-1])
    for name,off,value in [('size',4,255),('octaves',44,2),('compression',45,2),('volume',46,255),('chunk-huge',26,255),('channel',73,6)]:
        b=bytearray(s);b[off]=value;add('svx-bad-'+name,'SVX',b,False)
    for bits in (8,16,24):
        for ch in (1,2):
            for le in (False,True):
                for unsigned in ([False,True] if bits==8 else [False]):
                    raw=bytes(range(256))*(3 if bits==24 else 1)
                    width=bits//8
                    vals=[int.from_bytes(raw[i:i+width],'little' if le else 'big',signed=not unsigned)-(128 if unsigned else 0) for i in range(0,len(raw),width)]
                    add('raw-%d-%d-%d-%d'%(bits,ch,le,unsigned),'RAW',raw,bits=bits,ch=ch,rate=48000,values=vals,params={'littleEndian':int(le),'unsigned8':int(unsigned)})
    for r in rows:files.append({k:r[k] for k in ('path','bytes','sha256')})
    manifest={'generatorVersion':VERSION,'generatorSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'seed':SEED,'bounds':{'maxCases':max_cases,'maxFixtureBytes':max_fixture,'maxTotalBytes':max_total},'materializedBytes':total,'cases':rows,'files':files}
    (out/'CORPUS_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
    lines=[]
    for r in rows:
        d=r['declaration'];par=r['parameters']
        lines.append('\t'.join(map(str,[r['id'],r['format'],int(r['valid']),d['bits'],d['channels'],d['rate'],d['frames'],par.get('littleEndian',0),par.get('unsigned8',0),r['path'],d['pcmOracle']])))
    (out/'cases.tsv').write_text('\n'.join(lines)+'\n')
    return manifest

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('output',type=Path);p.add_argument('--max-cases',type=int,default=512);a=p.parse_args()
    m=generate(a.output,a.max_cases);print(json.dumps({'cases':len(m['cases']),'bytes':m['materializedBytes']}))
