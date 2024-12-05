#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_link.data.bss")
#pragma data_seg(".audio_link.data")
#pragma const_seg(".audio_link.text.const")
#pragma code_seg(".audio_link.text")
#endif
/*****************************************************************
>file name : lib/media/cpu/br22/audio_link.c
>author :
>create time : Fri 7 Dec 2018 14:59:12 PM CST
*****************************************************************/
#include "system/includes.h"
#include "cpu/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_link.h"

#if TCFG_IIS_NODE_ENABLE || TCFG_MULTI_CH_IIS_NODE_ENABLE || TCFG_MULTI_CH_IIS_RX_NODE_ENABLE

#define test_32bit 0//使用24bit配置流程，配置成32bit测试32bit的情况

#define IIS_IO_INIT_AFTER_ALINK_START  1// alink_start之后，再初始化输出io,避免iis起始io输出不定态引起一声杂音的问题
/* #define ALINK_DEBUG_INFO */
#ifdef ALINK_DEBUG_INFO
#define alink_printf  printf
#else
#define alink_printf(...)
#endif

u32 alink_sr_tab[ALINK_SR_NUM] = {ALINK_SR_192000, ALINK_SR_176400, ALINK_SR_96000, ALINK_SR_88200, ALINK_SR_48000, ALINK_SR_44100, ALINK_SR_32000, ALINK_SR_24000, ALINK_SR_22050, ALINK_SR_16000, ALINK_SR_12000, ALINK_SR_11025, ALINK_SR_8000};


/*
 *              BR52_ALINK
 * 配置：24BIT、IIS_RX、IIS_BASIC、DUAL_BUFFER
 * 现象：第二次ALNK_EN时，访问的第一个点偏移65534*4 bytes
 * 解决方式：第二次以左对齐方式打开，HWPTR跑一个点之后再切回IIS基础模式
 */
#define FIRST_EN_CONFIG 0  //br56 fix_bug
static u8 first_en = FIRST_EN_CONFIG; //记录是否为首次打开
void alink0_info_dump();
static u32 *ALNK0_BUF_ADR[] = {
    (u32 *)(&(JL_ALNK0->ADR0)),
    (u32 *)(&(JL_ALNK0->ADR1)),
    (u32 *)(&(JL_ALNK0->ADR2)),
    (u32 *)(&(JL_ALNK0->ADR3)),
};

static volatile u32 *ALNK0_HWPTR[] = {
    (u32 *)(&(JL_ALNK0->HWPTR0)),
    (u32 *)(&(JL_ALNK0->HWPTR1)),
    (u32 *)(&(JL_ALNK0->HWPTR2)),
    (u32 *)(&(JL_ALNK0->HWPTR3)),
};

static volatile u32 *ALNK0_SWPTR[] = {
    (u32 *)(&(JL_ALNK0->SWPTR0)),
    (u32 *)(&(JL_ALNK0->SWPTR1)),
    (u32 *)(&(JL_ALNK0->SWPTR2)),
    (u32 *)(&(JL_ALNK0->SWPTR3)),
};

static volatile u32 *ALNK0_SHN[] = {
    (u32 *)(&(JL_ALNK0->SHN0)),
    (u32 *)(&(JL_ALNK0->SHN1)),
    (u32 *)(&(JL_ALNK0->SHN2)),
    (u32 *)(&(JL_ALNK0->SHN3)),
};

static u32 PFI_ALNK0_DAT[] = {
    PFI_ALNK0_DAT0,
    PFI_ALNK0_DAT1,
    PFI_ALNK0_DAT2,
    PFI_ALNK0_DAT3,
};

static u32 FO_ALNK0_DAT[] = {
    FO_ALNK0_DAT0,
    FO_ALNK0_DAT1,
    FO_ALNK0_DAT2,
    FO_ALNK0_DAT3,
};


enum {
    MCLK_EXTERNAL	= 0u,
    MCLK_SYS_CLK		,
    MCLK_OSC_CLK 		,
    MCLK_PLL_CLK		,
};

enum {
    SCLK_EXTERNAL	= 0u,
    SCLK_MCKD_SDIV		,
};

static ALINK_PARM *p_alink0_parm = NULL;

__attribute__((always_inline))
s32 *alink_get_addr(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return NULL;
    }
    s32 *buf_addr = (s32 *)(*ALNK0_BUF_ADR[hw_channel_parm->ch_idx]);
    return buf_addr;
}

__attribute__((always_inline))
u32 alink_get_shn(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int shn = *ALNK0_SHN[hw_channel_parm->ch_idx];
    return shn/* *((IIS_CH_NUM_TEST == 1)?2:1) */;
}

__attribute__((always_inline))
void alink_set_shn(void *hw_channel, u32 len)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }

    *ALNK0_SHN[hw_channel_parm->ch_idx] = len;
    __asm_csync();
}

__attribute__((always_inline))
void alink_set_shn_with_no_sync(void *hw_channel, u32 len)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }

    *ALNK0_SHN[hw_channel_parm->ch_idx] = len;

}

__attribute__((always_inline))
u32 alink_get_swptr(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int swptr = *ALNK0_SWPTR[hw_channel_parm->ch_idx];
    return swptr;
}

void alink_set_swptr(void *hw_channel, u32 value)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    *ALNK0_SWPTR[hw_channel_parm->ch_idx] = value;
}

__attribute__((always_inline))
u32 alink_get_hwptr(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int hwptr = *ALNK0_HWPTR[hw_channel_parm->ch_idx];
    return hwptr;
}

