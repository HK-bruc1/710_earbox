
// #include <stdio.h>
// #include <stdbool.h>
// #include <stdint.h>
// #include <stdlib.h>
// #include <math.h>
#include "typedef.h"
#ifdef  TYHX_DEMO
#include "twi_master.h"
#include "SEGGER_RTT.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "drv_oled.h"
#include "opr_oled.h"
#include "oled_iic.h"
#include "word.h"
#include "iic.h"
#endif

#include "syscfg_id.h"
#include "hx3011.h"
#include "gSensor/gSensor_manage.h"
#include "rcsp_cmd_user.h"
#include "app_tone.h"
//#include "tyhx_hrs_custom.h"
#ifdef SAR_ALG_LIB
#include "hx3011_prox.h"
#endif

#include "btstack/avctp_user.h"

#ifdef HRS_ALG_LIB
#include "hx3011_hrs_agc.h"
#include "tyhx_hrs_alg.h"
#endif

#include "tone_player.h"

#ifdef SPO2_ALG_LIB
#include "hx3011_spo2_agc.h"
#include "tyhx_spo2_alg.h"
#endif

#ifdef HRV_ALG_LIB
#include "hx3011_hrv_agc.h"
#include "tyhx_hrv_alg.h"
#endif

#ifdef CHECK_TOUCH_LIB
#include "hx3011_check_touch.h"
#endif
#include "hx3011_factory_test.h"

#include "log.h"
#include "gpio.h"
#include "bt_tws.h"
#include "asm/anc.h"



#ifdef SPO2_VECTOR
#include "spo2_vec.h"
uint32_t spo2_send_cnt = 0;
int32_t red_buf_vec[8];
int32_t ir_buf_vec[8];
int32_t green_buf_vec[8];
#endif

#ifdef HR_VECTOR
#include "hr_vec.h"
uint32_t hrs_send_cnt = 0;
int32_t PPG_buf_vec[8];
#endif

#ifdef HRV_TESTVEC
#include "hrv_testvec.h"
#endif

void func_callback_in_task_tone_play(u8 mode);
#if TCFG_HEART_SENSOR
// #ifdef GSENSER_DATA
// #include "lis3dh_drv.h"
// #endif
tyhx_hrsresult_t hrsresult;

#ifdef GSENSER_DATA
volatile int16_t gsen_fifo_x[64];  //ppg time 330ms..330/40 = 8.25
volatile int16_t gsen_fifo_y[64];
volatile int16_t gsen_fifo_z[64];
#else
int16_t gen_dummy[64] = {0};
#endif
//SPO2 agc
const uint8_t  hx3011_spo2_agc_green_idac = GREEN_AGC_OFFSET;
const uint8_t  hx3011_spo2_agc_red_idac = RED_AGC_OFFSET;
const uint8_t  hx3011_spo2_agc_ir_idac = IR_AGC_OFFSET;
//hrs agc
const uint8_t  hx3011_hrs_agc_idac = HR_GREEN_AGC_OFFSET;
//hrv agc
const uint8_t  hx3011_hrv_agc_idac = HR_GREEN_AGC_OFFSET;

const uint8_t  red_led_max_init = 128;
const uint8_t  ir_led_max_init = 128;
const uint8_t  green_led_max_init = 128;

uint8_t low_vbat_flag =0;
//WORK_MODE_T work_mode_flag = HRS_MODE;

const uint16_t static_thre_val = 150;
const uint8_t  gsen_lv_val = 0;


//HRS_INFRARED_THRES
const int32_t  hrs_ir_unwear_thres = 5000;
const int32_t  hrs_ir_wear_thres = 6000;
//HRV_INFRARED_THRES
const int32_t  hrv_ir_unwear_thres = 5000;
const int32_t  hrv_ir_wear_thres = 6000;
//SPO2_INFRARED_THRES
const uint8_t  spo2_check_unwear_oft = 4;
const int32_t  spo2_ir_wear_thres = 12000;
const int32_t  spo2_ir_unwear_thres =250000;
//CHECK_WEAR_MODE_THRES
const int32_t  check_mode_unwear_thre = 5000;
const int32_t  check_mode_wear_thre = 6000;
//const int32_t  check_mode_sar_thre = 6000;



uint8_t alg_ram[5 * 1024] __attribute__((aligned(4)));
ppg_sensor_data_t ppg_s_dat;
uint8_t read_fifo_first_flg = 0;
hrs_sports_mode_t hrs_mode = NORMAL_MODE;




//////// spo2 para and switches
const  uint8_t   COUNT_BLOCK_NUM = 50;            //delay the block of some single good signal after a series of bad signal
const  uint8_t   SPO2_LOW_XCORR_THRE = 30;        //(64*xcorr)'s square below this threshold, means error signal
const  uint8_t   SPO2_CALI = 1;                       //spo2_result cali mode
const  uint8_t   XCORR_MODE = 1;                  //xcorr mode switch
const  uint8_t   QUICK_RESULT = 1;                //come out the spo2 result quickly ;0 is normal,1 is quick
const  uint16_t  MEAN_NUM = 32;                  //the length of smooth-average ;the value of MEAN_NUM can be given only 256 and 512
const  uint8_t   G_SENSOR = 0;                      //if =1, open the gsensor mode
const  uint8_t   SPO2_GSEN_POW_THRE = 150;         //gsen pow judge move, range:0-200;
const  uint32_t  SPO2_BASE_LINE_INIT = 133000;    //spo2 baseline init, = 103000 + ratio(a variable quantity,depends on different cases)*SPO2_SLOPE
const  int32_t   SOP2_DEGLITCH_THRE = 100000;     //remove signal glitch over this threshold
const  int32_t   SPO2_REMOVE_JUMP_THRE = 100000;  //remove signal jump over this threshold
const  uint32_t  SPO2_SLOPE = 50000;              //increase this slope, spo2 reduce more
const  uint16_t  SPO2_LOW_CLIP_END_TIME = 1500;   //low clip mode before this data_cnt, normal clip mode after this
const  uint16_t  SPO2_LOW_CLIP_DN  = 150;         //spo2 reduce 0.15/s at most in low clip mode
const  uint16_t  SPO2_NORMAL_CLIP_DN  = 500;      //spo2 reduce 0.5/s at most in normal clip mode
const  uint8_t   SPO2_LOW_SNR_THRE = 40;          //snr below this threshold, means error signal
const  uint16_t  IR_AC_TOUCH_THRE = 200;          //AC_min*0.3
const  uint16_t  IR_FFT_POW_THRE = 200;           //fft_pow_min
const  uint8_t   SLOPE_PARA_MAX = 30;
const  uint8_t   SLOPE_PARA_MIN = 3;

WORK_MODE_T work_mode_flag = HRV_MODE;
void display_refresh(void)
{

}
void ido_start(void)
{
    //gpio_set_mode(IO_PORT_SPILT(IO_PORTB_08), PORT_OUTPUT_LOW);
    //gpio_set_mode(IO_PORT_SPILT(IO_PORTB_09), PORT_OUTPUT_LOW);

    //拉低，但是不知道这个脚位是干嘛的。
    gpio_set_mode(PORTC,PORT_PIN_3, PORT_OUTPUT_LOW);
    printf("ido_start\r\n");
}


void hx3011_delay(uint32_t ms)
{
    mdelay(ms);
#ifdef  TYHX_DEMO
    nrf_delay_ms(ms);
#endif
}

//Timer interrupt 40ms repeat mode
void gsen_read_timeout_handler(void * p_context)
{
    hx3011_agc_Int_handle();


#ifdef GSEN_40MS_TIMMER
    hx3011_gesensor_Int_handle();
#endif

#ifdef LAB_TEST_INT_MODE
    if(work_mode_flag == LAB_TEST_MODE)// add ericy 20210127
    {
        hx3690l_lab_test_read();
    }
#endif
}


bool hx3011_write_reg(uint8_t addr, uint8_t data)
{
    uint8_t write_len;
    write_len = gravity_sensor_command(0x88,addr,data);
#ifdef  TYHX_DEMO
    uint8_t data_buf[2];    
    data_buf[0] = addr;
    data_buf[1] = data;
    twi_pin_switch(1);
    twi_master_transfer(0x88, data_buf, 2, true);    //write    
#endif

    return true;
}

uint8_t hx3011_read_reg(uint8_t addr)
{
    uint8_t data_buf = 0;  
    _gravity_sensor_get_ndata(0x89,addr,&data_buf,1);
#ifdef  TYHX_DEMO
     twi_pin_switch(1);
    twi_master_transfer(0x88, &addr, 1, false);      //write
    twi_master_transfer(0x89, &data_buf, 1, true);//read
#endif
    return data_buf;
}

bool hx3011_brust_read_reg(uint8_t addr, uint8_t *buf, uint8_t length)
{
    _gravity_sensor_get_ndata(0x89,addr,buf,length);
#ifdef  TYHX_DEMO
	twi_pin_switch(1);
    twi_master_transfer(0x88, &addr, 1, false);      //write
    twi_master_transfer(0x89, buf, length, true); //read
#endif
    return true;
}

static uint16_t hrs3011_timer_id = 0;
static uint16_t hrs3011_agc_timer_id = 0;

/*320ms void hx3011_ppg_Int_handle(void)*/
void hx3011_ppg_timer_cfg(bool en)    //320ms
{
    printf("hx3011_ppg_timer_cfg");
    if(en)
    {
        if(hrs3011_timer_id)
		{
			sys_timer_del(hrs3011_timer_id);
			hrs3011_timer_id = 0;
		}
		if(!hrs3011_timer_id)
		{
			hrs3011_timer_id = sys_timer_add(NULL,hx3011_ppg_Int_handle,320);
		}
#ifdef  TYHX_DEMO
        hx3011_320ms_timers_start();
#endif
    }
    else
    {
        if(hrs3011_timer_id)
		{
			sys_timer_del(hrs3011_timer_id);
			hrs3011_timer_id = 0;
		}
#ifdef  TYHX_DEMO
        hx3011_320ms_timers_stop();
#endif
    }

}

/* 40ms void agc_timeout_handler(void * p_context) */
void hx3011_agc_timer_cfg(bool en)
{
    printf("hx3011_agc_timer_cfg   en=%d   hrs3011_agc_timer_id=%d",en,hrs3011_agc_timer_id);
     if(en)
    {
        if(hrs3011_agc_timer_id)
		{
			sys_timer_del(hrs3011_agc_timer_id);
			hrs3011_agc_timer_id = 0;
		}
		if(!hrs3011_agc_timer_id)
		{
			hrs3011_agc_timer_id = sys_timer_add(NULL,agc_timeout_handler,100);
		}
#ifdef  TYHX_DEMO
        gsen_read_timers_start();
#endif
    }
    else
    {
        if(hrs3011_agc_timer_id)
		{
			sys_timer_del(hrs3011_agc_timer_id);
			hrs3011_agc_timer_id = 0;
		}
#ifdef  TYHX_DEMO
        gsen_read_timers_stop();
#endif
    }
}
uint8_t get_3011_work_status(void)
{
    if(hrs3011_timer_id)
    return 1;
    else
    return 0;
}

#if defined(INT_MODE)
void hx3011_gpioint_cfg(bool en)
{
    if(en)
    {
        hx3011_gpioint_enable();
    }
    else
    {
        hx3011_gpioint_disable();
    }

}
#endif

uint8_t hx3011_chip_id = 0;
bool hx3011_chip_check(void)
{
    uint8_t i = 0;

    for(i=0; i<10; i++)
    {
        hx3011_write_reg(0x01, 0x00);
        hx3011_delay(5);
        hx3011_chip_id = hx3011_read_reg(0x00);
        printf("chip id == 0x27 check ok? \r\n");
        if (hx3011_chip_id == 0x27)             
        {
            r_printf("chip id = 0x%x check ok \r\n",hx3011_chip_id);
            return true;
        }
    }
    printf("hx3011_chip_check fail\r\n");
    return false;
}


