// Last Update:2019-04-03 17:15:33
/**
 * @file ts.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-04-01
 */

#include "include.h"
#include "ts.h"
#include "dbg.h"
#include "bitstream.h"
#include "crc.h"

#define TS_HEADER_LEN 4
/*
 * 6 - pes header len, start code + stream id + pes packet length
 * 3 - optional pes header len
 * 5 - pts len
 * */
#define PES_HEADER_LEN ( 6 + 3 + 5)
#define PCR_LEN 6
#define PTS_LEN 5
#define OPTIONAL_PES_HEADER_LEN 3
#define TS_PACKET_SIZE 188

static uint8_t h264_aud[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xF0 };

static int pat_write_payload( ts_packet_t *pkt, void *arg )
{
    ts_pat_t pat;
    ts_stream_t *ts = (ts_stream_t *)pkt->ts;
    uint8_t *buf_ptr = NULL;
    int i = 0;
    program_t *program_list = NULL;
    uint16_t section_length = 0;
    uint32_t crc = 0;

    if ( !pkt || !pkt->ts ) {
        return -1;
    }

    section_length = 5 + ts->param->program_count*4 + 4;// start + n loop + crc 

    buf_ptr = ts->buf_ptr;

    memset( &pat, 0, sizeof(ts_pat_t) );
    pat.table_id = 0x00;
    pat.section_syntax_indicator = 1;
    pat.transport_stream_id_hi = 0x00;
    pat.transport_stream_id_lo = 0x01;
    pat.section_length_hi = (section_length>>8) & 0x0f;
    pat.section_length_lo = section_length&0xff;
    pat.reversed_2 = 0x3;
    pat.current_next_indicator = 1;
    *(ts_pat_t *)buf_ptr = pat;
    buf_ptr += sizeof( ts_pat_t );

    program_list = ts->param->program_list;
    if ( !program_list )
        goto err;
    ASSERT( program_list );

    for ( i=0; i<ts->param->program_count; i++ ) {
        *buf_ptr++ = program_list->program_num>>8&0xff;
        *buf_ptr++ = program_list->program_num&0xff;
        ASSERT( program_list->program_map_pid < (1<<14) );
        *buf_ptr++ = (0x7 << 5) | ((program_list->program_map_pid>>8)&0x1f);
        *buf_ptr++ = program_list->program_map_pid & 0xff;
        program_list++;
    }

    crc = calc_crc( ts->buf + 5, buf_ptr - ts->buf - 5 );
    AV_WB32( buf_ptr, crc );
    buf_ptr += 4;

    ts->buf_ptr = buf_ptr;

    return 0;
err:
    return -1;
}

static ts_packet_t *new_pat( ts_stream_t *ts )
{
    ts_packet_t *pat = NULL;

    pat = (ts_packet_t *)malloc( sizeof(ts_packet_t) ); 
    if ( !pat )
        goto err;

    MEMSET( pat );
    pat->has_pointer_field = 1;
    pat->has_pcr = 0;
    pat->write_payload = pat_write_payload;
    pat->adp_filling = 0;
    pat->ts = ts;

    return pat;
err:
    return NULL;
}

static int pmt_write_payload( ts_packet_t *pkt, void *arg )
{
    ts_pmt_t pmt;
    ts_stream_t *ts = NULL;
    uint8_t *buf_ptr = NULL;
    int section_length = 0;
    int crc = 0;

    ASSERT( pkt );
    if ( !pkt || !pkt->ts || !pkt->ts->buf_ptr )
        return -1;

    memset( &pmt, 0, sizeof(ts_pmt_t) );

    ts = pkt->ts;

    ASSERT( ts->buf_ptr );
    section_length = 9 + 2*5 + 4;
    buf_ptr = ts->buf_ptr;
    pmt.table_id = 0x02;
    pmt.section_syntax_indicator = 1;
    pmt.program_number_hi = ts->param->program_list[0].program_num>>8;
    pmt.program_number_lo = ts->param->program_list[0].program_num & 0xff;
    pmt.reversed_2 = 0x3;
    pmt.current_next_indicator = 1;
    pmt.reversed_3 = 0x7;
    pmt.pcr_pid_hi = (ts->param->video_pid >> 8) & 0x1f;
    pmt.pcr_pid_lo = ts->param->video_pid & 0xff;
    pmt.reversed_4 = 0xf;
    pmt.section_length_hi = (section_length >> 8) & 0x0f;
    pmt.section_length_lo = section_length & 0xff;
    *(ts_pmt_t *)buf_ptr = pmt;
    buf_ptr += sizeof( ts_pmt_t );

    /* N loop */
    *buf_ptr++ = 0x1b;// stream_type : 1byte
    *buf_ptr++ =  (0x7 << 5) | (( ts->param->video_pid >> 8) & 0x1f);// elementary_PID : high
    *buf_ptr++ = ts->param->video_pid & 0xff;// elementary_PID : low
    *buf_ptr++ = 0xf0;// es info length
    *buf_ptr++ = 0x00;

    *buf_ptr++ = 0x0f;// stream_type : 1byte
    *buf_ptr++ =  (0x7 << 5) | (( ts->param->audio_pid >> 8) & 0x1f);// elementary_PID : high
    *buf_ptr++ = ts->param->audio_pid & 0xff;// elementary_PID : low
    *buf_ptr++ = 0xf0;// es info length
    *buf_ptr++ = 0x00;

    crc = calc_crc( ts->buf + 5, buf_ptr - ts->buf - 5 );
    AV_WB32( buf_ptr, crc );
    buf_ptr += 4;

    ts->buf_ptr = buf_ptr;

    return 0;
}