__attribute__((always_inline))
u32 alink_get_len(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    u32 len = JL_ALNK0->LEN;
    return len;
}

void alink_set_hwptr(void *hw_channel, u32 value)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    *ALNK0_HWPTR[hw_channel_parm->ch_idx] = value;
}

void alink_set_ch_ie(void *hw_channel, u32 value)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    ALINK_CHx_IE(hw_channel_parm->module, hw_channel_parm->ch_idx, value);
}

int alink_get_ch_ie(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return 0;
    }
    int ret =  ALINK_CHx_IE_GET(hw_channel_parm->module, hw_channel_parm->ch_idx);
    return ret;
}

void alink_clr_ch_pnd(void *hw_channel)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    if (!hw_channel_parm) {
        return;
    }
    ALINK_CLR_CHx_PND(hw_channel_parm->module, hw_channel_parm->ch_idx);
}


//==================================================
static void alink_io_in_init(u8 gpio)
{
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_INPUT_FLOATING);
}

static void alink_io_out_init(u8 gpio)
{
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_OUTPUT_LOW);
}

static void alink_io_uninit(u8 gpio)
{
    gpio_set_mode(IO_PORT_SPILT(gpio), PORT_HIGHZ);
}

__attribute__((always_inline))
static void *alink_addr(void *hw_alink, u8 ch)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    u32 buf_index;
    s32 *buf_addr = (s32 *)alink_get_addr(&hw_alink_parm->ch_cfg[ch]);

    if (hw_alink_parm->buf_mode == ALINK_BUF_CIRCLE) {
        s32 swptr = alink_get_swptr(&hw_alink_parm->ch_cfg[ch]);
        buf_addr = (s32 *)(buf_addr + swptr);				//buf为dma起始地址,计算当前循环buf写数位置
        return buf_addr;
    } else {
        u8 index_table[4] = {12, 13, 14, 15};
        u8 bit_index = index_table[ch];
        buf_index = (ALINK_SEL(hw_alink_parm->module, CON0) & BIT(bit_index)) ? 0 : 1;
        buf_addr = buf_addr + ((hw_alink_parm->dma_len / 8) * buf_index);
        return buf_addr;
    }
}

__attribute__((always_inline))
static u32 alink_isr_get_len(void *hw_alink, u8 ch, u32 *remain)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    u32 len = 0;
    /* printf("hw_alink_parm->buf_mode %d\n", hw_alink_parm->buf_mode); */
    if (hw_alink_parm->buf_mode == ALINK_BUF_CIRCLE) {
        s32 shn = alink_get_shn(&hw_alink_parm->ch_cfg[ch]);
        s32 swptr = alink_get_swptr(&hw_alink_parm->ch_cfg[ch]);
        s32 dma_len = alink_get_len(&hw_alink_parm->ch_cfg[ch]);
        if ((swptr + shn) >= dma_len) {					//处理边界情况
            len = dma_len - swptr;
            *remain = (shn - len) * 4; //
        } else {
            len = shn;
            *remain = 0;//
        }
        len = len * 4;
    } else {
        len = hw_alink_parm->dma_len / 2;
    }

    return len;
}

__attribute__((always_inline))
u32 alink_timestamp(u32 module_idx, u8 ch_idx)
{
    return 	p_alink0_parm->timestamp[ch_idx];
}
#define timestamp_threshold (100)
__attribute__((always_inline))
void alink0_cal_timestamp(u8 ch)
{
    if (p_alink0_parm->buf_mode != ALINK_BUF_CIRCLE) {//double buf
        u32 time = audio_jiffies_usec();
        u32 point_offset = 1;
        if (p_alink0_parm->bitwide) {
            point_offset = 2;
        }
        struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)&p_alink0_parm->ch_cfg[ch];
        int timeout = (1000000 / p_alink0_parm->sample_rate) * (clk_get("sys") / 1000000) * 5 / 4;
        u32 swn = alink_get_shn(hw_channel_parm);
        while (--timeout > 0 && ((swn == alink_get_shn(hw_channel_parm)) || ((swn != alink_get_shn(hw_channel_parm)) && (alink_get_shn(hw_channel_parm) % point_offset))));//等到消耗偶数个点后，才往后走，避免奇数个点的情况下更新给同步模块有1个点的偏差
        /* time = time - ((1000000 / p_alink0_parm->sample_rate)*alink_get_shn(hw_channel_parm)/point_offset);//shn会 滞后hwptr一个样点 */
        time = time - ((u64)1000000 * (u64)alink_get_shn(hw_channel_parm) / p_alink0_parm->sample_rate / point_offset); //shn会 滞后hwptr一个样点

        if (p_alink0_parm->timestamp_cnt[ch] <= timestamp_threshold) {
            if (p_alink0_parm->timestamp[ch]) {
                p_alink0_parm->timestamp_gap[ch] += (time - p_alink0_parm->timestamp[ch]);
            }
            if (p_alink0_parm->timestamp_cnt[ch] == timestamp_threshold) {
                p_alink0_parm->timestamp_gap[ch] /= timestamp_threshold;
            }
            p_alink0_parm->timestamp_cnt[ch]++;

            p_alink0_parm->timestamp[ch] = time;
            /* printf("[%d]pre\n", ch); */
        } else if (p_alink0_parm->timestamp_cnt[ch] > timestamp_threshold) {
            p_alink0_parm->timestamp[ch] += p_alink0_parm->timestamp_gap[ch];

            int diff_us = p_alink0_parm->timestamp[ch] - time;
            if (__builtin_abs(diff_us) > (1000000 / p_alink0_parm->sample_rate)) {
                /* p_alink0_parm->timestamp[ch] = time; */
                if (__builtin_abs(diff_us) > 10 * (1000000 / p_alink0_parm->sample_rate)) {
                    if (diff_us < 0) {
                        p_alink0_parm->timestamp[ch] += 10;
                    }
                    if (diff_us > 0) {
                        p_alink0_parm->timestamp[ch] -= 10;
                    }

                } else {
                    if (diff_us < 0) {
                        p_alink0_parm->timestamp[ch] += 1;
                    }
                    if (diff_us > 0) {
                        p_alink0_parm->timestamp[ch] -= 1;
                    }
                }
                /* putchar('S'+ch); */
                /* printf("[%d]diff %d, %d %d\n", ch, diff_us, p_alink0_parm->timestamp[ch],time); */
                /* if (__builtin_abs(diff_us) > 40) */
            }
            if (__builtin_abs(diff_us) > 1000) {
                /* printf("alink_ch[%d]diff %d\n", ch, diff_us); */
            }
        }

    }
}