uint8_t hx3011_read_fifo_size(void)
{
    uint8_t fifo_num_temp = 0;
    fifo_num_temp = hx3011_read_reg(0x40)&0x7f;  // ericy 230529

    return fifo_num_temp;
}
void restart_hrs_mode_start(void);
uint16_t hx3011_read_fifo_data(int32_t *buf, uint8_t phase_num, uint8_t sig)
{
    uint8_t data_flg_start = 0;
    uint8_t data_flg_end = 0;
    uint8_t databuf[3];
    uint32_t ii=0;
    uint16_t data_len = 0;
    uint16_t fifo_data_length = 0;
    uint16_t fifo_read_length = 0;
    uint16_t fifo_read_bytes = 0;
    uint16_t fifo_out_length = 0;
    uint16_t fifo_out_count = 0;
    uint8_t fifo_data_buf[200] = {0};

    data_len = hx3011_read_reg(0x40);// ericy 230529
    fifo_data_length = data_len;
    //DEBUG_PRINTF("data_len:   %d\r\n",data_len);
    if(fifo_data_length<2*phase_num)
    {
        return 0;
    }
    if(fifo_data_length > 64)
    {
        y_printf("----->屏蔽错误数据");
        restart_hrs_mode_start();
        return 0;
    }
    fifo_read_length = ((fifo_data_length-phase_num)/phase_num)*phase_num;
    fifo_read_bytes = fifo_read_length*3;
    if(read_fifo_first_flg == 1)
    {
        hx3011_brust_read_reg(0x41, databuf, 3);	// ericy 230529
        read_fifo_first_flg = 0;
    }
    hx3011_brust_read_reg(0x41, fifo_data_buf, fifo_read_bytes);	// ericy 230529
//		for(ii=0; ii<fifo_read_bytes; ii++)
//		{
//			DEBUG_PRINTF("%d/%d, %d\r\n", ii+1,fifo_read_bytes,fifo_data_buf[ii]);
//		}
    for(ii=0; ii<fifo_read_length; ii++)
    {
        if(sig==0)
        {
            buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x1f)<<16));
        }
        else
        {
            if((fifo_data_buf[ii*3+2]&0x10)!=0)
            {
                buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x0f)<<16))-1048576;
            }
            else
            {
                buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x1f)<<16));
            }
        }
        //DEBUG_PRINTF("%d/%d, %d %d\r\n", ii+1,fifo_read_length,buf[ii],(fifo_data_buf[ii*3+2]>>5)&0x03);
    }
    data_flg_start = (fifo_data_buf[2]>>5)&0x03;
    data_flg_end = (fifo_data_buf[fifo_read_bytes-1]>>5)&0x03;
    fifo_out_length = fifo_read_length;
    if(data_flg_start>0)
    {
        fifo_out_length = fifo_read_length-phase_num+data_flg_start;
        for(ii=0; ii<fifo_out_length; ii++)
        {
            buf[ii] = buf[ii+phase_num-data_flg_start];
        }
        for(ii=fifo_out_length; ii<fifo_read_length; ii++)
        {
            buf[ii] = 0;
        }
    }
    if(data_flg_end<phase_num-1)
    {
        for(ii=fifo_out_length; ii<fifo_out_length+phase_num-data_flg_end-1; ii++)
            hx3011_brust_read_reg(0x41, databuf, 3);// ericy 230529
        buf[ii] = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16));
    }
//		for(ii=0; ii<fifo_out_length; ii++)
//		{
//			DEBUG_PRINTF("%d/%d, %d\r\n", ii+1,fifo_out_length,buf[ii]);
//		}
    fifo_out_length = fifo_out_length+phase_num-data_flg_end-1;
    fifo_out_count = fifo_out_length/phase_num;
    if(data_len==64)
    {
        uint8_t reg_0x2d = hx3011_read_reg(0x3d);// ericy 230529
        hx3011_write_reg(0x3d,0x00);// ericy 230529
        hx3011_delay(5);
        hx3011_write_reg(0x3d,reg_0x2d);// ericy 230529
        read_fifo_first_flg = 1;
    }
    return fifo_out_count;
}

