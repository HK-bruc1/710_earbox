

	OVERLAY : AT(0x800000) SUBALIGN(4)
    {
        .overlay_bank0
        {
            *(.bank.code.0)
        }
        .overlay_bank1
        {
            *(.bank.code.1)
        }
        .overlay_bank2
        {
            *(.bank.code.2)
        }
        .overlay_bank3
        {
            *(.bank.code.3)
        }
        .overlay_bank4
        {
            *(.bank.code.4)
        }
        .overlay_bank5
        {
            *(.bank.code.5)
        }
        .overlay_bank6
        {
            *(.bank.code.6)
        }
        .overlay_bank7
        {
            *(.bank.code.7)
        }
        .overlay_bank8
        {
            *(.bank.code.8)
        }
        .overlay_bank9
        {
            *(.bank.code.9)
        }
        .overlay_bank10
        {
            *(.bank.code.10)
        }
    } > ram0



