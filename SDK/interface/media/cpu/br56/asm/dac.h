#ifndef __CPU_DAC_H__
#define __CPU_DAC_H__


#include "generic/typedef.h"
#include "generic/atomic.h"
#include "os/os_api.h"
#include "audio_src.h"
#include "media/audio_cfifo.h"
#include "system/spinlock.h"
#include "audio_def.h"
#include "audio_output_dac.h"

/***************************************************************************
  							Audio DAC Features
Notes:以下为芯片规格定义，不可修改，仅供引用
***************************************************************************/
#define AUDIO_DAC_CHANNEL_NUM	2	//DAC通道数

#define DACVDD_LDO_1_20V        0
#define DACVDD_LDO_1_25V        1
#define DACVDD_LDO_1_30V        2
#define DACVDD_LDO_1_35V        3

#define AUDIO_DAC_SYNC_IDLE             0
#define AUDIO_DAC_SYNC_START            1
#define AUDIO_DAC_SYNC_NO_DATA          2
#define AUDIO_DAC_SYNC_ALIGN_COMPLETE   3

#define AUDIO_SRC_SYNC_ENABLE   1
#define SYNC_LOCATION_FLOAT      1
#if SYNC_LOCATION_FLOAT
#define PCM_PHASE_BIT           0
#else
#define PCM_PHASE_BIT           8
#endif

#define DA_LEFT        0
#define DA_RIGHT       1

/************************************
 *              DAC模式
 *************************************/
// TCFG_AUDIO_DAC_MODE
#define DAC_MODE_SINGLE                    (0)
#define DAC_MODE_DIFF                      (1)
#define DAC_MODE_VCMO                      (2)

#define DA_SOUND_NORMAL                 0x0
#define DA_SOUND_RESET                  0x1
#define DA_SOUND_WAIT_RESUME            0x2

#define DACR32_DEFAULT		8192

/************************************
             dac性能模式
************************************/
#define DAC_TRIM_SEL_RDAC_LP            0
#define DAC_TRIM_SEL_RDAC_LN            0
#define DAC_TRIM_SEL_RDAC_RP            1
#define DAC_TRIM_SEL_RDAC_RN            1
#define DAC_TRIM_SEL_PA_LP              2
#define DAC_TRIM_SEL_PA_LN              2
#define DAC_TRIM_SEL_PA_RP              3
#define DAC_TRIM_SEL_PA_RN              3
#define DAC_TRIM_SEL_VCM_L              4
#define DAC_TRIM_SEL_VCM_R              5

#define DAC_TRIM_CH_L                  0
#define DAC_TRIM_CH_R                  1

/************************************
             dac性能模式
************************************/
// TCFG_DAC_PERFORMANCE_MODE
#define	DAC_MODE_HIGH_PERFORMANCE          (0)
#define	DAC_MODE_LOW_POWER		           (1)

/************************************
             hpvdd档位
************************************/
#define DAC_HPVDD_18V              (0)
#define DAC_HPVDD_12V              (1)


#define DAC_ANALOG_OPEN_PREPARE         (1) //DAC打开前，即准备打开
#define DAC_ANALOG_OPEN_FINISH          (2)	//DAC打开后，即打开完成
#define DAC_ANALOG_CLOSE_PREPARE        (3) //DAC关闭前，即准备关闭
#define DAC_ANALOG_CLOSE_FINISH         (4) //DAC关闭后，即关闭完成
//在应用层重定义 audio_dac_power_state 函数可以获取dac模拟开关的状态
//void audio_dac_power_state(u8 state){}

struct audio_dac_hdl;
struct dac_platform_data {
    u8 vcm_cap_en;      //配1代表走外部通路,vcm上有电容时,可以提升电路抑制电源噪声能力，提高ADC的性能，配0相当于vcm上无电容，抑制电源噪声能力下降,ADC性能下降
    u8 power_on_mode;
    u8 performance_mode;
    u8 power_mode;          // DAC 功率模式， 0:20mw  1:30mw  2:50mw  3:80mw
    u8 dacldo_vsel;         // DACLDO电压档位:0~15
    u8 pa_isel0;            // 电流档位:2~6
    u8 hpvdd_sel;
    u8 l_ana_gain;
    u8 r_ana_gain;
    u8 dcc_level;
    u8 bit_width;             	// DAC输出位宽
    u8 fade_en;
    u8 fade_points;
    u8 fade_volume;
    u8 classh_en;           // CLASSH使能(当输出功率为50mW时可用)
    u8 classh_mode;         // CLASSH 模式  0：蓝牙最低电压1.2v  1:蓝牙最低电压1.15v
    u16 dma_buf_time_ms;    // DAC dma buf 大小
    s16 *dig_vol_tab;
    u32 digital_gain_limit;
    u32 max_sample_rate;    	// 支持的最大采样率
    u32 classh_down_step;   // DAC classh 电压下降步进，1us/setp，配置范围[0.1s, 8s]，建议配置3s
};