AUDIO_IIS_AT_RAM
void alink0_isr_base(u8 ch)
{
    audio_alink_lock(0);
    u32 len;
    alink0_cal_timestamp(ch);//记录double buf alink中断产生的时间戳

    s32 *buf_addr = (s32 *)alink_addr(p_alink0_parm, ch);
    if (p_alink0_parm->ch_cfg[ch].isr_cb) {
        if (p_alink0_parm->buf_mode == ALINK_BUF_CIRCLE) {//循环buf
            if (p_alink0_parm->ch_cfg[ch].dir == ALINK_DIR_TX) {//循环buf输出
                u32 remain;
                len = alink_isr_get_len(p_alink0_parm, ch, &remain);
                p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, len);
            } else {
                u32 remain = 0;
                int out_len = (JL_ALNK0->PNS & 0xffff) << 2;
                len = alink_isr_get_len(p_alink0_parm, ch, &remain);
                if (len >= out_len) {
                    len = out_len;
                    p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, len);
                } else {
                    p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, len);
                    if ((len + remain) >= out_len) {
                        if (remain) {
                            buf_addr = (s32 *)(*ALNK0_BUF_ADR[ch]);
                            p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, out_len - len);
                        }
                    }
                }
                alink_set_shn(&p_alink0_parm->ch_cfg[ch], out_len / 4);
            }
        } else {//兵乓buf
            p_alink0_parm->ch_cfg[ch].isr_cb(p_alink0_parm->ch_cfg[ch].private_data, buf_addr, p_alink0_parm->dma_len / 2);
        }
    } else {
        if (p_alink0_parm->buf_mode == ALINK_BUF_CIRCLE) {//循环buf
            if (p_alink0_parm->ch_cfg[ch].dir == ALINK_DIR_RX) {//循环buf接收,回调函数未配置时，由驱动内部更新shn
                alink_set_shn(&p_alink0_parm->ch_cfg[ch], (JL_ALNK0->PNS & 0xffff));
            }
        }
    }
    audio_alink_unlock(0);
}

___interrupt AUDIO_IIS_AT_RAM
static void alink0_dma_isr(void)
{
    for (int ch = 3; ch >= 0 ; ch--) {
        if (!((JL_ALNK0->CON2) & BIT(ch + 12))) {
            //putchar('R' + ch);
            continue;
        }
        if (!((JL_ALNK0->CON2) & BIT(ch + 4))) {
            //putchar('M' + ch);
            continue;
        }

        alink0_isr_base(ch);
        //printf("------ %d\n", ch);
        ALINK_CLR_CHx_PND(p_alink0_parm->module, ch);
    }
}

static void alink_set_bitwide(void *hw_alink, u32 bitwide)
{
    alink_printf("ALINK_BW = %d\n", bitwide);
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if ((hw_alink_parm->mode == ALINK_MD_BASIC) && (bitwide == ALINK_BW_20BIT)) {
        alink_printf("ALINK_BW_20BIT only for DSP MODE, cur_mode is ALINK_MD_BASIC");
        return;
    }
    hw_alink_parm->bitwide = bitwide;
    u8 module = hw_alink_parm->module;
    u8 bitwide_mode = 0;

    switch (bitwide) {
    case ALINK_BW_16BIT:
        bitwide = 0;
        break;
    case ALINK_BW_20BIT:
        bitwide = 1;
        break;
    case ALINK_BW_24BIT:
        if (test_32bit) {
            bitwide = 3;
        } else {
            bitwide = 2;
        }
        break;
    case ALINK_BW_32BIT:
        bitwide = 3;
        break;
    default:
        bitwide = 0;
        break;
    }
    ALINK_BWLEN_MODE(module, bitwide);
}

