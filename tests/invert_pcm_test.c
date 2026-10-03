#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "invert_pcm.h"
static void preparation(void)
{
    int32_t master[2050],private_data[2050];unsigned i,step,ready;
    struct pt_pcm source={master,2050,2050,48000,1,8};
    struct pt_invert_pcm workspace={0},saved;struct pt_invert_pcm_job job={0},before;
    for(i=0;i<2050;++i)master[i]=(int)(i%256)-128;
    for(step=0;step<3;++step) {
        memset(&workspace,0,sizeof(workspace));saved=workspace;
        for(i=0;i<2050;++i)private_data[i]=999;
        assert(pt_invert_pcm_begin(&job,&workspace,&source,private_data,2050)==PT_PCM_OK);
        assert(!job.copied && !memcmp(&workspace,&saved,sizeof(saved)) && private_data[0]==999);
        for(i=0;i<step;++i) {
            size_t previous=job.copied;ready=77;
            assert(pt_invert_pcm_prepare(&job,&ready)==PT_PCM_OK && !ready);
            assert(job.copied-previous<=4096/sizeof(int32_t));
            assert(!memcmp(&workspace,&saved,sizeof(saved)));
        }
        pt_invert_pcm_cancel(&job);pt_invert_pcm_cancel(&job);
        assert(!memcmp(&workspace,&saved,sizeof(saved)));
    }
    assert(pt_invert_pcm_begin(&job,&workspace,&source,private_data,2050)==PT_PCM_OK);before=job;
    assert(pt_invert_pcm_prepare(&job,(unsigned *)master)==PT_PCM_ALIAS);
    assert(pt_invert_pcm_prepare(&job,(unsigned *)private_data)==PT_PCM_ALIAS);
    assert(pt_invert_pcm_prepare(&job,(unsigned *)&workspace)==PT_PCM_ALIAS);
    assert(pt_invert_pcm_prepare(&job,(unsigned *)&source)==PT_PCM_ALIAS);
    assert(pt_invert_pcm_prepare(&job,&job.complete)==PT_PCM_ALIAS);
    assert(!memcmp(&job,&before,sizeof(job)) && master[0]==-128);
    ready=77;assert(pt_invert_pcm_prepare(NULL,&ready)==PT_PCM_INVALID && ready==77);
    for(step=0,ready=0;!ready;++step){assert(step<3);assert(pt_invert_pcm_prepare(&job,&ready)==PT_PCM_OK);}
    assert(step==3 && !memcmp(private_data,master,sizeof(master)) && workspace.source==&source);
    assert(pt_invert_pcm_prepare(&job,&ready)==PT_PCM_OK && ready);pt_invert_pcm_cancel(&job);
    saved=workspace;
    assert(pt_invert_pcm_begin(&job,&workspace,&source,private_data,2050)==PT_PCM_OK);
    source.rate=44100;ready=77;
    assert(pt_invert_pcm_prepare(&job,&ready)==PT_PCM_INVALID && !ready && !job.copied);
    source.rate=48000;
    assert(pt_invert_pcm_prepare(&job,&ready)==PT_PCM_INVALID && !memcmp(&workspace,&saved,sizeof(saved)));
    pt_invert_pcm_cancel(&job);
    {union {struct pt_invert_pcm_job job;int32_t data[1024];} alias;
        memset(&alias,0,sizeof(alias));source=(struct pt_pcm){alias.data,1024,1024,48000,1,8};
        assert(pt_invert_pcm_begin(&alias.job,&workspace,&source,private_data,2050)==PT_PCM_ALIAS);
        assert(!alias.data[0] && !alias.data[1023]);
    }
    for(i=0;i<2050;++i)assert(master[i]==(int)(i%256)-128);
}
int main(void)
{
    int32_t original[8]={-128,-64,-1,0,1,63,64,127},copy[8],before[8];
    struct pt_pcm source={original,8,8,48000,1,8};
    struct pt_invert_pcm w,saved;struct pt_invert_loop a={0},b={0},old;
    memset(&w,0,sizeof(w));assert(pt_invert_pcm_init(&w,&source,copy,8)==PT_PCM_OK);
    assert(!memcmp(copy,original,sizeof(copy)) && w.pcm.data!=source.data);
    assert(pt_invert_loop_bind(&a,8,0,8) && pt_invert_loop_bind(&b,8,0,8));
    assert(pt_invert_loop_speed(&a,15) && pt_invert_loop_speed(&b,15));
    assert(pt_invert_pcm_update(&w,&a)==PT_PCM_OK && copy[1]==63 && original[1]==-64);
    assert(pt_invert_pcm_update(&w,&b)==PT_PCM_OK && copy[1]==-64);
    assert(pt_invert_pcm_update(&w,&a)==PT_PCM_OK && copy[2]==0);
    assert(pt_invert_pcm_reset(&w)==PT_PCM_OK && !memcmp(copy,original,sizeof(copy)));
    saved=w;memcpy(before,copy,sizeof(copy));
    assert(pt_invert_pcm_init(&w,&source,original,8)==PT_PCM_ALIAS);
    assert(!memcmp(&w,&saved,sizeof(w)) && !memcmp(copy,before,sizeof(copy)));
    assert(pt_invert_pcm_init(&w,&source,copy,7)==PT_PCM_CAPACITY);
    assert(!memcmp(&w,&saved,sizeof(w)) && !memcmp(copy,before,sizeof(copy)));
    a.end=10;old=a;assert(pt_invert_pcm_update(&w,&a)==PT_PCM_INVALID);
    assert(!memcmp(&a,&old,sizeof(a)) && !memcmp(copy,before,sizeof(copy)));
    source.bits=16;assert(pt_invert_pcm_reset(&w)==PT_PCM_INVALID);
    assert(!memcmp(&w,&saved,sizeof(w)) && !memcmp(copy,before,sizeof(copy)));
    {
        int32_t all[256],private_data[256];unsigned i,pass;
        struct pt_pcm src={all,256,256,48000,1,8};struct pt_invert_loop clock={0};
        for(i=0;i<256;++i)all[i]=(int)i-128;
        assert(pt_invert_pcm_init(&w,&src,private_data,256)==PT_PCM_OK);
        assert(pt_invert_loop_bind(&clock,256,0,256) && pt_invert_loop_speed(&clock,15));
        for(pass=0;pass<2;++pass) {
            for(i=0;i<256;++i)assert(pt_invert_pcm_update(&w,&clock)==PT_PCM_OK);
            for(i=0;i<256;++i)assert(private_data[i]==(pass?all[i]:-1-all[i]) && all[i]==(int)i-128);
        }
    }
    preparation();puts("INVERT private PCM PASS");return 0;
}
