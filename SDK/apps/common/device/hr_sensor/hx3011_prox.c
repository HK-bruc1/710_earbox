/*****************************************************************************
    * @file     hx90xx.c
    * @author
    * @date
    * @Emial
    * @brief    HX3011 module
******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "hx3011_prox.h"
#include "hx3011.h"

#ifdef TYHX_DEMO
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble_hci.h"
#include "ble_gap.h"
#include "ble_err.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "segger_rtt.h"
#include "twi_master.h"
#include "app_timer.h"
#endif

/* ��Ʒ��Ҫ�Ķ��ĵط�
#define NORMALWEARDIFF      6000     //HX3918 ������Ӧ���ʱ��Ӧ�Ľ��, һ���ǿ���������
hx_set_wear_detect_mode(NORMAL_WEAR_MODE)  // ���ֱ�ƽ�����������
hx_set_wear_detect_mode(SLEEP_WEAR_MODE)    //���ֱ���⵽����˯�ߺ���Ҫ����
*/

/* --------------------------------����˵��------------------------------------------------------------
1. ������HX3918��Ϊ�����Ӧ���ֵĿ��Ƴ���
2.  HX3918 ����3ͨ��CAP������;
3. ��ȡ��RAW DATA����ԭʼ��ģ����ת��������������,
   LP�Ǿ���һ���˲�����(�˳���Ƶ��������),
   BL��RAW���������˲����ģ��˳���Ƶ���������¶ȡ�ʪ�ȡ�ѹ��������Ư�ƣ���
4. �����ֹ���ģʽ��ʵ��ʹ��ѡ��,
    ���ֵģʽ, ������,оƬ��У׼��Ӧֵ��0����, ������Զ��,LP-BL�����һ����Ĳ�ֵ, ���ֵģʽ;
	����ֵģʽ,��Ҫ�û��ڹ�����ʱ��, �������Ҫ��¼�ֱ��Կյ�һ��BASEֵ(LPֵ), ������LP-BASEֵ�����ж�;
	���ֵģʽ������΢��, ������Ϊ�ϵ������У׼, ��������ſ����޷����������. ����ֵģʽ�޴�����.
5. 	hx3011_chip_id ��оƬID�ı���, ��ȷ��IDֵ��0x27, ��ΪHX3918 ��PPG��CAP, ����������ж��ǲ���оƬ�Ѿ��ϵ�,
	��PPG�����Ѿ���ʼ��, �������0X27, ��߿��Ʋ���Ҫ�ϵ��µ�, �����������ģʽ,CAP��������Ҫ��������,
	PPG����û��ͬʱ����ʼ��, ���Զ�����hx3011_chip_id = 0, ��������ǻ����оƬ�����µ����
6.  ע��CSx��CHx, CSx��ָ�ⲿPAD�ӵ�оƬ�Ķ�ӦCS�ܽ�. CH��ָ�ڲ��ɼ����ݵ�3��ͨ��, CSx�������ڲ�ͬ��CH��;
-------------------------------- -------------------------------- -----------------------------------*/

#define ANALOG_DIFF        //ģ����, һ��������ģʽ
//#define DIGITAL_DIFF     //���ֲ��

//#define TEMP_COMPENSATION

#ifdef ANALOG_DIFF
#define INDEPENDDENT_CH  // ����ͨ���궨��, ����һ��PAD��Ӧһ��CH
//#define DIFFERENCE_CH      //���ͨ������, ����PAD��Ϊһ��CH�Ĳ�����˺͸���, ���Ÿ첲�����Ϊ����
#endif

//#define HRS_VDD_ALWAYS_ON   //HRS ģ�鳤����

extern uint8_t hx3011_chip_id;   // оƬID, Ӧ�õ���0x27
int16_t nv_wear_offset[2] = {0};    // оƬ����У׼������ͨ����OFFSETֵ, ֵ��¼��fash����
int16_t nv_base_data[2] = {0};     // оƬ����У׼������ͨ���ĶԿ�LPֵ, ֵ��¼��flash����
//extern uint16_t sendcnt;    // �������ݲɼ�����, ����ǲ����õ�, ���Բ���
int16_t gdiff[2] = {0};      // gdiff[0], gdiff[1] �ֱ𱣴�WEAR_CH, REF_CH�Ľ��
int16_t lp[2] = {0};
int16_t bl[2] = {0};

