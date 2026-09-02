#include "eeprom.h"
#include "string.h"
#include "plib5t_flash.h"

/* K1921VG5T main flash: 512 pages x 1 KB, 8-byte (2 words) programming granule.
 * Addresses passed in carry the 0x08000000 bus offset — the FLASH controller
 * ignores ADDR bits 31-19, so they behave exactly like on K1921VK035. */

void save_flash_nolib(uint8_t *data, int length, uint32_t add)
{
    uint32_t data_to_FLASH[MEM_FLASH_BUS_WIDTH_WORDS];
    uint32_t chunk = sizeof(data_to_FLASH);

    if (length < (int)chunk) {
        length = chunk;
    }
    if (length > 1024) {
        length = 1024;
    }

    if (add % MEM_FLASH_PAGE_SIZE == 0) {
        FLASH_ErasePage(add, FLASH_Region_Main);
    }

    for (uint32_t i = 0; i < (uint32_t)length; i += chunk) {
        for (uint32_t w = 0; w < MEM_FLASH_BUS_WIDTH_WORDS; w++) {
            data_to_FLASH[w] = 0xFFFFFFFFu;
        }
        uint32_t bytes_left = (uint32_t)length - i;
        for (uint32_t b = 0; b < chunk && b < bytes_left; b++) {
            ((uint8_t *)data_to_FLASH)[b] = data[i + b];
        }
        FLASH_WriteData(add + i, data_to_FLASH, FLASH_Region_Main);
    }
}

void read_flash_bin(uint8_t* data , uint32_t add , int out_buff_len)
{
    uint32_t rawData[MEM_FLASH_BUS_WIDTH_WORDS];
    uint32_t chunk = sizeof(rawData);

    for (uint32_t i = 0; i < (uint32_t)out_buff_len; i += chunk) {
        FLASH_ReadData(add + i, rawData, FLASH_Region_Main);
        for (uint32_t b = 0; b < chunk && (uint32_t)out_buff_len > i + b; b++) {
            data[i + b] = ((uint8_t *)rawData)[b];
        }
    }
}
