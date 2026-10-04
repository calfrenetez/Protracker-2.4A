#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sample_usage.h"
#include "mod_project.h"
static void empty(struct pt_sample *s)
{memset(s,0,sizeof(*s));s->pcm.bits=8;s->pcm.channels=1;s->pcm.rate=PT_CLASSIC_RATE;}
int main(void)
{
    struct pt_project p={0};struct pt_sample samples[8];uint16_t order=0;
    struct pt_sample_usage_scan *scan=calloc(1,sizeof(*scan));
    struct pt_sample_usage_preview *out=calloc(1,sizeof(*out)),*before=malloc(sizeof(*before));
    struct pt_sample_usage_options options={0};struct pt_extension ext={0};
    int32_t pcm[8]={1,-2,3,-4,11,12,13,14},saved[8];unsigned i,ready=9,steps=0;
    assert(scan && out && before);pt_channels_init(&p.channels);assert(pt_channels_resize(&p.channels,16)==PT_CHANNEL_OK);
    p.channels.track[15].muted=1;p.pattern_count=256;p.order_count=1;p.orders=&order;p.sample_count=8;p.samples=samples;
    p.events=calloc(256*64*16,sizeof(*p.events));assert(p.events);p.bpm=125;p.speed=6;
    for(i=0;i<8;++i)empty(samples+i);
    samples[0].pcm=(struct pt_pcm){pcm,8,4,44100,1,24};strcpy(samples[0].name,"MASTER");
    samples[1].volume=64;strcpy(samples[2].name,"NAMED EMPTY");
    options.revision=17;options.generation=3;options.reserved_slots[3]=1;options.protected_slots[4]=1;
    p.events[0].instrument=1; /* Instrument-only, no pitch. */
    p.events[15].instrument=1;p.events[15].kind=PT_NOTE_PERIOD;p.events[15].pitch=428;
    p.events[(size_t)255*64*16+63*16+15].instrument=1; /* Off-order, final muted track. */
    p.events[16].kind=PT_NOTE_PERIOD;p.events[16].pitch=428; /* Inherited zero field. */
    memcpy(saved,pcm,sizeof(saved));memset(out,0x5a,sizeof(*out));*before=*out;
    assert(pt_sample_usage_begin(scan,&p,&options)==PT_USAGE_OK);
    do {assert(pt_sample_usage_step(scan,&p,17,3,out,&ready)==PT_USAGE_OK);assert(++steps<=1024);
        if(!ready)assert(!memcmp(out,before,sizeof(*out)));
    } while(!ready);
    assert(steps==1024 && out->rows[0].references==3 && out->rows[0].patterns==2);
    assert(out->rows[0].flags&PT_USAGE_REFERENCED);assert(out->rows[1].flags&PT_USAGE_ELIGIBLE);
    assert(out->rows[2].flags&PT_USAGE_ELIGIBLE);assert(out->rows[3].flags&PT_USAGE_RESERVED);
    assert(out->rows[4].flags&PT_USAGE_PROTECTED);assert(pt_sample_usage_free(out,5));assert(!pt_sample_usage_free(out,2));
    assert(out->free_count==3 && out->eligible_count==2 && out->protected_count==3);
    assert(out->rows[0].active_bytes==16 && out->rows[0].resident_bytes==32);
    for(i=0;i<255;++i)assert(!out->selected[i]);assert(!memcmp(saved,pcm,sizeof(saved)));
    assert(pt_sample_usage_current(out,&p,17,3));assert(!pt_sample_usage_current(out,&p,18,3));
    p.channels.selected=15;assert(pt_sample_usage_current(out,&p,17,3)); /* Cursor only. */
    p.channels.track[4].muted=1;assert(!pt_sample_usage_current(out,&p,17,3));
    p.channels.track[4].muted=0;assert(pt_sample_usage_current(out,&p,17,3));
    *before=*out;ready=7;assert(pt_sample_usage_step(scan,&p,18,3,out,&ready)==PT_USAGE_STALE);
    assert(ready==7 && !memcmp(out,before,sizeof(*out)));
    assert(pt_sample_usage_begin(scan,&p,&options)==PT_USAGE_OK);
    assert(pt_sample_usage_step(scan,&p,17,3,out,(unsigned *)(pcm+7))==PT_USAGE_ALIAS);
    assert(!memcmp(saved,pcm,sizeof(saved)) && !scan->next_event);
    assert(pt_sample_usage_step(scan,&p,17,3,(struct pt_sample_usage_preview *)scan,&ready)==PT_USAGE_ALIAS);
    assert(!scan->next_event);
    ext=(struct pt_extension){0x58585858,0,1,NULL};p.extensions=&ext;p.extension_count=1;
    assert(pt_sample_usage_begin(scan,&p,&options)==PT_USAGE_OK);steps=0;
    do {assert(pt_sample_usage_step(scan,&p,17,3,out,&ready)==PT_USAGE_OK);assert(++steps<=1024);}while(!ready);
    assert(out->eligible_count==0 && out->free_count==0 && out->protected_count==8);
    for(i=0;i<8;++i)assert(out->rows[i].flags&PT_USAGE_UNKNOWN);
    p.extensions=NULL;p.extension_count=0;samples[5].pcm.data=pcm+4;samples[5].pcm.capacity=1;
    assert(pt_sample_usage_begin(scan,&p,&options)==PT_USAGE_OK);steps=0;
    do {assert(pt_sample_usage_step(scan,&p,17,3,out,&ready)==PT_USAGE_OK);assert(++steps<=1024);}while(!ready);
    assert(!pt_sample_usage_free(out,5)); /* Reserved storage is never a free slot. */
    samples[5].pcm.capacity=SIZE_MAX;assert(pt_sample_usage_begin(scan,&p,&options)==PT_USAGE_INVALID);
    free(p.events);free(scan);free(out);free(before);
    puts("SAMPLE USAGE PASS: all256 patterns16tracks, off-order/instrument-only/inherited refs, masks/unknown protection, empty selection, strict free slots, bounded scan, stale/alias and full capacity");return 0;
}