//flash ��д����, ���μ�¼nv_wear_offset[0],nv_wear_offset[1],nv_base_data[0],nv_base_data[1]
uint32_t fds_read[4] = {0}; //����flash �洢���ݵı���;
uint32_t fds_write[4] = {0}; //дflash�ı���, flash����ֱ��¼



uint8_t  is_factory_mode = 0;    // ����ģʽ, ������ȡ�����Կ�ʱOFFSET�ͶԿյ�nv_baseֵ;
uint8_t wear_detect_mode = NORMAL_WEAR_MODE;
static bool cap_wear = false;
static bool cap_enable = false;

/**
 * set touch detect  mode
 * @param current_mode ���õĹ���ģʽ
 */
void hx_set_wear_detect_mode(uint8_t current_mode)
{
    if(current_mode>SLEEP_WEAR_MODE)
    {
        //TYHX_LOG("wear mode set fail , exceed the limit\r\n");
        return;
    }
    wear_detect_mode = current_mode;

}


/**
 * ��һ��16bit ����ֵ
 * @param addh �Ĵ�����ַ
 * @param addl �Ĵ�����ַ
*/
int16_t hx3011_read_two_reg_low_triple(uint8_t addh, uint8_t addl)
{
    uint8_t val_l = 0;
    uint8_t val_h = 0;
    uint8_t val_m= 0;
    int32_t data = 0;
		hx3011_write_reg(0x35,0x08); // man lock
    val_l = hx3011_read_reg(addl-1);
    val_m = hx3011_read_reg(addl);
    val_h = hx3011_read_reg(addh);
		hx3011_write_reg(0x35,0x00); // unlock
    data =((val_l>>4) | (val_m<<4)|((val_h & 0x0f) << 12)) ;
    if (data > 32767)
        return (int16_t)(data - 65536);
    else
        return (int16_t)data;
}

/**
 * оƬ�ϵ�,  ��Ӧ�ĵ�Դ���ƹܽŸ�ʵ�ʵ�·���,����ʵ�ʵ�·���ÿ���
* @param NULL
*/
void hx3011_power_up(void)
{
#ifdef HRS_VDD_ALWAYS_ON
    nrf_gpio_pin_set(4);		//оƬ�ĵ�Դ��
#else
    nrf_gpio_pin_set(4); //io ���� ����VDD��
#endif
    nrf_delay_ms(50);
}

/**
 * оƬ�µ�,  ��Ӧ�ĵ�Դ���ƹܽŸ�ʵ�ʵ�·���,����ʵ�ʵ�·���ÿ���
* @param NULL
*/
void hx3011_power_down(void)
{

#ifdef HRS_VDD_ALWAYS_ON
    nrf_gpio_pin_set(4);		//оƬ��Դ��
#else
    nrf_gpio_pin_clear(4); //io ���� ����VDD�ر�
#endif

    nrf_delay_ms(50);
}

/**
 * оƬ�����ϵ�
* @param NULL
*/
void hx3011_power_restart(void)
{
    hx3011_power_down();
    hx3011_power_up();
}

/**
 * HX3918 ��оƬID
* @param NULL
*/
uint8_t hx3011_check_device_id(void)
{
    return hx3011_read_reg(0x00);
}


/**
 * HX3918 дOFFSET��ֵ
* @param offset_value дOFFSET��ֵ
* @param chx_offset ͨ��0/1/2 ѡ��
*/
void  hx3011_write_offset(uint16_t offset_value, uint8_t chx_offset)
{
    uint8_t  write_val = 0;
    uint8_t byte_high =0,  byte_low = 0;

    byte_high = 0x61 + chx_offset*2;
    byte_low =	0x60 + chx_offset*2;
    write_val = ((uint8_t)(offset_value >> 8))&0xF;
    hx3011_write_reg(byte_high,write_val);
    write_val = (uint8_t)offset_value;
    hx3011_write_reg(byte_low,write_val);

}

