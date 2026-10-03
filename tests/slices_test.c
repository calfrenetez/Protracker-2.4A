#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "slices.h"
static int32_t data[1024],before[1024];
static uint32_t markers[4096];
struct slice_storage {
    int32_t active[16];
    size_t count;
    int32_t spare[16];
};
struct slice_outputs {uint32_t markers[2];size_t count;};
static void slices_output_safety(void)
{
    static const unsigned bits[]={8,16,24};
    struct slice_storage storage,image;
    struct slice_outputs output,output_image;
    struct pt_slice_options options={64,0,500,5},options_image;
    struct pt_pcm pcm,pcm_image;
    size_t b,c,i,n;uint32_t next;
    for(b=0;b<3;++b)for(c=1;c<=2;++c) {
        int32_t unit=bits[b]==8?7:(bits[b]==16?257:65539);
        memset(&pcm,0,sizeof(pcm));pcm.data=storage.active;pcm.capacity=sizeof(storage)/sizeof(*pcm.data);
        pcm.frames=8;pcm.rate=48000;pcm.channels=(uint8_t)c;pcm.bits=(uint8_t)bits[b];
        memset(&storage,0x5a,sizeof(storage));
        for(i=0;i<8;++i) {
            storage.active[i*c]=(int32_t)(i+1)*unit;
            if(c==2)storage.active[i*c+1]=-(int32_t)(i+1)*unit;
        }
        memcpy(&image,&storage,sizeof(storage));memcpy(&pcm_image,&pcm,sizeof(pcm));memcpy(&options_image,&options,sizeof(options));
        memset(&output,0x5a,sizeof(output));output.count=99;
        assert(pt_auto_slice(&pcm,&options,output.markers,2,&output.count)==PT_SLICE_OK);
        assert(output.count==1 && output.markers[0]==0);
        assert(!memcmp(&storage,&image,sizeof(storage)));
        /* Only emitted marker bytes are outputs: adjacent real scalar is legal. */
        assert(pt_auto_slice(&pcm,&options,output.markers,4096,&output.count)==PT_SLICE_OK);
        assert(output.count==1 && output.markers[1]==0x5a5a5a5aUL);
        for(i=0;i<8;++i) {
            unsigned char *source=(unsigned char *)&storage;
            size_t offsets[]={0,4,8*c*sizeof(int32_t)-4,8*c*sizeof(int32_t),
                sizeof(storage.active),sizeof(storage.active)+sizeof(size_t)-1,
                sizeof(storage)-4,sizeof(storage)-1};
            uint32_t *marker=(uint32_t *)(void *)(source+offsets[i]);
            memcpy(&output_image,&output,sizeof(output));
            assert(pt_auto_slice(&pcm,&options,marker,2,&output.count)==PT_SLICE_ALIAS);
            assert(!memcmp(&storage,&image,sizeof(storage)) && !memcmp(&output,&output_image,sizeof(output)));
            assert(pt_auto_slice(&pcm,&options,output.markers,2,(size_t *)(void *)(source+offsets[i]))==PT_SLICE_ALIAS);
            assert(!memcmp(&storage,&image,sizeof(storage)) && !memcmp(&output,&output_image,sizeof(output)));
            assert(pt_pcm_crossfade_loop(&pcm,0,8,2,marker)==PT_PCM_ALIAS);
            assert(!memcmp(&storage,&image,sizeof(storage)));
        }
        /* The actual size_t spare member and descriptor member must be protected. */
        memcpy(&output_image,&output,sizeof(output));
        assert(pt_auto_slice(&pcm,&options,output.markers,2,&storage.count)==PT_SLICE_ALIAS);
        assert(pt_auto_slice(&pcm,&options,output.markers,2,&pcm.capacity)==PT_SLICE_ALIAS);
        assert(pt_auto_slice(&pcm,&options,&pcm.frames,2,&output.count)==PT_SLICE_ALIAS);
        assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&pcm.frames)==PT_PCM_ALIAS);
        assert(pt_auto_slice(&pcm,&options,(uint32_t *)(void *)&options,2,&output.count)==PT_SLICE_ALIAS);
        assert(pt_auto_slice(&pcm,&options,output.markers,2,(size_t *)(void *)&options)==PT_SLICE_ALIAS);
        assert(pt_auto_slice(&pcm,&options,(uint32_t *)(void *)&output.count,2,&output.count)==PT_SLICE_ALIAS);
        assert(!memcmp(&storage,&image,sizeof(storage)) && !memcmp(&pcm,&pcm_image,sizeof(pcm)) &&
            !memcmp(&options,&options_image,sizeof(options)) && !memcmp(&output,&output_image,sizeof(output)));
        /* Geometry/capacity errors still precede aliases, without any mutation. */
        assert(pt_auto_slice(&pcm,&options,(uint32_t *)storage.spare,0,&storage.count)==PT_SLICE_CAPACITY);
        assert(pt_auto_slice(&pcm,&options,NULL,2,&output.count)==PT_SLICE_INVALID);
        assert(pt_pcm_crossfade_loop(&pcm,0,8,5,(uint32_t *)storage.spare)==PT_PCM_INVALID);
        assert(pt_pcm_crossfade_loop(&pcm,0,8,2,NULL)==PT_PCM_INVALID);
        assert(!memcmp(&storage,&image,sizeof(storage)));
        /* Oversized declared storage fails closed, after the old errors. */
        next=99;pcm.capacity=SIZE_MAX/sizeof(*pcm.data)+1;
        assert(pt_auto_slice(&pcm,&options,output.markers,2,&output.count)==PT_SLICE_ALIAS);
        assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&next)==PT_PCM_ALIAS);
        assert(pt_auto_slice(&pcm,&options,output.markers,0,&output.count)==PT_SLICE_CAPACITY);
        pcm.capacity=SIZE_MAX/sizeof(*pcm.data);
        assert(pt_auto_slice(&pcm,&options,output.markers,2,&output.count)==PT_SLICE_ALIAS);
        assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&next)==PT_PCM_ALIAS);
        assert(next==99 && !memcmp(&storage,&image,sizeof(storage)) && !memcmp(&output,&output_image,sizeof(output)));
        pcm.capacity=sizeof(storage)/sizeof(*pcm.data);
        assert(pt_auto_slice(&pcm,&options,(uint32_t *)(uintptr_t)(UINTPTR_MAX-1),2,&output.count)==PT_SLICE_ALIAS);
        assert(pt_auto_slice(&pcm,&options,output.markers,2,(size_t *)(uintptr_t)(UINTPTR_MAX-1))==PT_SLICE_ALIAS);
        assert(pt_pcm_crossfade_loop(&pcm,0,8,2,(uint32_t *)(uintptr_t)(UINTPTR_MAX-1))==PT_PCM_ALIAS);
        assert(!memcmp(&storage,&image,sizeof(storage)) && !memcmp(&output,&output_image,sizeof(output)));
        /* Successful edits retain exact integer/low-bit behavior and all spare bytes. */
        next=99;assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&next)==PT_PCM_OK && next==2);
        for(i=0;i<2;++i)for(n=0;n<c;++n) {
            size_t head=i*c+n,tail=(6+i)*c+n;
            int32_t expected=(int32_t)(((int64_t)image.active[tail]*(1-i)+
                (int64_t)image.active[head]*(i+1))/2);
            assert(storage.active[tail]==expected);
        }
        assert(!memcmp(&storage,&image,6*c*sizeof(int32_t)));
        assert(!memcmp((unsigned char *)&storage+8*c*sizeof(int32_t),
            (unsigned char *)&image+8*c*sizeof(int32_t),sizeof(storage)-8*c*sizeof(int32_t)));
    }
    /* Empty input writes only its count; unused marker pointers are not outputs. */
    pcm=(struct pt_pcm){NULL,0,0,48000,1,24};output.count=99;
    assert(pt_auto_slice(&pcm,&options,NULL,0,&output.count)==PT_SLICE_OK && output.count==0);
    assert(pt_auto_slice(&pcm,&options,(uint32_t *)(uintptr_t)UINTPTR_MAX,0,&output.count)==PT_SLICE_OK);
    pcm.capacity=1;output.count=99;
    assert(pt_auto_slice(&pcm,&options,NULL,0,&output.count)==PT_SLICE_ALIAS && output.count==99);
    pcm.data=(int32_t *)(uintptr_t)(UINTPTR_MAX-3);pcm.capacity=2;
    assert(pt_auto_slice(&pcm,&options,NULL,0,&output.count)==PT_SLICE_ALIAS && output.count==99);
    pcm.data=storage.active;pcm.capacity=sizeof(storage)/sizeof(*pcm.data);
    memcpy(&image,&storage,sizeof(storage));
    assert(pt_auto_slice(&pcm,&options,NULL,0,&storage.count)==PT_SLICE_ALIAS);
    assert(!memcmp(&storage,&image,sizeof(storage)));
    options.minimum_spacing=0;
    assert(pt_auto_slice(&pcm,&options,NULL,0,&storage.count)==PT_SLICE_INVALID);
    assert(!memcmp(&storage,&image,sizeof(storage)));
    puts("SLICES OUTPUT PASS: six exact-format cases, complete declared-storage/output protection, preserved precedence and crossfade beforeimages");
}
int main(void)
{
    struct pt_pcm pcm={data,1024,512,48000,2,24};struct pt_slice_options options={64,4,500,5};
    size_t count=99,i;uint32_t loop_start=99;
    memset(data,0,sizeof(data));
    for(i=128;i<132;++i) {data[i*2]=1000;data[i*2+1]=-1000;}
    for(i=300;i<304;++i) {data[i*2]=2000;data[i*2+1]=-2000;}
    for(i=320;i<324;++i) {data[i*2]=2000;data[i*2+1]=-2000;}
    memcpy(before,data,sizeof(data));memset(markers,0x5a,sizeof(markers));
    assert(pt_auto_slice(&pcm,&options,markers,1,&count)==PT_SLICE_CAPACITY && count==99 && markers[0]==0x5a5a5a5aUL);
    assert(pt_auto_slice(&pcm,&options,markers,4096,&count)==PT_SLICE_OK);
    assert(count==3 && markers[0]==0 && markers[1]==128 && markers[2]==300);
    assert(!memcmp(data,before,sizeof(data)) && pt_slices_valid(pcm.frames,markers,count));
    assert(pt_slice_insert(512,markers,&count,4096,200)==PT_SLICE_OK && count==4 && markers[2]==200);
    assert(pt_slice_insert(512,markers,&count,4096,200)==PT_SLICE_OK && count==4);
    assert(pt_slice_remove(512,markers,&count,2)==PT_SLICE_OK && count==3 && markers[2]==300);
    assert(pt_slice_insert(512,markers,&count,4096,512)==PT_SLICE_INVALID);
    assert(pt_slice_insert(512,markers,&count,3,400)==PT_SLICE_CAPACITY && count==3);
    assert(pt_auto_slice(&pcm,&options,(uint32_t *)data,4096,&count)==PT_SLICE_ALIAS && !memcmp(data,before,sizeof(data)));
    for(i=0;i<1024;++i)data[i]=1000;
    assert(pt_auto_slice(&pcm,&options,markers,4096,&count)==PT_SLICE_OK && count==1);
    memset(data,0,sizeof(data));assert(pt_auto_slice(&pcm,&options,markers,4096,&count)==PT_SLICE_OK && count==1);
    pcm.frames=0;assert(pt_auto_slice(&pcm,&options,NULL,0,&count)==PT_SLICE_OK && !count);
    pcm.frames=8;pcm.channels=2;
    for(i=0;i<8;++i) {data[i*2]=(int32_t)(i*100);data[i*2+1]=-(int32_t)(i*100);}
    memcpy(before,data,sizeof(data));
    assert(pt_pcm_crossfade_loop(&pcm,0,8,5,&loop_start)==PT_PCM_INVALID && loop_start==99 && !memcmp(before,data,sizeof(data)));
    assert(pt_pcm_crossfade_loop(&pcm,0,8,2,(uint32_t *)data)==PT_PCM_ALIAS);
    assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&loop_start)==PT_PCM_OK && loop_start==2);
    assert(data[12]==300 && data[13]==-300 && data[14]==100 && data[15]==-100);
    assert(!memcmp(data,before,12*sizeof(*data)) && pt_pcm_validate(&pcm)==PT_PCM_OK);
    pcm.bits=24;data[0]=-8388608;data[1]=8388607;data[12]=8388607;data[13]=-8388608;
    assert(pt_pcm_crossfade_loop(&pcm,0,8,2,&loop_start)==PT_PCM_OK && pt_pcm_validate(&pcm)==PT_PCM_OK);
    slices_output_safety();
    puts("SLICES PASS: deterministic non-destructive stereo transient proposals, minimum spacing, manual markers, silence, capacity/alias safety and full-precision crossfade loops");return 0;
}
