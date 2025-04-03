#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lp_touch_key_identify_algo.data.bss")
#pragma data_seg(".lp_touch_key_identify_algo.data")
#pragma const_seg(".lp_touch_key_identify_algo.text.const")
#pragma code_seg(".lp_touch_key_identify_algo.text")
#endif
#include "system/includes.h"
#include "asm/lp_touch_key_identify_algo.h"



#define MEDIAN_FILT_LENGTH				(11)

//#If the long press time exceed 30s and the stable time exceed 15s, then try to release the key state
#define TOUCHALGO_LONGPRESS_LIMIT		(60*50)  //   #60s,

//#If the stable time exceed 15s, then try to synchronize the reference value with current x
#define TOUCHALGO_STABLE_COUNT_TH		(15*50)  //   #15s

#define TOUCHALGO_STABLE_COUNT_FAST_TH  (3*50)  //   #3s

//# The uneven detect interval
#define TOUCHALGO_UNEVEN_DET_INTERVAL	(20)


#define REF_IIR_ALPHA					2
#define REF_IIR_ALPHA_FAST				32

#define DEBOUNCE_DOWN_TH				3	//按下几个点算按下
#define DEBOUNCE_UP_TH					2	//抬起几个点算抬起

#define	ALGO_DEBUG						0

#define TOUCHALGO_TNTERF_TH             (30<<6)

typedef struct {
    u8 state;   //0:init, 1:down state, 2:down down state
    u16 count;
    u32 stabled_x;
} multidown_state_t;

typedef struct {

    u16 med_dat0;
    u16 med_dat1;

    u32 ref_iir_state;

    u16 debounce_down;
    u16 debounce_up;

    u16 temp_th;
    u16 edge_down_th;


    u16 x_buf[MEDIAN_FILT_LENGTH];
    u8 x_buf_in;

    u8 key_state;

    //uneven
    u16 uneven_max;
    u16 uneven_tmp;

    //stable count
    u16 stable_count;

    // long press count
    u16 longpress_count;

    //xsmooth
    u32 x_iir_state;

    u32 interference_value;

    multidown_state_t multidown;

    u8 count;

#if ALGO_DEBUG	//for debug
    int dat0;
    int dat1;
#endif

} TouchIdentifyAlgo_t;


#define ABS(a, b)         (a > b) ? (a -b) : (b-a)


static void x_bubble_sort(u16 *buf, u16 len)
{
    u16 tmp, i;
__bcpare:
    tmp = 0xffff;
    for (i = 0; i < (len - 1); i ++) {
        if (buf[i] > buf[i + 1]) {
            tmp = buf[i];
            buf[i] = buf[i + 1];
            buf[i + 1] = tmp;
        }
    }
    if (tmp != 0xffff) {
        goto __bcpare;
    }
}

static u16 _medfilt(TouchIdentifyAlgo_t *tia, u16 x)
{
    //缓存数据
    tia->x_buf[tia->x_buf_in] = x;
    tia->x_buf_in ++;
    if (tia->x_buf_in >= MEDIAN_FILT_LENGTH) {
        tia->x_buf_in = 0;
    }
    //拷到临时buf
    u16 x_tmp_buf[MEDIAN_FILT_LENGTH];
    for (u8 i = 0; i < MEDIAN_FILT_LENGTH; i ++) {
        x_tmp_buf[i] = tia->x_buf[i];
    }
    //临时buf排序
    x_bubble_sort(x_tmp_buf, MEDIAN_FILT_LENGTH);
    //返回中间值
    return x_tmp_buf[(MEDIAN_FILT_LENGTH / 2)];
}

static void _medfilt_init(TouchIdentifyAlgo_t *tia)
{
    tia->x_buf_in = 0;
    for (u8 i = 0; i < MEDIAN_FILT_LENGTH; i ++) {
        tia->x_buf[i] = 0;
    }
}