static void alink_sr(void *hw_alink, u32 rate)
{
    alink_printf("ALINK_SR = %d\n", rate);
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    hw_alink_parm->sample_rate = rate;
    u8 module = hw_alink_parm->module;
    u32 mclk_fre = 0;
    u8 clk_bit_width;
    switch (hw_alink_parm->bitwide) {
    case ALINK_BW_32BIT:
        clk_bit_width = 32;
        break;
    case ALINK_BW_24BIT:
        if (test_32bit) {
            clk_bit_width = 32;
        } else {
            if (hw_alink_parm->mode != ALINK_MD_BASIC) {//dsp长短短帧时钟位宽配置与数据位宽必须一致
                clk_bit_width = 24;
            } else {
                //24bit数据位宽使用32bit的时钟配置
                clk_bit_width = 32;
            }
        }
        break;
    case ALINK_BW_16BIT:
        clk_bit_width = 16;
        /* clk_bit_width = 32;//验证时钟32bit, 位宽16bit输出 */
        break;
    default:
        clk_bit_width = 32;
        break;
    }

    if (hw_alink_parm->role == ALINK_ROLE_SLAVE) {
        ALINK_MSRC(module, MCLK_EXTERNAL);	/*MCLK source*/
        ALINK_SCLK_SEL(module, SCLK_EXTERNAL);
        return;
    } else {
        ALINK_MSRC(module, MCLK_PLL_CLK);	/*MCLK source*/
        ALINK_SCLK_SEL(module, SCLK_MCKD_SDIV);
    }

    if (hw_alink_parm->mode == ALINK_MD_DSP_SHORT) {
        ALINK_LRCK_DUTY_SET(module, 1);
    } else {
        ALINK_LRCK_DUTY_SET(module, clk_bit_width);
    }

    switch (rate) {
    case ALINK_SR_192000:
    case ALINK_SR_96000:
    case ALINK_SR_48000:
    case ALINK_SR_32000:
    case ALINK_SR_24000:
    case ALINK_SR_16000:
    case ALINK_SR_12000:
    case ALINK_SR_8000:
        mclk_fre = 12288000;
        break ;

    case ALINK_SR_176400:
    case ALINK_SR_88200:
    case ALINK_SR_44100:
    case ALINK_SR_22050:
    case ALINK_SR_11025:
        mclk_fre = 11289600;
        break;

    default:
        mclk_fre = 11289600;
        break;
    }

    u16 lrck_prd = clk_bit_width * (hw_alink_parm->slot_num);
    u32 sclk_fre = rate * lrck_prd;
    u8 sclk_div = mclk_fre / sclk_fre;

    ALINK_LRCK_PRD_SET(module, lrck_prd - 1);
    ALINK_SDIV(module, sclk_div - 1);
    alink_printf("mclk : %d, sclk : %d, lrck : %d, sclk_div : %d, lrck_prd : %d\n", mclk_fre, sclk_fre, rate, sclk_div, lrck_prd);

    clk_set_api("alink0", mclk_fre);


}

void alink_set_irq_handler(void *hw_alink, void *hw_channel, void *priv, void (*handle)(void *priv, void *addr, int len))
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;

    u8 ch_idx = hw_channel_parm->ch_idx;
    hw_alink_parm->ch_cfg[ch_idx].private_data = hw_channel_parm->private_data;
    hw_alink_parm->ch_cfg[ch_idx].isr_cb = hw_channel_parm->isr_cb;
}

