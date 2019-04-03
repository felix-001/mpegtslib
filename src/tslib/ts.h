// Last Update:2019-04-03 15:25:20
/**
 * @file ts.h
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-04-01
 */

#ifndef TS_H
#define TS_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;

struct ts_stream;

typedef struct {
    int program_num;
    int program_map_pid;
} program_t;

typedef struct {
    uint8_t audio_stream_id;
    uint8_t video_stream_id;
    uint16_t pmt_pid;
    uint16_t audio_pid;
    uint16_t video_pid;
    int64_t pcr;
    uint8_t program_count;
    program_t program_list[32];
    int (*output)( uint8_t *buf, uint32_t len );// output ts packet to file
} ts_param_t;

typedef struct{
#ifdef WORDS_BIGENDIAN
    u8 sync_byte                            :8;   /* always be 0x47 */
    u8 transport_error_indicator            :1;
    u8 payload_unit_start_indicator         :1;   /* if a PES-packet starts in the TS-packet */
    u8 transport_priority                   :1;   /* meanless to IRD, can be ignored */
    u8 pid_hi                               :5;
    u8 pid_lo                               :8;
    u8 transport_scrambling_control         :2;   /* 00: no scramble, 01: reserved,
                                                           10: even key scrambled, 11: odd key scrambled */
    u8 adaptation_field_control             :2;   /* 00: reserved
                                                           01: no adaptation field, payload only
                                                           10: adaptation field only, no payload
                                                           11: adaptation field followed by payload */
    u8 continuity_counter                   :4;
#else
    u8 sync_byte                            :8;
    u8 pid_hi                               :5;
    u8 transport_priority                   :1;
    u8 payload_unit_start_indicator         :1;
    u8 transport_error_indicator            :1;
    u8 pid_lo                               :8;
    u8 continuity_counter                   :4;
    u8 adaptation_field_control             :2;
    u8 transport_scrambling_control         :2;
#endif
} ts_header_t;

typedef struct{
    u8 adaption_field_length                :8;
    u8 adaption_file_extension_flag         :1;
    u8 transport_private_data_flag          :1;
    u8 splicing_point_flag                  :1;
    u8 OPCR_flag                            :1;
    u8 PCR_flag                             :1;
    u8 elementary_stream_priority_indicator :1;
    u8 random_access_indicator              :1;
    u8 discontinuity_indicator              :1;
} adp_field_t;

#define PAT_SECT_HEADER_LEN                  8
typedef struct{
#ifdef WORDS_BIGENDIAN
    u8 table_id                             :8;
    u8 section_syntax_indicator             :1;
    u8                                      :3;
    u8 section_length_hi                    :4;
    u8 section_length_lo                    :8;
    u8 transport_stream_id_hi               :8;
    u8 transport_stream_id_lo               :8;
    u8 reversed_2                           :2;
    u8 version_number                       :5;
    u8 current_next_indicator               :1;
    u8 section_number                       :8;
    u8 last_section_number                  :8;
#else
    u8 table_id                             :8;
    u8 section_length_hi                    :4;
    u8                                      :3;
    u8 section_syntax_indicator             :1;
    u8 section_length_lo                    :8;
    u8 transport_stream_id_hi               :8;
    u8 transport_stream_id_lo               :8;
    u8 current_next_indicator               :1;
    u8 version_number                       :5;
    u8 reversed_2                           :2;
    u8 section_number                       :8;
    u8 last_section_number                  :8;
#endif
} ts_pat_t;


typedef struct ts_packet {
    uint8_t has_pointer_field;
    /*
     * pat & pmt : filling 0xff at the end, don't use adaption field 
     * pes       : use adaption field to filling if 188 bytes not enough
     */
    uint8_t adp_filling;
    struct ts_stream *ts;
    int payload_len;
    uint8_t has_pcr;
    int (*write_payload)( struct ts_packet *pkt, void *arg );
} ts_packet_t;

