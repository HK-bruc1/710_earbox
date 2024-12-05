
#ifndef _AUDIO_ANC_DEBUG_TOOL_H_
#define _AUDIO_ANC_DEBUG_TOOL_H_

#include "app_anctool.h"

enum {
    ANC_DEBUG_STA_CLOSE = 0,
    ANC_DEBUG_STA_OPEN,
    ANC_DEBUG_STA_RUN,
};

//debug 启动
void audio_anc_debug_tool_open(void);

//debug 关闭
void audio_anc_debug_tool_close(void);

/*
   debug 数据发送
   return len表示写入成功，0则写入失败
*/
int audio_anc_debug_send_data(u8 *buf, int len);

u8 audio_anc_debug_busy_get(void);


#endif/*_AUDIO_ANC_DEBUG_TOOL_H_*/
