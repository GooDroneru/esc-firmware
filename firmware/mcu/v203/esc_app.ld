
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00001000, LENGTH = 60K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

INCLUDE ch32v20x.ld

INCLUDE ../../../dependencies/common/esc/esc_sectors.ld

/* Firmware must fit within available flash (total flash minus bootloader area) */
ASSERT(_etext <= ORIGIN(FLASH) + LENGTH(FLASH), "ERROR: Firmware exceeds available flash size (max = LENGTH(FLASH) - _app_start_addr)")