static u16 medfilt3(TouchIdentifyAlgo_t *tia, u16 x)
{
    u16 ret = x;
    if (tia->med_dat0 >= tia->med_dat1) {
        if (x >= tia->med_dat0) {
            ret = tia->med_dat0;
        } else if (x < tia->med_dat1) {
            ret = tia->med_dat1;
        }
    } else {
        if (x >= tia->med_dat1) {
            ret = tia->med_dat1;
        } else if (x < tia->med_dat0) {
            ret = tia->med_dat0;
        }
    }

    tia->med_dat0 = tia->med_dat1;
    tia->med_dat1 = x;
    return ret;
}

static u16 ref_iir(TouchIdentifyAlgo_t *tia, u16 x, u16 alpha)
{
    if (tia->count <= 5) {
        tia->ref_iir_state = (u32)x << 6;
    }
    tia->ref_iir_state = (tia->ref_iir_state * (256 - alpha) + ((u32)x << 6) * alpha + 128) >> 8;

    return tia->ref_iir_state;
}

static u16 x_iir(TouchIdentifyAlgo_t *tia, u16 x, u16 alpha)
{
    if (tia->count <= 10) {
        tia->x_iir_state = (u32)x << 6;
    }
    tia->x_iir_state = (tia->x_iir_state * (256 - alpha) + ((u32)x << 6) * alpha + 128) >> 8;

    return tia->x_iir_state;
}

static u16 interf_iir(TouchIdentifyAlgo_t *tia, u16 x, u16 alpha)
{
    if (tia->count <= 10) {
        tia->interference_value = (u32)x << 6;
    }
    tia->interference_value = (tia->interference_value * (256 - alpha) + ((u32)x << 6) * alpha + 128) >> 8;

    return tia->interference_value;
}

static s16 highpass(TouchIdentifyAlgo_t *tia, u16 x)
{
    s16 hp = ((s16)x) - ((s16)tia->med_dat0);
    return hp;
}

static u16 uneven(TouchIdentifyAlgo_t *tia, u16 hp)
{
    u16 ret = 0;
    if (hp > tia->uneven_tmp) {
        tia->uneven_tmp = hp;
    }

    ret = tia->uneven_tmp;
    if (ret < tia->uneven_max) {
        ret = tia->uneven_max;
    }

    if ((tia->count % TOUCHALGO_UNEVEN_DET_INTERVAL) == 0) {
        tia->uneven_max = tia->uneven_tmp;
        tia->uneven_tmp = hp;
    }

    return ret;
}

static void stable(TouchIdentifyAlgo_t *tia, u16 hp, u16 th)
{
    if (hp < th) {
        tia->stable_count += 1;
    } else {
        tia->stable_count = 0;
    }
}


static void interference_detect(TouchIdentifyAlgo_t *tia, s16 _hp)
{
    u16 hp;
    if (tia->key_state == 0) {
        if (_hp > (s16)tia->temp_th) {
            hp = _hp;
        } else {
            hp = 0;
        }
    } else {
        if (_hp < (-((s16)tia->temp_th))) {
            hp = -_hp;
        } else {
            hp = 0;
        }
    }
    interf_iir(tia, hp, 5);
}

static void update_multidown_state(TouchIdentifyAlgo_t *tia, u16 x, u16 adaptive_down_th)
{

    switch (tia->multidown.state) {
    case 0:
        //init
        tia->multidown.stabled_x = x << 6;
        tia->multidown.state = 1;
        tia->multidown.count = 0;
        break;
    case 1:
        //down state
        tia->multidown.count++;
        if (x + adaptive_down_th < (tia->multidown.stabled_x >> 6) && tia->multidown.count > 20) {
            tia->multidown.state = 2;
            tia->multidown.count = 0;
        } else {
            if (tia->multidown.count < 10) {
                tia->multidown.stabled_x = ((u32)tia->multidown.stabled_x * (256 - 32) + ((u32)x << 6) * 32) >> 8;
            } else {
                tia->multidown.stabled_x = ((u32)tia->multidown.stabled_x * (256 - 2) + ((u32)x << 6) * 2) >> 8;
            }
        }
        break;
    case 2:
        //down down state
        if (x + adaptive_down_th < (tia->multidown.stabled_x >> 6)) {
            tia->multidown.count++;
            if (tia->multidown.count > 5) {
                //down down detected
                //reset ref
                tia->ref_iir_state = tia->multidown.stabled_x;
                tia->multidown.state = 0;
            }
        } else {
            tia->multidown.state = 1;
        }
        break;
    }
}

