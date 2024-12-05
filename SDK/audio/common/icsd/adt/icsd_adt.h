#ifndef _ICSD_ADT_H
#define _ICSD_ADT_H

#define ICSD_VDT_LIB          1
#define ICSD_WAT_LIB          1
#define ICSD_EIN_LIB          1
#define ICSD_WIND_LIB         1


#include "generic/typedef.h"
#include "os/os_type.h"
#include "math.h"
#include "asm/math_fast_function.h"
#include "stdlib.h"
#include "icsd_common_v2.h"
#include "icsd_adt_alg.h"

#if ICSD_WIND_LIB
#include "icsd_wind.h"
#endif

#if 0
#define _adt_printf printf                  //打开智能免摘库打印信息
#else
#define _adt_printf icsd_printf_off
#endif

#if 0
//#define _wat_printf printf                  //打开广域点击库打印信息
#else
//#define _wat_printf icsd_printf_off
#endif

#if 0
#define _wind_printf printf                 //打开风噪检测库打印信息
#else
#define _wind_printf icsd_printf_off
#endif

#define WAT_BUF_LEN    				128*2//128*24
#define WIND_RUNBUF_LEN  			512

#define ADT_VDT_EN					BIT(0) //智能免摘
#define ADT_WDT_EN					BIT(1) //风噪检测
#define ADT_WAT_EN					BIT(2) //广域点击
#define ADT_ENV_EN                  BIT(3) //环境声检测

#define ADT_PATH_3M_EN         		BIT(0)
extern u8 ADT_PATH_CONFIG;
#define ADT_INF_1					BIT(0)//state
#define ADT_INF_2					BIT(1)//talk+ref
#define ADT_INF_3					BIT(2)//data
#define ADT_INF_4                   BIT(3)//err+ref
#define ADT_INF_5                   BIT(4)//ANC状态
#define ADT_INF_6                   BIT(6)//power
#define ADT_INF_7                   BIT(7)//ref power
#define ADT_INF_8                   BIT(8)//dither
#define ADT_INF_9					BIT(9)//PNC 原始数据
#define ADT_INF_10					BIT(10)//ANC 原始数据
#define ADT_INF_11					BIT(11)//TRANS 原始数据
#define ADT_INF_12					BIT(12)//open talk mic
extern const u16 ADT_DEBUG_INF;

#define	VDT_EXT_NOISE_PWR               BIT(0)
#define	VDT_EXT_DIT              		BIT(1)
#define	VDT_EXT_COR              		BIT(2)
#define VDT_EXT_FCXY                    BIT(3)

//实时自适应
int icsd_rtanc_get_libfmt();
int icsd_rtanc_set_infmt(void *alloc_ptr, u8 ch_num, void *param);

struct adt_function {
    //sys
    void (*os_time_dly)(int tick);
    int (*os_sem_pend)(OS_SEM *sem, int timeout);
    int (*os_sem_del)(OS_SEM  *p_sem, int block);
    int (*os_sem_create)(OS_SEM *p_sem, int cnt);
    int (*os_sem_set)(OS_SEM *p_sem, u16 cnt);
    int (*os_sem_post)(OS_SEM *p_sem);
    void (*local_irq_disable)();
    void (*local_irq_enable)();
    void (*icsd_post_adttask_msg)(u8 cmd, u8 id);
    void (*icsd_post_srctask_msg)(u8 cmd);
    void (*icsd_post_anctask_msg)(u8 cmd);
    void (*icsd_post_rtanctask_msg)(u8 cmd);
    int (*jiffies_usec2offset)(unsigned long begin, unsigned long end);
    unsigned long (*jiffies_usec)(void);
    int (*audio_anc_debug_send_data)(u8 *buf, int len);
    u8(*audio_anc_debug_busy_get)(void);
    u8(*audio_adt_talk_mic_analog_close)();
    u8(*audio_adt_talk_mic_analog_open)();
    u8(*audio_anc_mic_gain_get)(u8 mic_sel);
    //tws
    int (*tws_api_get_role)(void);
    int (*tws_api_get_tws_state)();
    //anc
    u8(*anc_dma_done_ppflag)();
    void (*anc_core_dma_ie)(u8 en);
    void (*anc_core_dma_stop)(void);
    void (*anc_dma_on)(u8 out_sel, int *buf, int len);
    void (*anc_dma_on_double)(u8 out_sel, int *buf, int len);
    void (*icsd_adt_hw_suspend)();
    void (*icsd_adt_hw_resume)();
    //dac
    int (*audio_dac_read_anc_reset)();
    int (*audio_dac_read_anc)(s16 points_offset, void *data, int len, u8 read_channel);
    //src
    void (*icsd_adt_src_write)(void *data, int len, void *resample);
    void (*icsd_adt_src_push)(void *resample);
    void (*icsd_adt_src_close)(void *resample);
    void *(*icsd_adt_src_init)(int in_rate, int out_rate, int (*handler)(void *, void *, int));
};
extern struct adt_function	*ADT_FUNC;

typedef struct {
    u8 voice_state;
    u8 wind_lvl;
    u8 wat_result;
} __adt_result;