void *alink_channel_init_base(void *hw_alink, void *hw_channel, u8 ch_idx)
{
    if (!hw_alink) {
        return NULL;
    }
    if (!hw_channel) {
        return NULL;
    }

    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
    u8 module = hw_alink_parm->module;

    if (hw_alink_parm->ch_cfg[ch_idx].ch_en) {
        printf("This channel % is en\n", ch_idx);
        return &hw_alink_parm->ch_cfg[ch_idx];
    }
    if (ch_idx >= 4) {
        printf("This module's channels are fully registered, fail!!!!!\n");
        return NULL;
    }
    hw_alink_parm->ch_cfg[ch_idx].bit_width = hw_alink_parm->bitwide;
    hw_alink_parm->ch_cfg[ch_idx].module = module;
    hw_alink_parm->ch_cfg[ch_idx].ch_idx = ch_idx;
    hw_alink_parm->ch_cfg[ch_idx].data_io = hw_channel_parm->data_io;
    hw_alink_parm->ch_cfg[ch_idx].ch_ie = hw_channel_parm->ch_ie;
    hw_alink_parm->ch_cfg[ch_idx].dir = hw_channel_parm->dir;
    hw_alink_parm->ch_cfg[ch_idx].private_data = hw_channel_parm->private_data;
    hw_alink_parm->ch_cfg[ch_idx].isr_cb = hw_channel_parm->isr_cb;
    hw_alink_parm->ch_cfg[ch_idx].ch_mode = hw_channel_parm->ch_mode;
    hw_alink_parm->ch_cfg[ch_idx].buf = dma_malloc(hw_alink_parm->dma_len);
    memset(hw_alink_parm->ch_cfg[ch_idx].buf, 0x00, hw_alink_parm->dma_len);


    /* printf(">>> alink_channel_init %x\n", (u32)hw_alink_parm->ch_cfg[ch_idx].buf); */
    u32 *buf_addr;

    //===================================//
    //           ALNK CH DMA BUF         //
    //===================================//
    buf_addr = ALNK0_BUF_ADR[ch_idx];
    *buf_addr = (u32)(hw_alink_parm->ch_cfg[ch_idx].buf);

    //===================================//
    //          ALNK CH DAT IO INIT      //
    //===================================//
    if (hw_alink_parm->ch_cfg[ch_idx].dir == ALINK_DIR_RX) {
        alink_io_in_init(hw_alink_parm->ch_cfg[ch_idx].data_io);
        gpio_set_fun_input_port(hw_alink_parm->ch_cfg[ch_idx].data_io, PFI_ALNK0_DAT[ch_idx]);
    } else {
#if !IIS_IO_INIT_AFTER_ALINK_START
        alink_io_out_init(hw_alink_parm->ch_cfg[ch_idx].data_io);
        gpio_set_fun_output_port(hw_alink_parm->ch_cfg[ch_idx].data_io, FO_ALNK0_DAT[ch_idx], 1, 1);
#endif
    }

    ALINK_CHx_DIR_MODE(module, ch_idx, hw_alink_parm->ch_cfg[ch_idx].dir);
    //===================================//
    //           ALNK工作模式            //
    //===================================//
#if FIRST_EN_CONFIG
    if ((hw_alink_parm->bitwide == ALINK_BW_24BIT) &&
        (hw_alink_parm->ch_cfg[ch_idx].dir == ALINK_DIR_RX) &&
        (hw_alink_parm->mode == ALINK_MD_BASIC) &&
        (hw_alink_parm->ch_cfg[ch_idx].ch_mode == ALINK_CH_MD_BASIC_IIS)) {
        hw_alink_parm->chx_sw[ch_idx] = 1;
    }
#endif
    ALINK_CHx_MODE_SEL(module, ch_idx, hw_alink_parm->ch_cfg[ch_idx].ch_mode);

    ALINK_CLR_CHx_PND(module, ch_idx);
    ALINK_CHx_IE(module, ch_idx, hw_alink_parm->ch_cfg[ch_idx].ch_ie);

    hw_alink_parm->ch_cfg[ch_idx].ch_en = 1;
    alink_printf("alink%d, ch%d init, 0x%x\n", module, ch_idx, (int)&hw_alink_parm->ch_cfg[ch_idx]);

    hw_alink_parm->init_cnt++;

    return &hw_alink_parm->ch_cfg[ch_idx];
}
void alink_channel_io_init(void *hw_alink, void *hw_channel, u8 ch_idx)
{
    if (!hw_alink) {
        return ;
    }

    if (!hw_channel) {
        return ;
    }

#if IIS_IO_INIT_AFTER_ALINK_START
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    /* struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel; */

    //===================================//
    //          ALNK CH DAT IO INIT      //
    //===================================//
    if (hw_alink_parm->ch_cfg[ch_idx].dir == ALINK_DIR_RX) {
    } else {
        alink_io_out_init(hw_alink_parm->ch_cfg[ch_idx].data_io);
        gpio_set_fun_output_port(hw_alink_parm->ch_cfg[ch_idx].data_io, FO_ALNK0_DAT[ch_idx], 1, 1);
    }
#endif
}
void *alink_channel_init(void *hw_alink, void *hw_channel)
{
    u8 ch_idx;
    void *ret = NULL;
    for (ch_idx = 0; ch_idx < 4; ch_idx++) {
        ret = alink_channel_init_base(hw_alink, hw_channel, ch_idx);
        if (ret) {
            return ret;
        }
    }
    return NULL;
}

void alink0_info_dump()
{
    alink_printf("JL_ALNK0->CON0 = 0x%x", JL_ALNK0->CON0);
    alink_printf("JL_ALNK0->CON1 = 0x%x", JL_ALNK0->CON1);
    alink_printf("JL_ALNK0->CON2 = 0x%x", JL_ALNK0->CON2);
    alink_printf("JL_ALNK0->CON3 = 0x%x", JL_ALNK0->CON3);
    alink_printf("JL_ALNK0->CON4 = 0x%x", JL_ALNK0->CON4);
    alink_printf("JL_ALNK0->LEN  = 0x%x", JL_ALNK0->LEN);
    alink_printf("JL_ALNK0->ADR0 = 0x%x", JL_ALNK0->ADR0);
    alink_printf("JL_ALNK0->ADR1 = 0x%x", JL_ALNK0->ADR1);
    alink_printf("JL_ALNK0->ADR2 = 0x%x", JL_ALNK0->ADR2);
    alink_printf("JL_ALNK0->ADR3 = 0x%x", JL_ALNK0->ADR3);
    alink_printf("JL_ALNK0->PNS  = 0x%x", JL_ALNK0->PNS);
    alink_printf("JL_ALNK0->HWPTR0 = 0x%x", JL_ALNK0->HWPTR0);
    alink_printf("JL_ALNK0->HWPTR1 = 0x%x", JL_ALNK0->HWPTR1);
    alink_printf("JL_ALNK0->HWPTR2 = 0x%x", JL_ALNK0->HWPTR2);
    alink_printf("JL_ALNK0->HWPTR3 = 0x%x", JL_ALNK0->HWPTR3);
    alink_printf("JL_ALNK0->SWPTR0 = 0x%x", JL_ALNK0->SWPTR0);
    alink_printf("JL_ALNK0->SWPTR1 = 0x%x", JL_ALNK0->SWPTR1);
    alink_printf("JL_ALNK0->SWPTR2 = 0x%x", JL_ALNK0->SWPTR2);
    alink_printf("JL_ALNK0->SWPTR3 = 0x%x", JL_ALNK0->SWPTR3);
    alink_printf("JL_ALNK0->SHN0  = 0x%x", JL_ALNK0->SHN0);
    alink_printf("JL_ALNK0->SHN1  = 0x%x", JL_ALNK0->SHN1);
    alink_printf("JL_ALNK0->SHN2  = 0x%x", JL_ALNK0->SHN2);
    alink_printf("JL_ALNK0->SHN3  = 0x%x", JL_ALNK0->SHN3);
    alink_printf("JL_ALNK0->BLOCK = 0x%x", JL_ALNK0->BLOCK);
    alink_printf("JL_LSBCLK->PRP_CON1 = 0x%x", JL_LSBCLK->PRP_CON1);
}

