#ifndef ADTS_HEADER_H_
#define ADTS_HEADER_H_

#include <cstdint>

struct AdtsHeader {
    uint32_t syncword;            // 12 bit 同步字 '1111 1111 1111'，一个ADTS帧的开始
    uint8_t id;                   // 1 bit 0代表MPEG-4, 1代表MPEG-2。
    uint8_t layer;                // 2 bit 必须为0
    uint8_t protection_absent;    // 1 bit 1代表没有CRC，0代表有CRC
    uint8_t profile;              // 1 bit AAC级别（MPEG-2 AAC中定义了3种profile，MPEG-4 AAC中定义了6种profile）
    uint8_t sampling_freq_index;  // 4 bit 采样率
    uint8_t private_bit;          // 1bit 编码时设置为0，解码时忽略
    uint8_t channel_cfg;          // 3 bit 声道数量
    uint8_t original_copy;        // 1bit 编码时设置为0，解码时忽略
    uint8_t home;                 // 1 bit 编码时设置为0，解码时忽略

    uint8_t copyright_identification_bit;    // 1 bit 编码时设置为0，解码时忽略
    uint8_t copyright_identification_start;  // 1 bit 编码时设置为0，解码时忽略
    uint32_t aac_frame_length;               // 13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
    uint32_t adts_buffer_fullness;           // 11 bit 缓冲区充满度，0x7FF说明是码率可变的码流，不需要此字段。CBR可能需要此字段，不同编码器使用情况不同。这个在使用音频编码的时候需要注意。

    /* number_of_raw_data_blocks_in_frame
     * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
     * 所以说number_of_raw_data_blocks_in_frame == 0
     * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
     */
    uint8_t number_of_raw_data_block_in_frame;  // 2 bit
};

bool ParseAdtsHeader(const uint8_t *frame_buf, const int frame_buf_size, AdtsHeader *header);

#endif
