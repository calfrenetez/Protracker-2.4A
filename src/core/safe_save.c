#include "safe_save.h"
enum pt_save_result pt_safe_save(const struct pt_save_ops *ops,const void *data,size_t length)
{
    enum pt_save_result result=PT_SAVE_BEGIN;size_t pos=0;
    const unsigned char *bytes=data;
    if(!ops || !ops->begin || !ops->write || !ops->finish || !ops->verify ||
       !ops->publish || !ops->abort || (!data && length))return PT_SAVE_INVALID;
    if(!ops->begin(ops->context))goto fail;
    result=PT_SAVE_WRITE;
    while(pos<length) {
        size_t n=ops->write(ops->context,bytes+pos,length-pos);
        if(!n || n>length-pos)goto fail;
        pos+=n;
    }
    result=PT_SAVE_FINISH;if(!ops->finish(ops->context))goto fail;
    result=PT_SAVE_VERIFY;if(!ops->verify(ops->context,data,length))goto fail;
    result=PT_SAVE_PUBLISH;if(!ops->publish(ops->context))goto fail;
    return PT_SAVE_OK;
fail:
    ops->abort(ops->context);return result;
}
