#ifndef PT_SAMPLER_PIN_CURRENT_CASES_H
#define PT_SAMPLER_PIN_CURRENT_CASES_H
#include "../src/editor/sampler_internal.h"
static void pin_current_fixture(const struct pt_allocator *a)
{
    unsigned bits;
    for(bits=8;bits<=24;bits+=8) {
        struct pt_document d;struct pt_sampler s;struct pt_sample original;
        struct pt_pattern_history history;struct pt_pattern_command commands[2];struct pt_event_change changes[2];
        struct pt_sample_version *pin[4],*refused;struct pt_pcm pcm[4],out,sentinel;
        int32_t data[8]={1,17,-31,63,-127,15,23,27};size_t bytes;unsigned generation;
        pt_document_init(&d,a);assert(pt_document_new(&d,4,SIZE_MAX)==PT_PROJECT_OK);
        pt_sampler_init(&s,a,1024*1024);
        assert(pt_pattern_history_init(&history,&d.project,commands,2,changes,2)==PT_EDIT_OK);
        d.project.samples[0].pcm=(struct pt_pcm){data,8,4,48000,2,(uint8_t)bits};
        assert(pt_sampler_pin(&s,&d.project,0,s.generation,&pcm[0],&pin[0])==PT_EDIT_OK);
        generation=s.generation;bytes=s.bytes;s.budget=0;
        assert(pt_sampler_pin_current(&s,&d.project,0,generation,pin[0],&pcm[1],&pin[1])==PT_EDIT_OK);
        assert(pin[1]==pin[0] && pcm[1].data==pcm[0].data && s.bytes==bytes);
        memset(&sentinel,0x5a,sizeof(sentinel));out=sentinel;refused=NULL;original=d.project.samples[0];
        assert(pt_sampler_pin_current(&s,&d.project,0,generation+1,pin[0],&out,&refused)==PT_EDIT_CONFLICT);
        assert(pt_sampler_pin_current(&s,&d.project,1,generation,pin[0],&out,&refused)==PT_EDIT_CONFLICT);
        ++d.project.samples[0].volume;
        assert(pt_sampler_pin_current(&s,&d.project,0,generation,pin[0],&out,&refused)==PT_EDIT_CONFLICT);
        d.project.samples[0]=original;d.project.samples[0].pcm.data=data;
        assert(pt_sampler_pin_current(&s,&d.project,0,generation,pin[0],&out,&refused)==PT_EDIT_CONFLICT);
        d.project.samples[0]=original;++d.project.samples[0].pcm.capacity;
        assert(pt_sampler_pin_current(&s,&d.project,0,generation,pin[0],&out,&refused)==PT_EDIT_CONFLICT);
        d.project.samples[0]=original;
        assert(!refused && !memcmp(&out,&sentinel,sizeof(out)) && s.bytes==bytes);
        s.budget=1024*1024;
        assert(pt_sampler_attributes(&s,&d.project,&history,0,"shared master",32,0)==PT_EDIT_OK);
        assert(pt_sampler_pin_current(&s,&d.project,0,s.generation,pin[0],&out,&refused)==PT_EDIT_CONFLICT);
        assert(pt_sampler_pin(&s,&d.project,0,s.generation,&pcm[2],&pin[2])==PT_EDIT_OK);
        assert(pin[2]!=pin[0] && pcm[2].data==pcm[0].data); /* Metadata version shares backing. */
        assert(pt_sampler_pin_current(&s,&d.project,0,s.generation,pin[2],&pcm[3],&pin[3])==PT_EDIT_OK);
        pt_pattern_history_release(&history);pt_sampler_release(&s);pt_document_release(&d);
        assert(s.bytes && !memcmp(pcm[0].data,data,sizeof(data)) && !memcmp(pcm[3].data,data,sizeof(data)));
        pt_sampler_unpin(pin[0]);pt_sampler_unpin(pin[2]);assert(s.bytes);
        pt_sampler_unpin(pin[1]);assert(s.bytes && pcm[3].data[4]==-127);
        pt_sampler_unpin(pin[3]);assert(!s.bytes);
    }
    puts("CURRENT PIN PASS: exact8/16/24 master reuse, budget-free retain, stale/descriptor refusal, shared backing and final release");
}
#endif