/**
 * HX3918дOFFSET��ֵ, ���ڲ�����������OFFSET��ȡ
* @param offset_value дOFFSET��ֵ
* @param chx_offset ͨ��0/1/2 ѡ��
*/
static int16_t readlp_after_writeoffset(int16_t offset,uint8_t chx_offset)
{
    int16_t lpout;
    hx3011_write_offset(offset,chx_offset);
    hx3011_delay(32);
    lpout = hx3011_read_diff_lp(chx_offset);
    hx3011_delay(33);
    lpout = hx3011_read_diff_lp(chx_offset);
    return lpout;
}

/**
 * HX3918 ��OFFSET��ֵ
* @param chx_offset ͨ��0/1/2 ѡ��
*/
uint16_t  hx3011_read_offset(uint8_t chx_offset)
{
    uint16_t writeoffset = 0;
    int16_t lp=0;
    uint16_t offset = 0;
    uint16_t recordoffset = 0;
    uint16_t mid;
    static uint8_t cnt = 0;
    hx3011_write_reg(0x8F,0x07); 	//offset_dac_en_i2c
    recordoffset = 1020;
    cnt = 0;
    while(writeoffset<=recordoffset)
    {
        mid =(writeoffset+recordoffset)/2;
        lp = readlp_after_writeoffset(mid,chx_offset);

        if(hx_abs(lp)>READ_DEVATION)
        {
            if(lp>READ_DEVATION)
            {
                writeoffset = mid+1;
            }
            else
            {
                recordoffset = mid-1;
            }
            cnt ++;
        }
        else
        {
            offset = mid;
            //TYHX_LOG(" final ch=%d,offset =%d lp=%d\r\n",chx_offset,offset,lp);
            break;
        }
        ////TYHX_LOG(" ch=%d,lp =%d,writeoffset=%d,recordoffset=%d\r\n",chx_offset,lp,writeoffset,recordoffset);
    }
    return offset;
}


/**
 * HX3918��ȡraw data ����baseline  data  //��ȡ����raw����bl ͨ��0x38 �Ĵ�������
* @param chx_raw ͨ��0/1/2 ѡ��
*/
int16_t  hx3011_read_raw_bl(uint8_t chx_raw)
{
    int16_t raw = 0;
    uint8_t byte_high =0,  byte_low = 0;

    byte_high = 0xBE + chx_raw*3;
    byte_low = 0xBD + chx_raw*3;

    raw = hx3011_read_two_reg_low_triple(byte_high,byte_low);
    return raw;
}


/**
 * HX3918 ��ȡLP data ����diff data   //��ȡ����lp����diffͨ��0x38 �Ĵ�������
* @param chx_raw ͨ��0/1/2 ѡ��
*/
int16_t  hx3011_read_diff_lp(uint8_t chx_diff)
{
    int16_t diff = 0;
    uint8_t byte_high =0,  byte_low = 0;

    byte_high = 0xC7+ chx_diff*3;
    byte_low =	0xC6 + chx_diff*3;

    diff = hx3011_read_two_reg_low_triple(byte_high,byte_low);
    return diff;
}

/**
 * HX3918 ���ýӽ���ֵ��Զ����ֵ
* @param chx ͨ��0/1/2 ѡ��
* @param high_thres �ӽ���ֵ
* @param low_thres Զ����ֵ
* @param base_data
*/
void hx3011_config_thres(uint8_t chx,int16_t high_thres, int16_t low_thres, int16_t base_data)
{
    
    ////TYHX_LOG("[config_thres]CHX = %d, high thres =%d, low_thres=%d, base_data=%d\r\n",chx,high_thres,low_thres,base_data);
    //if ((low_thres + base_data) <= 0)
    {
      //  return;
    }
    ////TYHX_LOG("[config_thres]CHX = %d, high thres =%d, low_thres=%d\r\n",chx,high_thres,low_thres);
    int16_t thres = 0;

    //if(NORMALWEARDIFF<=5000)
	if(1)
    {
    	thres = 400/32;
    }
    else
    {    	
        thres = (high_thres + base_data)/32;
		if(thres<0)
		{
			thres = 0;
		}
    }
    //TYHX_LOG("[config_thres]CHX = %d, high thres =%d\r\n",chx,thres);
    hx3011_write_reg(0x9D+1*chx, (thres&0x00FF));
    hx3011_write_reg(0xA0+1*chx, (thres&0xFF00)>>8);
	//if(NORMALWEARDIFF<=5000)
	if(1)
    {
    	thres = 320/32;
    }
    else
    {
    	thres = (low_thres + base_data)/32;
		if(thres<0)
		{
			thres = 0;
		}
    }
    //TYHX_LOG("[config_thres]CHX = %d,low_thres=%d\r\n",chx,thres);
    hx3011_write_reg(0xA5+1*chx, (thres&0x00FF));
    hx3011_write_reg(0xA8+1*chx, (thres&0xFF00)>>8);
}