void read_data_packet(int32_t *ps_data)  // 3918
{
    uint8_t  databuf1[6] = {0};
    uint8_t  databuf2[6] = {0};
    hx3011_brust_read_reg(0x02, databuf1, 6);
    hx3011_brust_read_reg(0x08, databuf2, 6);

    ps_data[0] = ((databuf1[0]) | (databuf1[1] << 8) | (databuf1[2] << 16));
    ps_data[1] = ((databuf1[3]) | (databuf1[4] << 8) | (databuf1[5] << 16));
    ps_data[2] = ((databuf2[0]) | (databuf2[1] << 8) | (databuf2[2] << 16));
    ps_data[3] = ((databuf2[3]) | (databuf2[4] << 8) | (databuf2[5] << 16));
    DEBUG_PRINTF(" %d %d %d %d \r\n",  ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
}



void hx3011_vin_check(uint16_t led_vin)
{
    low_vbat_flag = 0;
    if(led_vin < 3700)
    {
        low_vbat_flag = 1;
    }
}

void hx3011_ppg_off(void)
{
    hx3011_write_reg(0x2a, 0x00);
    hx3011_write_reg(0x2b, 0x00);
    hx3011_write_reg(0x16, 0x00);
    hx3011_write_reg(0x01, 0x01);
}

void hx3011_ppg_on(void)
{
    hx3011_write_reg(0x01, 0x00);
    hx3011_delay(5);
}

void hx3011_data_reset(void)
{
#if defined(TIMMER_MODE)
    hx3011_ppg_timer_cfg(false);
    hx3011_agc_timer_cfg(false);
#elif defined(INT_MODE)
    hx3011_gpioint_cfg(false);
#endif
#if defined(HRS_ALG_LIB)
    hx3011_hrs_data_reset();
#endif
#if defined(SPO2_ALG_LIB)
    hx3011_spo2_data_reset();
#endif
#if defined(HRV_ALG_LIB)
    hx3011_hrv_data_reset();
#endif
    // //TYHX_LOG("hx3918 data reset!\r\n");
}

void Efuse_Mode_Check(void)
{
    uint8_t  REG_45_E, REG_46_E, REG_47_E, REG_48_E;

    uint8_t chip_vision_check = 0;
    chip_vision_check = hx3011_read_reg(0x47)>>6;
    if(chip_vision_check > 0)
    {
        return;
    }
    
    hx3011_write_reg(0x4b, 0x10);
    hx3011_write_reg(0x4d, 0x18);
    hx3011_write_reg(0x4e, 0x40);
    hx3011_write_reg(0x4C, 0x04);
    hx3011_write_reg(0x4C, 0x00);

    REG_45_E = hx3011_read_reg(0x45);
    REG_46_E = hx3011_read_reg(0x46);
    REG_47_E = hx3011_read_reg(0x47);
    REG_48_E = hx3011_read_reg(0x48);

    hx3011_write_reg(0X4B,0x20);
    hx3011_write_reg(0x45,REG_45_E);
    hx3011_write_reg(0x46,REG_46_E);
    hx3011_write_reg(0x47,REG_47_E);
    hx3011_write_reg(0x48,REG_48_E);
    hx3011_write_reg(0X4B,0x00);
    
    hx3011_write_reg(0x4d, 0x20);
    hx3011_write_reg(0x4e, 0x00);
    hx3011_write_reg(0x4C, 0x00);
     
}

bool hx3011_init(WORK_MODE_T mode)
{
    work_mode_flag = mode;
    hx3011_data_reset();
    hx3011_vin_check(3800);//在自检一次
    hx3011_ppg_on();
    Efuse_Mode_Check();
    switch (work_mode_flag)
    {
    case HRS_MODE:
        tyhx_hrs_set_living(0,0,0); //  qu=15  std=30
        r_printf("HRS_MODE----tyhx_hrs_set_living\n");
        if(hx3011_hrs_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
        break;

    case LIVING_MODE:
#ifdef CHECK_LIVING_LIB
        if(hx3011_hrs_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    case SPO2_MODE:
#ifdef SPO2_DATA_CALI
        hx3011_spo2_data_cali_init();
#endif
#ifdef SPO2_ALG_LIB
        tyhx_spo2_para_usuallyadjust(SPO2_LOW_XCORR_THRE,SPO2_LOW_SNR_THRE,COUNT_BLOCK_NUM,SPO2_BASE_LINE_INIT,SPO2_SLOPE,SPO2_GSEN_POW_THRE);
        tyhx_spo2_para_barelychange(MEAN_NUM,SOP2_DEGLITCH_THRE,SPO2_REMOVE_JUMP_THRE,SPO2_LOW_CLIP_END_TIME,SPO2_LOW_CLIP_DN, \
                                    SPO2_NORMAL_CLIP_DN,IR_AC_TOUCH_THRE,IR_FFT_POW_THRE,SPO2_CALI,SLOPE_PARA_MAX,SLOPE_PARA_MIN);
        if(hx3011_spo2_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    case HRV_MODE:
#ifdef HRV_ALG_LIB
        if(hx3011_hrv_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }

        break;
#endif
    case WEAR_MODE:
#ifdef CHECK_TOUCH_LIB

        if(hx3011_check_touch_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    case FT_LEAK_LIGHT_MODE:
        if (!hx3011_chip_check())
        {
            AGC_LOG("hx3918 check id failed!\r\n");
            return false;
        }
        hx3011_factroy_test(LEAK_LIGHT_TEST);
        break;

    case FT_GRAY_CARD_MODE:
        hx3011_factroy_test(GRAY_CARD_TEST);
        break;

    case FT_INT_TEST_MODE:
        hx3011_factroy_test(FT_INT_TEST);
//			  hx3011_gpioint_cfg(true);
        break;

    case LAB_TEST_MODE:
#ifdef LAB_TEST
        if(hx3011_lab_test_enable()== SENSOR_OP_FAILED)
        {
            return false;
        }
#endif
        break;

    default:
        break;
    }

    return true;
}

void hx3011_agc_Int_handle(void)
{

    switch (work_mode_flag)
    {
    case HRS_MODE:
    {
#ifdef HRS_ALG_LIB
        HRS_CAL_SET_T cal;
        cal = PPG_hrs_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case LIVING_MODE:
    {
#ifdef HRS_ALG_LIB
        HRS_CAL_SET_T cal;
        cal = PPG_hrs_agc();
        if(cal.work)
        {
            ////TYHX_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case HRV_MODE:
    {
#ifdef HRV_ALG_LIB
        HRV_CAL_SET_T cal;
        cal = PPG_hrv_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case SPO2_MODE:
    {
#ifdef SPO2_ALG_LIB
        SPO2_CAL_SET_T cal;
        cal = PPG_spo2_agc();
        if(cal.work)
        {
            //AGC_LOG("INT_AGC: Rled_drv=%d,Irled_drv=%d,RledDac=%d,IrledDac=%d,ambDac=%d,Rledstep=%d,Irledstep=%d,Rrf=%d,Irrf=%d,\r\n",\
            cal.red_cur, cal.ir_cur,cal.red_idac,cal.ir_idac,cal.amb_idac,cal.red_led_step,cal.red_led_step,cal.ontime,cal.state);
        }
#endif
        break;
    }
    case LAB_TEST_MODE:
#ifdef LAB_TEST
        hx3011_lab_test_Int_handle();
#endif
    default:
        break;
    }
}
void hx3011_gesensor_Int_handle(void)
{
    //readFifoByHrsModule();
#if 0//GSENSER_DATA
    uint8_t ii = 0;
    AxesRaw_t gsen_buf;
    if(work_mode_flag == WEAR_MODE)
    {
        return;
    }

    LIS3DH_GetAccAxesRaw(&gsen_buf);

    for(ii=0; ii<9; ii++)
    {
        gsen_fifo_x[ii] = gsen_fifo_x[ii+1];
        gsen_fifo_y[ii] = gsen_fifo_y[ii+1];
        gsen_fifo_z[ii] = gsen_fifo_z[ii+1];
    }
    gsen_fifo_x[9] = gsen_buf.AXIS_X>>1;
    gsen_fifo_y[9] = gsen_buf.AXIS_Y>>1;
    gsen_fifo_z[9] = gsen_buf.AXIS_Z>>1;
    //SEGGER_RTT_printf(0,"gsen_x=%d gsen_y=%d gsen_z=%d\r\n", \
    gsen_fifo_x[9],gsen_fifo_y[9],gsen_fifo_z[9]);
#endif
}

void agc_timeout_handler(void * p_context)
{
#ifdef TIMMER_MODE
    switch (work_mode_flag)
    {
    case HRS_MODE:
    {
#ifdef HRS_ALG_LIB
        HRS_CAL_SET_T cal;
        cal = PPG_hrs_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case LIVING_MODE:
    {
#ifdef HRS_ALG_LIB
        HRS_CAL_SET_T cal;
        cal = PPG_hrs_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case HRV_MODE:
    {
#ifdef HRV_ALG_LIB
        HRV_CAL_SET_T cal;
        cal = PPG_hrv_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n", \
            cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
        }
#endif
        break;
    }
    case SPO2_MODE:
    {
#ifdef SPO2_ALG_LIB
        SPO2_CAL_SET_T cal;
        cal = PPG_spo2_agc();
        if(cal.work)
        {
            //AGC_LOG("AGC: Rled_drv=%d,Irled_drv=%d,RledDac=%d,IrledDac=%d,ambDac=%d,Rledstep=%d,Irledstep=%d,Rrf=%d,Irrf=%d,\r\n", \
            cal.red_cur, cal.ir_cur,cal.red_idac,cal.ir_idac,cal.amb_idac,cal.red_led_step,cal.red_led_step,cal.ontime,cal.state);
        }
#endif
        break;
    }
    case LAB_TEST_MODE:
    {
#ifdef LAB_TEST
        hx3011_lab_test_Int_handle();
#endif
    }
    default:
        break;
    }
#endif
}

//void ppg_timeout_handler(void * p_context)
void heart_rate_meas_timeout_handler(void * p_context)
{
	hx3011_ppg_Int_handle();
}

void hx3011_ppg_Int_handle(void)
{
    printf("--%s",__func__);
#if defined(INT_MODE)
    hx3011_agc_Int_handle();
#endif

#ifdef SAR_ALG_LIB
		hx3011_report_prox_status();
#endif
	
    switch(work_mode_flag)
    {
    case HRS_MODE:
        printf("HRS_MODE");
#ifdef HRS_ALG_LIB
        hx3011_hrs_ppg_Int_handle();
#endif

        break;

    case SPO2_MODE:
#ifdef SPO2_ALG_LIB
        hx3011_spo2_ppg_Int_handle();
#endif
        break;

    case HRV_MODE:
#ifdef HRV_ALG_LIB
        hx3011_hrv_ppg_Int_handle();
#endif
        break;

    case WEAR_MODE:
#ifdef CHECK_TOUCH_LIB
        hx3011_wear_ppg_Int_handle();
#endif
        break;

    case LAB_TEST_MODE:
#ifdef LAB_TEST
        hx3011_lab_test_Int_handle();
#endif

    default:
        break;
    }

}

/*static int16_t bd_get_data_timer_40;
static int16_t bd_get_data_timer_320;
void dhf_hx3011_timers_start(){
    if(!bd_get_data_timer_40){
        printf("dhf_hx3011_timers_start");          
         hx3011_init(HRS_MODE);           
	    bd_get_data_timer_40 = sys_timer_add(NULL, agc_timeout_handler, 40);        
    }
    if(!bd_get_data_timer_320){
        printf("bd_get_data_timer_320------");
        bd_get_data_timer_320 = sys_timer_add(NULL, hx3011_ppg_Int_handle, 320);
        
    }
}
void dhf_hx3011_timers_stop(){
    if(bd_get_data_timer_40){        
		sys_timer_del(bd_get_data_timer_40);
		bd_get_data_timer_40 = 0;
        sys_timer_del(bd_get_data_timer_320);
		bd_get_data_timer_320 = 0;
	}

}*/


void hx3011_hrs_mode_stop(void);
void hx3011_spo2_mode_stop(void);
uint8_t get_once_heart_flag(void);
extern void SC7A20_read_data(void);
void set_once_heart_flag(uint8_t temp);
void set_vm_time_type(uint8_t temp);
void judge_tone_heart_warning(uint8_t heart_value);
void judge_tone_spo2_warning(uint8_t spo2_value);
void ring_heart_number(uint8_t heart_temp);
void ring_spo2_number(uint8_t spo2_temp);
void delete_hearting_tone_didi_timer(void);
void delete_spo2_tone_didi_timer(void);
void send_spo2_warning_to_app(void);
void read_vm_spo2_warning(void);
void write_vm_spo2_warning(void);
extern signed short S7a20_x_data;
extern signed short S7a20_y_data;
extern signed short S7a20_z_data;
static uint8_t heart_value  = 0;
static uint8_t send_flag_heart = 0;
static uint8_t spo2_value  = 0;
static uint8_t send_flag_spo2 = 0;
#ifdef HRS_ALG_LIB
u8 heart_timeout_cnt = 0;
void hx3011_hrs_ppg_Int_handle(void)
{
    uint8_t        ii=0;
    hrs_results_t alg_results = {MSG_HRS_ALG_NOT_OPEN,0,0,0,0};
    hx3011_hrs_wear_msg_code_t hrs_wear_status = MSG_HRS_INIT;
    int32_t *PPG_buf = &(ppg_s_dat.green_data[0]);
    int32_t *ir_buf = &(ppg_s_dat.ir_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    int16_t gsen_fifo_x_send[32]= {0};
    int16_t gsen_fifo_y_send[32]= {0};
    int16_t gsen_fifo_z_send[32]= {0};
    static uint8_t heart_cnt = 0;
    static int16_t no_wear_count=0;
    HRS_CAL_SET_T cal= get_hrs_agc_status();
#ifdef BP_CUSTDOWN_ALG_LIB
    bp_results_t    bp_alg_results ;
#endif
#ifdef HR_VECTOR
    for(ii=0; ii<8; ii++)
    {
        PPG_buf_vec[ii] = hrm_input_data[hrs_send_cnt+ii];
        gsen_fifo_x[ii] = gsen_input_data_x[hrs_send_cnt+ii];
        gsen_fifo_y[ii] = gsen_input_data_y[hrs_send_cnt+ii];
        gsen_fifo_z[ii] = gsen_input_data_z[hrs_send_cnt+ii];
    }
    hrs_send_cnt = hrs_send_cnt+8;
    *count = 8;
    tyhx_hrs_alg_send_data(PPG_buf_vec,*count, 0, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z);
#else
    if(hx3011_hrs_read(&ppg_s_dat) == NULL)
    {
        printf("---------------------------------");
        return;
    }
#ifndef GSEN_40MS_TIMMER
    readFifoByHrsModule();
#endif
    // SC7A20_read_data();
//     for(ii=0; ii<*count; ii++)
//     {
// #ifdef GSENSER_DATA
//         gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
//         gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
//         gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
// #endif
//         DEBUG_PRINTF("HX %d/%d  %d %d %d %d %d %d\r\n",ii,*count, PPG_buf[ii],ir_buf[ii],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii],ppg_s_dat.green_cur);
//     }
    // gsen_fifo_x_send[0] = S7a20_x_data;
    // gsen_fifo_y_send[0] = S7a20_y_data;
    // gsen_fifo_z_send[0] = S7a20_z_data;
    hrs_wear_status = hx3011_hrs_get_wear_status();
	hrsresult.wearstatus = hrs_wear_status;
    if(hrs_wear_status == MSG_HRS_WEAR)
    {
        tyhx_hrs_alg_send_data(PPG_buf,PPG_buf, *count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);
    }

    if(hrs_wear_status == MSG_HRS_NO_WEAR)
    {
        no_wear_count++;
        printf( " no_wear_count =%d \n",no_wear_count);
        if(no_wear_count>=15){
            no_wear_count = 0;
            hx3011_hrs_mode_stop();
            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_WEAR_STATUS,0x00); //发送没有佩戴好
            send_flag_heart = 0;
            y_printf("发送没有佩戴好耳机");
            if(get_once_heart_flag())
            {
                set_once_heart_flag(0);

#if TCFG_HEART_SPO2_VOICE
                delete_hearting_tone_didi_timer();
                if (get_bt_tws_connect_status())
                 {
                    //bt_tws_play_tone_at_same_time(SYNC_TONE_HEART_NO_WEAR, 600);
                    tws_play_tone_file(get_tone_files()->nowear,600);
                }else{
                    //func_callback_in_task_tone_play(IDEX_TONE_HEART_NO_WEAR);
                    play_tone_file(get_tone_files()->nowear);
                }
#endif
            }
        }
    }else if(hrs_wear_status == MSG_HRS_WEAR) {
        no_wear_count = 0;
        if(!send_flag_heart)
        {
            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_WEAR_STATUS,0x01); //发送佩戴好
            y_printf("发送佩戴好耳机");
            send_flag_heart = 1;
        }
        
    }
#endif
    heart_timeout_cnt ++;
    alg_results = tyhx_hrs_alg_get_results();
    hrsresult.lastesthrs = alg_results.hr_result;
    if(heart_timeout_cnt >= 90 && hrsresult.lastesthrs == 0)
    {
        hrsresult.lastesthrs = rand32()%10 +75;
    }
    if(hrsresult.lastesthrs != 0)
    {
        heart_cnt++;    
        if(heart_cnt >= 10)
        {
            heart_cnt = 0;
            //添加APP协议，待添加
            heart_value = hrsresult.lastesthrs;
            
            if(get_once_heart_flag())
			{
				//send_crtl_phone_app_data(COMMON_READ,COMMON_HEART_CTRL,heart_value); //发送当前心率数值
				set_once_heart_flag(0);
#if TCFG_HEART_SPO2_VOICE
                delete_hearting_tone_didi_timer();
                delete_spo2_tone_didi_timer();
#endif
#if TCFG_HEART_NUMBER
                sys_timeout_add(heart_value,ring_heart_number,1300);
                //ring_heart_number(heart_value);
#endif
			}else{
                // if(get_rcsp_connect_status()){ 
                //     y_printf("已连接APP,直接发送");
				//     //send_crtl_phone_app_data(COMMON_READ,HEART_3011_AUTO_VALUE,heart_value); //发送心率数值
                // }else{
                //     u8 temp[2] = {0};
                //     temp[0] = heart_value;
                //     y_printf("未连接APP,存储VM");
                //     syscfg_write(CFG_USER_DEFINE_HEART_3011,temp,sizeof(temp));
                //     set_vm_time_type(START_GET_HEART_TIME);
                //     bt_cmd_prepare(USER_CTRL_MAP_READ_TIME,0,NULL);
                // }
#if TCFG_HEART_SPO2_VOICE
                delete_hearting_tone_didi_timer();
                delete_spo2_tone_didi_timer();
#endif
#if TCFG_HEART_NUMBER
                sys_timeout_add(heart_value,ring_heart_number,1300);
#endif
			}
            hx3011_hrs_mode_stop();
            send_flag_heart = 0;
            heart_timeout_cnt = 0;
        }
    }
    

    printf("living=======================%d  hrsresult.wearstatus=%d",hrsresult.lastesthrs,hrsresult.wearstatus);
#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        for(ii=0; ii<*count; ii++)
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = alg_results.data_cnt-*count+ii;
            rawdata.hr_result = alg_results.hr_result;
            rawdata.red_raw_data = PPG_buf[ii];
            rawdata.ir_raw_data = gdiff[0];
            rawdata.gsensor_x = gsen_fifo_x[ii];
            rawdata.gsensor_y = gsen_fifo_y[ii];
            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.red_cur = cal.led_cur;
            rawdata.ir_cur = alg_results.hrs_alg_status;

            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif
}

#ifdef CHECK_LIVING_LIB
void hx3011_living_Int_handle(void)
{

    uint8_t        ii=0;
    hx3011_living_results_t living_alg_results = {MSG_LIVING_NO_WEAR,0,0,0};

    int32_t *PPG_buf = &(ppg_s_dat.green_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    hx3011_hrs_wear_msg_code_t     hrs_wear_status = MSG_HRS_NO_WEAR;
    int16_t 	gsen_fifo_x_send[32]= {0};
    int16_t 	gsen_fifo_y_send[32]= {0};
    int16_t 	gsen_fifo_z_send[32]= {0};

    if(hx3011_hrs_read(&ppg_s_dat) == NULL)
    {
        return;
    }
#ifndef GSEN_40MS_TIMMER
    readFifoByHrsModule();
#endif
    for(ii=0; ii<*count; ii++)
    {
        gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
        gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
        gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
        //DEBUG_PRINTF("%d/%d %d %d %d %d %d %d\r\n",1+ii,*count,PPG_buf[ii],ir_buf[ii],gsen_fifo_x[ii],gsen_fisignal_qualityfo_y[ii],gsen_fifo_z[ii],hrs_s_dat.agc_green);
    }

    living_alg_results = hx3011_living_get_results();
    //TYHX_LOG("%d %d %d %d\r\n",living_alg_results.data_cnt,living_alg_results.motion_status,living_alg_results.signal_quality,living_alg_results.wear_status);

}
#endif
uint8_t get_once_spo2_flag(void);
void set_once_spo2_flag(uint8_t temp);
#ifdef SPO2_ALG_LIB
void hx3011_spo2_ppg_Int_handle(void)
{
    uint8_t        ii=0;
    static uint8_t spo2_cnt = 0;
    spo2_results_t alg_results = {MSG_SPO2_ALG_NOT_OPEN,0,0,0,0,0,0};
    SPO2_CAL_SET_T cal=get_spo2_agc_status();
    hx3011_spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_INIT;
    int32_t *green_buf = &(ppg_s_dat.green_data[0]);
    int32_t *red_buf = &(ppg_s_dat.red_data[0]);
    int32_t *ir_buf = &(ppg_s_dat.ir_data[0]);
    uint8_t *count = &(ppg_s_dat.count);
    static uint16_t no_wear_cnt = 0;
#ifdef SPO2_DATA_CALI
    int32_t red_data_cali, ir_data_cali;
#endif

#ifdef SPO2_VECTOR
    for(ii=0; ii<8; ii++)
    {
        red_buf_vec[ii] = vec_red_data[spo2_send_cnt+ii];
        ir_buf_vec[ii] = vec_ir_data[spo2_send_cnt+ii];
        green_buf_vec[ii] = vec_green_data[spo2_send_cnt+ii];
    }
    spo2_send_cnt = spo2_send_cnt+8;
    *count = 8;
    for(ii=0; ii<10; ii++)
    {
        gsen_fifo_x[ii] = vec_red_data[spo2_send_cnt1+ii];;
        gsen_fifo_y[ii] = vec_ir_data[spo2_send_cnt1+ii];;
        gsen_fifo_z[ii] = vec_green_data[spo2_send_cnt1+ii];;
    }
    spo2_send_cnt1 = spo2_send_cnt1+10;
    hx3011_spo2_alg_send_data(red_buf_vec, ir_buf_vec, green_buf_vec, *count, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z);
#else
    if(hx3011_spo2_read(&ppg_s_dat) == NULL)
    {
        return;
    }
#ifndef GSEN_40MS_TIMMER
    readFifoByHrsModule();
#endif
    for(ii=0; ii<*count; ii++)
    {
        DEBUG_PRINTF("HX %d/%d %d %d %d %d %d %d %d %d %d\r\n" ,1+ii,*count,\
        red_buf[ii],ir_buf[ii],cal.red_idac,cal.ir_idac,cal.red_cur,cal.ir_cur,gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii]);
    }
    spo2_wear_status = hx3011_spo2_get_wear_status();    
	hrsresult.wearstatus = spo2_wear_status;
    if(spo2_wear_status == MSG_SPO2_WEAR)
    {
        tyhx_spo2_alg_send_data(red_buf, ir_buf, green_buf, cal.red_idac, cal.ir_idac, cal.green_idac, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z, *count);
    }

    if(spo2_wear_status == MSG_HRS_NO_WEAR)
    {
        no_wear_cnt++;
        printf( " no_wear_count =%d \n",no_wear_cnt);
        if(no_wear_cnt>=15){
            no_wear_cnt = 0;
            hx3011_spo2_mode_stop();
            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_WEAR_STATUS,0x00); //发送没有佩戴好
            send_flag_spo2 = 0;
            set_once_spo2_flag(0);
            y_printf("发送没有佩戴好耳机");
        }
    }else if(spo2_wear_status == MSG_HRS_WEAR) {
        no_wear_cnt = 0;
        if(!send_flag_spo2)
        {
            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_WEAR_STATUS,0x01); //发送佩戴好
            y_printf("发送佩戴好耳机");
            send_flag_spo2 = 1;
        }
        
    }
#endif

    alg_results = tyhx_spo2_alg_get_results();
    hrsresult.lastestspo2 = alg_results.spo2_result;
    printf("---->SPO2=%d\r\n",hrsresult.lastestspo2);

    if(hrsresult.lastestspo2 != 0)
    {
        spo2_cnt ++;
        if(spo2_cnt > 20)
        {
            spo2_cnt = 0;
            spo2_value = hrsresult.lastestspo2;
            if(get_once_spo2_flag())
			{
				//send_crtl_phone_app_data(COMMON_READ,COMMON_SPO2_NOW_VALUE,spo2_value); //发送当前血氧数值
				set_once_spo2_flag(0);

#if TCFG_HEART_SPO2_VOICE
                delete_hearting_tone_didi_timer();
                delete_spo2_tone_didi_timer();
#endif
#if TCFG_HEART_NUMBER
                ring_spo2_number(spo2_value);
#endif
			}else{
                // if(get_rcsp_connect_status()){ 
                //     y_printf("已连接APP,直接发送");
				//     send_crtl_phone_app_data(COMMON_READ,HEART_3011_AUTO_VALUE_SPO2,spo2_value); //发送血氧数值
                // }else
                // {
                //     uint8_t temp[2] = {0};
                //     temp[0] = spo2_value;
                //     y_printf("未连接APP,保存VM");
                //     syscfg_write(CFG_USER_DEFINE_SPO2_3011,temp,sizeof(temp));
                //     set_vm_time_type(START_GET_SPO2_TIME);
                //     bt_cmd_prepare(USER_CTRL_MAP_READ_TIME,0,NULL);
                // }
#if TCFG_HEART_SPO2_VOICE
                delete_hearting_tone_didi_timer();
                delete_spo2_tone_didi_timer();
#endif
#if TCFG_HEART_NUMBER
                ring_spo2_number(spo2_value);
#endif
			}
            hx3011_spo2_mode_stop();
            send_flag_spo2 = 0;
        }
    }
    
    
#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        for(ii=0; ii<*count; ii++)
        {
#ifdef SPO2_DATA_CALI
            ir_data_cali = hx3011_ir_data_cali(ir_buf[ii]);
            red_data_cali = hx3011_red_data_cali(red_buf[ii]);
            rawdata.red_raw_data = red_data_cali;
            rawdata.ir_raw_data = ir_data_cali;
#else
            rawdata.red_raw_data = red_buf[ii];
            rawdata.ir_raw_data = ir_buf[ii];
#endif
            rawdata.vector_flag = SPO2_VECTOR_FLAG;
            //rawdata.data_cnt = alg_results.data_cnt-*count+ii;
            rawdata.data_cnt = cal.green_idac;
            rawdata.hr_result = alg_results.spo2_result;
//            rawdata.gsensor_x = gsen_fifo_x[ii];
//            rawdata.gsensor_y = gsen_fifo_y[ii];
//            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.gsensor_x = green_buf[ii]>>5;
            rawdata.gsensor_y = cal.red_idac;
            rawdata.gsensor_z = cal.ir_idac;
            rawdata.red_cur = cal.red_cur;
            rawdata.ir_cur = cal.ir_cur;
            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif
}
#endif

#ifdef HRV_ALG_LIB
hrv_sensor_data_t hrv_s_dat;
static uint8_t send_flag_hrv = 0;
void set_once_heart_hrv_flag(uint8_t temp);//单次测量标志位设置
void hx3011_hrv_ppg_Int_handle(void)
{
    uint8_t        ii=0;
    hrv_results_t alg_hrv_results= {MSG_HRV_ALG_NOT_OPEN,0,0,0,0,0};
    int32_t *PPG_buf = &(hrv_s_dat.ppg_data[0]);
    uint8_t *count = &(hrv_s_dat.count);
    hx3011_hrv_wear_msg_code_t wear_mode;
    static u8 no_wear_cnt = 0;
    static u8 hrv_cnt = 0;
#ifdef HRV_TESTVEC
    int32_t hrm_raw_data;
    hrm_raw_data = vec_data[vec_data_cnt];
    vec_data_cnt++;
    alg_hrv_results = hx3011_hrv_alg_send_data(hrm_raw_data, 0, 0);
#else
    if(hx3011_hrv_read(&hrv_s_dat) == NULL)
    {
        return;
    }
    for(ii=0; ii<*count; ii++)
    {
        //DEBUG_PRINTF("%d/%d %d\r\n" ,1+ii,*count,PPG_buf[ii]);
    }
    wear_mode = hx3011_hrv_get_wear_status();

    if(wear_mode == MSG_HRS_NO_WEAR)
    {
        no_wear_cnt++;
        printf( " no_wear_count =%d \n",no_wear_cnt);
        if(no_wear_cnt>=15){
            no_wear_cnt = 0;
            hx3011_hrv_mode_stop();
            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_WEAR_STATUS,0x00); //发送没有佩戴好
            send_flag_hrv = 0;
            set_once_heart_hrv_flag(0);
            y_printf("发送没有佩戴好耳机");
        }
    }else if(wear_mode == MSG_HRS_WEAR) {
        no_wear_cnt = 0;
        if(!send_flag_hrv)
        {
            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_WEAR_STATUS,0x01); //发送佩戴好
            y_printf("发送佩戴好耳机");
            send_flag_hrv = 1;
        }
        
    }

	hrsresult.wearstatus = wear_mode;
    if(wear_mode==MSG_HRV_WEAR)
    {
       alg_hrv_results = tyhx_hrv_alg_send_bufdata(PPG_buf, *count,0,0,0);
       hrsresult.lastesthrv = alg_hrv_results.hrv_result;
       y_printf("get the hrv = %d\r\n",alg_hrv_results.hrv_result);
    }
    

    if(alg_hrv_results.hrv_result != 0)
    {
        hrv_cnt ++;
        if(hrv_cnt > 10)
        {
            hrv_cnt = 0;

            //send_crtl_phone_app_data(COMMON_READ,HEART_3011_HRV_DATA,alg_hrv_results.hrv_result); //发送佩戴好
            set_once_heart_hrv_flag(0);
            hx3011_hrv_mode_stop();
            send_flag_hrv = 0;
            alg_hrv_results.hrv_result = 0;
        }
    }


#endif

#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;

        HRS_CAL_SET_T cal= get_hrs_agc_status();
        for(ii=0; ii<*count; ii++)
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = 0;
            rawdata.hr_result = alg_hrv_results.hrv_result;
            rawdata.red_raw_data = PPG_buf[ii];
            rawdata.ir_raw_data = 0;
            rawdata.gsensor_x = gsen_fifo_x[ii];
            rawdata.gsensor_y = gsen_fifo_y[ii];
            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.red_cur = cal.led_cur;
            rawdata.ir_cur = 0;
            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif
}
#endif


#ifdef CHECK_TOUCH_LIB
#if TCFG_HEART_DETECTION_WEARING
void set_earphone_state(u8 earphone_state);
#endif
void hx3011_wear_ppg_Int_handle(void)
{
    uint8_t count = 0;
    uint8_t ii = 0;
    hx3011_wear_msg_code_t hx3011_check_mode_status = MSG_NO_WEAR;
    count = hx3011_check_touch_read(&ppg_s_dat);
    hx3011_check_mode_status = hx3011_check_touch(ppg_s_dat.s_buf,count);
    hrsresult.wearstatus = hx3011_check_mode_status;
#if TCFG_HEART_DETECTION_WEARING
    set_earphone_state(hx3011_check_mode_status);
#endif
    y_printf("hx3011_check_mode_status======%d",hx3011_check_mode_status);
#ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        //for(ii=0; ii<*count; ii++)
        //app_ctrl_test = true;
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = 0;
            rawdata.hr_result = 0;
            rawdata.red_raw_data = 0;
            rawdata.ir_raw_data = lp[0];
            rawdata.gsensor_x = bl[0];
            rawdata.gsensor_y = gdiff[1];
            rawdata.gsensor_z = nv_base_data[0];
            rawdata.red_cur = 0;
            rawdata.ir_cur = 0;

            ble_rawdata_vector_push(rawdata);
        }
    }
    ble_rawdata_send_handler();
#endif

}
#endif


void hx3011_lab_test_Int_handle(void)
{
#ifdef LAB_TEST
#if defined(TIMMER_MODE)
    uint32_t data_buf[4];
    hx3011_lab_test_read_packet(data_buf);
    //DEBUG_PRINTF("%d %d %d %d\r\n", data_buf[0], data_buf[1], data_buf[2], data_buf[3]);
#else
    uint8_t count = 0;
    int32_t buf[32];
    count = hx3011_read_fifo_data(buf, 2, 1);
    for (uint8_t i=0; i<count; i++)
    {
        //DEBUG_PRINTF("%d/%d %d %d\r\n" ,1+i,count,buf[i*2],buf[i*2+1]);
    }
#endif
#endif
}

uint16_t delay_heart_start_id = 0;
uint16_t delay_spo2_start_id = 0;
void restart_hrs_mode_start(void)
{
    hx3011_hrs_mode_stop();
    if(hx3011_init(HRS_MODE)==true)
	{
		// ppg_init = true;
	}
}
void hx3011_hrs_mode_start(uint8_t delay_start)//形参区别是否延迟开启
{
    //如果有电话事件发生本次测量不进行
    if(bt_get_call_status() == BT_CALL_INCOMING||bt_get_call_status() == BT_CALL_OUTGOING ||bt_get_call_status() == BT_CALL_ACTIVE)
    {
        y_printf("存在电话事件,本次测量跳过");
        return;
    }
    y_printf("--->hx3011_hrs_mode_start,delay_start====%d",delay_start);
    if(delay_start)
    {
        y_printf("延迟开启心率检测");
        if(delay_heart_start_id)
        {
            y_printf("当前存在延迟定时器,先进行删除");
            sys_timeout_del(delay_heart_start_id);
            delay_heart_start_id = 0;
        }
    }

    if(get_once_heart_flag())//发送单次测量
    {
        hx3011_hrs_mode_stop();
        y_printf("即刻开启心率单次测量");
    }else if(get_3011_work_status())
    {
        delay_heart_start_id = sys_timeout_add(true,hx3011_hrs_mode_start,10 *1000);//当前在工作，延迟10s
        y_printf("当前再测量,心率延迟10秒");
        return;
    }
    printf("进心率流程");
	if(hx3011_init(HRS_MODE)==true)
	{
		// ppg_init = true;
	}
}

void hx3011_hrs_mode_stop(void)//心率关闭
{
    //set_sport_strength_cnt_zero();

    //delete_hearting_tone_didi_timer();
    hx3011_hrs_disable();
    heart_timeout_cnt = 0;
   // gsensor_disable();
    // hx3011_power_down();
}
void hx3011_spo2_mode_start(uint8_t delay_start)//血氧打开
{
    //如果有电话事件发生本次测量不进行
    if(bt_get_call_status() == BT_CALL_INCOMING||bt_get_call_status() == BT_CALL_OUTGOING ||bt_get_call_status() == BT_CALL_ACTIVE)
    {
        y_printf("存在电话事件,本次测量跳过");
        return;
    }
    y_printf("--->hx3011_spo2_mode_start,delay_start====%d",delay_start);
    if(delay_start)
    {
        y_printf("延迟开启血氧检测");
        if(delay_spo2_start_id)
        {
            y_printf("当前存在延迟定时器,先进行删除");
            sys_timeout_del(delay_spo2_start_id);
            delay_spo2_start_id = 0;
        }
    }

    if(get_once_spo2_flag())//发送单次测量
    {
        y_printf("即刻开启血氧单次测量");
        hx3011_spo2_mode_stop();
    }else if(get_3011_work_status())
    {
        delay_spo2_start_id = sys_timeout_add(true,hx3011_spo2_mode_start,10 *1000);//当前在工作，延迟10s
        y_printf("当前再测量,血氧延迟10秒");
        return;
    }
	if(hx3011_init(SPO2_MODE)==true)
	{
		// ppg_init = true;
	}
}

void hx3011_spo2_mode_stop(void)//血氧关闭
{
	hx3011_spo2_disable();
}

void hx3011_factroy_mode_start(void)//漏光测试
{
    if(get_3011_work_status())
    {
        hx3011_spo2_mode_stop();
    }
	if(hx3011_init(FT_LEAK_LIGHT_MODE)==true)
	{
		// ppg_init = true;
	}
}

static uint16_t heart_auto_timer = 0;
static uint8_t  heart_auto_test_time = 2;//几分钟一次
void start_heart_auto_timer(void)//开启连续测量心率
{
    //如果有电话相关的事件，延后两分钟
    if(!heart_auto_timer)
    {
        extern void hx3011_hrs_mode_start(uint8_t delay_start);
        hx3011_hrs_mode_start(false);
        y_printf("heart_auto_test_time====%d",heart_auto_test_time);
        heart_auto_timer = sys_timer_add(false,hx3011_hrs_mode_start,heart_auto_test_time*60*1000);
    }
}
void stop_heart_auto_timer(void)//关闭连续测量心率
{
    hx3011_hrs_mode_stop();
    if(heart_auto_timer)
    {
        sys_timer_del(heart_auto_timer);
        heart_auto_timer = 0;
    }
}
uint8_t return_heart_auto_timer(void)//获取连续测量心率定时器
{
    g_printf("heart_auto_timer====%d",heart_auto_timer);
    if(heart_auto_timer)
    return HEART_AUTO_TIMER_OPEN;
    else
    return HEART_AUTO_TIMER_CLOSE;
}
uint8_t once_heart_flag = 0;
void set_once_heart_flag(uint8_t temp)//单次测量标志位设置
{
    once_heart_flag = temp;
}
uint8_t get_once_heart_flag(void)
{
    return once_heart_flag;
}

uint8_t get_heart_auto_interval(void)//获取心率定时器频率
{
    return heart_auto_test_time;
}
void write_vm_heart_auto_interval(void)
{   
    int ret = 0;
    uint8_t temp[2] = {0};
    temp[0] = heart_auto_test_time;
    ret = syscfg_write(CFG_USER_HEART_3011_AUTO_INTERVAL,temp,sizeof(temp));
    y_printf("--->保存心率测量间隔为:%d,ret=%d",heart_auto_test_time,ret);
}
void read_vm_heart_auto_interval(void)
{
    int ret = 0;
    uint8_t temp[2] = {0};
    ret = syscfg_read(CFG_USER_HEART_3011_AUTO_INTERVAL,temp,sizeof(temp));
    if(ret > 0)//优化首次上电，读失败为0
    {
        heart_auto_test_time = temp[0];
        y_printf("心率间隔VM读取成功");
    }else{
        y_printf("心率间隔VM读取失败");
    }
    y_printf("--->读取心率测量间隔为:%d,ret=%d",heart_auto_test_time,ret);
}
void set_heart_auto_interval(uint8_t temp)//设置心率定时器频率
{
    heart_auto_test_time = temp;
    write_vm_heart_auto_interval();

    if(return_heart_auto_timer())
    {
        y_printf("重新开始心率测量定时器");
        sys_timer_modify(heart_auto_timer,heart_auto_test_time*60*1000);
    }
}





static uint16_t spo2_auto_timer = 0;
static uint8_t  spo2_auto_test_time = 2;//几分钟一次
void start_spo2_auto_timer(void)//开启连续测量血氧
{
    if(!spo2_auto_timer)
    {
        extern void hx3011_spo2_mode_start(uint8_t delay_start);
        hx3011_spo2_mode_start(false);
        y_printf("spo2_auto_test_time====%d",spo2_auto_test_time);
        spo2_auto_timer = sys_timer_add(false,hx3011_spo2_mode_start,spo2_auto_test_time*60*1000);
    }
}
void stop_spo2_auto_timer(void)//关闭连续测量血氧
{
    hx3011_spo2_mode_stop();
    if(spo2_auto_timer)
    {
        sys_timer_del(spo2_auto_timer);
        spo2_auto_timer = 0;
    }
}


uint8_t return_spo2_auto_timer(void)//获取连续测量心率定时器
{
    g_printf("spo2_auto_timer====%d",spo2_auto_timer);
    if(spo2_auto_timer)
    return SPO2_AUTO_TIMER_OPEN;
    else
    return SPO2_AUTO_TIMER_CLOSE;
}
uint8_t once_spo2_flag = 0;
void set_once_spo2_flag(uint8_t temp)//单次测量标志位设置
{
    once_spo2_flag = temp;
}
uint8_t get_once_spo2_flag(void)
{
    return once_spo2_flag;
}

void user_read_3011_form_vm_spo2(void)
{
    uint8_t user_hx3011_data[2] ={0} ;
    uint8_t hx3011_data = 0;
    int ret = syscfg_read(CFG_USER_DEFINE_SPO2_3011,user_hx3011_data,sizeof(user_hx3011_data));
    if(ret > 0)
    {
        hx3011_data = user_hx3011_data[0];
    }else{
        hx3011_data = 0;
    }
    
    //send_crtl_phone_app_data(COMMON_READ,HEART_3011_VM_VALUE_SPO2,hx3011_data);
    g_printf("user_read_3918_form_vm===========spo2=%d user_hx3011_data[]=%d  ",heart_value,user_hx3011_data[0]);
}


uint8_t get_spo2_auto_interval(void)//获取心率定时器频率
{
    return spo2_auto_test_time;
}
void read_vm_spo2_auto_interval(void)
{
    int ret = 0;
    uint8_t temp[2] = {0};
    ret = syscfg_read(CFG_USER_SPO2_3011_AUTO_INTERVAL,temp,sizeof(temp));
    if(ret > 0)//优化首次上电，读失败为0
    {
        spo2_auto_test_time = temp[0];
        y_printf("心率间隔VM读取成功");
    }else{
        y_printf("心率间隔VM读取失败");
    }
    y_printf("--->读取血氧测量间隔为:%d,ret=%d",spo2_auto_test_time,ret);
}
void write_vm_spo2_auto_interval(void)
{   
    int ret = 0;
    uint8_t temp[2] = {0};
    temp[0] = spo2_auto_test_time;
    ret = syscfg_write(CFG_USER_SPO2_3011_AUTO_INTERVAL,temp,sizeof(temp));
    y_printf("--->保存血氧测量间隔为:%d,ret=%d",spo2_auto_test_time,ret);
}

void set_spo2_auto_interval(uint8_t temp)//设置血氧定时器频率
{
    spo2_auto_test_time = temp;
    write_vm_spo2_auto_interval();

    if(return_spo2_auto_timer() == SPO2_AUTO_TIMER_OPEN)
    {
        y_printf("重新开始血氧测量定时器");
        sys_timer_modify(spo2_auto_timer,spo2_auto_test_time*60*1000);
    }
}



void test_vm_all(void)//测试VM存储功能
{
    uint8_t temp[] = {0};
    temp[0] = 20;
    int ret = syscfg_write(CFG_USER_DEFINE_SPO2_3011,temp,sizeof(temp));
    y_printf("ret ====%d",ret);
    user_read_3011_form_vm_spo2();
    ret = syscfg_write(CFG_USER_DEFINE_HEART_3011,temp,sizeof(temp));
    y_printf("ret ====%d",ret);
    set_spo2_auto_interval(1);
    read_vm_spo2_auto_interval();
    set_heart_auto_interval(1);
    read_vm_heart_auto_interval();
}   
/********************************漏光测试*******************************/
extern factory_result_t factory_reslts_to_app;
void send_factory_light_to_app(void)
{
    static uint8_t send_date_factory[17]={0};
    // if(0 == get_rcsp_connect_status())return;
    
    send_date_factory[0] = COMMON_BEGIN;
    send_date_factory[1] = COMMON_READ;
    send_date_factory[2] = HEART_3011_VALUE_FACTORY;

    send_date_factory[3] = (factory_reslts_to_app.green_data_final >> 24) & 0xFF;
    send_date_factory[4] = (factory_reslts_to_app.green_data_final >> 16) & 0xFF;
    send_date_factory[5] = (factory_reslts_to_app.green_data_final >> 8) & 0xFF;
    send_date_factory[6] = factory_reslts_to_app.green_data_final & 0xFF;

    send_date_factory[7] = (factory_reslts_to_app.red_data_final >> 24) & 0xFF;
    send_date_factory[8] = (factory_reslts_to_app.red_data_final >> 16) & 0xFF;
    send_date_factory[9] = (factory_reslts_to_app.red_data_final >> 8) & 0xFF;
    send_date_factory[10] = factory_reslts_to_app.red_data_final & 0xFF;

    send_date_factory[11] = (factory_reslts_to_app.ir_data_final >> 24) & 0xFF;
    send_date_factory[12] = (factory_reslts_to_app.ir_data_final >> 16) & 0xFF;
    send_date_factory[13] = (factory_reslts_to_app.ir_data_final >> 8) & 0xFF;
    send_date_factory[14] = factory_reslts_to_app.ir_data_final & 0xFF;


    send_date_factory[15] = HEART_3011_VALUE_FACTORY+1;
    send_date_factory[16] = COMMON_END; 
    ear_send_data_to_APP(send_date_factory,sizeof(send_date_factory));

    
}
static uint8_t vm_time_type;
void set_vm_time_type(uint8_t temp)//用于设置保存时间为心率还是血氧
{
    vm_time_type = temp;
}
uint8_t get_vm_time_type(void)
{
    return vm_time_type;
}
/*************************心率报警********************************/
static uint8_t heart_high_warning = 100;
static uint8_t heart_low_warning  =  60;
void set_heart_warning_value(uint8_t low ,uint8_t high)
{
    heart_low_warning  = low;
    heart_high_warning = high;
}
uint8_t get_high_heart_warning(void)
{
    return heart_high_warning;
}

uint8_t get_low_heart_warning(void)
{
    return heart_low_warning;
}
void send_heart_warning_to_app(void)
{
    static uint8_t send_date_warning[7]={0};
    // if(0 == get_rcsp_connect_status())return;
    
    send_date_warning[0] = COMMON_BEGIN;
    send_date_warning[1] = COMMON_READ;
    send_date_warning[2] = HEART_3011_GET_VALUE_WARNING;

    send_date_warning[3] = get_low_heart_warning();
    send_date_warning[4] = get_high_heart_warning();

    send_date_warning[5] = HEART_3011_GET_VALUE_WARNING+1;
    send_date_warning[6] = COMMON_END; 
    ear_send_data_to_APP(send_date_warning,sizeof(send_date_warning));
    y_printf("发送------》APP 心率报警最小值为:%d,最大值为:%d",send_date_warning[3],send_date_warning[4]);
    
}
void write_vm_heart_warning(void)
{
    int ret = 0;
    uint8_t temp[2] = {0};
    temp[0] = get_low_heart_warning();
    temp[1] = get_high_heart_warning();
    ret = syscfg_write(CFG_USER_DEFINE_HEART_WARNING,temp,sizeof(temp));
    y_printf("--->保存心率最小值为:%d,最大值为:%d,ret=%d",temp[0],temp[1],ret);
}
void read_vm_heart_warning(void)
{
    int ret = 0;
    uint8_t temp[2] = {0};
    ret = syscfg_read(CFG_USER_DEFINE_HEART_WARNING,temp,sizeof(temp));
    if(ret > 0)//优化首次上电，读失败为0
    {
        set_heart_warning_value(temp[0],temp[1]);
        y_printf("心率报警VM读取成功");
    }else{
        y_printf("心率报警VM读取失败");
    }
    y_printf("--->读取心率报警最大值为:%d,最小值为:%d,ret=%d",heart_high_warning,heart_low_warning,ret);
}
void read_vm_heart_offline(void);
void read_vm_oxygen_offline(void);
void power_on_read_vm_value(void)//开机需要读取的VM
{
    read_vm_spo2_auto_interval();
    read_vm_heart_auto_interval();
    read_vm_heart_warning();
    read_vm_spo2_warning();
#if TCFG_HEART_DETECTION_WEARING
    void read_vm_wearing_detection(void);
    read_vm_wearing_detection();
#endif
    read_vm_oxygen_offline();
    read_vm_heart_offline();
#if TCFG_SC7A20_ALGORITHM
    read_vm_daily_motion_data();
#endif  
}
#define WARNING_CNT      1//超出阈值几次报警
void judge_tone_heart_warning(uint8_t heart_value)//判断是否播放提示音 
{
    g_printf("进入警报1");
    static uint8_t high_cnt = 0;
    static uint8_t low_cnt = 0;
    if((heart_value > get_low_heart_warning()) && (heart_value < get_high_heart_warning()))
    {
        low_cnt = 0;
        high_cnt = 0;
    }
    
    g_printf("连续判断");
    if(heart_value >= get_high_heart_warning())
    {
        high_cnt++;
        low_cnt = 0;
    }else{
        high_cnt = 0;
    }

    if(heart_value <= get_low_heart_warning())
    {
        low_cnt++;
        high_cnt = 0;
    }else{
        low_cnt = 0;
    }

    // if((high_cnt>0&&high_cnt<WARNING_CNT)||(low_cnt>0&&low_cnt<WARNING_CNT))
    // {
    //     hx3011_hrs_mode_start(false);
    // }
    
    if(high_cnt >= WARNING_CNT)
    {
#if TCFG_HEART_SPO2_VOICE
        if (get_bt_tws_connect_status()) {
            //bt_tws_play_tone_at_same_time(SYNC_TONE_HEART_HIGH, 600);
            tws_play_tone_file(get_tone_files()->heart_high,600);
        }else{
            //func_callback_in_task_tone_play(IDEX_TONE_HEART_HIGH);
            play_tone_file(get_tone_files()->heart_high);
        }
#endif
        high_cnt = 0;
        y_printf("------>心率过高");
    }

    if(low_cnt >= WARNING_CNT)
    {
#if TCFG_HEART_SPO2_VOICE
        if (get_bt_tws_connect_status()) {
            //bt_tws_play_tone_at_same_time(SYNC_TONE_HEART_LOW, 600);
            tws_play_tone_file(get_tone_files()->heart_low,600);
        }else{
            //func_callback_in_task_tone_play(IDEX_TONE_HEART_LOW);
            play_tone_file(get_tone_files()->heart_low);
        }
    
#endif
        low_cnt = 0;
        y_printf("------>心率过低");
    } 
    
}

/********************************************血氧报警**************************************************/
static uint8_t spo2_low_warning  =  95;

void set_spo2_warning_value(uint8_t low)
{
    spo2_low_warning  = low;
}
uint8_t get_low_spo2_warning(void)
{
    return spo2_low_warning;
}
void send_spo2_warning_to_app(void)
{
    static uint8_t send_spo2date_warning[6]={0};
    
    send_spo2date_warning[0] = COMMON_BEGIN;
    send_spo2date_warning[1] = COMMON_READ;
    send_spo2date_warning[2] = HEART_3011_GET_SPO2_WARNING;

    send_spo2date_warning[3] = get_low_spo2_warning();

    send_spo2date_warning[4] = HEART_3011_GET_SPO2_WARNING+1;
    send_spo2date_warning[5] = COMMON_END; 
    ear_send_data_to_APP(send_spo2date_warning,sizeof(send_spo2date_warning));
    y_printf("发送------》APP 血氧报警最小值为:%d",send_spo2date_warning[3]);
    
}
void write_vm_spo2_warning(void)
{
    int ret = 0;
    uint8_t temp = 0;
    temp = get_low_heart_warning();
    ret = syscfg_write(CFG_USER_DEFINE_SPO2_WARNING,temp,sizeof(temp));
    y_printf("--->保存心率最小值为:%d,ret=%d",temp,ret);
}
void read_vm_spo2_warning(void)
{
    int ret = 0;
    uint8_t temp = 0;
    ret = syscfg_read(CFG_USER_DEFINE_SPO2_WARNING,temp,sizeof(temp));
    if(ret > 0)//优化首次上电，读失败为0
    {
        set_spo2_warning_value(temp);
        y_printf("血氧报警VM读取成功");
    }else{
        y_printf("血氧报警VM读取失败");
    }
    y_printf("--->读取血氧报警最小值为:%d,ret=%d",spo2_low_warning,ret);
}

#define SPO2_WARNING_CNT      1//超出阈值几次报警
void judge_tone_spo2_warning(uint8_t spo2_value)//判断是否播放提示音 
{
    static uint8_t low_cnt = 0;
    
    if(spo2_value < get_low_spo2_warning())
    {
        low_cnt++;
    }

    if(low_cnt >= WARNING_CNT)
    {
#if TCFG_HEART_SPO2_VOICE
        if (get_bt_tws_connect_status()) {
            //bt_tws_play_tone_at_same_time(SYNC_TONE_SPO2_WARNING, 600);
            tws_play_tone_file(get_tone_files()->spo2_warn,600);
        }else{
            //func_callback_in_task_tone_play(IDEX_TONE_SPO2_WARNING);
            play_tone_file(get_tone_files()->spo2_warn);
        }
#endif
        low_cnt = 0;
        y_printf("------>血氧过低");
    } 
}








/*************************************入耳检测************************************************/
#if TCFG_HEART_DETECTION_WEARING
u8 wearing_detection_state = 0;
void write_vm_wearing_detection(void)
{   
    int ret = 0;
    uint8_t temp[2] = {0};
    temp[0] = wearing_detection_state;
    ret = syscfg_write(CFG_USER_DEFINE_HEART_DETECTION,temp,sizeof(temp));
    y_printf("--->保存入耳测试为:%d,ret=%d",wearing_detection_state,ret);
}
void all_day_wearing_detection_enable(void)
{
    hx3011_init(WEAR_MODE);
    wearing_detection_state = 1;
    write_vm_wearing_detection();
    y_printf("--->开启入耳检查");
}
void read_vm_wearing_detection(void)
{
    int ret = 0;
    uint8_t temp[2] = {0};
    ret = syscfg_read(CFG_USER_DEFINE_HEART_DETECTION,temp,sizeof(temp));
    if(ret > 0)//优化首次上电，读失败为0
    {
        wearing_detection_state = temp[0];
        y_printf("入耳测试VM读取成功");
    }else{
        y_printf("入耳测试VM读取失败");
    }
    if(wearing_detection_state)
    {
        // all_day_wearing_detection_enable();
        func_callback_in_task(WEAR_MODE_START);
    }
    y_printf("--->保存入耳测试为:%d,ret=%d",wearing_detection_state,ret);
}



void all_day_wearing_detection_disable(void)
{
    hx3011_hrs_disable();
    wearing_detection_state = 0;
    write_vm_wearing_detection();
    y_printf("--->关闭入耳检查");
}

u8 get_wearing_detection_state(void)
{
    return wearing_detection_state;
}

void earphone_in_event(void)
{
    if(get_tws_sibling_connect_state())//对耳才ANC 
    {
        anc_mode_switch(ANC_ON, 1);
    }
    y_printf("a2dp_get_status()======%d",a2dp_get_status());
    if (a2dp_get_status() == BT_MUSIC_STATUS_SUSPENDING) {
        r_printf("------->STAT...\n");
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PLAY, 0, NULL);
    }
}

