#include "adts_header.h"

bool ParseAdtsHeader(const uint8_t *frame_buf, const int frame_buf_size, AdtsHeader *header) {
    if (frame_buf_size < 7) {
        return false;
    }

    if (frame_buf[0] == 0xFF && (frame_buf[1]&0xF0) == 0xF0) {
        header->id = ((uint8_t)frame_buf[1] & 0x08) >> 3;
        header->layer = ((uint8_t)frame_buf[1] & 0x06) >> 1;
        header->protection_absent = (uint8_t)frame_buf[1] & 0x01;
        header->profile = ((uint8_t)frame_buf[2] & 0xc0) >> 6;
        header->sampling_freq_index = ((uint8_t)frame_buf[2] & 0x3c) >> 2;
        header->private_bit = ((uint8_t)frame_buf[2] & 0x02) >> 1;
        header->channel_cfg = ((((uint8_t)frame_buf[2] & 0x01) << 2) | (((unsigned int)frame_buf[3] & 0xc0) >> 6));
        header->original_copy = ((uint8_t)frame_buf[3] & 0x20) >> 5;
        header->home = ((uint8_t)frame_buf[3] & 0x10) >> 4;
        header->copyright_identification_bit = ((uint8_t)frame_buf[3] & 0x08) >> 3;
        header->copyright_identification_start = (uint8_t)frame_buf[3] & 0x04 >> 2;

        header->aac_frame_length = (((((unsigned int)frame_buf[3]) & 0x03) << 11) |
            (((unsigned int)frame_buf[4] & 0xFF) << 3) |
            ((unsigned int)frame_buf[5] & 0xE0) >> 5);
        
        header->adts_buffer_fullness = (((unsigned int)frame_buf[5] & 0x1f) << 6 |
            ((unsigned int)frame_buf[6] & 0xfc) >> 2);
        header->number_of_raw_data_block_in_frame = ((uint8_t)frame_buf[6] & 0x03);

        return true;
    }

    return false;
}