void TouchIdentifyAlgo_Init(TouchIdentifyAlgo_t *tia)
{
    tia->count = 0;

    tia->key_state = 0; //keyup

    tia->med_dat0 = 0;
    tia->med_dat1 = 0;

    tia->debounce_down = 0;

    tia->uneven_max = 0;
    tia->uneven_tmp = 0;

    tia->stable_count = 0;

    tia->longpress_count = 0;

    tia->x_iir_state = 0;


    _medfilt_init(tia);

    tia->interference_value = 0;

    tia->multidown.state = 0;
    tia->multidown.stabled_x = 0;
    tia->multidown.count = 0;
}


void TouchIdentifyAlgo_Update(TouchIdentifyAlgo_t *tia, u16 x)
{
    u16 dat0, dat1;
    s16 hp;
    u16 hp_abs;
    u16 ref, x_smoothed;
    u8 key = 0;
    u16 uneven_val = 0;
    u8 in_interference = 0;

    dat0 = medfilt3(tia, x);


    dat1 = _medfilt(tia, dat0);

    //#must after the medfilt3
    hp = highpass(tia, x);

    hp_abs = hp < 0 ? -hp : hp;

    uneven_val = uneven(tia, hp_abs);

    interference_detect(tia, hp);

    if ((tia->interference_value > TOUCHALGO_TNTERF_TH) && (tia->count >= 20)) {
        in_interference = 1;
    } else {
        x_iir(tia, dat0, 5);
    }

#if ALGO_DEBUG	//for debug
    tia->dat0 = dat0;
    tia->dat1 = dat1;
#endif

    if (tia->count < 3) {
        tia->count++;
        return;
    }

    if (tia->count < 50) {
        ref_iir(tia, dat0, REF_IIR_ALPHA_FAST * 4);
        tia->count++;
        return;
    }

    ref = (tia->ref_iir_state + 32) >> 6;

    x_smoothed = (tia->x_iir_state + 32) >> 6;

    u16 adaptive_temp_th = tia->temp_th;
    u16 adaptive_down_th = tia->edge_down_th;
    u16 adaptive_up_th = adaptive_down_th / 2;

    stable(tia, hp_abs, adaptive_temp_th * 2);

    if (!in_interference) {
        if (dat1 > (ref + adaptive_temp_th)) {
            if (uneven_val < (adaptive_temp_th * 2)) {
                ref_iir(tia, dat1, REF_IIR_ALPHA_FAST);
            }
        } else if (((dat1 + adaptive_temp_th) >= ref) && (dat1 <= (ref + adaptive_temp_th))) {
            ref_iir(tia, dat1, REF_IIR_ALPHA);
        } else {
            if ((uneven_val < (adaptive_temp_th * 2)) && (tia->key_state == 0)) {
                ref_iir(tia, dat1, 1);
            }
        }
        //# restore reference
        if ((tia->stable_count > TOUCHALGO_STABLE_COUNT_TH) && (tia->key_state == 0)) {
            tia->ref_iir_state = tia->x_iir_state;
        }
    }

    if (tia->key_state == 0) {
        key = 0;
        tia->debounce_up = 0;
        if (((dat0  + adaptive_down_th) < ref) && (!in_interference)) {
            tia->debounce_down ++;
            if (tia->debounce_down >= DEBOUNCE_DOWN_TH) {
                key = 1;  //key down
                tia->longpress_count = 0;
            }
        } else {
            tia->debounce_down = 0;
        }
    } else {
        key = 1;
        tia->debounce_down = 0;
        //if edge is detected, release the key.
        //may detect double click in signle click scene, so using adaptive_down_th*3/2 to avoid it
        if ((dat0 + adaptive_up_th) > ref) { // || hp > adaptive_down_th*3/2) {
            tia->debounce_up ++;
            if (tia->debounce_up >= DEBOUNCE_UP_TH) {
                key = 0; //key up
            }
        } else {
            tia->debounce_up = 0;
            tia->longpress_count += 1;
            if ((tia->longpress_count > TOUCHALGO_LONGPRESS_LIMIT)
                && ((tia->stable_count > TOUCHALGO_STABLE_COUNT_TH) || (dat0 > (x_smoothed + adaptive_down_th)))) {
                key = 0;
                tia->longpress_count = 0;
                //reset state
                tia->ref_iir_state = dat0 << 6;
            }
        }
    }

    if ((!in_interference) && (tia->key_state == 1)) {
        update_multidown_state(tia, x, adaptive_down_th);
    } else {
        tia->multidown.state = 0;
    }

    tia->key_state = key;

    if (tia->count < 240) {
        tia->count++;
    }

    return;
}