void alink_set_da2sync_ch(void *hw_alink)
{
    if (hw_alink == NULL) {
        return;
    }

    ALINK_PARM *hw_alink_user = (ALINK_PARM *)hw_alink;		//传入参数
    u8 module = hw_alink_user->module;
    //选用与蓝牙同步关联的通道
    u8 ch_idx = hw_alink_user->da2sync_ch;
    if (ch_idx != 0xff) {
        printf("module %d, sel ch_idx %d\n", module, ch_idx);
        ALINK_DA2BTSRC_SEL(module, ch_idx);
    }
}

void *alink_init(void *hw_alink)
{
    if (hw_alink == NULL) {
        return NULL;
    }
    ALINK_PARM *hw_alink_user = (ALINK_PARM *)hw_alink;		//传入参数
    ALINK_PARM *hw_alink_parm = NULL;						//驱动赋值参数
    u8 module = hw_alink_user->module;
    hw_alink_parm = p_alink0_parm;
    if (hw_alink_parm && hw_alink_parm->init_cnt != 0) {	//ALNK0已被初始化过
        return hw_alink_parm;
    } else if (!hw_alink_parm) {							//ALNK0初始化
        p_alink0_parm = zalloc(sizeof(ALINK_PARM));
        hw_alink_parm = p_alink0_parm;
    }

    hw_alink_parm->module 	= hw_alink_user->module;
    hw_alink_parm->mclk_io 	= hw_alink_user->mclk_io;
    hw_alink_parm->sclk_io 	= hw_alink_user->sclk_io;
    hw_alink_parm->lrclk_io = hw_alink_user->lrclk_io;
    hw_alink_parm->mode 	= hw_alink_user->mode;
    hw_alink_parm->role 	= hw_alink_user->role;
    hw_alink_parm->clk_mode = hw_alink_user->clk_mode;
    hw_alink_parm->bitwide 	= hw_alink_user->bitwide;
    hw_alink_parm->dma_len 	= hw_alink_user->dma_len;
    hw_alink_parm->buf_mode = hw_alink_user->buf_mode;
    hw_alink_parm->sample_rate = hw_alink_user->sample_rate;
    hw_alink_parm->slot_num = hw_alink_user->slot_num;
    ALINK_CON_RESET(module);
    ALINK_HWPTR_RESET(module);
    ALINK_SWPTR_RESET(module);
    ALINK_ADR_RESET(module);
    ALINK_SHN_RESET(module);
    ALINK_PNS_RESET(module);
    ALINK_CLR_ALL_PND(module);


    //选用与蓝牙同步关联的通道
    u8 ch_idx = hw_alink_user->da2sync_ch;
    if (ch_idx != 0xff) {
        alink_printf("module %d, sel ch_idx %d\n", module, ch_idx);
        ALINK_DA2BTSRC_SEL(module, ch_idx);
    }


    //===================================//
    //        输出时钟配置               //
    //===================================//
    if (hw_alink_parm->role == ALINK_ROLE_MASTER) {
        //主机输出时钟
        alink_io_out_init(hw_alink_parm->mclk_io);
        alink_io_out_init(hw_alink_parm->lrclk_io);
        alink_io_out_init(hw_alink_parm->sclk_io);
        gpio_set_fun_output_port(hw_alink_parm->mclk_io, FO_ALNK0_MCLK, 1, 1);
        gpio_set_fun_output_port(hw_alink_parm->lrclk_io, FO_ALNK0_LRCK, 1, 1);
        gpio_set_fun_output_port(hw_alink_parm->sclk_io, FO_ALNK0_SCLK, 1, 1);
    } else {
        //从机输入时钟
        alink_io_in_init(hw_alink_parm->mclk_io);
        alink_io_in_init(hw_alink_parm->sclk_io);
        alink_io_in_init(hw_alink_parm->lrclk_io);
        gpio_set_fun_input_port(hw_alink_parm->mclk_io, PFI_ALNK0_MCLK);
        gpio_set_fun_input_port(hw_alink_parm->sclk_io, PFI_ALNK0_SCLK);
        gpio_set_fun_input_port(hw_alink_parm->lrclk_io, PFI_ALNK0_LRCK);
    }
    //===================================//
    //        基本模式/扩展模式          //
    //===================================//
    ALINK_DSP_MODE(module, hw_alink_parm->mode);
    //===================================//
    //         数据位宽		             //
    //===================================//
    alink_set_bitwide(hw_alink_parm, hw_alink_parm->bitwide);
    //===================================//
    //             时钟边沿选择          //
    //===================================//
    ALINK_SCLKINV(module, hw_alink_parm->clk_mode);
    //===================================//
    //            通道SLOT数量	         //
    //===================================//
    if (hw_alink_parm->mode == ALINK_MD_BASIC) {
        hw_alink_parm->slot_num = ALINK_SLOT_NUM2;
    } else if (hw_alink_parm->mode == ALINK_MD_DSP_LONG) {
        hw_alink_parm->slot_num = (hw_alink_parm->slot_num < ALINK_SLOT_NUM2) ? ALINK_SLOT_NUM2 : hw_alink_parm->slot_num;
    }
    ALINK_SLOT_SET(module, hw_alink_parm->slot_num - 1);

    //===================================//
    //            设置数据采样率       	 //
    //===================================//
    alink_sr(hw_alink_parm, hw_alink_parm->sample_rate);

    //===================================//
    //            设置DMA MODE       	 //
    //===================================//
    ALINK_DMA_MODE_SEL(module, hw_alink_parm->buf_mode);
    if (hw_alink_parm->buf_mode == ALINK_BUF_CIRCLE) {
        ALINK_OPNS_SET(module, hw_alink_parm->dma_len / 16);
        if (hw_alink_parm->rx_pns) {
            ALINK_IPNS_SET(module, hw_alink_parm->rx_pns);
        } else {
            ALINK_IPNS_SET(module, hw_alink_parm->dma_len / 16);
        }

        //一个点需要2bytes, LR = 2
        ALINK_LEN_SET(module, hw_alink_parm->dma_len / 4);
    } else {
        //一个点需要2bytes, LR = 2, 双buf = 2
        ALINK_LEN_SET(module, hw_alink_parm->dma_len / 8);
    }
    u32 block = 0;
    if (hw_alink_parm->bitwide == ALINK_BW_16BIT) {
        block = JL_ALNK0->LEN * 2 / hw_alink_parm->slot_num;
    } else {
        block = JL_ALNK0->LEN / hw_alink_parm->slot_num;
    }
    ALINK_BLOCK_SET(module, block);

    request_irq(IRQ_ALINK0_IDX, 5, alink0_dma_isr, 0);

    return hw_alink_parm;
}