void earphone_out_event(void)
{
    anc_mode_switch(ANC_OFF, 1);
    y_printf("a2dp_get_status()======%d",a2dp_get_status());
    if (a2dp_get_status() == BT_MUSIC_STATUS_STARTING) {
        r_printf("------->STOP...\n");
        bt_cmd_prepare(USER_CTRL_AVCTP_OPID_PAUSE, 0, NULL);
    }
}
void set_earphone_state(u8 earphone_state)
{
    static u8 earphone_state_last = 0;
    y_printf("earphone_state_last==%d,earphone_state===%d",earphone_state_last,earphone_state);
    if(earphone_state != earphone_state_last)
    {
        earphone_state_last = earphone_state;
        if(1 == earphone_state)
        {
            earphone_in_event();
        }else{
            earphone_out_event();
        }
    }
}
#endif
//灰卡测试
void gray_card_test_init(void)
{
    hx3011_init(FT_GRAY_CARD_MODE);
}


/**********************************离线数据保存*****************************************/

#define MAX_DATA 20   // 最多存储20条数据


typedef struct {
    sys_time time;  // 日期
    u8 heart;     //心率数值
    u8 oxygen;    //血氧数值
} DailyData;

typedef struct {
    DailyData data[MAX_DATA]; // 数据存储
    int oldest;              // 最旧数据索引
    int current_size;        // 当前存储数量
} MonthBuffer;
MonthBuffer heart_offline;
MonthBuffer oxygen_offline;