typedef struct {
    uint8_t frame_type;
    uint8_t *frame;
    uint32_t len;
    int64_t timestamp;
} frame_info_t;

typedef struct  ts_stream {
    ts_param_t *param;
    ts_packet_t *pat;
    ts_packet_t *pmt;
    ts_packet_t *pes;
    ts_header_t *header;
    adp_field_t *adp;
    uint8_t *buf_ptr;
    uint8_t *buf;
} ts_stream_t;

#define PMT_SECT_HEADER_LEN                  12
typedef struct{
#ifdef WORDS_BIGENDIAN
    u8 table_id                             :8;
    u8 section_syntax_indicator             :1;
    u8                                      :3;
    u8 section_length_hi                    :4;
    u8 section_length_lo                    :8;
    u8 program_number_hi                    :8;
    u8 program_number_lo                    :8;
    u8 reversed_2                           :2;
    u8 version_number                       :5;
    u8 current_next_indicator               :1;
    u8 section_number                       :8;
    u8 last_section_number                  :8;
    u8 reversed_3                           :3;
    u8 pcr_pid_hi                           :5;
    u8 pcr_pid_lo                           :8;
    u8 reversed_4                           :4;
    u8 program_info_length_hi               :4;
    u8 program_info_length_lo               :8;
#else
    u8 table_id                             :8;
    u8 section_length_hi                    :4;
    u8                                      :3;
    u8 section_syntax_indicator             :1;
    u8 section_length_lo                    :8;
    u8 program_number_hi                    :8;
    u8 program_number_lo                    :8;
    u8 current_next_indicator               :1;
    u8 version_number                       :5;
    u8 reversed_2                           :2;
    u8 section_number                       :8;
    u8 last_section_number                  :8;
    u8 pcr_pid_hi                           :5;
    u8 reversed_3                           :3;
    u8 pcr_pid_lo                           :8;
    u8 program_info_length_hi               :4;
    u8 reversed_4                           :4;
    u8 program_info_length_lo               :8;
#endif
} ts_pmt_t;

typedef struct {
#ifdef WORDS_BIGENDIAN
    u8                                      :2;
    u8 PES_scrambling_control               :2;
    u8 PES_priority                         :1;
    u8 data_alignment_indicator             :1;
    u8 copyright                            :1;
    u8 original_or_copy                     :1;
    u8 PTS_DTS_flag                         :2;
    u8 ESCR_flag                            :1;
    u8 ES_rate_flag                         :1;
    u8 DSM_trick_mode_flag                  :1;
    u8 additional_copyright_info_flag       :1;
    u8 PES_crc_flag                         :1;
    u8 PES_extension_flag                   :1;
    u8 PES_header_data_length               :8;
#else
    u8 original_or_copy                     :1;
    u8 copyright                            :1;
    u8 PES_priority                         :1;
    u8 data_alignment_indicator             :1;
    u8 PES_scrambling_control               :2;
    u8 reversed                             :2;
    u8 PES_extension_flag                   :1;
    u8 PES_crc_flag                         :1;
    u8 additional_copyright_info_flag       :1;
    u8 DSM_trick_mode_flag                  :1;
    u8 ES_rate_flag                         :1;
    u8 ESCR_flag                            :1;
    u8 PTS_DTS_flag                         :2;
    u8 PES_header_data_length               :8;
#endif
} optional_pes_header_t;


enum {
    FRAME_TYPE_AUDIO,
    FRAME_TYPE_VIDEO
};

#define MEMSET( ptr ) memset( ptr, 0, sizeof(*ptr) )

extern ts_stream_t *new_ts_stream( ts_param_t *param  );
extern int ts_write_pmt( ts_stream_t *ts );
extern int ts_write_pat( ts_stream_t *ts );
extern int ts_write_frame( ts_stream_t *ts, frame_info_t *frame );

#endif  /*TS_H*/
