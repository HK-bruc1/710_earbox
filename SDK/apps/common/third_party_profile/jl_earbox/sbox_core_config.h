/*
 * @Author:  shenzihao@zh-jieli.com
 * @Date: 2025-03-29 11:32:31
 * @LastEditors:  shenzihao@zh-jieli.com
 * @LastEditTime: 2025-05-13 09:54:51
 * @FilePath: \SDK\apps\common\third_party_profile\jl_earbox\sbox_core_config.h
 * @Description: 
 *      此文件是配置彩屏仓通信协议相关的配置：
 *      1.是否打开数据保护，防止踩踏
 *      2.通信BUF的大小定义，需要和仓同步这个大小，需要 耳机buf >= 仓buf                                        
 *      3.仓通信的profile handle 的定义,ble发数类型的定义，具体发数接口的载入
 *      4.协议库底层通知事件的定义，不同SDK接口不一致，需要根据SDK具体情况去改写
 * 
 * Copyright (c) 2025 by zh_jieli, All Rights Reserved. 
 */

#ifndef SBOX_CORE_CONFIG_H
#define SBOX_CORE_CONFIG_H
#include <stdint.h>
#include "classic/tws_api.h"
#include "cpu.h"
#define SEND_BUF_MAX_LEN            256                 //彩屏仓协议定义的数据传输buf大小，必须为16的倍数
#define SBOX_TR_BUF_LEN     ((SEND_BUF_MAX_LEN + 16) * 5) //最大缓存5包

#define SBOX_CORE_DEBUG             (BIT(0))           //协议库打印
#define SBOX_UART_FUNCTION          1                   //和982串口通信指令
#define SBOX_OPEN_DATA_PROTECT      1               //是否采用CBUF缓存BLE数据
#define ATT_CHARACTERISTIC_ae99_01_VALUE_HANDLE 0x000d

struct box_info{
    u8 cmd;
    u8 data[SEND_BUF_MAX_LEN];
    u16 lens;
};

typedef struct {
    // u8 local_ble_addr[6];
    // u8 (*get_box_ble_addr)(void);
    // void (*set_box_ble_addr)(void *data);
    unsigned short sbox_ble_send_handle;
    char  sbox_handle_type;
    void (*sbox_ble_send_data)(void *data, int len);
}s_box_func_hdl;

extern s_box_func_hdl sbox_func_hdl;
extern u8 sbox_tr_rbuf[];

int sbox_tr_ibuf_to_cbuf(u8 *buf, u32 len, u8 cmd);
int sbox_tr_cbuf_read(u8 *buf, u32 *len, u8 *cmd);
#endif