/**
 * HX3918 init
* @param NULL
*/
bool hx3011_prox_reg_init(void)
{
    bool write_ok = true;

#ifdef ANALOG_DIFF
    if (hx3011_chip_id != PROX_CHIPID)
    {
        //TYHX_LOG("100hz\r\n");
        write_ok &= hx3011_write_reg(0x18,0x40);    //prf =100HZ
        write_ok &= hx3011_write_reg(0x19,0x01);    //
    }
    write_ok &= hx3011_write_reg(0x6a,0x00);    //CH0~N disable
    write_ok &= hx3011_write_reg(0x51,0x00);    //default��
    write_ok &= hx3011_write_reg(0x52,0x02);    //default��
    write_ok &= hx3011_write_reg(0x53,0x00);    //default��


#ifdef INDEPENDDENT_CH //��ʵ��Ӳ������, Ҫ����·ͼ
    write_ok &= hx3011_write_reg(0x55,0x0C);   //CS1->CH0
    write_ok &= hx3011_write_reg(0x57,0x30);   //CS2->CH1
#elif defined(DIFFERENCE_CH) //��ʵ��Ӳ������, Ҫ����·ͼ
    write_ok &= hx3011_write_reg(0x55,0x30);   // CS2->CH1
    write_ok &= hx3011_write_reg(0x57,0x2c);   // CS1->CH0[P],CS2->CH0[N]
#endif
		write_ok &= hx3011_write_reg(0x59,0x0c);   //CH2  CS1


#ifdef FULL25PF
    write_ok &= hx3011_write_reg(0x5B,0x11);   //full range = 2.5pf 0x11=2.5pf 0x22 = 3.75pf, 0x33 = 5pf
#elif defined(FULL125PF)
		write_ok &= hx3011_write_reg(0x5B,0x00);   //full range 0x00 = 1.25pf
#elif defined(FULL375PF)
    write_ok &= hx3011_write_reg(0x5B,0x22);   //full range = 2.5pf 0x11=2.5pf 0x22 = 3.75pf, 0x33 = 5pf
#elif defined(FULL50PF)
    write_ok &= hx3011_write_reg(0x5B,0x33);   //full range = 2.5pf 0x11=2.5pf 0x22 = 3.75pf, 0x33 = 5pf
#endif
    write_ok &= hx3011_write_reg(0x5D,0x25);   //OSR AVG
    write_ok &= hx3011_write_reg(0x5E,0x22);   //OSR nosr1_num_i2c nosr2_num_i2c
    write_ok &= hx3011_write_reg(0x5F,0x11);   // AVG ch1_avg_num_i2c ch2_avg_num_i2c

    write_ok &= hx3011_write_reg(0x66,0x08);   //����ʱ��
    write_ok &= hx3011_write_reg(0x67,0x00);   //
    write_ok &= hx3011_write_reg(0x68,0x08);   //����ʱ��
    write_ok &= hx3011_write_reg(0x69,0x00);   //

    write_ok &= hx3011_write_reg(0x6E,0x11);   //LP alpha
    write_ok &= hx3011_write_reg(0x77,0x70);   //output ѡ��LP��BL

    //write_ok &= hx3011_write_reg(0x7A,0x07);   //��������У׼��������=16384

    //write_ok &= hx3011_write_reg(0x99,0x40);  // ch0 coe  ��������
		//write_ok &= hx3011_write_reg(0x9A,0x40);   // ch1 coe  ��������
		//write_ok &= hx3011_write_reg(0x94,0x02);   // diff_en_ch01
    if(is_factory_mode)
    {
        //TYHX_LOG("is_factory_mode\r\n");

        //write_ok &= hx3011_write_reg(0x70,0x44); // bl_up_alpha1
        //write_ok &= hx3011_write_reg(0x72,0x40);
        //write_ok &= hx3011_write_reg(0x73,0x04);
        write_ok &= hx3011_write_reg(0x90,(0x1<<WEAR_CH)); //ch1=P+ ch0=N-
        write_ok &= hx3011_write_reg(0x8F,0x00);//00
        write_ok &= hx3011_write_reg(0x95,0x00);
        hx3011_config_thres(WEAR_CH,WEAR_HIGH_DIFF, WEAR_LOW_DIFF,0); // 
        hx3011_config_thres(REF_CH,REF_HIGH_DIFF, REF_LOW_DIFF,0); // 
				write_ok &= hx3011_write_reg(0x6a,0x03); //CH0, CH1 enable

    }
    else
    {
        //write_ok &= hx3011_write_reg(0x70,0x44);//ch0 ch1 up
        //write_ok &= hx3011_write_reg(0x72,0x40);
        //write_ok &= hx3011_write_reg(0x73,0x04);
				write_ok &= hx3011_write_reg(0x6a,0x03); //CH0, CH1 enable
        write_ok &= hx3011_write_reg(0x90,(0x1<<WEAR_CH));////ch1=P+ ch0=N-
        write_ok &= hx3011_write_reg(0x8F,(0x1<<WEAR_CH)); // ch1 offset_dac_en
        write_ok &= hx3011_write_reg(0x95,0x00);//0xaa
        hx3011_write_offset(nv_wear_offset[0], WEAR_CH);
        hx3011_write_offset(nv_wear_offset[1], REF_CH);
        //hx3011_config_thres(CH0,CH0_HIGH_DIFF, CH0_LOW_DIFF,nv_base_data[0]); // wear0
        hx3011_config_thres(WEAR_CH,WEAR_HIGH_DIFF, WEAR_LOW_DIFF,0); // wear0
        hx3011_config_thres(REF_CH,REF_HIGH_DIFF, REF_LOW_DIFF,0);
        //hx3011_write_reg(0x7A,0x07);
    }
    

#endif
    hx3011_delay(50);
    return write_ok;
}