static ts_packet_t* new_pmt( ts_stream_t *ts )
{
    ts_packet_t *pmt = NULL;
    
    pmt = (ts_packet_t *)malloc( sizeof(ts_packet_t) ); 
    if ( !pmt )
        goto err;

    MEMSET( pmt );
    pmt->has_pointer_field = 1;
    pmt->has_pcr = 0;
    pmt->ts = ts;
    pmt->write_payload = pmt_write_payload;
    pmt->adp_filling = 0;
    pmt->ts = ts;

    return pmt;
err:
    return NULL;
}

static int pes_write_payload( struct ts_packet *pkt, void *arg )
{
    ts_stream_t *ts = NULL;
    ts_header_t *header = NULL;
    uint8_t *buf_ptr = NULL;
    frame_info_t *frame = (frame_info_t*)arg;
    uint16_t pes_pkt_len = 0;
    optional_pes_header_t opt;
    int64_t pts = frame->timestamp*90;
    int len = 0;
    static int offset = 0;

    if ( !pkt || !pkt->ts || !pkt->ts->buf_ptr || !frame )
        return -1;

    ts = pkt->ts;
    header = ts->header;
    buf_ptr = ts->buf_ptr;

    ASSERT( header );
    ASSERT( buf_ptr );
    ASSERT( frame );

    MEMSET( &opt );

    if ( pkt->has_pcr )
        pkt->has_pcr = 0;

    if ( header->payload_unit_start_indicator ) {
        offset = 0;
        opt.PTS_DTS_flag = 0x02;
        opt.reversed = 0x02;
        opt.PES_header_data_length = 0x05;// pts length
        pes_pkt_len = pkt->payload_len - 4 -2 ;// 4 packet start code + stream id
        //LOGI("pes_pkt_len = 0x%x\n", pes_pkt_len );
        /* packet start code prefix */
       *buf_ptr++ = 0x00;
       *buf_ptr++ = 0x00;
       *buf_ptr++ = 0x01;
       /* stream id */
       *buf_ptr++ =  frame->frame_type == FRAME_TYPE_AUDIO 
           ? ts->param->audio_stream_id : ts->param->video_stream_id;
       *buf_ptr++ = pes_pkt_len>>8;
       *buf_ptr++ = pes_pkt_len & 0xff;
       /* optional pes header */
       *(optional_pes_header_t *)buf_ptr = opt;
       buf_ptr += sizeof( optional_pes_header_t );
       /* pts */
       *buf_ptr++ = (0x02 << 4) | (pts >> 29 & 0xe ) | 0x01;
       *buf_ptr++ = pts>>22 & 0xff;
       *buf_ptr++ = ((pts>>14)&0xfe) | 0x01; 
       *buf_ptr++ = (pts>>7) & 0xff;
       *buf_ptr++ = ((pts<<1) & 0xfe ) | 0x01;
       /* aud */
       if ( frame->frame_type == FRAME_TYPE_VIDEO ) {
           memcpy( buf_ptr, h264_aud, sizeof(h264_aud) );
           buf_ptr += sizeof(h264_aud);
       }
       if ( header->adaptation_field_control == 0x03 ) {
           pkt->payload_len -= ( buf_ptr - ts->buf_ptr );
       }
    } 
    
    len = TS_PACKET_SIZE - (buf_ptr - ts->buf);
    //LOGI("len = %d\n", len );
    memcpy( buf_ptr, frame->frame + offset, len );
    buf_ptr += len;
    offset += len;

    ASSERT( len );

    if ( header->adaptation_field_control != 0x03 ) {
        pkt->payload_len -= 184;
    } else {
        pkt->payload_len -= len;
    }

    if ( header->payload_unit_start_indicator )
        header->payload_unit_start_indicator = 0;
    //LOGI("pkt->payload_len = %d\n", pkt->payload_len );

    return 0;
}