void alink_channel_close(void *hw_alink, void *hw_channel)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;

    if (!hw_alink_parm) {
        return;
    }
    if (!hw_channel_parm) {
        return;
    }

    ALINK_CHx_CLOSE(hw_channel_parm->module, hw_channel_parm->ch_idx);
    ALINK_CHx_IE(hw_channel_parm->module, hw_channel_parm->ch_idx, 0);

    alink_io_uninit(hw_channel_parm->data_io);
    if (hw_channel_parm->buf) {
        dma_free(hw_channel_parm->buf);
        hw_channel_parm->buf = NULL;
    }
    hw_channel_parm->ch_en = 0;
    hw_alink_parm->init_cnt--;
    alink_printf("alink_ch %d closed cnt %d\n", hw_channel_parm->ch_idx, hw_alink_parm->init_cnt);
}


int alink_start(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (!hw_alink_parm) {
        return -1;
    }

#if !FIRST_EN_CONFIG
    ALINK_EN(hw_alink_parm->module, 1);
    //alink0_info_dump();
#else
    if (ALINK_EN_GET(hw_alink_parm->module)) {
        alink_printf("alink is opened");
        return 0;
    }
    if (first_en) {
        //第一次正常打开
        alink_printf(">>> iis is first en");
        first_en = 0;
        ALINK_EN(hw_alink_parm->module, 1);
        return 0;
    }

    alink_printf(">>> iis is not first en");
    u32 ch_mode_clr = 0;            //需要清除的通道模式
    u32 ch_mode_set_iis = 0;        //需要设置为iis的通道
    u8 wait_pend_ch = 0xff;
    for (u8 i = 0; i < 4; i++) {
        if (hw_alink_parm->chx_sw[i]) {
            //设为左对齐模式
            ALINK_CHx_MODE_SEL(hw_alink_parm->module, i, ALINK_CH_MD_BASIC_LALIGN);
            u8 bit_low = i * 3;
            u8 bit_high = i * 3 + 1;
            ch_mode_clr |= (BIT(bit_low) | BIT(bit_high));
            ch_mode_set_iis |= BIT(bit_low);
            if (wait_pend_ch == 0xff) {
                wait_pend_ch = i;
            }
        }
    }
    alink_printf("ch_mode_clr 0x%x, ch_mode_set_iis 0x%x, wait_pend_ch 0x%x", ch_mode_clr, ch_mode_set_iis, wait_pend_ch);

    if (wait_pend_ch == 0xff) {
        //没有通道需要作模式切换操作，直接使能ALNK_EN后返回
        ALINK_EN(hw_alink_parm->module, 1);
        return 0;
    }

    local_irq_disable();
    ALINK_EN(hw_alink_parm->module, 1);

    while (*ALNK0_HWPTR[wait_pend_ch] < 1) {
        //等硬件指针为1，越过第一个点，再切回基础模式
    }
    JL_ALNK0->CON1 &= ~(ch_mode_clr);
    JL_ALNK0->CON1 |= ch_mode_set_iis;
    local_irq_enable();
#endif
    return 0;
}

int alink_uninit_base(void *hw_alink, void *hw_channel)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (!hw_alink_parm) {
        return 0;
    }

    alink_printf("audio_link_exit\n");


    if (hw_alink_parm->init_cnt == 0) {
        ALINK_EN(hw_alink_parm->module, 0);
        if (hw_alink_parm->role == ALINK_ROLE_MASTER) {
            if (hw_alink_parm->mclk_io != ALINK_CLK_OUPUT_DISABLE) {
                gpio_disable_fun_output_port(hw_alink_parm->mclk_io);
                gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->mclk_io), PORT_HIGHZ);
            }
            if ((hw_alink_parm->sclk_io != ALINK_CLK_OUPUT_DISABLE) && (hw_alink_parm->lrclk_io != ALINK_CLK_OUPUT_DISABLE)) {
                gpio_disable_fun_output_port(hw_alink_parm->sclk_io);
                gpio_disable_fun_output_port(hw_alink_parm->lrclk_io);
                gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->sclk_io), PORT_HIGHZ);
                gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->lrclk_io), PORT_HIGHZ);
            }
        } else {
            gpio_disable_fun_input_port(PFI_ALNK0_MCLK);
            gpio_disable_fun_input_port(PFI_ALNK0_SCLK);
            gpio_disable_fun_input_port(PFI_ALNK0_LRCK);
            gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->mclk_io), PORT_HIGHZ);
            gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->sclk_io), PORT_HIGHZ);
            gpio_set_mode(IO_PORT_SPILT(hw_alink_parm->lrclk_io), PORT_HIGHZ);
        }

        hw_alink_parm->init_cnt = 0;
        if (p_alink0_parm) {
            free(p_alink0_parm);
            p_alink0_parm = NULL;
        }
        alink_printf("audio_link_exit OK\n");
        return 1;
    }

    return 0;
}
int alink_uninit(void *hw_alink, void *hw_channel)
{
    alink_channel_close(hw_alink, hw_channel);
    return alink_uninit_base(hw_alink, hw_channel);
}