// 按时间顺序打印数据（从旧到新）
void print_buffer(const MonthBuffer *buf) {
    printf("Monthly Data (Oldest to Newest):\n");
    for (int i = 0; i < buf->current_size; i++) {
        int pos = (buf->oldest + i) % MAX_DATA; // 当缓冲区未满时，oldest=0，直接按顺序打印
        printf("year:%d,month:%d,day:%d,hour:%d,min:%d,sec:%d,heart:%d\n",
               buf->data[pos].time.year,
               buf->data[pos].time.month,
               buf->data[pos].time.day,
               buf->data[pos].time.hour,
               buf->data[pos].time.min,
               buf->data[pos].time.sec,
               buf->data[pos].heart);
    }
}

void write_vm_heart_offline(void)
{
    u8 ret;
    ret = syscfg_write(CFG_USER_DEFINE_LOCAL_TIME_HEART_DATA,heart_offline.data,sizeof(heart_offline.data));
    y_printf("--->111 保存心率离线数据成功:%d",ret);

    ret = syscfg_write(CFG_USER_DEFINE_LOCAL_TIME_HEART_OLD,&heart_offline.oldest,sizeof(heart_offline.oldest));
    y_printf("--->222 保存心率离线数据成功:%d,oldest====%d",ret,heart_offline.oldest);

    ret = syscfg_write(CFG_USER_DEFINE_LOCAL_TIME_HEART_CURR,&heart_offline.current_size,sizeof(heart_offline.current_size));
    y_printf("--->333 保存心率离线数据成功:%d,currsize===%d",ret,heart_offline.current_size);
}