static ts_packet_t* new_pes( ts_stream_t *ts )
{
    ts_packet_t *pes = NULL;

    pes = (ts_packet_t *)malloc( sizeof(ts_packet_t) ); 
    if ( !pes )
        goto err;

    MEMSET( pes );
    pes->has_pointer_field = 0;
    pes->has_pcr = 1;
    pes->write_payload = pes_write_payload;
    pes->ts = ts;
    pes->adp_filling = 1;

    return pes;
err:
    return NULL;
}

static ts_header_t *new_ts_header()
{
    ts_header_t  *header = (ts_header_t *)malloc( sizeof(ts_header_t) );
    if ( !header ) {
        return NULL;
    }

    MEMSET( header );
    header->sync_byte = 0x47;

    return header;
}

static adp_field_t *new_adp()
{
    adp_field_t *adp = (adp_field_t*)malloc( sizeof(adp_field_t) );
    if ( !adp )
        return NULL;

    MEMSET( adp );

    return adp;
}

ts_stream_t *new_ts_stream( ts_param_t *param  )
{
    ts_stream_t *ts = (ts_stream_t *)malloc( sizeof(ts_stream_t) );

    if ( !ts || !param )
        goto err;

    ASSERT( ts );
    ASSERT( param );

    MEMSET( ts );

    ts->param = (ts_param_t*)malloc( sizeof(ts_param_t) );
    if ( !ts )
        goto err;
    ts->buf = (uint8_t *)malloc( TS_PACKET_SIZE );
    if ( !ts->buf )
        goto err;

    ts->buf_ptr = ts->buf;
    memset( ts->buf_ptr, 0, TS_PACKET_SIZE );
    ts->header = new_ts_header();
    if ( !ts->header )
        goto err;
    ts->adp = new_adp();
    if ( !ts->adp )
        goto err;

    ASSERT( ts->param );
    *ts->param = *param;
    ts->pat = new_pat( ts );
    if ( !ts->pat )
        goto err;
    ts->pmt = new_pmt( ts );
    if ( !ts->pmt )
        goto err;
    ts->pes = new_pes( ts );
    if ( !ts->pes )
        goto err;

    return ts;

err:
    return NULL;
}

static int ts_write_packet( ts_packet_t *pkt, void *arg )
{
    ts_header_t *header = NULL;
    ts_stream_t *ts = NULL;
    uint8_t *buf_ptr = NULL;
    int ret = 0;

    ASSERT( pkt );
    ASSERT( pkt->ts );
    ASSERT( pkt->ts->header );
    ASSERT( pkt->ts->buf_ptr );

    if ( !pkt || !pkt->ts || !pkt->ts->header || !pkt->ts->buf_ptr ) {
        goto err;
    }

    ts = pkt->ts;
    header = ts->header;
    buf_ptr = ts->buf_ptr;

    if ( pkt->has_pcr ) {
        LOGI("has pcr\n");
        header->adaptation_field_control = 0x03;
    } else if ( pkt->adp_filling && pkt->payload_len ) {
        if ( TS_HEADER_LEN + pkt->payload_len < TS_PACKET_SIZE ) {
            header->adaptation_field_control = 0x03;
        } else {
            header->adaptation_field_control = 0x01;
        }
    } else {
        header->adaptation_field_control = 0x01;
    }

    /* write ts header : 4bytes */
    *(ts_header_t *)buf_ptr = *header;
    buf_ptr += sizeof(*header);
    
    /* write adaption field */
    if ( header->adaptation_field_control == 0x03 ) {
        int stuff_len = 0;
        int adp_len = 1;// flags

        ASSERT( ts->adp );
        if ( !ts->adp ) {
            LOGE("check adaption field error\n");
            goto err;
        }
        if ( pkt->has_pcr ) {
            adp_len += PCR_LEN;
        }
        stuff_len = TS_PACKET_SIZE - ( TS_HEADER_LEN + 1 + adp_len + pkt->payload_len );// 1 - adaption_field_length
        //LOGI("pkt->payload_len = %d\n", pkt->payload_len );
        if ( stuff_len > 0 ) {
            adp_len += stuff_len;
        }

        ts->adp->adaption_field_length = adp_len;
        if ( pkt->has_pcr ) {
            ts->adp->PCR_flag = 0x01;
            ts->adp->random_access_indicator = 0x01;
        } else {
            ts->adp->PCR_flag = 0x00;
            ts->adp->random_access_indicator = 0x00;
        }

        *(adp_field_t *)buf_ptr = *ts->adp;
        buf_ptr += sizeof(*ts->adp);
        if ( pkt->has_pcr ) {
            // (pcr/90000 * 27000000/300)%8589934592;
            int64_t pcr_base = ts->param->pcr%8589934592; 
            // (pcr/90000 * 27000000) % 300
            int64_t pcr_ext = (ts->param->pcr * 300) % 300; 

            *buf_ptr++ = pcr_base >> 25;
            *buf_ptr++ = pcr_base >> 17;
            *buf_ptr++ = pcr_base >>  9;
            *buf_ptr++ = pcr_base >>  1;
            *buf_ptr++ = pcr_base <<  7 | pcr_ext >> 8 | 0x7e;
            *buf_ptr++ = pcr_ext;
        }

        if ( stuff_len > 0 ) {
            memset( buf_ptr, 0xff, stuff_len );
            buf_ptr += stuff_len;
        }
        
    } else if ( header->adaptation_field_control != 0x01 ){
        LOGE("check adaption field error\n");
        ASSERT( 0 );
        goto err;
    }

    /* write pointer field : 1byte */
    if ( header->payload_unit_start_indicator 
         && pkt->has_pointer_field ) {
        *buf_ptr++ = 0x00;    
    }

    ts->buf_ptr = buf_ptr;
    /* write payload */
    if ( pkt->write_payload ) {
       ret = pkt->write_payload( pkt, arg ); 
       if ( ret < 0 ) {
           LOGE("write payload error\n");
           goto err;
       }
    } else {
        LOGE("check pkt->write_payload error\n");
        ASSERT( 0 );
        goto err;
    }

    /* write stuff */
    if ( !pkt->adp_filling
         && (( ts->buf_ptr - ts->buf ) < TS_PACKET_SIZE) ) {
        memset( ts->buf_ptr, 0xff, TS_PACKET_SIZE - (ts->buf_ptr - ts->buf ) );
    }

    ts->buf_ptr = ts->buf;
    ASSERT( ts->param->output );
    if ( ts->param->output ) {
        ts->param->output( ts->buf, TS_PACKET_SIZE );
    }
    
err:
    return -1;
}