//�����������ݸ�APP
//extern uint32_t sendDataToClient(int16_t sendnum1,int16_t sendnum2,int16_t sendnum3,int16_t sendnum4);

/**
 * hx3918 ��ȡ������Ӧ�Ľ��,���=0, prox��Ӧ�Ľ��Ϊ�����, =1 ���
* @param NULL
*/
uint8_t hx3011_report_prox_status(void)
{

   
    lp[0] = hx3011_read_diff_lp(WEAR_CH);
    bl[0] = hx3011_read_raw_bl(WEAR_CH);

    lp[1] = hx3011_read_diff_lp(REF_CH);
    bl[1] = hx3011_read_raw_bl(REF_CH);


    gdiff[0] = lp[0]- bl[0];
    gdiff[1] = lp[1]- bl[1];

	//TYHX_LOG("SARresult = lp,%d,%d, bl,%d,%d, giff, %d,%d, offset, %d, nv_data,%d\r\n",lp[0],lp[1], bl[0],bl[1],gdiff[0],gdiff[1], nv_wear_offset[0],nv_base_data[0]);
}

uint8_t hx3011_get_prox_status(void)
{
//	hx3011_report_prox_status();
#ifdef TEMP_COMPENSATION   //�¶Ȳ���
    //�����Ӧ�Ը��»���Ҫ��, ���ô�, Ҫ�����������,
    if(gdiff[1]>15000)
    {    	
        gdiff[0] = gdiff[0]*0.9; // ������Ÿ���,��Ӧֵ����,Ϊ0.9, ��Ӧֵ�½���Ϊ1.1
    }
#endif
    if((gdiff[0]>=WEARTHRES) && (cap_enable==false))
    {
    	//TYHX_LOG("change cap to enable\r\n");
        cap_enable = true;
    }
    if(cap_enable)
    {
        if(gdiff[0]<WEARTHRES)
        {
            if((gdiff[0]>=SLEEPWEARTHRES)&&(wear_detect_mode == SLEEP_WEAR_MODE ))
            {
                cap_wear = prox_detect;
                return cap_wear;
            }
            if((cap_wear) && (gdiff[0]>=WEARTHRES_LOW))
            {
                cap_wear = prox_detect;
                return cap_wear;
            }
//            //��һ������, ��ֹBL����0

//            if((gdiff[0]<WEARTHRES_LOW)&&(bl[0]==0))
//            {
//                cap_wear = prox_no_detect;
//                //TYHX_LOG("BL cali------------------------------\r\n");
//                hx3011_write_reg(0x6a,0x00);
//                nrf_delay_ms(10);
//                hx3011_write_reg(0x6a,0x03);
//                return cap_wear;
//            }
            cap_wear = prox_no_detect;
            return cap_wear;
        }
        else
        {
            cap_wear = prox_detect;
            return prox_detect;
        }
    }
    else
    {
        cap_wear = prox_detect;
        //TYHX_LOG("cap is diable\r\n");
        return cap_wear;
    }

}