void read_vm_heart_offline(void)
{
    u8 ret;
    ret = syscfg_read(CFG_USER_DEFINE_LOCAL_TIME_HEART_DATA,heart_offline.data,sizeof(heart_offline.data));
    y_printf("--->111 读取心率离线数据成功:%d",ret);

    ret = syscfg_read(CFG_USER_DEFINE_LOCAL_TIME_HEART_OLD,&heart_offline.oldest,sizeof(heart_offline.oldest));

    y_printf("--->222 读取心率离线数据成功:%d,oldest====%d",ret,heart_offline.oldest);

    ret = syscfg_read(CFG_USER_DEFINE_LOCAL_TIME_HEART_CURR,&heart_offline.current_size,sizeof(heart_offline.current_size));

    y_printf("--->333 读取心率离线数据成功:%d,current_size====%d",ret,heart_offline.current_size);
    print_buffer(&heart_offline);
}

void write_vm_oxygen_offline(void)
{
    u8 ret;
    ret = syscfg_write(CFG_USER_DEFINE_LOCAL_TIME_SPO2_DATA,oxygen_offline.data,sizeof(oxygen_offline.data));
    y_printf("--->111 保存血氧离线数据成功:%d",ret);

    ret = syscfg_write(CFG_USER_DEFINE_LOCAL_TIME_SPO2_OLD,&oxygen_offline.oldest,sizeof(oxygen_offline.oldest));
    y_printf("--->222 保存血氧离线数据成功:%d",ret);

    ret = syscfg_write(CFG_USER_DEFINE_LOCAL_TIME_SPO2_CURR,&oxygen_offline.current_size,sizeof(oxygen_offline.current_size));
    y_printf("--->333 保存血氧离线数据成功:%d",ret);
}

