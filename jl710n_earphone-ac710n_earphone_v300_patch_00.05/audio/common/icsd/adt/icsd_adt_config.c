#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".icsd_adt.data.bss")
#pragma data_seg(".icsd_adt.data")
#pragma const_seg(".icsd_adt.text.const")
#pragma code_seg(".icsd_adt.text")
#endif

#include "app_config.h"
#if ((defined TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN) && TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN && \
	 TCFG_AUDIO_ANC_ENABLE)
#include "icsd_adt.h"
#include "icsd_adt_app.h"

//==============================================//
//    功能使能
//==============================================//
const u8 ICSD_WIND_MODE = 0;//关闭独立风噪模式
const u8 ICSD_WIND_EN  =  TCFG_AUDIO_ANC_WIND_NOISE_DET_ENABLE;//风噪检测使能
const u8 ICSD_ENVNL_EN =  0;//环境声检测使能
const u8 ICSD_RTANC_EN =  0;//实时自适应
const u8 ICSD_HOWL_EN  =  0;//啸叫检测
const u8 ICSD_EIN_EN   =  0;//入耳检测
const u8 ICSD_ADT_EP_TYPE = ADT_TWS;

const u8 VDT_DATA_CHECK     = 0;//串口查看免摘数据通路
const u8 WIND_DATA_CHECK    = 0;//串口查看风噪数据通路
const u8 BT_ADT_INF_EN	    = 0;//查看ADT启动信息
const u8 BT_ADT_DP_STATE_EN = 0;//查看ADT数据通路状态
const u8 BT_VDT_INF_EN      = 0;//查看免摘信息
const u8 BT_WIND_INF_EN     = 0;//查看免摘信息
//==============================================//
//    环境声参数配置
//==============================================//
const float env_alpha = 0.25;//0 ~ 1之间,越大越快

#endif/*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/
