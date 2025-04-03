#include "system/includes.h"
#include "cpu/includes.h"
#include "app_config.h"



void sdpg_config(int enable)
{

    if (enable == 0) {                  //断电，IO设高阻
        JL_IOMC->STPG_CON |=  BIT(0);
        JL_PORTP->HD  |=  BIT(2);
        udelay(1);

        JL_PORTP->OUT &= ~BIT(2);
        JL_PORTP->DIR &= ~BIT(2);
        mdelay(2);
        JL_IOMC->STPG_CON &= ~BIT(0);
        JL_PORTP->HD  &= ~BIT(2);

        JL_PORTP->PU  &= ~BIT(2);
        JL_PORTP->PD  &= ~BIT(2);
        JL_PORTP->DIE &= ~BIT(2);
        JL_PORTP->DIR |=  BIT(2);

    } else if (enable <= 4) {           //供电，IO_HD常开

        if (JL_PORTP->HD & BIT(2)) {
            return;
        }

        JL_IOMC->STPG_CON |=  BIT(0);
        JL_PORTP->HD  |=  BIT(2);
        udelay(1);

        JL_PORTP->OUT |=  BIT(2);
        JL_PORTP->DIR &= ~BIT(2);
        mdelay(4);
        JL_IOMC->STPG_CON &= ~BIT(0);

    } else {                            //断电，IO输出0

        JL_IOMC->STPG_CON |=  BIT(0);
        JL_PORTP->HD  |=  BIT(2);
        udelay(1);

        JL_PORTP->OUT &= ~BIT(2);
        JL_PORTP->DIR &= ~BIT(2);
        mdelay(2);
        JL_IOMC->STPG_CON &= ~BIT(0);
        JL_PORTP->HD  &= ~BIT(2);
    }
}