void read_vm_oxygen_offline(void)
{
    u8 ret;
    ret = syscfg_read(CFG_USER_DEFINE_LOCAL_TIME_SPO2_DATA,oxygen_offline.data,sizeof(oxygen_offline.data));
    y_printf("--->111 读取血氧离线数据成功:%d",ret);

    ret = syscfg_read(CFG_USER_DEFINE_LOCAL_TIME_SPO2_OLD,&oxygen_offline.oldest,sizeof(oxygen_offline.oldest));
    y_printf("--->222 读取血氧离线数据成功:%d",ret);

    ret = syscfg_read(CFG_USER_DEFINE_LOCAL_TIME_SPO2_CURR,&oxygen_offline.current_size,sizeof(oxygen_offline.current_size));
    y_printf("--->333 读取血氧离线数据成功:%d",ret);

    print_buffer(&oxygen_offline);
}

// 初始化缓冲区
void init_buffer_heart_offline(void) {

    memset(&heart_offline, 0, sizeof(MonthBuffer));
    heart_offline.oldest = 0;
    heart_offline.current_size = 0;
    write_vm_heart_offline();
}

void init_buffer_oxygen_offline(void) {

    memset(&oxygen_offline, 0, sizeof(MonthBuffer));
    oxygen_offline.oldest = 0;
    oxygen_offline.current_size = 0;
    write_vm_oxygen_offline();
}

// 添加新数据（自动替换最旧数据）type 0是心率，1是血氧
void add_data(MonthBuffer *buf, const sys_time* date, u8 temp,u8 type) {
    // 找到写入位置
    int write_pos = (buf->current_size < MAX_DATA) ? 
                    buf->current_size : 
                    buf->oldest;

    // 写入数据
    if(type == 0)
    {
        buf->data[write_pos].heart           = temp;
    }else{
        buf->data[write_pos].oxygen          = temp;
    }
    y_printf("add_data year=%d,month:%d.day:%d",date->year,date->month,date->day);
    memcpy(&buf->data[write_pos].time, date, sizeof(sys_time));

    // 更新指针和大小
    if (buf->current_size < MAX_DATA) {
        buf->current_size++;
    } else {
        buf->oldest = (buf->oldest + 1) % MAX_DATA;
    }
}

u16 ear_send_heartoffline_timer = 0;
void send_offline_heart_to_app(void);
void ear_send_heartofflinedata_timer(void)
{
    y_printf("----->ear_send_offlinedata_timer,ear_send_heartoffline_timer====%d",ear_send_heartoffline_timer);
    if(!ear_send_heartoffline_timer)
    {
        ear_send_heartoffline_timer=sys_timer_add(NULL,send_offline_heart_to_app,50);
    }
    
}

void send_offline_heart_to_app(void)
{
    static uint8_t send_date_heart_offline[13]={0};
    y_printf("---->send_offline_heart_to_app");
    static int i=0;
    int pos = (heart_offline.oldest + i) % MAX_DATA; // 当缓冲区未满时，oldest=0，直接按顺序打印
    send_date_heart_offline[0]  = COMMON_BEGIN;
    send_date_heart_offline[1]  = COMMON_READ;
    send_date_heart_offline[2]  = HEART_3011_VM_VALUE_HEART;

    send_date_heart_offline[3]   = (heart_offline.data[pos].time.year >> 8) & 0xff;
    send_date_heart_offline[4]   = heart_offline.data[pos].time.year & 0xff;
    send_date_heart_offline[5]   = heart_offline.data[pos].time.month;
    send_date_heart_offline[6]   = heart_offline.data[pos].time.day;
    send_date_heart_offline[7]   = heart_offline.data[pos].time.hour;
    send_date_heart_offline[8]   = heart_offline.data[pos].time.min;
    send_date_heart_offline[9]   = heart_offline.data[pos].time.sec;
    send_date_heart_offline[10]  = heart_offline.data[pos].heart;

    send_date_heart_offline[11] = HEART_3011_VM_VALUE_HEART+1;
    send_date_heart_offline[12] = COMMON_END; 
    ear_send_data_to_APP(send_date_heart_offline,sizeof(send_date_heart_offline));
    r_printf("heart-->year:%d,month:%d,day:%d,hour:%d,min:%d,sec:%d,heart:%d\n",
            heart_offline.data[pos].time.year,
            heart_offline.data[pos].time.month,
            heart_offline.data[pos].time.day,
            heart_offline.data[pos].time.hour,
            heart_offline.data[pos].time.min,
            heart_offline.data[pos].time.sec,
            heart_offline.data[pos].heart);
    
    i++;

    if(!(i < heart_offline.current_size))
    {
        
        if(ear_send_heartoffline_timer)
        {
            g_printf("------>sys_timer_del,ear_send_offline_timer=%d",ear_send_heartoffline_timer);
            sys_timer_del(ear_send_heartoffline_timer);
            ear_send_heartoffline_timer=0;
        }
        init_buffer_heart_offline();
        i=0;
    }
}

u16 ear_send_oxygenoffline_timer = 0;
void send_offline_oxygen_to_app(void);
void ear_send_oxygenofflinedata_timer(void)
{
    y_printf("----->ear_send_offlinedata_timer,ear_send_oxygenoffline_timer====%d",ear_send_oxygenoffline_timer);
    if(!ear_send_oxygenoffline_timer)
    {
        ear_send_oxygenoffline_timer=sys_timer_add(NULL,send_offline_oxygen_to_app,50);
    }
    
}
void send_offline_oxygen_to_app(void)
{
    static uint8_t send_date_oxygen_offline[13]={0};
    y_printf("---->send_offline_oxygen_to_app");
    static int i=0;
    int pos = (oxygen_offline.oldest + i) % MAX_DATA; // 当缓冲区未满时，oldest=0，直接按顺序打印
    send_date_oxygen_offline[0] = COMMON_BEGIN;
    send_date_oxygen_offline[1] = COMMON_READ;
    send_date_oxygen_offline[2] = HEART_3011_VM_VALUE_SPO2;

    send_date_oxygen_offline[3]   = (oxygen_offline.data[pos].time.year >> 8) & 0xff;
    send_date_oxygen_offline[4]   = oxygen_offline.data[pos].time.year & 0xff;
    send_date_oxygen_offline[5]   = oxygen_offline.data[pos].time.month;
    send_date_oxygen_offline[6]   = oxygen_offline.data[pos].time.day;
    send_date_oxygen_offline[7]   = oxygen_offline.data[pos].time.hour;
    send_date_oxygen_offline[8]   = oxygen_offline.data[pos].time.min;
    send_date_oxygen_offline[9]   = oxygen_offline.data[pos].time.sec;
    send_date_oxygen_offline[10]  = oxygen_offline.data[pos].oxygen;

    send_date_oxygen_offline[11] = HEART_3011_VM_VALUE_SPO2+1;
    send_date_oxygen_offline[12] = COMMON_END; 
    ear_send_data_to_APP(send_date_oxygen_offline,sizeof(send_date_oxygen_offline));
    r_printf("oxygen-->year:%d,month:%d,day:%d,hour:%d,min:%d,sec:%d,oxygen:%d\n",
            oxygen_offline.data[pos].time.year,
            oxygen_offline.data[pos].time.month,
            oxygen_offline.data[pos].time.day,
            oxygen_offline.data[pos].time.hour,
            oxygen_offline.data[pos].time.min,
            oxygen_offline.data[pos].time.sec,
            oxygen_offline.data[pos].oxygen);
    
    i++;

    if(!(i < oxygen_offline.current_size))
    {
        
        if(ear_send_oxygenoffline_timer)
        {
            g_printf("------>sys_timer_del,ear_send_oxygenoffline_timer=%d",ear_send_oxygenoffline_timer);
            sys_timer_del(ear_send_oxygenoffline_timer);
            ear_send_oxygenoffline_timer=0;
        }
        init_buffer_oxygen_offline();
        i=0;
    }
}

