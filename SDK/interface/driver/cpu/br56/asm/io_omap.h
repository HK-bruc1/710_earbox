
//===============================================================================//
//
//      output function define
//
//===============================================================================//
#define FO_GP_OCH0         ((0 << 2)|BIT(1))
#define FO_GP_OCH1         ((1 << 2)|BIT(1))
#define FO_GP_OCH2         ((2 << 2)|BIT(1))
#define FO_GP_OCH3         ((3 << 2)|BIT(1))
#define FO_GP_OCH4         ((4 << 2)|BIT(1))
#define FO_GP_OCH5         ((5 << 2)|BIT(1))
#define FO_GP_OCH6         ((6 << 2)|BIT(1))
#define FO_GP_OCH7         ((7 << 2)|BIT(1))
#define FO_SD0_CLK        ((8 << 2)|BIT(1)|BIT(0))
#define FO_SD0_CMD        ((9 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA0        ((10 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA1        ((11 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA2        ((12 << 2)|BIT(1)|BIT(0))
#define FO_SD0_DA3        ((13 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_CLK        ((14 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA0        ((15 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA1        ((16 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA2        ((17 << 2)|BIT(1)|BIT(0))
#define FO_SPI1_DA3        ((18 << 2)|BIT(1)|BIT(0))
#define FO_IIC0_SCL        ((19 << 2)|BIT(1)|BIT(0))
#define FO_IIC0_SDA        ((20 << 2)|BIT(1)|BIT(0))
#define FO_UART0_TX        ((21 << 2)|BIT(1)|BIT(0))
#define FO_UART1_TX        ((22 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM0_H        ((23 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM1_H        ((24 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM0_L        ((25 << 2)|BIT(1)|BIT(0))
#define FO_MCPWM1_L        ((26 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_MCLK        ((27 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_SCLK        ((28 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_LRCK        ((29 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT0        ((30 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT1        ((31 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT2        ((32 << 2)|BIT(1)|BIT(0))
#define FO_ALNK0_DAT3        ((33 << 2)|BIT(1)|BIT(0))
#define FO_PLED_IO_1        ((34 << 2)|BIT(1)|BIT(0))
#define FO_PLED_IO_0        ((35 << 2)|BIT(1)|BIT(0))

//===============================================================================//
//
//      IO output select sfr
//
//===============================================================================//
typedef struct {
    __RW __u8 PA0_OUT;
    __RW __u8 PA1_OUT;
    __RW __u8 PA2_OUT;
    __RW __u8 PA3_OUT;
    __RW __u8 PA4_OUT;
    __RW __u8 PA5_OUT;
    __RW __u8 PA6_OUT;
    __RW __u8 PB0_OUT;
    __RW __u8 PB1_OUT;
    __RW __u8 PB2_OUT;
    __RW __u8 PB3_OUT;
    __RW __u8 PB4_OUT;
    __RW __u8 PB5_OUT;
    __RW __u8 PB6_OUT;
    __RW __u8 PB7_OUT;
    __RW __u8 PB8_OUT;
    __RW __u8 PB9_OUT;
    __RW __u8 PB10_OUT;
    __RW __u8 PB11_OUT;
    __RW __u8 PC0_OUT;
    __RW __u8 PC1_OUT;
    __RW __u8 PC2_OUT;
    __RW __u8 PC3_OUT;
    __RW __u8 PC4_OUT;
    __RW __u8 USBDP_OUT;
    __RW __u8 USBDM_OUT;
    __RW __u8 PP0_OUT;
    __RW __u8 PP1_OUT;
    __RW __u8 PP2_OUT;
} JL_OMAP_TypeDef;

#define JL_OMAP_BASE      (ls_base + map_adr(0x36, 0x00))
#define JL_OMAP           ((JL_OMAP_TypeDef   *)JL_OMAP_BASE)


