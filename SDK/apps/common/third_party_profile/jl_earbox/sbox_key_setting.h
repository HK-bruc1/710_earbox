#ifndef __SBOX_KEY_SETTING_H__
#define __SBOX_KEY_SETTING_H__
#include "app_config.h"
#include "includes.h"
#include "typedef.h"

enum {
    SBOX_ONE_CLICK=0,           //单击
    SBOX_DOUBLE_CLICK,    //双击
    SBOX_THIRD_CLICK,         //三击
    SBOX_LONG_PRESS,          //⻓按
    SBOX_OP_NULL = 0xFF,
};

enum {
    SBOX_NO_FUNTION = 0,      //(⽆功能)
    SBOX_PLAY_PAUSE,          //播放/暂停
    SBOX_VOL_UP,              //音量加
    SBOX_VOL_DOWN,            //音量减  
    SBOX_MUSIC_PREV,          //上⼀曲
    SBOX_MUSIC_NEXT,          //下⼀曲
    SBOX_EQ_SWITCH,           //切换EQ  
    SBOX_VOICE_ASSISTANT,     //语⾳助⼿
    SBOX_LOW_LATENCY,         //低延时模式(普通模式/低延迟模式)
    SBOX_ANC_MODE,            //anc模式(正常模式/降噪模式/通透模式)
    SBOX_FUN_MAX_ = 0xFF,
};///手势功能枚举
void deal_sbox_key_setting(u8 *key_setting_info, u8 write_vm, u8 tws_sync);

#endif