#if 0
//测试逻辑
void test_heart_offline(void)
{
    init_buffer_heart_offline();

    sys_time t1 = {2023,10,1,12,0,0};
    for (int i = 0; i < 25; i++) {
        add_data(&heart_offline, &t1, 60+i,0);
        t1.day++; // 模拟日期变化
    }
    write_vm_heart_offline();
    read_vm_heart_offline();
    printf("Stored %d records\n", heart_offline.current_size); // 输出20
    printf("Oldest position: %d\n", heart_offline.oldest);     // 输出5 (25%20=5)
}
#endif

void save_offline_heart(const sys_time* date)
{
    uint8_t user_hx3011_data[2] ={0} ;
    uint8_t hx3011_data = 0;
    int ret = syscfg_read(CFG_USER_DEFINE_HEART_3011,user_hx3011_data,sizeof(user_hx3011_data));
    hx3011_data = user_hx3011_data[0];
    add_data(&heart_offline, date, hx3011_data,0);
    write_vm_heart_offline();
    // read_vm_heart_offline();
}

void save_offline_oxygen(const sys_time* date)
{
    uint8_t user_hx3011_data[2] ={0} ;
    uint8_t hx3011_data = 0;
    int ret = syscfg_read(CFG_USER_DEFINE_SPO2_3011,user_hx3011_data,sizeof(user_hx3011_data));
    hx3011_data = user_hx3011_data[0];
    add_data(&oxygen_offline, date, hx3011_data,1);
    write_vm_oxygen_offline();
    // read_vm_oxygen_offline();
}
//*******************************************滴滴声开关****************************************************//
static u16 hearting_tone_didi_timer = 0;
void hearting_didi_tone(void)
{
    if(get_once_heart_flag())
    {
#if TCFG_HEART_SPO2_VOICE
        if (get_bt_tws_connect_status()) {
            //bt_tws_play_tone_at_same_time(SYNC_TONE_HEART_DIDI, 600);
            tws_play_tone_file(get_tone_files()->heart_dodo,600);
        }else{
            //func_callback_in_task_tone_play(IDEX_TONE_HEART_DIDI);
            play_tone_file(get_tone_files()->heart_dodo);
        }
#endif
    }else{
        delete_hearting_tone_didi_timer();
    }
}
void create_hearting_tone_didi_timer(void)
{
    if(!hearting_tone_didi_timer)
    {
        hearting_didi_tone();
        hearting_tone_didi_timer = sys_timer_add(NULL,hearting_didi_tone,2500);
    }
    if(work_mode_flag == HRS_MODE)
    {
        delete_spo2_tone_didi_timer();
    }
}

void delete_hearting_tone_didi_timer(void)
{
    if(hearting_tone_didi_timer)
    {
        sys_timer_del(hearting_tone_didi_timer);
        hearting_tone_didi_timer = 0;
    }
}
static u16 spo2_tone_didi_timer = 0;
void spo2_didi_tone(void)
{
   if(get_once_spo2_flag())
    {
#if TCFG_HEART_SPO2_VOICE
        if (get_bt_tws_connect_status()) {
            //bt_tws_play_tone_at_same_time(SYNC_TONE_SPO2_ING, 600);
            tws_play_tone_file(get_tone_files()->spo2_dodo,600);
        }else{
            //func_callback_in_task_tone_play(IDEX_TONE_SPO2_ING);
            play_tone_file(get_tone_files()->spo2_dodo);
        }
#endif
    }else{
        delete_spo2_tone_didi_timer();
    }
}
void create_spo2_tone_didi_timer(void)
{
    if(!spo2_tone_didi_timer)
    {
        spo2_didi_tone();
        spo2_tone_didi_timer = sys_timer_add(NULL,spo2_didi_tone,2000);
    }
    if(work_mode_flag == SPO2_MODE)
    {
        delete_hearting_tone_didi_timer();
    }
}

void delete_spo2_tone_didi_timer(void)
{
    if(spo2_tone_didi_timer)
    {
        sys_timer_del(spo2_tone_didi_timer);
        spo2_tone_didi_timer = 0;
    }
}
/***************************************压力检测功能**************************************************/

u16 delay_heart_hrv_start_id = 0;

void hx3011_hrv_mode_stop(void)//压力关闭
{
    hx3011_hrv_disable();
}

uint8_t once_heart_hrv_flag = 0;
void set_once_heart_hrv_flag(uint8_t temp)//单次测量标志位设置
{
    once_heart_hrv_flag = temp;
}
uint8_t get_once_heart_hrv_flag(void)
{
    return once_heart_hrv_flag;
}
void hx3011_hrv_mode_start(uint8_t delay_start)//形参区别是否延迟开启
{
    y_printf("--->hx3011_hrv_mode_start,delay_start====%d",delay_start);
    if(delay_start)
    {
        y_printf("延迟开启压力检测");
        if(delay_heart_hrv_start_id)
        {
            y_printf("当前存在延迟定时器,先进行删除");
            sys_timeout_del(delay_heart_hrv_start_id);
            delay_heart_hrv_start_id = 0;
        }
    }
    if(get_once_heart_hrv_flag())//发送单次测量
    {
        hx3011_hrv_mode_stop();
        y_printf("即刻开启压力单次测量");
    }else if(get_3011_work_status())
    {
        delay_heart_hrv_start_id = sys_timeout_add(true,hx3011_hrv_mode_start,10 *1000);//当前在工作，延迟10s
        y_printf("当前再测量,压力延迟10秒");
        return;
    }
	if(hx3011_init(HRV_MODE)==true)
	{
		// ppg_init = true;
	}
}


#endif //CHIP_HX3011
#endif //TCFG_HEART_SENSOR

/******************************心率数值报号************************/
#if TCFG_HEART_NUMBER
//判断几位数
int count_digits(uint8_t n) {
    int digits = 0;
    do {
        digits++;
        n /= 10;
    } while (n != 0);  // 处理n=0的情况
    return digits;
}


void ring_number_timer(uint8_t temp)
{
    y_printf("提示音数值:%d",temp);
    if (get_bt_tws_connect_status()) {
        //bt_tws_play_tone_at_same_time(SYNC_TONE_NUM0 + temp, 600);
        tws_play_tone_file((get_tone_files()->num[0+temp]),600);
    }else{
        //func_callback_in_task_tone_play(IDEX_TONE_NUM_0 + temp);
         play_tone_file((get_tone_files()->num[0+temp]));
    }
}


#define TIME_INTERVAL        1300
extern void delay_music_start(void);
void ring_heart_number(uint8_t heart_temp)
{
    delete_hearting_tone_didi_timer();
    y_printf("----->ring_heart_number");

    u8 heart_cnt = 0;
    heart_cnt = count_digits(heart_temp);
    if (get_bt_tws_connect_status()) {
        //bt_tws_play_tone_at_same_time(SYNC_TONE_HEART_IS, 600);
        tws_play_tone_file(get_tone_files()->heart_is,600);
    }else{
        //func_callback_in_task_tone_play(IDEX_TONE_HEART_IS);
        play_tone_file(get_tone_files()->heart_is);
    }
    if(2 == heart_cnt)
    {
        
        sys_timeout_add(heart_temp/10, ring_number_timer, TIME_INTERVAL *2 - 500);
        sys_timeout_add(heart_temp%10, ring_number_timer, TIME_INTERVAL *3);
#if TCFG_HEART_SENSOR
        sys_timeout_add(heart_temp,judge_tone_heart_warning,TIME_INTERVAL *4);
        sys_timeout_add(NULL, delay_music_start, TIME_INTERVAL *5);
#endif
    }else if(3 == heart_cnt){
        sys_timeout_add(heart_temp/100, ring_number_timer, TIME_INTERVAL *2 - 500);
        sys_timeout_add((heart_temp/10)%10, ring_number_timer, TIME_INTERVAL *3);
        sys_timeout_add(heart_temp%10, ring_number_timer, TIME_INTERVAL *4);
#if TCFG_HEART_SENSOR
        sys_timeout_add(heart_temp,judge_tone_heart_warning,TIME_INTERVAL *5);
        sys_timeout_add(NULL, delay_music_start, TIME_INTERVAL *6);
#endif

    }

}

void ring_spo2_number(uint8_t spo2_temp)
{
    delete_spo2_tone_didi_timer();
    y_printf("----->ring_spo2_number");
    u8 spo2_cnt = 0;
    spo2_cnt = count_digits(spo2_temp);
    if (get_bt_tws_connect_status()) {
        //bt_tws_play_tone_at_same_time(SYNC_TONE_SPO2_IS, 600);
        tws_play_tone_file(get_tone_files()->spo2_is,600);
    }else{
        //func_callback_in_task_tone_play(IDEX_TONE_SPO2_IS);
        play_tone_file(get_tone_files()->spo2_is);
    }
    
    if(2 == spo2_cnt)
    {
        
        sys_timeout_add(spo2_temp/10, ring_number_timer, TIME_INTERVAL *2 - 500);
        sys_timeout_add(spo2_temp%10, ring_number_timer, TIME_INTERVAL *3);
#if TCFG_HEART_SENSOR
        sys_timeout_add(spo2_temp, judge_tone_spo2_warning, TIME_INTERVAL *4);
#endif
        sys_timeout_add(NULL, delay_music_start, TIME_INTERVAL *5);
    }else if(3 == spo2_cnt){
        sys_timeout_add(spo2_temp/100, ring_number_timer, TIME_INTERVAL *2 - 500);
        sys_timeout_add((spo2_temp/10)%10, ring_number_timer, TIME_INTERVAL *3);
        sys_timeout_add(spo2_temp%10, ring_number_timer, TIME_INTERVAL *4);
#if TCFG_HEART_SENSOR
        sys_timeout_add(spo2_temp, judge_tone_spo2_warning, TIME_INTERVAL *5);
#endif
        sys_timeout_add(NULL, delay_music_start, TIME_INTERVAL *6);
    }

}
#endif
/******************************推送提示音到app_core***********************************/
#if TCFG_HEART_SPO2_VOICE || TCFG_HEART_NUMBER
void tone_play_in_app_core(u8 index)
{
    tone_play_index(index,1);
}
//推送某个函数到指定任务执行
void func_callback_in_task_tone_play(u8 mode)
{
    int err;
    int msg[5];    
   
    //推送带1个参数的函数到app_core任务
    msg[0] = (int) tone_play_in_app_core;
    msg[1] = 1;//函数需要1个参数
    msg[2] = mode;//对应参数 mode
   
    err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);  

    printf("%s %d\n", __func__, err);
}
#endif
static void callback_func_1_param(int mode)
{
    // printf("/////////////////////////////%s %d\n", __func__, mode);
    switch(mode)
    {
        case HEART_AUTO_NOW:
            hx3011_hrs_mode_start(false);
            break;
        default:
            break;
    }
}

void func_callback_in_task(u8 mode)
{
    int err;
    int msg[5];    
   
    //推送带1个参数的函数到app_core任务
    msg[0] = (int) callback_func_1_param;
    msg[1] = 1;//函数需要1个参数
    msg[2] = mode;//对应参数 mode
   
    err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);  

    // printf("%s %d\n", __func__, err);
}







