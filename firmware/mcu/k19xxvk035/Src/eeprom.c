#include "eeprom.h"
#include "string.h"
#include "plib035_mflash.h"

#define page_size 0x400                 

void save_flash_nolib(uint8_t *data, int length, uint32_t add)
{   
    uint32_t start_addr;
    uint32_t data_to_FLASH[2];

    if(length < 8) {
        length = 8;
    }
    if(length > 1024) {
        length = 1024;
    }

    start_addr = add;
    MFLASH_ErasePage(start_addr, MFLASH_Region_Main);

    for(uint32_t i = 0; i < (uint32_t)length; i += 8) {
        data_to_FLASH[0] = 0xFFFFFFFFu;
        data_to_FLASH[1] = 0xFFFFFFFFu;

        uint32_t bytes_left = (uint32_t)length - i;
        uint32_t first_word_bytes = (bytes_left >= 4u) ? 4u : bytes_left;
        memcpy(&data_to_FLASH[0], &data[i], first_word_bytes);

        if (bytes_left > 4u) {
            uint32_t second_word_bytes = bytes_left - 4u;
            if (second_word_bytes > 4u) {
                second_word_bytes = 4u;
            }
            memcpy(&data_to_FLASH[1], &data[i + 4u], second_word_bytes);
        }

        MFLASH_WriteData(start_addr + i, data_to_FLASH, MFLASH_Region_Main);
    }
}

void read_flash_bin(uint8_t* data , uint32_t add , int out_buff_len)
{
    uint32_t rawData[2] = {0};
    uint32_t j = 0;
    for(uint32_t i = 0; i < out_buff_len; i += 8) {
        MFLASH_ReadData(add + (j * 8), rawData, MFLASH_Region_Main);
        if ((i + 0u) < (uint32_t)out_buff_len) data[i + 0u] = (uint8_t)rawData[0];
        if ((i + 1u) < (uint32_t)out_buff_len) data[i + 1u] = (uint8_t)(rawData[0]>>8);
        if ((i + 2u) < (uint32_t)out_buff_len) data[i + 2u] = (uint8_t)(rawData[0]>>16);
        if ((i + 3u) < (uint32_t)out_buff_len) data[i + 3u] = (uint8_t)(rawData[0]>>24);
        if ((i + 4u) < (uint32_t)out_buff_len) data[i + 4u] = (uint8_t)rawData[1];
        if ((i + 5u) < (uint32_t)out_buff_len) data[i + 5u] = (uint8_t)(rawData[1]>>8);
        if ((i + 6u) < (uint32_t)out_buff_len) data[i + 6u] = (uint8_t)(rawData[1]>>16);
        if ((i + 7u) < (uint32_t)out_buff_len) data[i + 7u] = (uint8_t)(rawData[1]>>24);
        
        j++;
    }
}
