#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mod_project.h"
#include "mod_inspect.h"

int main(int argc,char **argv)
{
    FILE *f;long length;uint8_t *mod,*roundtrip,*project;size_t n,w;
    struct pt_project p,unchanged;struct pt_project_storage s;struct pt_project_requirements need;
    struct pt_mod_export_report report;struct pt_mod_info info;
    assert(argc==2);f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));length=ftell(f);assert(length>0);
    rewind(f);mod=malloc((size_t)length);roundtrip=malloc((size_t)length);assert(mod && roundtrip);
    assert(fread(mod,1,(size_t)length,f)==(size_t)length);assert(!fclose(f));
    assert(pt_mod_project_probe(mod,(size_t)length,&need)==PT_PROJECT_OK);memset(&s,0,sizeof(s));
    s.order_capacity=need.orders;s.orders=calloc(need.orders,sizeof(*s.orders));
    s.event_capacity=need.events;s.events=calloc(need.events,sizeof(*s.events));
    s.sample_capacity=need.samples;s.samples=calloc(need.samples,sizeof(*s.samples));
    s.pcm_capacity=need.pcm_values;s.pcm=calloc(need.pcm_values?need.pcm_values:1,sizeof(*s.pcm));
    s.extension_capacity=need.extensions;s.extensions=calloc(need.extensions,sizeof(*s.extensions));
    s.extension_bytes=need.extension_bytes;s.extension_data=malloc(need.extension_bytes);
    assert(s.orders && s.events && s.samples && s.pcm && s.extensions && s.extension_data);
    memset(&p,0xa5,sizeof(p));unchanged=p;s.event_capacity=0;
    assert(pt_mod_project_decode(mod,(size_t)length,&s,&p)==PT_PROJECT_CAPACITY && !memcmp(&p,&unchanged,sizeof(p)));
    s.event_capacity=need.events;
    assert(pt_mod_project_decode(mod,(size_t)length,&s,&p)==PT_PROJECT_OK);
    assert(pt_project_validate(&p,NULL)==PT_PROJECT_OK);
    assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && !report.issues && report.classification==PT_CONVERSION_LOSSLESS);
    assert(report.bytes==(size_t)length);
    assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_OK && w==(size_t)length);
    assert(!memcmp(mod,roundtrip,w));
    assert(pt_mod_inspect(roundtrip,w,&info)==PT_MOD_OK && !info.warnings);
    assert(pt_project_size(&p,&n)==PT_PROJECT_OK);project=malloc(n);assert(project);
    assert(pt_project_encode(&p,project,n,&w)==PT_PROJECT_OK && w==n);
    assert(pt_project_probe(project,n,&need)==PT_PROJECT_OK);
    assert(pt_project_decode(project,n,&s,&p)==PT_PROJECT_OK);
    assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_OK && !memcmp(mod,roundtrip,w));
    /* Preserved one-word/zero-length loop headers must not export pointers
       outside their sample, even though the project has no enabled loop. */
    {
        uint8_t *h=s.extension_data+20+30;uint8_t before[4];memcpy(before,h+26,4);
        assert(p.samples[1].loop==PT_LOOP_NONE);h[26]=0x7f;h[27]=0xff;h[28]=0;h[29]=1;
        assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_LOOPS));
        memset(roundtrip,0xa5,(size_t)length);w=123;
        assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_UNSUPPORTED && w==123);
        for(n=0;n<(size_t)length;++n)assert(roundtrip[n]==0xa5);
        h[29]=0;assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_LOOPS));
        memcpy(h+26,before,4);
        assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_OK && !memcmp(mod,roundtrip,w));
    }
    /* A saved MIDI assignment is metadata even before its route becomes MIDI. */
    p.channels.track[0].midi_channel=16;
    assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && report.issues==PT_EXPORT_METADATA);
    memset(roundtrip,0xa5,(size_t)length);w=123;
    assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_UNSUPPORTED && w==123);
    for(n=0;n<(size_t)length;++n)assert(roundtrip[n]==0xa5);
    p.channels.track[0].midi_channel=1;
    assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_OK && !memcmp(mod,roundtrip,w));
    /* Potential transformations are reported; direct writer never makes them. */
    p.channels.track[0].route=PT_MIDI;
    assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_MIDI_AUDIO));
    assert(report.classification==PT_CONVERSION_INCOMPLETE);memset(roundtrip,0xa5,(size_t)length);
    assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_UNSUPPORTED && roundtrip[0]==0xa5);
    p.channels.track[0].route=PT_PAULA;p.events[0].kind=PT_NOTE_OFF;p.events[0].pitch=0;
    assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_OFF));
    assert(pt_mod_export_direct(&p,roundtrip,(size_t)length,&w)==PT_PROJECT_UNSUPPORTED && roundtrip[0]==0xa5);
    p.events[0].kind=PT_NOTE_NONE;p.samples[0].pcm.bits=16;
    assert(pt_mod_export_analyse(&p,&report)==PT_PROJECT_OK && (report.issues&PT_EXPORT_PRECISION));
    free(mod);free(roundtrip);free(project);free(s.orders);free(s.events);free(s.samples);free(s.pcm);free(s.extensions);free(s.extension_data);
    puts("MOD PROJECT PASS: MOD -> project -> MOD byte identity, strict validation, staged import, explicit loss reports and no silent downgrade");
    return 0;
}
