#ifndef __IIC_HW_H__
#define __IIC_HW_H__

#define HW_IIC0_IRQ_PRIORITY  3//中断优先级，范围:0~7(低~高)
#define MAX_HW_IIC_NUM                  1
// typedef enum {
//     HW_IIC_0,
//     // HW_IIC_1,
// } hw_iic_dev;

enum {
    HW_IIC_0,
    // HW_IIC_1,
};


#endif