/**
 * hx3918 ��flash��ȡ�Կ�У׼���OFFSET��NV_BASE
* @param NULL
*/
void hx3011_read_nv(void)
{

    //��flash��ȡ4��ֵ, �ֱ�����ǰ�����ֵ; ��������û���Ҫ�Լ����, flash�洢��λ�ò��ܱ��û�����ɾ��
    //flash_fds_data_get(4);
    nv_wear_offset[0] = fds_read[0];
    nv_wear_offset[1] = fds_read[1];
    nv_base_data[0] = fds_read[2];
    nv_base_data[1] = fds_read[3];

	//���flash��¼��ֵ��������, ���ÿ�����Ӧ�������ֵģʽ
    if ( (nv_wear_offset[0] == 0&& nv_base_data[0] == 0) || nv_wear_offset[0] == 0xFFFF)
    {
			nv_wear_offset[0] = 0;
			nv_base_data[0] = 0;
			nv_wear_offset[1] = 0;
			nv_base_data[1] = 0;
			cap_enable = false;
    }
		//TYHX_LOG("[hx3011_read_nv]\r\n");
		//TYHX_LOG("OFF[0]:	%d,  OFF[1]:%d\r\n",nv_wear_offset[0],nv_wear_offset[1]);
		//TYHX_LOG("BASE[0]:	%d, BASE[1]:%d\r\n",nv_base_data[0],nv_base_data[1]);


    
}

/**
 * hx3918 ������Ӧ��ʼ��, ͨ����hx3011_chip_id �ж�,���жϿ�����Ӧ�Ƕ����������Ǹ�PPGһ����
* @param NULL
*/
bool hx3011_prox_init(void)
{
#ifdef DEBUG_PPG_ONLY
    return true;
#endif
    int32_t i = 0;
    bool ret = false;
    cap_wear = false;
    //TYHX_LOG("[hx3011_prox_init]prox device_id is : 0x%.2x !\r\n", hx3011_chip_id);
    if (hx3011_chip_id != PROX_CHIPID)
    {
        hx3011_power_restart();
        //i2c_hrs_init();
				twi_master_init();
        hx3011_read_nv();
        hx3011_ppg_on();
        
        for (i = 0; i<5; i++)
        {
            hx3011_chip_id = hx3011_check_device_id();
            //TYHX_LOG(" power restart ,prox device_id is : 0x%.2x !\r\n", hx3011_chip_id);
            if (hx3011_chip_id == PROX_CHIPID)
            {
								hx3011_prox_reg_init();
                break;
            }
        }

        if(hx3011_chip_id == 0)
        {
            //TYHX_LOG("no proximity device\r\n");
            //i2c_hrs_uninit();
					
            hx3011_power_down();
        }
    }
    else
    {
        hx3011_chip_id = hx3011_check_device_id();
        //TYHX_LOG("read prox device_id is : 0x%.2x !\r\n", hx3011_chip_id);
        hx3011_read_nv();
        hx3011_prox_reg_init();
    }
    hx3011_delay(50);
    return ret;
}