void TouchIdentifyAlgo_Reset(TouchIdentifyAlgo_t *tia)
{
    tia->count = 0;
    tia->key_state = 0;
}

u16 TouchIdentifyAlgo_GetKey(TouchIdentifyAlgo_t *tia)
{
    return tia->key_state;
}

//=======================================================================//
//                              算法 API                                 //
//=======================================================================//

static TouchIdentifyAlgo_t *tia_ch[5];


u32 lp_touch_key_identify_algorithm_get_tia_size(void)
{
    return (sizeof(TouchIdentifyAlgo_t));
}

void lp_touch_key_identify_algorithm_set_tia_addr(u32 ch_idx, void *addr)
{
    tia_ch[ch_idx] = (TouchIdentifyAlgo_t *)addr;
}

void *lp_touch_key_identify_algorithm_get_tia_addr(u32 ch_idx)
{
    return (void *)tia_ch[ch_idx];
}

void lp_touch_key_identify_algorithm_init(u32 ch_idx, u32 temp_th, u32 edge_down_th)
{
    tia_ch[ch_idx]->temp_th = temp_th;
    tia_ch[ch_idx]->edge_down_th = edge_down_th;
    TouchIdentifyAlgo_Init(tia_ch[ch_idx]);
    TouchIdentifyAlgo_Reset(tia_ch[ch_idx]);
}

u32 lp_touch_key_identify_algorithm_analyze(u32 ch_idx, u32 res)
{
    TouchIdentifyAlgo_Update(tia_ch[ch_idx], res);
    return TouchIdentifyAlgo_GetKey(tia_ch[ch_idx]);
}

void lp_touch_key_identify_algo_set_temp_th(u32 ch_idx, u32 new_temp_th)
{
    if (tia_ch[ch_idx]->temp_th == new_temp_th) {
        return;
    }
    tia_ch[ch_idx]->temp_th = new_temp_th;
}

void lp_touch_key_identify_algo_set_edge_down_th(u32 ch_idx, u32 new_edge_down_th)
{
    u16 edge_down_th_min = 3 * tia_ch[ch_idx]->temp_th;
    if (new_edge_down_th < edge_down_th_min) {
        new_edge_down_th = edge_down_th_min;
    }

    if (tia_ch[ch_idx]->edge_down_th == new_edge_down_th) {
        return;
    }

    tia_ch[ch_idx]->edge_down_th = new_edge_down_th;
}

u32 lp_touch_key_identify_algo_get_edge_down_th(u32 ch_idx)
{
    return tia_ch[ch_idx]->edge_down_th;
}

void lp_touch_key_identify_algo_reset(u32 ch_idx)
{
    TouchIdentifyAlgo_Reset(tia_ch[ch_idx]);
}

u32 lp_touch_key_identify_algo_count_status(u32 ch_idx)
{
    if (tia_ch[ch_idx]->count < 50) {
        return 1;
    }
    return 0;
}

void lp_touch_key_identify_algo_get_ref_lim(u32 ch_idx, u16 *lim_l, u16 *lim_h, u32 *algo_valid)
{
    if (tia_ch[ch_idx]->count < 50) {
        *algo_valid = 0;
    } else {
        u16 ref = (tia_ch[ch_idx]->ref_iir_state + 32) >> 6;
        ref = ref > tia_ch[ch_idx]->temp_th ? ref : tia_ch[ch_idx]->temp_th;
        *lim_l = ref - tia_ch[ch_idx]->temp_th;
        *lim_h = ref + tia_ch[ch_idx]->temp_th;
        *algo_valid = 1;
    }
}


