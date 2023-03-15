#ifndef RTP_H_
#define RTP_H_

#include <cstdint>

#define RTP_VERSION              2

#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97

#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400

 /*
  *    0                   1                   2                   3
  *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |                           timestamp                           |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |           synchronization source (SSRC) identifier            |
  *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
  *   |            contributing source (CSRC) identifiers             |
  *   :                             ....                              :
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  */
struct RtpHeader
{
    /* byte 0 */
    uint8_t csrc_length : 4;//CSRC¼ÆÊýÆ÷£¬Õ¼4Î»£¬Ö¸Ê¾CSRC ±êÊ¶·ûµÄ¸öÊý¡£
    uint8_t extension : 1;//Õ¼1Î»£¬Èç¹ûX=1£¬ÔòÔÚRTP±¨Í·ºó¸úÓÐÒ»¸öÀ©Õ¹±¨Í·¡£
    uint8_t padding : 1;//Ìî³ä±êÖ¾£¬Õ¼1Î»£¬Èç¹ûP=1£¬ÔòÔÚ¸Ã±¨ÎÄµÄÎ²²¿Ìî³äÒ»¸ö»ò¶à¸ö¶îÍâµÄ°ËÎ»×é£¬ËüÃÇ²»ÊÇÓÐÐ§ÔØºÉµÄÒ»²¿·Ö¡£
    uint8_t version : 2;//RTPÐ­ÒéµÄ°æ±¾ºÅ£¬Õ¼2Î»£¬µ±Ç°Ð­Òé°æ±¾ºÅÎª2¡£

    /* byte 1 */
    uint8_t payload_type : 7;//ÓÐÐ§ÔØºÉÀàÐÍ£¬Õ¼7Î»£¬ÓÃÓÚËµÃ÷RTP±¨ÎÄÖÐÓÐÐ§ÔØºÉµÄÀàÐÍ£¬ÈçGSMÒôÆµ¡¢JPEMÍ¼ÏñµÈ¡£
    uint8_t marker : 1;//±ê¼Ç£¬Õ¼1Î»£¬²»Í¬µÄÓÐÐ§ÔØºÉÓÐ²»Í¬µÄº¬Òå£¬¶ÔÓÚÊÓÆµ£¬±ê¼ÇÒ»Ö¡µÄ½áÊø£»¶ÔÓÚÒôÆµ£¬±ê¼Ç»á»°µÄ¿ªÊ¼¡£

    /* bytes 2,3 */
    uint16_t seq;//Õ¼16Î»£¬ÓÃÓÚ±êÊ¶·¢ËÍÕßËù·¢ËÍµÄRTP±¨ÎÄµÄÐòÁÐºÅ£¬Ã¿·¢ËÍÒ»¸ö±¨ÎÄ£¬ÐòÁÐºÅÔö1¡£½ÓÊÕÕßÍ¨¹ýÐòÁÐºÅÀ´¼ì²â±¨ÎÄ¶ªÊ§Çé¿ö£¬ÖØÐÂÅÅÐò±¨ÎÄ£¬»Ö¸´Êý¾Ý¡£

    /* bytes 4-7 */
    uint32_t timestamp;//Õ¼32Î»£¬Ê±´Á·´Ó³ÁË¸ÃRTP±¨ÎÄµÄµÚÒ»¸ö°ËÎ»×éµÄ²ÉÑùÊ±¿Ì¡£½ÓÊÕÕßÊ¹ÓÃÊ±´ÁÀ´¼ÆËãÑÓ³ÙºÍÑÓ³Ù¶¶¶¯£¬²¢½øÐÐÍ¬²½¿ØÖÆ¡£

    /* bytes 8-11 */
    uint32_t ssrc;//Õ¼32Î»£¬ÓÃÓÚ±êÊ¶Í¬²½ÐÅÔ´¡£¸Ã±êÊ¶·ûÊÇËæ»úÑ¡ÔñµÄ£¬²Î¼ÓÍ¬Ò»ÊÓÆµ»áÒéµÄÁ½¸öÍ¬²½ÐÅÔ´²»ÄÜÓÐÏàÍ¬µÄSSRC¡£

   /*±ê×¼µÄRTP Header »¹¿ÉÄÜ´æÔÚ 0-15¸öÌØÔ¼ÐÅÔ´(CSRC)±êÊ¶·û
   
   Ã¿¸öCSRC±êÊ¶·ûÕ¼32Î»£¬¿ÉÒÔÓÐ0¡«15¸ö¡£Ã¿¸öCSRC±êÊ¶ÁË°üº¬ÔÚ¸ÃRTP±¨ÎÄÓÐÐ§ÔØºÉÖÐµÄËùÓÐÌØÔ¼ÐÅÔ´

   */
};

struct RtpPacket
{
    struct RtpHeader header;
    uint8_t payload[0];
};

#endif //RTP_H_