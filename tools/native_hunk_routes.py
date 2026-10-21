"""Bounded executable HUNK decoder; keep section identity when VMAs overlap."""
import hashlib
import struct

HEADER, CODE, DATA, BSS, RELOC32, SYMBOL, DEBUG, END = 1011,1001,1002,1003,1004,1008,1009,1010

def parse(data):
    if not isinstance(data,bytes) or len(data)>64*1024*1024 or len(data)%4:
        raise ValueError('Bounded aligned executable bytes required')
    at=0
    def word():
        nonlocal at
        if at+4>len(data):raise ValueError('Truncated word')
        v=struct.unpack_from('>I',data,at)[0];at+=4;return v
    def body(n):
        nonlocal at
        if n>len(data)-at:raise ValueError('Truncated body')
        b=data[at:at+n];at+=n;return b
    if word()!=HEADER or word()!=0:raise ValueError('Unnamed executable header required')
    count,first,last=word(),word(),word()
    if first!=0 or not 0<count<=4096 or last+1!=count:raise ValueError('Invalid section table')
    sizes=[word() for _ in range(count)]
    if any(v&0xc0000000==0xc0000000 for v in sizes):raise ValueError('Extended allocation unsupported')
    sections=[]
    for index,allocation in enumerate(sizes):
        kind=word();words=word();length=words*4
        if kind not in (CODE,DATA,BSS):raise ValueError('Unsupported section kind')
        if length>(allocation&0x3fffffff)*4:raise ValueError('Section exceeds allocation')
        code=body(length) if kind!=BSS else b''
        symbols={};relocations={};debug=[]
        while True:
            record=word()
            if record==END:break
            if record==RELOC32:
                while True:
                    n=word()
                    if not n:break
                    target=word()
                    if target>=count or n>len(data)//4:raise ValueError('Invalid relocation group')
                    for _ in range(n):
                        offset=word()
                        if kind==BSS or offset%2 or offset+4>length or offset in relocations:
                            raise ValueError('Invalid or duplicate relocation')
                        relocations[offset]=target
            elif record==SYMBOL:
                while True:
                    n=word()
                    if not n:break
                    raw=body(n*4)
                    name=raw.rstrip(b'\0').decode('ascii')
                    offset=word()
                    if not name or b'\0' in raw.rstrip(b'\0') or name in symbols or offset>length:
                        raise ValueError('Invalid or duplicate section symbol')
                    symbols[name]=offset
            elif record==DEBUG:
                b=body(word()*4);debug.append({'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()})
            else:raise ValueError('Unsupported record '+str(record))
        sections.append({'index':index,'kind':kind,'allocation':allocation,'length':length,
                         'body':code,'symbols':symbols,'relocations':relocations,'debug':debug})
    if at!=len(data):raise ValueError('Trailing bytes')
    for s in sections:
        for offset,target in s['relocations'].items():
            addend=struct.unpack_from('>I',s['body'],offset)[0]
            if addend>sections[target]['length']:raise ValueError('Relocation target addend out of bounds')
    return sections

def symbol(sections,name):
    matches=[(s['index'],s['symbols'][name]) for s in sections if name in s['symbols']]
    if len(matches)!=1:raise ValueError('Unique section-associated symbol required: '+name)
    return matches[0]

def references(sections,name):
    target,addend=symbol(sections,name)
    return [{'section':s['index'],'offset':o,'targetSection':t,'targetOffset':addend}
            for s in sections for o,t in s['relocations'].items()
            if t==target and struct.unpack_from('>I',s['body'],o)[0]==addend]

def pointer(sections,name):
    section,offset=symbol(sections,name);s=sections[section]
    if offset not in s['relocations']:raise ValueError('Symbol is not a relocated pointer')
    return (s['relocations'][offset],struct.unpack_from('>I',s['body'],offset)[0])