/*DAC数字相关的变量*/
struct digital_module {
    u8 inited;
};

/*DAC模拟相关的变量*/
struct analog_module {
    u8 inited;
    u16 dac_test_volt;
};

struct trim_init_param_t {
    u8 clock_mode;
    u8 power_level;
    s16 precision;
    struct audio_dac_trim *dac_trim;
    struct audio_dac_hdl *dac;              /* DAC设备*/
};

struct audio_dac_trim {
    s16 left;
    s16 right;
    s16 vcomo;
};

// *INDENT-OFF*
struct audio_dac_sync {
    u8 channel;
    u8 start;
    u8 fast_align;
    int fast_points;
    u32 input_num;
    int phase_sub;
    int in_rate;
    int out_rate;
#if AUDIO_SRC_SYNC_ENABLE
    struct audio_src_sync_handle *src_sync;
    void *buf;
    int buf_len;
    void *filt_buf;
    int filt_len;
#else
    struct audio_src_base_handle *src_base;
#endif
#if SYNC_LOCATION_FLOAT
    double pcm_position;
#else
    u32 pcm_position;
#endif
    void *priv;
    void (*handler)(void *priv, u8 state);
};

struct audio_dac_sync_node {
    u8 triggered;
    u8 network;
    u32 timestamp;
    void *hdl;
    struct list_head entry;
    void *ch;
};

struct audio_dac_channel_attr {
    u8  write_mode;         /*DAC写入模式*/
    u16 delay_time;         /*DAC通道延时*/
    u16 protect_time;       /*DAC延时保护时间*/
};

struct audio_dac_channel {
    u8  state;              /*DAC状态*/
    u8  pause;
    u8  samp_sync_step;     /*数据流驱动的采样同步步骤*/
    struct audio_dac_channel_attr attr;     /*DAC通道属性*/
    struct audio_sample_sync *samp_sync;    /*样点同步句柄*/
    struct audio_cfifo_channel fifo;        /*DAC cfifo通道管理*/

    //struct audio_dac_sync sync;
    //struct list_head sync_list;
};

struct audio_dac_hdl {
    struct analog_module analog;
    struct digital_module digital;
    const struct dac_platform_data *pd;
    OS_SEM sem;
    void (*fade_handler)(u8 left_gain, u8 right_gain);
    u8 avdd_level;
    u8 lpf_i_level;
    volatile u8 state;
    volatile u8 agree_on;
    u8 ng_threshold;
    u8 gain;
    u8 channel;
    u16 d_volume[2];
    u32 sample_rate;
    u32 digital_gain_limit;
    u32 output_buf_len;
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让DAC真正跑之前写入的PCM点数
    u16 irq_points;
    s16 protect_time;
    s16 protect_pns;
    s16 fadein_frames;
    s16 fade_vol;
    u8 protect_fadein;
    u8 vol_set_en;
    u8 dec_channel_num;
    u8 sound_state;
    s16 *output_buf;
    u8 *mono_lr_diff_tmp_buf;

    u8 anc_dac_open;

    u8 dac_read_flag;	//AEC可读取DAC参考数据的标志

    u8 fifo_state;
    u16 unread_samples;             /*未读样点个数*/
    struct audio_cfifo fifo;        /*DAC cfifo结构管理*/
    struct audio_dac_channel main_ch;

    u16 mute_timer;           //DAC PA Mute Timer
    u8 mute_ch;               //DAC PA Mute Channel
	u8 dvol_mute;             //DAC数字音量是否mute
#if 0
    struct audio_dac_sync sync;
    struct list_head sync_list;
    u8 sync_step;
#endif

	u8 active;
    /*******************************************/
    /**sniff退出时，dac模拟提前初始化，避免模拟初始化延时,影响起始同步********/
    u8 power_on;
    u8 need_close;
    OS_MUTEX mutex;
    /*******************************************/
    spinlock_t lock;
/*******************************************/
	struct list_head sync_list;
};


#endif

