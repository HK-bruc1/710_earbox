/**
  ******************************************************************************
  * @file           : hx3011_PROX.h
  * @version        : v1.0
  * @brief          :
*/
#ifndef __hx3011_PROX_H_
#define __hx3011_PROX_H_

// #include <stdbool.h>
// #include <stdint.h>




#define CH0   0
#define CH1   1
#define CH2   2




#define CH0_OFFSET   0
#define CH1_OFFSET   1
#define CH2_OFFSET   2


#define RAW0   0
#define RAW1   1
#define RAW2   2

#define BL0   0
#define BL1   1
#define BL2   2


#define LP0   0
#define LP1   1
#define LP2   2


#define DIFF0   0
#define DIFF1   1
#define DIFF2   2

// user  to config
#define WEAR_CH   CH1
#define REF_CH    CH0

#define hx_min(x,y)  ( (x)>(y)?(y):(x) )
#define hx_abs(x)    ((x)>0?(x):(-(x)))

#define RELATIVE_MODE  0  //���ֵģʽ, �����=LP-BL, �޷�����ſ���ʱ�Ŀ�����Ӧ,��������
#define ABSOLUTE_MODE  1  //����ֵģʽ, �����=LP-NV_BASE �����������

#define PROX_CHIPID         0x27


#define NORMALWEARDIFF      8000                 //����������Ե���ƽ��ֵ, �������
#define WEARTHRES 	        NORMALWEARDIFF/2      //��ͨ�����ֵ
#define WEARTHRES_LOW 	    (0.8*NORMALWEARDIFF)/2      //��ͨ�����ֵ
#define SLEEPWEARTHRES      NORMALWEARDIFF*0.3    //˯�������ֵ

#define WEAR_HIGH_DIFF		NORMALWEARDIFF/6
#define WEAR_LOW_DIFF     	NORMALWEARDIFF/8
#define REF_HIGH_DIFF 		NORMALWEARDIFF/6
#define REF_LOW_DIFF 		NORMALWEARDIFF/8



//HX3918 cap ���Ե��ݵ�����������, ��С
//#define FULL0625PF  //HX3918 full range cap = 0.625pf //һ�㲻�����, �������������, ˵����Ӧpad��С,
//#define FULL125PF   //HX3918 full range cap = 1.25pf
#define FULL25PF      //HX3918 full range cap = 2.5pf, һ��PAD��СĬ�����.
//#define FULL375PF   //HX3918 full range cap = 3.75 pf
//#define FULL50PF    //HX3918 full range cap = 5.0 pf

//READ_DEVATION ,read offset ��ƫ�����, һ��offset = 0.0586pf ���ղ�ͬ����, ��Ӧ��code, ��10%����
#ifdef FULL125PF 
#define READ_DEVATION        1680
#elif defined(FULL25PF)
#define READ_DEVATION        845
#elif defined(FULL375PF)
#define READ_DEVATION        560
#elif defined(FULL375PF)
#define READ_DEVATION        420
#endif




typedef enum
{
    prox_no_detect = 0,   // ������Ӧû�м�⵽
    prox_detect =1,       // ������Ӧ��⵽
} prox_result_t;

typedef enum {
	NORMAL_WEAR_MODE = 0,// 
	SLEEP_WEAR_MODE = 1,//
} wear_mode_t;




extern int16_t gdiff[2];
extern int16_t lp[2];
extern int16_t bl[2];
extern int16_t nv_base_data[2];
extern int16_t nv_wear_offset[2];


#define FDSLEN 4  //flash �洢�ĳ���

extern uint32_t fds_read[FDSLEN]; //flash ��
extern uint32_t fds_write[FDSLEN];//flash д
extern uint8_t wear_detect_mode;



void hx3011_power_up(void);
void hx3011_power_down(void);
void hx3011_power_restart(void);
uint8_t hx3011_check_device_id(void);
bool hx3011_prox_reg_init(void);
uint8_t hx3011_report_prox_status(void);

void hx3011_prox_factory_hanging_cali(void);
bool hx3011_prox_init(void);
void hx3011_factory_cali_save_nv(void);
int16_t hx3011_read_diff_lp(uint8_t chx_diff);
bool hx3011_get_wear_result(uint8_t ppg_wear);
bool hx3011_prox_factory_init(void);
void hx_set_wear_detect_mode(uint8_t current_mode);

#endif
