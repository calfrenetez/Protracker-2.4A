#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"
static const unsigned periods[36]={856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,214,202,190,180,170,160,151,143,135,127,120,113};
static const char *notes[12]={"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"};
void pt_editor_note(const struct pt_event *e,char out[4])
{
    unsigned i,n;
    memcpy(out,"---",4);
    if(e->kind==PT_NOTE_OFF) {memcpy(out,"OFF",4);return;}
    if(e->kind==PT_NOTE_MIDI) {
        /* Display MIDI note 0 as C-0 (not scientific pitch notation). */
        n=e->pitch;if(n/12>9) {snprintf(out,4,"%03u",n);return;}
    } else if(e->kind==PT_NOTE_PERIOD) {
        for(i=0;i<36 && periods[i]!=e->pitch;++i) {}
        if(i==36) {memcpy(out,"???",4);return;}
        n=i+12;
    } else return;
    out[0]=notes[n%12][0];out[1]=notes[n%12][1];out[2]=(char)('0'+n/12);out[3]=0;
}
void pt_editor_status(struct pt_editor *e,const char *s)
{snprintf(e->status,sizeof(e->status),"%s",s);}
static void *sample_allocate(void *context,size_t bytes) {(void)context;return malloc(bytes);}
static void sample_release(void *context,void *data) {(void)context;free(data);}
static void source_close(struct pt_editor *e)
{
    e->sampler.budget+=e->sample_source.allocated_bytes;pt_document_release(&e->sample_source);e->source_selected=0;
    if(e->panel==11)e->panel=5;
    ++e->sample_ui;
}
void pt_editor_dispose(struct pt_editor *e)
{if(e) {pt_pattern_history_release(&e->history);source_close(e);pt_sampler_release(&e->sampler);pt_song_release(&e->song);}}
enum pt_edit_result pt_editor_source_load(struct pt_editor *e,const uint8_t *bytes,size_t length)
{
    struct pt_document next;enum pt_project_result result;size_t available;
    if(!e || !bytes || (length>=5 && !memcmp(bytes,"PT24G",5)))return PT_EDIT_UNSUPPORTED;
    if(e->sampler.bytes>e->sampler.budget)return PT_EDIT_CAPACITY;
    available=e->sampler.budget-e->sampler.bytes;pt_document_init(&next,&e->sampler.allocator);
    result=pt_document_load(&next,bytes,length,available);
    if(result!=PT_PROJECT_OK)return result==PT_PROJECT_CAPACITY?PT_EDIT_CAPACITY:PT_EDIT_UNSUPPORTED;
    source_close(e);e->sample_source=next;e->sampler.budget-=next.allocated_bytes;
    e->source_selected=1;e->panel=11;++e->sample_ui;
    pt_editor_status(e,"MOD SOURCE READY - CHOOSE INSTRUMENT AND IMPORT");return PT_EDIT_OK;
}
void pt_editor_wave_bounds(const struct pt_editor *e,uint32_t *start,uint32_t *end)
{
    uint32_t frames=e->sample && e->sample<=e->project->sample_count?e->project->samples[e->sample-1].pcm.frames:0;
    *start=0;*end=frames;
    if(e->wave_slot==e->sample && e->wave_frames==frames && e->wave_start<e->wave_end && e->wave_end<=frames) {*start=e->wave_start;*end=e->wave_end;}
}
void pt_editor_sample_all(struct pt_editor *e)
{
    if(e->slice_pending && (e->slice_slot!=e->sample || e->slice_generation!=e->sampler.generation)) {e->slice_pending=0;++e->sample_ui;}
    e->sample_range_slot=e->sample;e->sample_start=0;e->sample_marking=0;
    e->sample_end=e->sample && e->sample<=e->project->sample_count?e->project->samples[e->sample-1].pcm.frames:0;
    if(e->format_slot!=e->sample && e->sample) {
        e->format_slot=e->sample;e->format_bits=e->project->samples[e->sample-1].pcm.bits;e->format_rate=e->project->samples[e->sample-1].pcm.rate;
    }
    if(e->wave_slot!=e->sample || e->wave_frames!=e->sample_end) {
        e->wave_slot=e->sample;e->wave_frames=e->sample_end;e->wave_start=0;e->wave_end=e->sample_end;++e->sample_ui;
    }
}
void pt_editor_sample_result(struct pt_editor *e,enum pt_edit_result result)
{
    if(e->slice_pending && e->slice_generation!=e->sampler.generation) {e->slice_pending=0;++e->sample_ui;}
    pt_editor_status(e,result==PT_EDIT_OK?"SAMPLE UPDATED - CONTROL-Z TO UNDO":
        result==PT_EDIT_CANCELLED?"CONVERSION CANCELLED - SAMPLE AND HISTORY PRESERVED":
        result==PT_EDIT_CAPACITY?"SAMPLE MEMORY BUDGET OR ALLOCATION FAILED - NO CHANGE":
        result==PT_EDIT_UNSUPPORTED?"SAMPLE FORMAT OR SLICE REFERENCES UNSUPPORTED - NO CHANGE":"SAMPLE EDIT REFUSED - NO CHANGE");
}
static void sample_edit(struct pt_editor *e,enum pt_pcm_edit op,unsigned gain)
{
    enum pt_edit_result result;
    if(e->sample_range_slot!=e->sample)pt_editor_sample_all(e);
    if(e->sample_marking) {pt_editor_status(e,"FINISH THE SAMPLE RANGE OR SELECT ALL FIRST");return;}
    if(!e->sample || !e->sample_end) {pt_editor_status(e,"SELECT A NONEMPTY SAMPLE FIRST");return;}
    result=pt_sampler_edit(&e->sampler,e->project,&e->history,e->sample-1,op,e->sample_start,e->sample_end,gain);
    pt_editor_sample_result(e,result);
}
static void sampler_panel(struct pt_editor *e)
{e->panel=5;pt_editor_sample_all(e);pt_editor_status(e,"SAMPLER: CLICK TWICE FOR RANGE; +/- SAMPLE; CTRL-Z UNDO");}
/* All sample detail pages use the same range and immutable-version journal. */
static void sample_tab(struct pt_editor *e,unsigned page)
{
    e->panel=page;
    if(page==9 && e->sample) {e->format_slot=e->sample;e->format_bits=e->project->samples[e->sample-1].pcm.bits;e->format_rate=e->project->samples[e->sample-1].pcm.rate;++e->sample_ui;}
    pt_editor_status(e,page==6?"LOOPS: F FORWARD / P PINGPONG / O OFF / B BAKE FADE":
        page==7?"SLICES: M ADD / D DELETE / T AUTO / P APPLY / X CANCEL":
        page==8?"RANGE: I/O ZOOM; F FIT; V SELECTION; S/E ENTER FRAMES":
        page==10?"RAW: SET BITS/CHANNELS/SIGN/ORDER/RATE BEFORE L OR W":
        page==9?"FORMAT: 1/2/3 BITS; R RATE; F FILTER; P APPLY":
        "SAMPLER: CLICK TWICE FOR RANGE; +/- SAMPLE; CTRL-Z UNDO");
}
static void source_select(struct pt_editor *e,int which,int direction)
{
    unsigned *selected=which?&e->sample:&e->source_selected;
    unsigned limit=which?e->project->sample_count:e->sample_source.project.sample_count;
    if(direction<0 && *selected>1)--*selected;
    if(direction>0 && *selected<limit)++*selected;
    if(which)pt_editor_sample_all(e);
    ++e->sample_ui;pt_editor_status(e,"SOURCE SELECTION ONLY - IMPORT TO CHANGE SONG");
}
static void source_apply(struct pt_editor *e)
{
    enum pt_edit_result result;
    if(!e->sample_source.loaded || !e->source_selected || !e->sample || !e->sample_source.project.samples[e->source_selected-1].pcm.frames) {
        pt_editor_status(e,"SELECT A NONEMPTY SOURCE AND A DESTINATION SLOT");return;
    }
    result=pt_sampler_import_slot(&e->sampler,e->project,&e->history,e->sample-1,&e->sample_source.project,e->source_selected-1);
    pt_editor_sample_result(e,result);
    if(result==PT_EDIT_OK) {pt_editor_sample_all(e);pt_editor_status(e,"SOURCE INSTRUMENT IMPORTED - CONTROL-Z TO UNDO");}
}
static int sample_range(struct pt_editor *e)
{
    if(e->sample_range_slot!=e->sample)pt_editor_sample_all(e);
    if(e->sample_marking) {pt_editor_status(e,"FINISH THE SAMPLE RANGE OR SELECT ALL FIRST");return 0;}
    if(!e->sample || !e->sample_end) {pt_editor_status(e,"SELECT A NONEMPTY SAMPLE FIRST");return 0;}
    return 1;
}
static void wave_view(struct pt_editor *e,unsigned op)
{
    uint32_t start,end,span,frames,centre,step;
    if(!sample_range(e))return;
    frames=e->project->samples[e->sample-1].pcm.frames;pt_editor_wave_bounds(e,&start,&end);span=end-start;centre=start+span/2;
    if(op==0) {span=span/2+span%2;if(!span)span=1;start=centre>span/2?centre-span/2:0;}
    else if(op==1) {span=span>frames/2?frames:span*2;start=centre>span/2?centre-span/2:0;}
    else if(op==2) {start=0;span=frames;}
    else if(op==3) {start=e->sample_start;span=e->sample_end-start;}
    else {step=span/2;if(!step)step=1;if(op==4)start=start>step?start-step:0;else start=start>frames-step?frames:start+step;}
    if(start>frames-span)start=frames-span;
    e->wave_slot=e->sample;e->wave_frames=frames;e->wave_start=start;e->wave_end=start+span;++e->sample_ui;
    pt_editor_status(e,"WAVE VIEW CHANGED - SAMPLE AND SELECTION PRESERVED");
}
static void range_nudge(struct pt_editor *e,unsigned field,int direction)
{
    uint32_t frames;
    if(!sample_range(e))return;
    frames=e->project->samples[e->sample-1].pcm.frames;
    if(field==1) {if(direction<0 && e->sample_start)--e->sample_start;else if(direction>0 && e->sample_start+1<e->sample_end)++e->sample_start;}
    else {if(direction<0 && e->sample_end>e->sample_start+1)--e->sample_end;else if(direction>0 && e->sample_end<frames)++e->sample_end;}
    ++e->sample_ui;pt_editor_status(e,"RANGE ADJUSTED BY ONE FRAME");
}
static int apply(struct pt_editor *,struct pt_event);
static struct pt_event *current_event(struct pt_editor *e)
{return &e->project->events[(e->pattern*64+e->row)*e->project->channels.count+e->project->channels.selected];}
static int note_slice(struct pt_editor *e,unsigned value)
{
    struct pt_event event=*current_event(e);
    if(value && (event.kind!=PT_NOTE_PERIOD || !event.instrument || value>e->project->samples[event.instrument-1].slice_count)) {
        pt_editor_status(e,"SLICE REFUSED - NEED SAMPLE NOTE AND EXISTING MARKER");return 0;
    }
    event.slice=(uint16_t)value;
    if(!apply(e,event))return 0;
    pt_editor_status(e,value?"NOTE SLICE SAVED - ENHANCED PLAYBACK REQUIRED":"NOTE USES WHOLE SAMPLE - CONTROL-Z TO UNDO");return 1;
}
static void note_panel(struct pt_editor *e)
{
    e->panel=1;e->note_details=1;e->song_details=0;++e->sample_ui;
    pt_editor_status(e,"NOTE SLICE: S NUMBER / +/- STEP / U SAMPLE / C CLEAR");
}
static void note_sample(struct pt_editor *e)
{
    struct pt_event event=*current_event(e);
    if(event.kind!=PT_NOTE_PERIOD || !e->sample || !e->project->samples[e->sample-1].slice_count || event.slice>e->project->samples[e->sample-1].slice_count) {
        pt_editor_status(e,"USE SAMPLE REFUSED - NEED SAMPLE NOTE AND VALID SLICES");return;
    }
    event.instrument=(uint8_t)e->sample;
    if(apply(e,event))pt_editor_status(e,"NOTE INSTRUMENT SET - CHOOSE ITS SLICE NUMBER");
}
static void note_step(struct pt_editor *e,int direction)
{
    unsigned value=current_event(e)->slice;
    if(direction<0 && value)--value;
    else if(direction>0 && value<PT_PROJECT_SLICES)++value;
    (void)note_slice(e,value);
}
static int hexkey(unsigned raw);
static void channel_value(struct pt_editor *e,const struct pt_channel *value)
{
    enum pt_edit_result result=pt_pattern_channel_apply(e->project,&e->history,e->project->channels.selected,value);
    pt_editor_status(e,result==PT_EDIT_OK?"CHANNEL UPDATED - CONTROL-Z TO UNDO":
        result==PT_EDIT_PAULA_LIMIT?"PAULA LIMIT: CHANGE ANOTHER PAULA ROUTE FIRST":
        result==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - NO CHANGE":"CHANNEL CHANGE REFUSED");
}
static void number_begin(struct pt_editor *e,unsigned field)
{
    const struct pt_channel *channel=&e->project->channels.track[e->project->channels.selected];
    unsigned long value;
    if(field<4 && !sample_range(e))return;
    e->number_field=field;e->number_fresh=1;
    value=field==1?e->sample_start:field==2?e->sample_end:field==3?e->format_rate:field==4?e->raw_format.rate:field==5?channel->pan:field==6?channel->group:field==7?channel->midi_channel:current_event(e)->slice;
    snprintf(e->number_text,sizeof(e->number_text),field==8?"%04lX":field==5 || field==6?"%02lX":"%lu",value);++e->sample_ui;
    pt_editor_status(e,field==1?"ENTER START FRAME: DIGITS / RETURN APPLY / ESC CANCEL":field==2?"ENTER END FRAME: DIGITS / RETURN APPLY / ESC CANCEL":
        field==8?"SLICE HEX 0000 WHOLE / 0001 FIRST MARKER / RETURN APPLY":field==5?"PAN 00 LEFT TO FF RIGHT (HEX) / RETURN / ESC CANCEL":field==6?"GROUP 00 NONE TO 0F (HEX) / RETURN / ESC CANCEL":field==7?"MIDI CHANNEL 1-16 (DECIMAL) / RETURN / ESC CANCEL":"ENTER TARGET RATE 1-192000 HZ / RETURN / ESC CANCEL");
}
static void number_key(struct pt_editor *e,unsigned raw)
{
    size_t length=strlen(e->number_text),i;unsigned digit,base=e->number_field==5 || e->number_field==6 || e->number_field==8?16:10;uint32_t value=0,frames;int input;
    if(raw==0x45) {e->number_field=0;++e->sample_ui;pt_editor_status(e,"NUMBER ENTRY CANCELLED - VALUES PRESERVED");return;}
    if(raw==0x44) {
        for(i=0;i<length;++i) {
            digit=e->number_text[i]>='A'?(unsigned)(e->number_text[i]-'A'+10):(unsigned)(e->number_text[i]-'0');
            if(digit>=base || value>(UINT32_MAX-digit)/base)break;
            value=value*base+digit;
        }
        frames=e->sample && e->sample<=e->project->sample_count?e->project->samples[e->sample-1].pcm.frames:0;
        if(!length || i!=length || (e->number_field==1?value>=e->sample_end:e->number_field==2?value<=e->sample_start || value>frames:
            e->number_field==8?value>PT_PROJECT_SLICES:e->number_field==5?value>255:e->number_field==6?value>15:e->number_field==7?!value || value>16:!value || value>192000)) {
            pt_editor_status(e,e->number_field==8?"INVALID SLICE NUMBER - CORRECT OR ESC CANCEL":e->number_field>=5?"INVALID CHANNEL VALUE - CORRECT OR ESC CANCEL":e->number_field>=3?"INVALID RATE - ENTER 1 TO 192000 HZ OR ESC CANCEL":"INVALID FRAME RANGE - CORRECT VALUE OR ESC CANCEL");return;
        }
        if(e->number_field==8) {if(!note_slice(e,value))return;}
        else if(e->number_field>=5) {
            struct pt_channel channel=e->project->channels.track[e->project->channels.selected];
            if(e->number_field==5)channel.pan=(uint8_t)value;else if(e->number_field==6)channel.group=(uint8_t)value;else channel.midi_channel=(uint8_t)value;
            channel_value(e,&channel);
        } else {
            if(e->number_field==1)e->sample_start=value;else if(e->number_field==2)e->sample_end=value;else if(e->number_field==4)e->raw_format.rate=value;else e->format_rate=value;
            pt_editor_status(e,e->number_field==4?"RAW RATE SET - HEADERLESS FILES CONTAIN NO RATE":e->number_field==3?"TARGET RATE SET - APPLY TO CONVERT WHOLE SAMPLE":"EXACT FRAME RANGE SET");
        }
        e->number_field=0;++e->sample_ui;return;
    }
    if(raw==0x41 || raw==0x46) {if(e->number_fresh)e->number_text[0]=0;else if(length)e->number_text[length-1]=0;e->number_fresh=0;++e->sample_ui;return;}
    input=hexkey(raw);
    if(input>=0 && (unsigned)input<base) {
        if(e->number_fresh) {length=0;e->number_fresh=0;}
        if(length<10) {e->number_text[length]="0123456789ABCDEF"[input];e->number_text[length+1]=0;++e->sample_ui;}
    }
}
static void name_begin(struct pt_editor *e,unsigned kind)
{
    if(kind==2 && !e->sample) {pt_editor_status(e,"SELECT A SAMPLE SLOT FIRST");return;}
    memset(e->name_text,0,sizeof(e->name_text));
    if(kind==1)memcpy(e->name_text,e->project->channels.track[e->project->channels.selected].name,PT_CHANNEL_NAME);
    else if(kind==2)memcpy(e->name_text,e->project->samples[e->sample-1].name,PT_PROJECT_NAME);
    else memcpy(e->name_text,e->project->title,PT_PROJECT_NAME);
    e->name_entry=kind;e->name_fresh=1;++e->sample_ui;
    pt_editor_status(e,kind==1?"TRACK NAME: 15 CHARS / RETURN APPLY / ESC CANCEL":kind==2?"SAMPLE NAME: 31 CHARS / RETURN APPLY / ESC CANCEL":"SONG TITLE: 31 CHARS / RETURN APPLY / ESC CANCEL");
}
static void name_key(struct pt_editor *e,unsigned raw)
{
    static const unsigned keys[26]={0x20,0x35,0x33,0x22,0x12,0x23,0x24,0x25,0x17,0x26,0x27,0x28,0x37,0x36,0x18,0x19,0x10,0x13,0x21,0x14,0x16,0x34,0x11,0x32,0x15,0x31};
    size_t length=strlen(e->name_text);unsigned i;char ch=0;
    if(raw==0x45) {e->name_entry=0;++e->sample_ui;pt_editor_status(e,"NAME ENTRY CANCELLED - NAME PRESERVED");return;}
    if(raw==0x44) {
        if(e->name_entry==1) {
            struct pt_channel channel=e->project->channels.track[e->project->channels.selected];
            memset(channel.name,0,sizeof(channel.name));memcpy(channel.name,e->name_text,length);channel_value(e,&channel);
        } else if(e->name_entry==3) {
            enum pt_edit_result result=pt_pattern_title_apply(e->project,&e->history,e->name_text);
            pt_editor_status(e,result==PT_EDIT_OK?"SONG TITLE UPDATED - CONTROL-Z TO UNDO":result==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - TITLE UNCHANGED":"SONG TITLE CHANGE REFUSED");
        } else {
            const struct pt_sample *sample=&e->project->samples[e->sample-1];
            pt_editor_sample_result(e,pt_sampler_attributes(&e->sampler,e->project,&e->history,e->sample-1,e->name_text,sample->volume,sample->finetune));
        }
        e->name_entry=0;++e->sample_ui;return;
    }
    if(raw==0x41 || raw==0x46) {if(e->name_fresh)e->name_text[0]=0;else if(length)e->name_text[length-1]=0;e->name_fresh=0;++e->sample_ui;return;}
    for(i=0;i<26;++i)if(raw==keys[i])ch=(char)('A'+i);
    if(raw>=1 && raw<=10)ch=(char)('0'+raw%10);
    if(raw==0x40)ch=' ';else if(raw==0x0b)ch='-';else if(raw==0x39)ch='.';
    if(ch) {
        if(e->name_fresh) {length=0;e->name_fresh=0;}
        if(length<(e->name_entry==1?PT_CHANNEL_NAME:PT_PROJECT_NAME)-1) {e->name_text[length]=ch;e->name_text[length+1]=0;++e->sample_ui;}
    }
}
static void sample_attribute_step(struct pt_editor *e,unsigned row,int direction)
{
    const struct pt_sample *sample;int volume,finetune;
    if(!e->sample) {pt_editor_status(e,"SELECT A SAMPLE SLOT FIRST");return;}
    sample=&e->project->samples[e->sample-1];volume=sample->volume;finetune=sample->finetune;
    if(row==3) {finetune+=direction;if(finetune< -8)finetune=-8;if(finetune>7)finetune=7;}
    else {volume+=direction;if(volume<0)volume=0;if(volume>64)volume=64;}
    pt_editor_sample_result(e,pt_sampler_attributes(&e->sampler,e->project,&e->history,e->sample-1,sample->name,(unsigned)volume,finetune));
}
static void format_setting(struct pt_editor *e,unsigned bits,uint32_t rate)
{
    if(bits)e->format_bits=bits;
    if(rate)e->format_rate=rate;
    ++e->sample_ui;
    pt_editor_status(e,bits?"TARGET BITS SET - REDUCTION LOSES PRECISION; APPLY":e->format_filtered?"TARGET RATE SET - FILTERED RESAMPLING; APPLY":"TARGET RATE SET - LINEAR RESAMPLE HAS NO ANTIALIAS FILTER");
}
static void format_apply(struct pt_editor *e)
{
    enum pt_edit_result result;unsigned generation=e->sampler.generation;
    if(!sample_range(e))return;
    if(e->format_filtered && (uint64_t)e->format_rate*128<e->project->samples[e->sample-1].pcm.rate) {pt_editor_status(e,"FILTER LIMIT: USE INTERMEDIATE RATE OR LINEAR MODE");return;}
    result=pt_sampler_convert_quality(&e->sampler,e->project,&e->history,e->sample-1,e->format_bits,e->format_rate,e->format_filtered);
    if(result==PT_EDIT_UNSUPPORTED)pt_editor_status(e,"CONVERSION WOULD COLLAPSE MARKERS OR LOOP - REFUSED");
    else pt_editor_sample_result(e,result);
    if(generation!=e->sampler.generation)pt_editor_sample_all(e);
}
static void loop_edit(struct pt_editor *e,unsigned op)
{
    struct pt_sample *sample;enum pt_edit_result result;
    if(!sample_range(e))return;
    sample=&e->project->samples[e->sample-1];
    if(op==4) {
        if(!sample->loop) {pt_editor_status(e,"NO LOOP TO SELECT");return;}
        e->sample_start=sample->loop_start;e->sample_end=sample->loop_end;
        pt_editor_status(e,"CURRENT LOOP RANGE SELECTED");return;
    }
    result=pt_sampler_loop(&e->sampler,e->project,&e->history,e->sample-1,(enum pt_loop_kind)op,
        op?e->sample_start:0,op?e->sample_end:0,op==PT_LOOP_CROSSFADE?e->loop_fade:0);
    if(result==PT_EDIT_OK) {
        e->slice_pending=0;++e->sample_ui;
        pt_editor_status(e,op==PT_LOOP_CROSSFADE?"CROSSFADE BAKED; FORWARD LOOP SKIPS BLENDED HEAD":"LOOP UPDATED - CONTROL-Z TO UNDO");
    } else if(result==PT_EDIT_INVALID)pt_editor_status(e,"LOOP REFUSED: FADE MUST FIT IN HALF THE SELECTED RANGE");
    else pt_editor_sample_result(e,result);
}
static void slice_edit(struct pt_editor *e,unsigned op)
{
    struct pt_sample *sample;enum pt_edit_result result;size_t index;enum pt_slice_result sliced;
    if(op==5) {e->slice_pending=0;++e->sample_ui;pt_editor_status(e,"SLICE PROPOSAL CANCELLED - SAMPLE UNCHANGED");return;}
    if(!sample_range(e))return;
    sample=&e->project->samples[e->sample-1];
    if(e->slice_pending && (e->slice_slot!=e->sample || e->slice_generation!=e->sampler.generation))e->slice_pending=0;
    if(op==4) {
        if(!e->slice_pending) {pt_editor_status(e,"NO SLICE PROPOSAL TO APPLY");return;}
        result=pt_sampler_slices(&e->sampler,e->project,&e->history,e->sample-1,e->slice_markers,e->slice_count);
        if(result==PT_EDIT_OK) {e->slice_pending=0;pt_editor_status(e,"SLICE MARKERS APPLIED - CONTROL-Z TO UNDO");}
        else if(result==PT_EDIT_UNSUPPORTED || result==PT_EDIT_CONFLICT)pt_editor_status(e,"SLICE CHANGE WOULD RETARGET PATTERN NOTES - REFUSED");
        else pt_editor_sample_result(e,result);
        ++e->sample_ui;return;
    }
    if(op==3) {
        struct pt_slice_options options;
        options.minimum_spacing=(uint32_t)((uint64_t)sample->pcm.rate*e->slice_gap_ms/1000);
        if(!options.minimum_spacing)options.minimum_spacing=1;
        options.zero_radius=e->slice_zero?32:0;options.threshold_per_mille=(uint16_t)e->slice_threshold;options.envelope_shift=4;
        sliced=pt_auto_slice(&sample->pcm,&options,e->slice_markers,4096,&e->slice_count);
        if(sliced!=PT_SLICE_OK) {pt_editor_status(e,"AUTO SLICE REFUSED - INCREASE GAP OR THRESHOLD");return;}
    } else {
        if(!e->slice_pending) {
            e->slice_count=sample->slice_count;
            if(e->slice_count)memcpy(e->slice_markers,sample->slices,e->slice_count*sizeof(uint32_t));
        }
        if(op==0) {
            if(pt_slice_insert(sample->pcm.frames,e->slice_markers,&e->slice_count,4096,e->sample_start)!=PT_SLICE_OK) {pt_editor_status(e,"SLICE MARKER LIMIT - NO CHANGE");return;}
        } else if(op==1) {
            for(index=0;index<e->slice_count && e->slice_markers[index]<=e->sample_start;++index) {}
            if(!index) {pt_editor_status(e,"NO MARKER AT OR BEFORE RANGE START");return;}
            (void)pt_slice_remove(sample->pcm.frames,e->slice_markers,&e->slice_count,index-1);
        } else e->slice_count=0;
    }
    e->slice_pending=1;e->slice_slot=e->sample;e->slice_generation=e->sampler.generation;++e->sample_ui;
    pt_editor_status(e,"SLICE PROPOSAL: EDIT MARKERS THEN APPLY; PCM UNCHANGED");
}
static void sample_setting(struct pt_editor *e,unsigned setting,int direction)
{
    if(setting==0) {
        if(direction<0 && e->loop_fade>1)e->loop_fade/=2;
        if(direction>0 && e->loop_fade<65536)e->loop_fade*=2;
    } else if(setting==1) {
        if(direction<0 && e->slice_threshold>50)e->slice_threshold-=50;
        if(direction>0 && e->slice_threshold<1000)e->slice_threshold+=50;
    } else if(setting==2) {
        if(direction<0 && e->slice_gap_ms>10)e->slice_gap_ms-=10;
        if(direction>0 && e->slice_gap_ms<1000)e->slice_gap_ms+=10;
    } else e->slice_zero^=1;
    ++e->sample_ui;
    pt_editor_status(e,setting==0?"FADE LENGTH IN FRAMES; BAKE USES SELECTED RANGE":"AUTO OPTIONS CHANGED - RUN AUTO TO GENERATE PROPOSAL");
}
int pt_editor_init(struct pt_editor *e,struct pt_project *p)
{
    if(!e || pt_project_validate(p,NULL)!=PT_PROJECT_OK)return 0;
    memset(e,0,sizeof(*e));e->project=p;e->sample=p->sample_count?1:0;e->octave=1;e->new_channels=p->channels.count;
    if(pt_pattern_history_init(&e->history,p,e->commands,128,e->changes,2048)!=PT_EDIT_OK)return 0;
    {struct pt_allocator a={NULL,sample_allocate,sample_release};pt_sampler_init(&e->sampler,&a,32UL*1024*1024);pt_document_init(&e->sample_source,&a);pt_song_init(&e->song,&a,8UL*1024*1024);}
    e->raw_format=(struct pt_raw_format){8287,8,1,0,0};
    e->format_filtered=1;e->loop_fade=32;e->slice_threshold=500;e->slice_gap_ms=50;e->slice_zero=1;
    pt_editor_sample_all(e);
    e->clipboard.events=e->clipboard_events;e->clipboard.capacity=1024;
    pt_editor_status(e,"READY - F8 PLAY / F9 PATTERN / F10 STOP");return 1;
}
int pt_editor_dirty(const struct pt_editor *e) {return pt_pattern_dirty(&e->history);}
void pt_editor_saved(struct pt_editor *e)
{pt_pattern_mark_saved(&e->history);e->quit_pending=0;pt_editor_status(e,"PROJECT SAVED AND VERIFIED");}
static void visible(struct pt_editor *e)
{
    if(e->row<e->first_row)e->first_row=e->row;
    if(e->row>=e->first_row+PT_EDITOR_ROWS)e->first_row=e->row-PT_EDITOR_ROWS+1;
}
static void undo(struct pt_editor *e,int direction)
{
    unsigned generation=e->sampler.generation,song_generation=e->song.generation;
    enum pt_edit_result r=pt_pattern_undo(e->project,&e->history,direction);
    if(generation!=e->sampler.generation)pt_editor_sample_all(e);
    if(song_generation!=e->song.generation) {
        if(e->position>=e->project->order_count)e->position=e->project->order_count-1;
        if(e->pattern>=e->project->pattern_count || (e->panel==1 && e->song_details))e->pattern=e->project->orders[e->position];
        memset(&e->selection,0,sizeof(e->selection));++e->sample_ui;
    }
    pt_editor_status(e,r==PT_EDIT_OK?(direction<0?"UNDO":"REDO"):r==PT_EDIT_END?"NO MORE HISTORY":"UNDO CONFLICT - EDIT PRESERVED");
}
static void song_panel(struct pt_editor *e)
{e->panel=1;e->song_details=1;e->note_details=0;++e->sample_ui;pt_editor_status(e,"POSITION: ARROWS SELECT / ASSIGN; A ADD POS; N NEW PATTERN");}
static void song_position(struct pt_editor *e,int direction)
{
    if(direction<0 && e->position)--e->position;
    if(direction>0 && e->position+1<e->project->order_count)++e->position;
    e->pattern=e->project->orders[e->position];memset(&e->selection,0,sizeof(e->selection));++e->sample_ui;
}
static void song_edit(struct pt_editor *e,int action)
{
    enum pt_edit_result r;unsigned value=e->project->orders[e->position];
    if(action<2) {
        if((action==0 && !value) || (action==1 && value+1==e->project->pattern_count)) {pt_editor_status(e,"PATTERN LIMIT - NO CHANGE");return;}
        r=pt_song_assign(&e->song,e->project,&e->history,e->position,action?value+1:value-1);
    } else r=pt_song_append(&e->song,e->project,&e->history,e->pattern,action==3);
    if(r==PT_EDIT_OK) {
        if(action>=2)e->position=e->project->order_count-1;
        e->pattern=e->project->orders[e->position];memset(&e->selection,0,sizeof(e->selection));++e->sample_ui;
    }
    pt_editor_status(e,r==PT_EDIT_OK?"SONG UPDATED - CONTROL-Z TO UNDO":r==PT_EDIT_CAPACITY?"SONG OR MEMORY LIMIT - NO CHANGE":"SONG EDIT REFUSED - NO CHANGE");
}
static void channel_panel(struct pt_editor *e)
{
    e->panel=4;e->channel_details=0;++e->sample_ui;pt_editor_status(e,"CHANNEL: P/A/M ROUTE; U MUTE; S SOLO; D DETAILS");
}
/* Route is exclusive; mute and solo are independent saved channel properties. */
static void channel_edit(struct pt_editor *e,unsigned setting)
{
    unsigned selected=e->project->channels.selected;
    struct pt_channel value=e->project->channels.track[selected];
    if(setting==8)value.muted^=1;
    else if(setting==16)value.solo^=1;
    else value.route=(uint8_t)setting;
    channel_value(e,&value);
}
static int apply(struct pt_editor *e,struct pt_event event)
{
    struct pt_event_update u;enum pt_edit_result r;
    u.index=(e->pattern*64+e->row)*e->project->channels.count+e->project->channels.selected;u.event=event;
    r=pt_pattern_apply(e->project,&e->history,&u,1);
    pt_editor_status(e,r==PT_EDIT_OK?"PATTERN EDITED":r==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - NO CHANGE":"INVALID EVENT - NO CHANGE");
    return r==PT_EDIT_OK;
}
int pt_editor_selection(const struct pt_editor *e,struct pt_editor_selection *s)
{
    *s=e->selection;
    if(!s->active || s->pattern!=e->pattern) {memset(s,0,sizeof(*s));return 0;}
    if(s->marking) {
        unsigned c=e->project->channels.selected;
        s->r0=e->row<s->anchor_row?e->row:s->anchor_row;
        s->r1=(e->row>s->anchor_row?e->row:s->anchor_row)+1;
        s->c0=c<s->anchor_channel?c:s->anchor_channel;
        s->c1=(c>s->anchor_channel?c:s->anchor_channel)+1;
    }
    return 1;
}
static void unmark(struct pt_editor *e)
{memset(&e->selection,0,sizeof(e->selection));pt_editor_status(e,"BLOCK UNMARKED");}
static void mark(struct pt_editor *e)
{
    if(e->selection.active) {unmark(e);return;}
    e->selection.active=1;e->selection.marking=1;e->selection.pattern=e->pattern;
    e->selection.anchor_row=e->row;e->selection.anchor_channel=e->project->channels.selected;
    pt_editor_status(e,"MARKING BLOCK - MOVE CURSOR; COPY FREEZES SELECTION");
}
static void select_all(struct pt_editor *e)
{
    memset(&e->selection,0,sizeof(e->selection));e->selection.active=1;e->selection.pattern=e->pattern;
    e->selection.r1=64;e->selection.c1=e->project->channels.count;
    pt_editor_status(e,"WHOLE PATTERN SELECTED - COPY TO CLONE AT ROW 00 CH 1");
}
/* op: 0 copy, 1 paste, 2 clear, -1/+3 transpose down/up. */
static void block_edit(struct pt_editor *e,int op)
{
    struct pt_editor_selection s;enum pt_edit_result result;unsigned r,c,n=0;
    uint32_t revision=e->history.revision;char status[76];
    if(op!=1 && !pt_editor_selection(e,&s)) {pt_editor_status(e,"MARK A BLOCK FIRST - CONTROL-B OR EDIT OP. > MARK");return;}
    if(op==0) {
        result=pt_pattern_copy(e->project,e->pattern,s.r0,s.r1,s.c0,s.c1,&e->clipboard);
        if(result==PT_EDIT_OK) {
            s.marking=0;e->selection=s;
            snprintf(status,sizeof(status),"COPIED %u ROWS X %u CHANNELS - PASTE AT CURSOR",e->clipboard.rows,e->clipboard.channels);
            pt_editor_status(e,status);return;
        }
    } else if(op==1) {
        if(!e->clipboard.rows) {pt_editor_status(e,"CLIPBOARD EMPTY - COPY A BLOCK FIRST");return;}
        if(e->row+e->clipboard.rows>64 || e->project->channels.selected+e->clipboard.channels>e->project->channels.count) {
            pt_editor_status(e,"PASTE WOULD CROSS PATTERN EDGE - NO CHANGE");return;
        }
        result=pt_pattern_paste(e->project,&e->history,e->pattern,e->row,e->project->channels.selected,&e->clipboard,e->scratch,1024);
    } else if(op==2) {
        for(r=s.r0;r<s.r1;++r)for(c=s.c0;c<s.c1;++c) {
            e->scratch[n].index=(e->pattern*64+r)*e->project->channels.count+c;
            memset(&e->scratch[n].event,0,sizeof(e->scratch[n].event));++n;
        }
        result=pt_pattern_apply(e->project,&e->history,e->scratch,n);
    } else result=pt_pattern_transpose(e->project,&e->history,e->pattern,s.r0,s.r1,s.c0,s.c1,op<0?-1:1,e->scratch,1024);
    if(result==PT_EDIT_OK) {
        if(op!=1) {s.marking=0;e->selection=s;}
        pt_editor_status(e,e->history.revision==revision?"BLOCK ALREADY MATCHES - NO CHANGE":
            op==1?"BLOCK PASTED - CONTROL-Z TO UNDO":op==2?"BLOCK CLEARED - CONTROL-Z TO UNDO":"BLOCK TRANSPOSED - CONTROL-Z TO UNDO");
    } else pt_editor_status(e,result==PT_EDIT_UNSUPPORTED?"TRANSPOSE OUT OF RANGE OR RAW PERIOD - NO CHANGE":
        result==PT_EDIT_CAPACITY?"UNDO BUDGET EXCEEDED - NO CHANGE":"BLOCK EDIT REFUSED - NO CHANGE");
}
static enum pt_editor_action quit(struct pt_editor *e)
{
    if(!pt_editor_dirty(e) || e->quit_pending)return PT_UI_QUIT;
    e->quit_pending=1;pt_editor_status(e,"UNSAVED EDITS: ESC AGAIN TO DISCARD; OTHER KEY CANCELS");return PT_UI_NONE;
}
static void new_panel(struct pt_editor *e)
{
    e->panel=3;e->new_channels=e->project->channels.count;e->new_pending=0;
    pt_editor_status(e,"NEW SONG: CHOOSE CHANNELS; CREATE OR ENTER TO CONTINUE");
}
static enum pt_editor_action request_new(struct pt_editor *e)
{
    if(pt_editor_dirty(e) && !e->new_pending) {
        e->new_pending=1;pt_editor_status(e,"UNSAVED EDITS: CREATE AGAIN TO DISCARD; OTHER INPUT CANCELS");return PT_UI_NONE;
    }
    e->new_pending=0;return PT_UI_NEW;
}
static enum pt_editor_action request_load(struct pt_editor *e)
{
    e->quit_pending=0;
    if(pt_editor_dirty(e) && !e->load_pending) {
        e->load_pending=1;pt_editor_status(e,"UNSAVED EDITS: LOAD AGAIN TO DISCARD; OTHER KEY CANCELS");return PT_UI_NONE;
    }
    e->load_pending=0;return PT_UI_LOAD;
}
static void pattern_step(struct pt_editor *e,int d)
{e->pattern=(e->pattern+e->project->pattern_count+d)%e->project->pattern_count;memset(&e->selection,0,sizeof(e->selection));}
static int hexkey(unsigned raw)
{
    if(raw>=1 && raw<=9)return (int)raw;
    if(raw==10)return 0;
    switch(raw) {case 0x20:return 10;case 0x35:return 11;case 0x33:return 12;
        case 0x22:return 13;case 0x12:return 14;case 0x23:return 15;default:return -1;}
}
enum pt_editor_action pt_editor_key(struct pt_editor *e,unsigned raw,unsigned qualifier)
{
    struct pt_project *p=e->project;struct pt_event event;int n=-1,h;unsigned i;
    static const unsigned keys[24]={0x31,0x21,0x32,0x22,0x33,0x34,0x24,0x35,0x25,0x36,0x26,0x37,
        0x10,0x02,0x11,0x03,0x12,0x13,0x05,0x14,0x06,0x15,0x07,0x16};
    if(raw&0x80)return PT_UI_NONE;
    if(e->number_field) {if(!(qualifier&8))number_key(e,raw);return PT_UI_NONE;}
    if(e->name_entry) {if(!(qualifier&8))name_key(e,raw);return PT_UI_NONE;}
    if(e->panel==11) {
        if(raw==0x45 || raw==0x42 || ((qualifier&8) && raw==0x28)) {source_close(e);pt_editor_status(e,"MOD SOURCE CLOSED - APPLIED IMPORTS KEPT");}
        else if((qualifier&8) && raw==0x31)undo(e,(qualifier&3)?1:-1);
        else if((qualifier&8) && raw==0x21)return (qualifier&3)?PT_UI_SAVE_AS:PT_UI_SAVE;
        else if(qualifier&8)return PT_UI_NONE;
        else if(raw==0x28)return PT_UI_SOURCE_LOAD;
        else if(raw==0x44 || raw==0x17)source_apply(e);
        else if(raw==0x4f || raw==0x4e)source_select(e,0,raw==0x4f?-1:1);
        else if(raw==0x0b || raw==0x0c)source_select(e,1,raw==0x0b?-1:1);
        else if(raw==0x59 || raw==0x40)return PT_UI_STOP;
        return PT_UI_NONE;
    }
    if(e->panel==3 && raw==0x44)return request_new(e);
    if(e->new_pending)pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");
    e->new_pending=0;
    if((qualifier&8) && raw==0x18) {if((qualifier&3) && e->panel>=5) {e->load_pending=0;e->quit_pending=0;return PT_UI_SAMPLE_LOAD;}return request_load(e);}
    if(e->load_pending)pt_editor_status(e,"LOAD CANCELLED - EDITS PRESERVED");
    e->load_pending=0;
    if(raw==0x45 && e->panel==3) {e->panel=0;pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");return PT_UI_NONE;}
    if(raw==0x45 && e->panel==1 && e->song_details) {e->song_details=0;e->panel=0;++e->sample_ui;pt_editor_status(e,"POSITION EDITOR CLOSED");return PT_UI_NONE;}
    if(raw==0x45 && e->panel==1 && e->note_details) {e->note_details=0;++e->sample_ui;pt_editor_status(e,"EDIT OPERATIONS");return PT_UI_NONE;}
    if(raw==0x45 && e->panel==4 && e->channel_details) {channel_panel(e);return PT_UI_NONE;}
    if(raw==0x45 && (e->panel==4 || e->panel>=5)) {e->panel=0;pt_editor_status(e,"SETTINGS CLOSED");return PT_UI_NONE;}
    if(raw==0x45)return quit(e);
    e->quit_pending=0;
    if(e->panel==3) {
        if(raw==0x0b && e->new_channels>1)--e->new_channels;
        if(raw==0x0c && e->new_channels<16)++e->new_channels;
        return PT_UI_NONE;
    }
    /* Raw Amiga qualifiers: either Shift=bits0/1, Control=bit3. */
    if(qualifier&8) {
        if(raw==0x31)undo(e,(qualifier&3)?1:-1);
        else if(raw==0x21)return (qualifier&3)?PT_UI_SAVE_AS:PT_UI_SAVE;
        else if(raw==0x36) {if(qualifier&3)name_begin(e,2);else new_panel(e);} /* Control-N / Control-Shift-N */
        else if(raw==0x14 && (qualifier&3))name_begin(e,3); /* Control-Shift-T */
        else if(raw==0x37 && (qualifier&3))return PT_UI_EXPORT_MOD;
        else if(raw==0x19) {if(e->panel==1 && e->song_details) {e->song_details=0;e->panel=0;++e->sample_ui;}else song_panel(e);} /* Control-P */
        else if(raw==0x13) {if(e->panel==4)e->panel=0;else channel_panel(e);} /* Control-R */
        else if(raw==0x17) {if(e->panel==1 && e->note_details) {e->note_details=0;++e->sample_ui;}else note_panel(e);} /* Control-I */
        else if(raw==0x28) {if(e->panel>=5)e->panel=0;else sampler_panel(e);} /* Control-L */
        else if(e->panel>=5 && raw==0x20) {pt_editor_sample_all(e);pt_editor_status(e,"WHOLE SAMPLE SELECTED");}
        else if(e->panel==4 || e->panel>=5 || (e->panel==1 && (e->note_details || e->song_details)))return PT_UI_NONE;
        else if(raw==0x12) {e->panel=e->panel==1?0:1;e->note_details=0;e->song_details=0;} /* Control-E */
        else if(raw==0x35)mark(e); /* Control-B */
        else if(raw==0x20)select_all(e);
        else if(raw==0x33)block_edit(e,0);
        else if(raw==0x34)block_edit(e,1);
        else if(raw==0x46)block_edit(e,2);
        else if(raw==0x0b)block_edit(e,-1);
        else if(raw==0x0c)block_edit(e,3);
        return PT_UI_NONE;
    }
    if((qualifier&0x30) && raw>=0x4c && raw<=0x4f) {
        sample_attribute_step(e,raw<=0x4d?3:5,raw==0x4c || raw==0x4e?1:-1);return PT_UI_NONE;
    }
    if(e->panel==1 && e->song_details) {
        if(raw==0x4c || raw==0x4d)song_position(e,raw==0x4c?-1:1);
        else if(raw==0x4f || raw==0x4e)song_edit(e,raw==0x4f?0:1);
        else if(raw==0x20)song_edit(e,2);
        else if(raw==0x36)song_edit(e,3);
        else if(raw==0x44) {e->panel=0;e->song_details=0;++e->sample_ui;}
        else if(raw==0x57)return PT_UI_PLAY;
        else if(raw==0x58)return PT_UI_PATTERN;
        else if(raw==0x59 || raw==0x40)return PT_UI_STOP;
        return PT_UI_NONE;
    }
    if(e->panel==1 && e->note_details) {
        if(raw==0x21)number_begin(e,8);
        else if(raw==0x0b || raw==0x0c)note_step(e,raw==0x0b?-1:1);
        else if(raw==0x33)(void)note_slice(e,0);
        else if(raw==0x16)note_sample(e);
        else if(raw==0x28) {if(current_event(e)->instrument)e->sample=current_event(e)->instrument;sampler_panel(e);}
        else if(raw==0x1a || raw==0x1b) {if(raw==0x1a && e->sample>1)--e->sample;else if(raw==0x1b && e->sample<e->project->sample_count)++e->sample;pt_editor_sample_all(e);}
        else if(raw==0x4c || raw==0x4d) {e->row=(e->row+(raw==0x4c?63:1))%64;visible(e);}
        else if(raw==0x42 || raw==0x4e || raw==0x4f)pt_channels_step(&p->channels,raw==0x4f || (raw==0x42 && (qualifier&3))?-1:1);
        else if(raw>=0x50 && raw<=0x53) {i=(raw-0x50)*4;if(i<p->channels.count)p->channels.selected=(uint8_t)i;}
        else if(raw==0x57 || raw==0x44)return PT_UI_PLAY;
        else if(raw==0x58)return PT_UI_PATTERN;
        else if(raw==0x59 || raw==0x40)return PT_UI_STOP;
        return PT_UI_NONE;
    }
    if(e->panel>=5) {
        if(raw==0x0b || raw==0x0c) {if(raw==0x0b && e->sample>1)--e->sample;else if(raw==0x0c && e->sample<p->sample_count)++e->sample;pt_editor_sample_all(e);}
        else if(raw==0x42)sample_tab(e,e->panel>=8?5:e->panel+1);
        else if(raw==0x20) {pt_editor_sample_all(e);pt_editor_status(e,"WHOLE SAMPLE SELECTED");}
        else if(raw==0x57 || raw==0x44)return PT_UI_AUDITION;
        else if(raw==0x59 || raw==0x40)return PT_UI_STOP;
        else if(raw==0x33 && (e->panel==5 || e->panel==8))sample_tab(e,9);
        else if(e->panel==5 && raw==0x32)sample_tab(e,10);
        else if(e->panel==10) {
            if(raw==0x28)return PT_UI_RAW_LOAD;
            if(raw==0x11)return PT_UI_RAW_SAVE;
            if(raw==0x13)number_begin(e,4);
            else {
                if(raw>=1 && raw<=3) {e->raw_format.bits=(uint8_t)(raw*8);if(raw!=1)e->raw_format.unsigned8=0;}
                else if(raw==0x37)e->raw_format.channels=1;
                else if(raw==0x21)e->raw_format.channels=2;
                else if(raw==0x16 && e->raw_format.bits==8)e->raw_format.unsigned8^=1;
                else if(raw==0x12)e->raw_format.little_endian^=1;
                ++e->sample_ui;pt_editor_status(e,"RAW SETTINGS ONLY - SAMPLE AND HISTORY UNCHANGED");
            }
        }
        else if(e->panel==9) {
            if(raw>=1 && raw<=3)format_setting(e,raw*8,0);
            else if(raw==0x13)number_begin(e,3);
            else if(raw==0x19)format_apply(e);
            else if(raw==0x23) {e->format_filtered^=1;++e->sample_ui;format_setting(e,0,e->format_rate);}
        }
        else if(e->panel==8) {
            if(raw==0x17)wave_view(e,0);
            else if(raw==0x18)wave_view(e,1);
            else if(raw==0x23)wave_view(e,2);
            else if(raw==0x34)wave_view(e,3);
            else if(raw==0x4f || raw==0x4e)wave_view(e,raw==0x4f?4:5);
            else if(raw==0x21 || raw==0x12)number_begin(e,raw==0x21?1:2);
            else if(raw==0x1a || raw==0x1b)range_nudge(e,(qualifier&3)?2:1,raw==0x1a?-1:1);
        }
        else if(e->panel==6) {
            if(raw==0x11)return PT_UI_SAMPLE_SVX;
            else if(raw==0x23)loop_edit(e,PT_LOOP_FORWARD);
            else if(raw==0x19)loop_edit(e,PT_LOOP_PINGPONG);
            else if(raw==0x18)loop_edit(e,PT_LOOP_NONE);
            else if(raw==0x35)loop_edit(e,PT_LOOP_CROSSFADE);
            else if(raw==0x16)loop_edit(e,4);
            else if(raw==0x1a || raw==0x1b)sample_setting(e,0,raw==0x1a?-1:1);
        } else if(e->panel==7) {
            if(raw==0x37)slice_edit(e,0);
            else if(raw==0x22)slice_edit(e,1);
            else if(raw==0x33)slice_edit(e,2);
            else if(raw==0x14)slice_edit(e,3);
            else if(raw==0x19)slice_edit(e,4);
            else if(raw==0x32)slice_edit(e,5);
            else if(raw==0x31)sample_setting(e,3,0);
            else if(raw==0x1a || raw==0x1b)sample_setting(e,1,raw==0x1a?-1:1);
            else if(raw==0x4c || raw==0x4d)sample_setting(e,2,raw==0x4c?1:-1);
        }
        else if(raw==0x28)return (qualifier&3)?PT_UI_SOURCE_LOAD:PT_UI_SAMPLE_LOAD;
        else if(raw==0x11)return (qualifier&3)?PT_UI_SAMPLE_SVX:PT_UI_SAMPLE_SAVE;
        else if(raw==0x13)sample_edit(e,PT_PCM_REVERSE,0);
        else if(raw==0x36)sample_edit(e,PT_PCM_NORMALIZE,0);
        else if(raw==0x22)sample_edit(e,PT_PCM_REMOVE_DC,0);
        else if(raw==0x17)sample_edit(e,PT_PCM_FADE_IN,0);
        else if(raw==0x18)sample_edit(e,PT_PCM_FADE_OUT,0);
        else if(raw==0x24 || raw==0x25)sample_edit(e,PT_PCM_GAIN,raw==0x24?2000:500);
        return PT_UI_NONE;
    }
    if(e->panel==4) {
        if(raw==0x22) {e->channel_details^=1;++e->sample_ui;pt_editor_status(e,e->channel_details?"DETAILS: P PAN / G GROUP / M MIDI CHANNEL / N NAME":"CHANNEL: P/A/M ROUTE; U MUTE; S SOLO; D DETAILS");}
        else if(e->channel_details && raw==0x19)number_begin(e,5);
        else if(e->channel_details && raw==0x24)number_begin(e,6);
        else if(e->channel_details && raw==0x37)number_begin(e,7);
        else if(e->channel_details && raw==0x36)name_begin(e,1);
        else if(raw==0x19)channel_edit(e,PT_PAULA);
        else if(!e->channel_details && raw==0x20)channel_edit(e,PT_AMIGUS);
        else if(raw==0x37)channel_edit(e,PT_MIDI);
        else if(raw==0x16)channel_edit(e,8);
        else if(raw==0x21)channel_edit(e,16);
        else if(raw==0x42 || raw==0x4e || raw==0x4f)pt_channels_step(&p->channels,raw==0x4f || (raw==0x42 && (qualifier&3))?-1:1);
        else if(raw>=0x50 && raw<=0x53) {i=(raw-0x50)*4;if(i<p->channels.count)p->channels.selected=(uint8_t)i;}
        else if(raw==0x57)return PT_UI_PLAY;
        else if(raw==0x44)return (qualifier&3)?PT_UI_PATTERN:PT_UI_PLAY;
        else if(raw==0x58)return PT_UI_PATTERN;
        else if(raw==0x59 || (raw==0x40 && e->playback.active))return PT_UI_STOP;
        return PT_UI_NONE;
    }
    switch(raw) {
        case 0x57:return PT_UI_PLAY;
        case 0x58:return PT_UI_PATTERN;
        case 0x59:return PT_UI_STOP;
        case 0x44:return (qualifier&3)?PT_UI_PATTERN:PT_UI_PLAY;
        case 0x40:if(e->playback.active)return PT_UI_STOP;e->editing=!e->editing;pt_editor_status(e,e->editing?"EDIT ON - NOTES Z-M/Q-U; DELETE CLEARS; BACKSPACE OFF":"EDIT OFF");break;
        case 0x4c:e->row=(e->row+63)%64;visible(e);break;
        case 0x4d:e->row=(e->row+1)%64;visible(e);break;
        case 0x4e:if(++e->field==6) {e->field=0;pt_channels_step(&p->channels,1);}break;
        case 0x4f:if(!e->field) {e->field=5;pt_channels_step(&p->channels,-1);}else --e->field;break;
        case 0x42:pt_channels_step(&p->channels,(qualifier&3)?-1:1);break;
        case 0x50:case 0x51:case 0x52:case 0x53:
            i=(raw-0x50)*4;if(i<p->channels.count)p->channels.selected=(uint8_t)i;break;
        case 0x54:e->octave=0;break;
        case 0x55:e->octave=1;break;
        case 0x56:e->octave=2;break;
        case 0x0b:if(e->sample)--e->sample;break;
        case 0x0c:if(e->sample<p->sample_count)++e->sample;break;
        case 0x5a:pattern_step(e,-1);break; /* numeric keypad ( */
        case 0x5b:pattern_step(e,1);break;
        default:
            if(!e->editing)break;
            event=p->events[(e->pattern*64+e->row)*p->channels.count+p->channels.selected];
            if(raw==0x46) {memset(&event,0,sizeof(event));if(apply(e,event))e->row=(e->row+1)%64;}
            else if(e->field==0) {
                if(raw==0x41) {event.kind=PT_NOTE_OFF;event.pitch=0;event.slice=0;event.flags=0;event.velocity=0;}
                else {
                    for(i=0;i<24;++i)if(raw==keys[i]) {n=(int)(i+e->octave*12);break;}
                    if(n<0)break;
                    if(p->channels.track[p->channels.selected].route==PT_MIDI) {event.kind=PT_NOTE_MIDI;event.pitch=(uint16_t)(n+12);}
                    else {if(n>=36) {pt_editor_status(e,"NOTE OUTSIDE CLASSIC THREE-OCTAVE RANGE");break;}event.kind=PT_NOTE_PERIOD;event.pitch=(uint16_t)periods[n];}
                    event.instrument=(uint8_t)e->sample;event.slice=0;event.flags=0;event.velocity=0;
                }
                if(apply(e,event))e->row=(e->row+1)%64;
            } else if((h=hexkey(raw))>=0) {
                if(e->field==1)event.instrument=(uint8_t)((event.instrument&15)|(h<<4));
                if(e->field==2)event.instrument=(uint8_t)((event.instrument&240)|h);
                if(e->field==3)event.effect=(uint8_t)h;
                if(e->field==4)event.parameter=(uint8_t)((event.parameter&15)|(h<<4));
                if(e->field==5)event.parameter=(uint8_t)((event.parameter&240)|h);
                if(apply(e,event)) {if(e->field==5) {e->field=3;e->row=(e->row+1)%64;}else ++e->field;}
            }
            visible(e);break;
    }
    return PT_UI_NONE;
}
enum pt_editor_action pt_editor_click(struct pt_editor *e,int x,int y)
{
    unsigned c,r,f;
    if(x<0 || x>=640 || y<0 || y>=512)return PT_UI_NONE;
    if(e->number_field || e->name_entry) {pt_editor_status(e,"FINISH ENTRY WITH RETURN OR ESC FIRST");return PT_UI_NONE;}
    if(e->panel==11) {
        if(x>=230 && x<599 && y>=21 && y<97) {
            r=(unsigned)(y-2)/19;c=(unsigned)(x-230)/123;
            if(r==1) {if(!c)return PT_UI_SOURCE_LOAD;source_select(e,0,c==1?-1:1);}
            else if(r==3 && c!=1)source_select(e,1,c==0?-1:1);
            else if(r==4) {if(!c)source_apply(e);else if(c==1) {source_close(e);pt_editor_status(e,"MOD SOURCE CLOSED - APPLIED IMPORTS KEPT");}else return PT_UI_STOP;}
        }
        return PT_UI_NONE;
    }
    if(e->panel==3 && x>=230 && x<414 && y>=59 && y<97)return request_new(e);
    if(e->new_pending)pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");
    e->new_pending=0;
    if(x>=590 && y>=174 && y<193)return request_load(e);
    if(e->load_pending)pt_editor_status(e,"LOAD CANCELLED - EDITS PRESERVED");
    e->load_pending=0;
    if(e->panel==2 && x>=414 && x<599 && y>=21 && y<59)return quit(e);
    e->quit_pending=0;
    if(e->panel==3) {
        if(x>=230 && x<599 && y>=21 && y<40) {
            if(x<353 && e->new_channels>1)--e->new_channels;
            if(x>=476 && e->new_channels<16)++e->new_channels;
        } else if(x>=414 && x<599 && y>=59 && y<97) {e->panel=0;pt_editor_status(e,"NEW SONG CANCELLED - EDITS PRESERVED");}
        return PT_UI_NONE;
    }
    if(e->panel>=5 && x>=230 && x<599 && y>=2 && y<97) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(r==0) {sample_tab(e,5+((unsigned)(x-230+1)*4-1)/369);return PT_UI_NONE;}
        if(e->panel==10) {
            if(r==1) {if(c<2)return c==0?PT_UI_RAW_LOAD:PT_UI_RAW_SAVE;sample_tab(e,5);}
            else if(r==2) {e->raw_format.bits=(uint8_t)((c+1)*8);if(c)e->raw_format.unsigned8=0;}
            else if(r==3) {if(c<2)e->raw_format.channels=(uint8_t)(c+1);else if(e->raw_format.bits==8)e->raw_format.unsigned8^=1;}
            else if(r==4) {if(c<2)e->raw_format.little_endian=(uint8_t)c;else number_begin(e,4);}
            ++e->sample_ui;return PT_UI_NONE;
        }
        if(e->panel==9) {
            if(r==1)format_setting(e,(c+1)*8,0);
            else if(r==2)format_setting(e,0,c==0?8287:c==1?22050:44100);
            else if(r==3) {if(c==0)format_setting(e,0,48000);else if(c==1)number_begin(e,3);else {e->format_filtered^=1;++e->sample_ui;format_setting(e,0,e->format_rate);}}
            else if(r==4) {if(c==0)format_apply(e);else undo(e,c==1?-1:1);}
            return PT_UI_NONE;
        }
        if(e->panel==8) {
            if(r==1)wave_view(e,c);
            else if(r==2)wave_view(e,c==0?4:c==1?3:5);
            else if(r==3 || r==4) {if(c==1)number_begin(e,r-2);else range_nudge(e,r-2,c==0?-1:1);}
            return PT_UI_NONE;
        }
        if(e->panel==6) {
            if(r==1)loop_edit(e,c==0?PT_LOOP_FORWARD:c==1?PT_LOOP_PINGPONG:PT_LOOP_NONE);
            else if(r==2 && c!=1)sample_setting(e,0,c==0?-1:1);
            else if(r==3) {if(c==2)return PT_UI_SAMPLE_SVX;else loop_edit(e,c==0?PT_LOOP_CROSSFADE:4);}
            else if(r==4) {if(c==0)pt_editor_sample_all(e);else undo(e,c==1?-1:1);}
            return PT_UI_NONE;
        }
        if(e->panel==7) {
            if(r==1)slice_edit(e,c);
            else if(r==2)slice_edit(e,3+c);
            else if(r==3 && c!=1)sample_setting(e,1,c==0?-1:1);
            else if(r==4)sample_setting(e,c==1?3:2,c==0?-1:1);
            return PT_UI_NONE;
        }
        if(r==1)return c==0?PT_UI_SAMPLE_LOAD:c==1?PT_UI_SAMPLE_SAVE:PT_UI_AUDITION;
        if(r==2)sample_edit(e,c==0?PT_PCM_REVERSE:c==1?PT_PCM_NORMALIZE:PT_PCM_REMOVE_DC,0);
        if(r==3) {if(c==2)sample_tab(e,9);else sample_edit(e,PT_PCM_GAIN,c==0?500:2000);}
        if(r==4) {if(c==2)sample_tab(e,10);else sample_edit(e,c==0?PT_PCM_FADE_IN:PT_PCM_FADE_OUT,0);}
        return PT_UI_NONE;
    }
    if(e->panel>=5 && y>=PT_EDITOR_HEADER_Y && y<PT_EDITOR_BOTTOM_Y) {
        if(e->sample && e->sample<=e->project->sample_count && x>=10 && x<630 && y>=254 && y<470) {
            uint32_t frames=e->project->samples[e->sample-1].pcm.frames;
            uint32_t view_start,view_end,point;
            pt_editor_wave_bounds(e,&view_start,&view_end);
            point=view_start+(uint32_t)((uint64_t)(x-10)*(view_end-view_start)/619);
            if(e->sample_range_slot!=e->sample)pt_editor_sample_all(e);
            if(frames && !e->sample_marking) {e->sample_anchor=point;e->sample_marking=1;pt_editor_status(e,"RANGE START SET - CLICK END OR ALL");}
            else if(frames) {
                e->sample_start=point<e->sample_anchor?point:e->sample_anchor;
                e->sample_end=point>e->sample_anchor?point:e->sample_anchor;
                if(e->sample_start==frames)--e->sample_start;
                if(e->sample_end==e->sample_start)++e->sample_end;
                e->sample_marking=0;pt_editor_status(e,"SAMPLE RANGE SELECTED");
            }
        }
        return PT_UI_NONE;
    }
    if(x>=128 && x<590 && y>=174 && y<193) {name_begin(e,3);return PT_UI_NONE;}
    if(x>=128 && x<590 && y>=193 && y<211) {name_begin(e,2);return PT_UI_NONE;}
    if(e->panel==4 && x>=230 && x<599 && y>=2 && y<97) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(e->channel_details && r==1)number_begin(e,5+c);
        else if(e->channel_details && r==2)name_begin(e,1);
        else if(r==1)channel_edit(e,c==0?PT_PAULA:c==1?PT_AMIGUS:PT_MIDI);
        else if(r==2) {if(c<2)channel_edit(e,c==0?8:16);else {e->channel_details=1;++e->sample_ui;pt_editor_status(e,"DETAILS: P PAN / G GROUP / M MIDI CHANNEL / N NAME");}}
        else if(r==3 && c!=1)pt_channels_step(&e->project->channels,c==0?-1:1);
        else if(r==4) {if(c<2)undo(e,c==0?-1:1);else if(e->channel_details)channel_panel(e);else e->panel=0;}
        return PT_UI_NONE;
    }
    if(e->panel==1 && e->song_details && x>=230 && x<599 && y>=2 && y<97) {
        r=(unsigned)(y-2)/19;c=(unsigned)(x-230)/123;
        if(r==1) {if(c==0)song_position(e,-1);else song_edit(e,(int)c-1);}
        else if(r==2) {if(c==0)song_position(e,1);else song_edit(e,(int)c+1);}
        else if(r==3 && c<2)undo(e,c==0?-1:1);
        else if(r==4) {if(c==0) {e->panel=0;e->song_details=0;++e->sample_ui;}else return c==1?PT_UI_PLAY:PT_UI_STOP;}
        return PT_UI_NONE;
    }
    if(e->panel==1 && x>=230 && x<599 && y>=2 && y<97) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(e->note_details) {
            if(r==1) {if(c==1)number_begin(e,8);else note_step(e,c==0?-1:1);}
            else if(r==2) {if(c==0)note_sample(e);else if(c==1)(void)note_slice(e,0);else {if(current_event(e)->instrument)e->sample=current_event(e)->instrument;sampler_panel(e);}}
            else if(r==4) {if(c<2)undo(e,c==0?-1:1);else {e->note_details=0;++e->sample_ui;pt_editor_status(e,"EDIT OPERATIONS");}}
            return PT_UI_NONE;
        }
        if(r==1) {if(c<2)undo(e,c==0?-1:1);else mark(e);}
        else if(r==2)block_edit(e,(int)c);
        else if(r==3) {if(c==2)select_all(e);else block_edit(e,c==0?-1:3);}
        else if(r==4) {if(c==0)unmark(e);else if(c==1)e->panel=0;else note_panel(e);}
        return PT_UI_NONE;
    }
    if(e->panel==2 && x>=230 && x<599 && y>=2 && y<97) {
        if(y>=78)e->panel=0;
        else if(y>=59)return PT_UI_EXPORT_MOD;
        else if(y>=21) {if(e->panel==2)return PT_UI_SAVE_AS;undo(e,x<414?-1:1);}
        return PT_UI_NONE;
    }
    if(y>=PT_EDITOR_BOTTOM_Y && x>=4 && x<248) {c=(unsigned)(x-4)/61*4;if(c<e->project->channels.count)e->project->channels.selected=(uint8_t)c;}
    else if(y>=PT_EDITOR_PATTERN_Y && y<PT_EDITOR_PATTERN_Y+240 && x>=38 && x<638) {
        c=(unsigned)(x-38)/150+pt_channels_page(&e->project->channels)*4;
        r=(unsigned)(y-PT_EDITOR_PATTERN_Y)/12+e->first_row;
        if(c<e->project->channels.count && r<64) {
            e->project->channels.selected=(uint8_t)c;e->row=r;
            f=(unsigned)(x-38)%150;
            e->field=f<60?0:f<78?1:f<98?2:f<114?3:f<128?4:5;
        }
    } else if(y>=PT_EDITOR_HEADER_Y && y<PT_EDITOR_PATTERN_Y && x>=38 && x<638) {
        c=(unsigned)(x-38)/150+pt_channels_page(&e->project->channels)*4;
        if(c<e->project->channels.count) {e->project->channels.selected=(uint8_t)c;
            if((unsigned)(x-38)%150>=120)channel_panel(e);}
    } else if(x>=230 && x<599 && y>=PT_EDITOR_CONTROL_Y && y<PT_EDITOR_COMMAND_BOTTOM) {
        r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;c=(unsigned)(x-230)/123;
        if(r==0 && c==0)return PT_UI_PLAY;
        else if(r==0 && c==1)return PT_UI_STOP;
        else if(r==1 && c==0)return PT_UI_PATTERN;
        else if(r==1 && c==1)new_panel(e);
        else if(r==4 && c==0)return PT_UI_AUDITION;
        else if(r==2 && c==0) {e->editing=!e->editing;pt_editor_status(e,e->editing?"EDIT ON":"EDIT OFF");}
        else if(r==2 && c==1) {e->panel=1;e->note_details=0;e->song_details=0;}
        else if(r==2 && c==2)song_panel(e);
        else if(r==3 && c==1)e->panel=2;
        else if((r==3 && c==2) || (r==4 && c==1))sampler_panel(e);
        else pt_editor_status(e,"THIS CONTROL IS NOT YET CONNECTED");
    } else if(x>=190 && x<230 && y>=2 && y<173) {
        int direction=x<210?-1:1;r=(unsigned)(y-PT_EDITOR_CONTROL_Y)/PT_EDITOR_CONTROL_HEIGHT;
        if(r==0) {
            if(direction>0 && e->position+1<e->project->order_count)++e->position;
            if(direction<0 && e->position)--e->position;
            e->pattern=e->project->orders[e->position];memset(&e->selection,0,sizeof(e->selection));
        } else if(r==1)pattern_step(e,direction);
        else if(r==3 || r==5)sample_attribute_step(e,r,direction);
        else if(r==4) {if(direction<0 && e->sample)--e->sample;if(direction>0 && e->sample<e->project->sample_count)++e->sample;pt_editor_sample_all(e);}
        else pt_editor_status(e,"SAMPLE/SONG PARAMETER EDITING NOT YET CONNECTED");
    } else if(y>=PT_EDITOR_BOTTOM_Y && x>=416 && x<476)return PT_UI_PLAY;
    else if(y>=PT_EDITOR_BOTTOM_Y && x>=476 && x<552)return PT_UI_STOP;
    else if((y>=PT_EDITOR_BOTTOM_Y && x>=416 && x<552) || (x>=599 && y<97) || (x>=590 && y>=174 && y<193))
        pt_editor_status(e,"THIS CONTROL IS NOT YET CONNECTED");
    return PT_UI_NONE;
}