/**
 * hx3918 ������Ӧ��ʼ��, ͨ����hx3011_chip_id �ж�,���жϿ�����Ӧ�Ƕ����������Ǹ�PPGһ����
* @param NULL
*/
bool hx3011_prox_factory_init(void)
{
#ifdef DEBUG_PPG_ONLY
    return true;
#endif
    int32_t i = 0;
    bool ret = false;

    hx3011_chip_id = 0;

    hx3011_power_restart();
    //i2c_hrs_init();
    hx3011_ppg_on();
    hx3011_prox_reg_init();
    for (i = 0; i<5; i++)
    {
        hx3011_chip_id = hx3011_check_device_id();
        //TYHX_LOG(" power restart ,prox device_id is : 0x%.2x !\r\n", hx3011_chip_id);
        if (hx3011_chip_id == PROX_CHIPID)
        {
            break;
        }
    }
    if(hx3011_chip_id == 0)
    {
        //TYHX_LOG("no proximity device\r\n");
        //i2c_hrs_uninit();
        hx3011_power_down();
    }
    //hx3011_delay(50);
    return ret;
}


/**
 * hx3918 ������Ӧ���� �����������, �轫�ֱ����ջ���Զ����������Ӱ�����ֵ������.
* @param NULL
*/
void hx3011_prox_factory_hanging_cali(void)
{
    uint16_t offset0 = 0;
    uint16_t offset1 = 0;
    int16_t lp[2] = {0};
    //uint8_t device_id = 0x00;

    is_factory_mode = 1;
    hx3011_prox_factory_init();
    hx3011_delay(50);//delay 100ms

    offset0 = hx3011_read_offset(WEAR_CH);
    offset1 = hx3011_read_offset(REF_CH);


    lp[0] = hx3011_read_diff_lp(WEAR_CH);
    lp[1] = hx3011_read_diff_lp(REF_CH);

    ////TYHX_LOG("offset0,offset1=	%d  %d\r\n", offset0,offset1);

#ifdef INDEPENDDENT_CH
    if((offset0-offset1)<100)
    {
        //TYHX_LOG("ERR,  resize cap of ch0\r\n");
    }

#endif
    nv_wear_offset[0] = offset0;
    nv_wear_offset[1] = offset1;
    nv_base_data[0] = lp[0];
    nv_base_data[1] = lp[1];
    ////TYHX_LOG("base0,base1=	 %d  %d\r\n", lp[0],lp[1]);
    //TYHX_LOG("RESULT =	%d	%d	%d	%d\r\n", offset0,offset1,lp[0],lp[1]);

#ifdef DIFFERENCE_CH
    //��������ݶ�����OFFSET, NV_BASE��¼��FLASH����, У׼�洢����ѧϰ�ļ�¼Ҳ��Ҫ������¼���

    if((offset0 == 510) &&(offset1 == 510)&&(lp[0]==0)&&(lp[1]==0))
    {
        //TYHX_LOG("ERR,  read offset err, please power down and up, redo this!!!\r\n");
    }
    else
    {
        hx3011_factory_cali_save_nv();
    }
#endif
    hx3011_ppg_off();
    is_factory_mode = 0;

}

/**
 * hx3918 ����У׼���ֵ��FLASH, �˴�FLASH�ռ䲻�ܱ��û�����,
 Ҳ���ܱ��������õȲ�����ʽ����;
* @param NULL
*/
void hx3011_factory_cali_save_nv(void)
{

    fds_write[0] = nv_wear_offset[0];
    fds_write[1] = nv_wear_offset[1];

    fds_write[2] = nv_base_data[0];
    fds_write[3] = nv_base_data[1];


    //TYHX_LOG("[hx3011_factory_cali_save_nv]");
    for(int i=0; i<4; i++)
    {
        //TYHX_LOG("%d   ",fds_write[i]);
    }
	//�û���Ҫflash����
    //flash_fds_data_write(4);
}

/**
 * ������IR��PROX�ۺ����ж�������
* @param ppg_wear, PPG������IR���Ľ��
*/
bool hx3011_get_wear_result(uint8_t ppg_wear)
{
    uint8_t prox_result;
#ifdef DEBUG_PPG_ONLY
    return ppg_wear;
#endif
	
	prox_result = hx3011_get_prox_status();
	
    if(ppg_wear == 0)
    {
        return false;
    }
    else
    {
        
        if(prox_result == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}