int ts_write_pat( ts_stream_t *ts )
{
    ts_header_t *header = NULL;

    if ( !ts || !ts->pat || !ts->header || !ts->param ) {
        return -1;
    }

    header = ts->header;
    header->payload_unit_start_indicator = 1;
    header->adaptation_field_control = 0x01;

    ts_write_packet( ts->pat, NULL );

    return 0;
}

int ts_write_pmt( ts_stream_t *ts )
{
    ts_header_t *header = NULL;

    if ( !ts || !ts->pat || !ts->header || !ts->param ) {
        return -1;
    }

    ASSERT( ts && ts->pat && ts->header && ts->param );

    header = ts->header;
    header->pid_hi = (ts->param->pmt_pid >>8)&0x1f;
    header->pid_lo = (ts->param->pmt_pid)&0xff;
    header->payload_unit_start_indicator = 1;
    header->adaptation_field_control = 0x01;

    ts_write_packet( ts->pmt, NULL );

    return 0;
}

int ts_write_frame( ts_stream_t *ts, frame_info_t *frame )
{
    ts_packet_t *pes = NULL;
    ts_header_t *header = NULL;
    uint16_t pid = 0;
    static uint8_t audio_counter = 0;
    static uint8_t video_counter = 0;

    if ( !ts || !ts->pat || !ts->header || !ts->param || !frame ) {
        return -1;
    }

    ASSERT( frame->len );

    header = ts->header;

    if ( frame->frame_type == FRAME_TYPE_AUDIO ) {
        pid = ts->param->audio_pid;
    } else {
        pid = ts->param->video_pid;
    }

    ASSERT( pid );
    header = ts->header;
    header->payload_unit_start_indicator = 1;
    pes = ts->pes;

    //LOGI("frame->len = %d\n", frame->len );
    pes->payload_len = frame->len + PES_HEADER_LEN;
    if ( frame->frame_type == FRAME_TYPE_VIDEO ) {
        pes->payload_len += sizeof(h264_aud);
    }
    
    //LOGI("pes->payload_len = 0x%x\n", pes->payload_len );
    while( pes->payload_len > 0 ) {
        if ( frame->frame_type == FRAME_TYPE_AUDIO ) {
            pid = ts->param->audio_pid;
            header->continuity_counter = audio_counter++;
            if ( audio_counter == 16 )
                audio_counter = 0;
        } else {
            pid = ts->param->video_pid;
            header->continuity_counter = video_counter++;
            if ( video_counter == 16 )
                video_counter = 0;
        }
        header->pid_hi = (pid >>8)&0x1f;
        header->pid_lo = (pid)&0xff;
        ts_write_packet( pes, (void*)frame );
    }

    return 0;
}

void ts_stream_destroy( ts_stream_t *ts )
{
}