void alink_uninit_force(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;

    if (!hw_alink_parm) {
        return;
    }
    u8 module = hw_alink_parm->module;

    ALINK_EN(module, 0);
    for (int i = 0; i < 4; i++) {
        alink_channel_close(hw_alink, &hw_alink_parm->ch_cfg[i]);
    }
    ALINK_CON_RESET(module);
    hw_alink_parm->init_cnt = 0;

    if (p_alink0_parm) {
        free(p_alink0_parm);
        p_alink0_parm = NULL;
    }

    alink_printf("audio_link_force_exit OK\n");
}

int alink_get_sr(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    if (hw_alink_parm) {
        return hw_alink_parm->sample_rate;
    } else {
        return -1;
    }
}

int alink_set_sr(void *hw_alink, u32 sr)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;

    if (!hw_alink_parm) {
        return -1;
    }

    if (hw_alink_parm->sample_rate == sr) {
        return 0;
    }

    u8 is_alink_en = (ALINK_SEL(hw_alink_parm->module, CON0) & BIT(11));

    if (is_alink_en) {
        ALINK_EN(hw_alink_parm->module, 0);
    }

    alink_sr(hw_alink_parm, sr);

    if (is_alink_en) {
        ALINK_EN(hw_alink_parm->module, 1);
    }

    return 0;
}

u32 alink_get_dma_len(void *hw_alink)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    return ALINK_FIFO_LEN(hw_alink_parm->module)/* *((IIS_CH_NUM_TEST == 1)?2:1) */;
}

void alink_set_tx_pns(void *hw_alink, u32 len)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    ALINK_OPNS_SET(hw_alink_parm->module, len);
}

void alink_set_rx_pns(void *hw_alink, u32 len)
{
    ALINK_PARM *hw_alink_parm = (ALINK_PARM *)hw_alink;
    ALINK_IPNS_SET(hw_alink_parm->module, len);
}

void alink_sw_mute(void *hw_channel, u8 mute)
{
    struct alnk_hw_ch *hw_channel_parm = (struct alnk_hw_ch *)hw_channel;
#if 1
    if (mute) {
        /* alink_printf("===================ch[%d]alink_sw mute================== \n", hw_channel_parm->ch_idx); */
        alink_set_ch_ie(hw_channel_parm, 0);
        ALINK_SW_MUTE_EN(hw_channel_parm->module, hw_channel_parm->ch_idx, 1);
        ALINK_SW_MUTE_WAIT_ACK(hw_channel_parm->module, hw_channel_parm->ch_idx);
        alink_set_swptr(hw_channel_parm, 0);
        alink_set_hwptr(hw_channel_parm, 0);
        hw_channel_parm->sw_mute_status = 1;
        /* puts("================ end\n"); */
        //write data
        //set shn
    } else {
        alink_clr_ch_pnd(hw_channel_parm);
        alink_set_ch_ie(hw_channel_parm, 1);
        /* putchar('T'); */
        if (ALINK_SW_MUTE_STATUS(hw_channel_parm->module, hw_channel_parm->ch_idx)) {
            /* alink_printf("===================ch[%d]alink_sw unmute==================\n", hw_channel_parm->ch_idx); */
            ALINK_SW_MUTE_EN(hw_channel_parm->module, hw_channel_parm->ch_idx, 0);
            hw_channel_parm->sw_mute_status = 0;
            /* alink0_info_dump(); */
        }
        /* alink0_info_dump(); */
    }
#else
    //while(!(JL_ALNK0->CON2 & BIT(4)));  //wait ch0 pnd
    delay(1400);

    SFR(JL_ALNK0->CON2, 12, 1, 0); // close ch0 ie
    SFR(JL_ALNK0->CON3, 26, 1, 1); // sys_clk : sw_mute_sfr == 1

    while (!(JL_ALNK0->CON4 & BIT(0))); //wait sclk : ch0 sw_mute == 1

    //JL_TIMER3->PWM = 0xf;
    //JL_TIMER3->PWM = JL_ALNK0->CON4;

    JL_ALNK0->HWPTR0 = 0;
    JL_ALNK0->SWPTR0 = 0;
    JL_ALNK0->SHN0   = ALNKLEN - 1;  // tx

    //delay(10);
    JL_ALNK0->CON2 |= BIT(0);      // clr ch0 pnd
    SFR(JL_ALNK0->CON2, 12, 1, 1); // open ch0 ie
    SFR(JL_ALNK0->CON3, 26, 1, 0); //sw_mute == 0
#endif

}


#endif
