#include "record_channel.h"

enum pt_record_result pt_record_channel_message(struct pt_recorder *recorder,
    uint64_t position,unsigned track,unsigned channel,const uint8_t *message,
    size_t bytes)
{
    unsigned status,kind,note,velocity;size_t expected;
    if(!recorder||!recorder->armed||!recorder->store||track>=16||
       !channel||channel>16||!message||(bytes!=2&&bytes!=3)||
       bytes>UINTPTR_MAX-(uintptr_t)message)return PT_RECORD_INVALID;
    status=message[0];kind=status&0xf0U;
    if(status<0x80U||status>=0xf0U)return PT_RECORD_INVALID;
    expected=kind==0xc0U||kind==0xd0U?2U:3U;
    if(bytes!=expected)return PT_RECORD_INVALID;
    note=message[1];velocity=bytes==3?message[2]:0;
    if(note>127||velocity>127)return PT_RECORD_INVALID;
    if((status&15U)+1U!=channel||
       (kind!=0x80U&&kind!=0x90U))return PT_RECORD_IGNORED;
    return pt_record_input(recorder,position,track,note,velocity,kind==0x90U);
}