struct icsd_acoustic_detector_infmt {
    void *param;
    u16 sample_rate;     //当前播放采样率
    u8 ff_gain;
    u8 fb_gain;
    u8 adt_mode;         // TWS: 0 HEADSET: 1
    u8 dac_mode;		 //低压0 高压1
    u8 mic0_type;        // MIC0 类型
    u8 mic1_type;        // MIC1 类型
    u8 mic2_type;        // MIC2 类型
    u8 mic3_type;        // MIC3 类型
    u8 sensitivity;      //智能免摘灵敏度设置
    void *lfb_coeff;
    void *lff_coeff;
    void *ltrans_coeff;
    void *ltransfb_coeff;
    float gains_l_fbgain;
    float gains_l_ffgain;
    float gains_l_transgain;
    float gains_l_transfbgain;
    u8    gain_sign;
    u8    lfb_yorder;
    u8    lff_yorder;
    u8    ltrans_yorder;
    u8    ltransfb_yorder;
    u32   trans_alogm;
    u32   alogm;
    void *alloc_ptr;    //外部申请的ram地址
    /*
        算法结果输出
        voice_state: 0 无讲话； 1 有讲话
        wind_lvl: 0-70 风噪等级
    */
    void (*output_hdl)(u8 voice_state, u8 wind_lvl);    //算法结果输出
    void (*anc_dma_post_msg)(void);     //ANC task dma 抄数回调接口
    void (*anc_dma_output_callback);    //ANC DMA下采样数据输出回调，用于写卡分析
};

struct icsd_acoustic_detector_libfmt {
    int adc_isr_len;     //ADC中断长度
    int adc_sr;          //ADC 采样率
    int lib_alloc_size;  //算法ram需求大小
    u8 mic_num;			 //需要打开的mic个数
};

enum {
    ICSD_ANC_LFF_MIC  = 0,
    ICSD_ANC_LFB_MIC  = 1,
    ICSD_ANC_RFF_MIC  = 2,
    ICSD_ANC_RFB_MIC  = 3,
    ICSD_ANC_TALK_MIC = 4,
    ICSD_ANC_MIC_NULL = 0XFF,   //对应的MIC没有数值,则表示这个MIC没有开启
};

enum {
    ADT_DACMODE_LOW = 0,
    ADT_DACMODE_HIGH,
};
enum {
    ADT_TWS = 0,
    ADT_HEADSET,
};


extern s16 *adt_dac_loopbuf;
extern const u8 ICSD_RTANC_EN;
extern const u8 BT_ADT_INF_EN;
extern const u8 BT_ADT_DP_STATE_EN;
extern const u8 BT_VDT_INF_EN;
extern const u8 VDT_DATA_CHECK;
extern const u8 WIND_DATA_CHECK;
extern const u8 ICSD_WIND_3MIC;
extern const u8 BT_WIND_INF_EN;
extern const u8 ICSD_HOWL_EN;
extern const u8 ICSD_EIN_EN;
extern const u8 ICSD_ADT_WIND_MIC_TYPE;
extern const u8 ICSD_ADT_WIND_PHONE_TYPE;
extern const u8 ADT_VDT_USE_ANCDMAL;

extern int (*adt_printf)(const char *format, ...);
extern int (*wind_printf)(const char *format, ...);
extern const u8  ICSD_ADT_EP_TYPE;
extern const u8  ICSD_WIND_EN;
extern const u8  ICSD_ENVNL_EN;
extern const u8  ICSD_WIND_MODE;
extern const u32 ICSD_TWS_STA_SIBLING_CONNECTED;

//tws
void icsd_adt_tx_unsniff_req();
void icsd_adt_s2m_packet(s8 *data, u8 cmd);
void icsd_adt_m2s_packet(s8 *data, u8 cmd);
void icsd_adt_m2s_cb(void *_data, u16 len, bool rx);
void icsd_adt_s2m_cb(void *_data, u16 len, bool rx);
void icsd_adt_tws_m2s(u8 cmd);
void icsd_adt_tws_s2m(u8 cmd);
int  icsd_adt_tws_msync(u8 *data, s16 len);
int  icsd_adt_tws_ssync(u8 *data, s16 len);
//task
void icsd_srctask_cmd_check(u8 cmd);
void icsd_anctask_cmd_check(u8 cmd);
void icsd_adt_task_handler(int msg, int msg2);
void icsd_src_task_handler(int msg);
void icsd_adt_anctask_handle(u8 cmd);
void icsd_rtanc_task_handler(int msg);
//anc
void icsd_adt_dma_done();
void icsd_anc_dma_4ch_on(u8 out_sel_ch01, u8 out_sel_ch23, int *buf, int len);
//dac
void icsd_adt_set_audio_sample_rate(u16 sample_rate);
void icsd_adt_dac_loopbuf_malloc(u16 points);
void icsd_adt_dac_loopbuf_free();
//init
void icsd_adt_init_pre();
void adt_function_init();
//APP调用的LIB函数
void icsd_acoustic_detector_get_libfmt(struct icsd_acoustic_detector_libfmt *libfmt, u8 function);
void icsd_acoustic_detector_set_infmt(struct icsd_acoustic_detector_infmt *fmt);
void icsd_acoustic_detector_infmt_updata(struct icsd_acoustic_detector_infmt *fmt);
void icsd_acoustic_detector_open(void);
void icsd_acoustic_detector_close();
void icsd_acoustic_detector_resume(u8 mode, u8 anc_onoff);
void icsd_acoustic_detector_suspend();
void icsd_acoustic_detector_ancdma_done();//ancdma done回调
void icsd_acoustic_detector_mic_input_hdl(void *priv, s16 *buf, int len);
//OUTPUT
void icsd_adt_run_output(__adt_result *_adt_result);
void icsd_envnl_output(int result);
void icsd_wind_output(u8 wind_lvl);

void anc_dma_on_double_4ch(u8 ch1_out_sel, u8 ch2_out_sel, int *buf, int irq_point);
void icsd_adt_tone_play_handler(u8 idx);

#endif
