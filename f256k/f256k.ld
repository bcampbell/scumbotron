/* foenix uses first 16 bytes for mmu control? */
__rc0 = 0x10;
INCLUDE imag-regs.ld
ASSERT(__rc0 == 0x10, "Inconsistent zero page map.")
ASSERT(__rc31 == 0x2f, "Inconsistent zero page map.")


MEMORY {
    zp : ORIGIN = __rc31 + 1, LENGTH = 0x100 - (__rc31 + 1)
    ram (rw) : ORIGIN = 0x200, LENGTH = 0x2000-0x200    /* 0x10 for ROM header */
    rom (rw) : ORIGIN = 0x2010, LENGTH = 0xC000-0x2010
}



REGION_ALIAS("c_readonly", rom)
REGION_ALIAS("c_writeable", ram)

SECTIONS { INCLUDE c.ld }

/* Set initial soft stack address to just above last ram address. (It grows down.) */
__stack = ORIGIN(ram) + LENGTH(ram);

OUTPUT_FORMAT {
    BYTE(0xf2)
    BYTE(0x56)
    BYTE(5)     /* size (num of 8KB blocks) */
    BYTE(1)     /* starting slot */
    SHORT(_start)

    /* program name */
    BYTE(0x50)  /* 'P' */
    BYTE(0x00)

    BYTE(0x00)
    BYTE(0x00)
    BYTE(0x00)
    BYTE(0x00)
    BYTE(0x00)
    BYTE(0x00)
    BYTE(0x00)
    BYTE(0x00)

    FULL(rom)
}

