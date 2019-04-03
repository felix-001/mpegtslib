// Last Update:2019-03-22 10:59:36
/**
 * @file sim_ipc.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-03-20
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "ipc.h"
#include "dbg.h"

typedef struct {
    int running;
    ipc_dev_t *dev;
    char *video_file;
    char *audio_file;
    char *pic_file;
    int (*video_cb) ( uint8_t *frame, int len, int iskey, int64_t timestamp );
    int (*audio_cb) ( uint8_t *frame, int len, int64_t timestamp );
    int (*event_cb)( int event, void *data );
} sim_ipc_t;

typedef struct _LinkADTSFixheader {
    unsigned short syncword:		    12;
    unsigned char id:                       1;
    unsigned char layer:		    2;
    unsigned char protection_absent:        1;
    unsigned char profile:                  2;
    unsigned char sampling_frequency_index: 4;
    unsigned char private_bit:              1;
    unsigned char channel_configuration:    3;
    unsigned char original_copy:	    1;
    unsigned char home:                     1;
} LinkADTSFixheader;

typedef struct _LinkADTSVariableHeader {
    unsigned char copyright_identification_bit:		1;
    unsigned char copyright_identification_start:	1;
    unsigned short aac_frame_length:			13;
    unsigned short adts_buffer_fullness:		11;
    unsigned char number_of_raw_data_blocks_in_frame:   2;
} LinkADTSVariableHeader;


static int aacfreq[13] = {96000, 88200,64000,48000,44100,32000,24000, 22050 , 16000 ,12000,11025,8000,7350};

static void LinkParseAdtsfixedHeader(const unsigned char *pData, LinkADTSFixheader *_pHeader) {
    unsigned long long adts = 0;
    const unsigned char *p = pData;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++; adts <<= 8;
    adts |= *p ++;
    
    
    _pHeader->syncword                 = (adts >> 44);
    _pHeader->id                       = (adts >> 43) & 0x01;
    _pHeader->layer                    = (adts >> 41) & 0x03;
    _pHeader->protection_absent        = (adts >> 40) & 0x01;
    _pHeader->profile                  = (adts >> 38) & 0x03;
    _pHeader->sampling_frequency_index = (adts >> 34) & 0x0f;
    _pHeader->private_bit              = (adts >> 33) & 0x01;
    _pHeader->channel_configuration    = (adts >> 30) & 0x07;
    _pHeader->original_copy            = (adts >> 29) & 0x01;
    _pHeader->home                     = (adts >> 28) & 0x01;
}

static void LinkParseAdtsVariableHeader(const unsigned char *pData, LinkADTSVariableHeader *_pHeader) {
    unsigned long long adts = 0;
    adts  = pData[0]; adts <<= 8;
    adts |= pData[1]; adts <<= 8;
    adts |= pData[2]; adts <<= 8;
    adts |= pData[3]; adts <<= 8;
    adts |= pData[4]; adts <<= 8;
    adts |= pData[5]; adts <<= 8;
    adts |= pData[6];
    
    _pHeader->copyright_identification_bit = (adts >> 27) & 0x01;
    _pHeader->copyright_identification_start = (adts >> 26) & 0x01;
    _pHeader->aac_frame_length = (adts >> 13) & ((int)pow(2, 14) - 1);
    _pHeader->adts_buffer_fullness = (adts >> 2) & ((int)pow(2, 11) - 1);
    _pHeader->number_of_raw_data_blocks_in_frame = adts & 0x03;
}

static void * sim_ipc_video_task( void *instance )
{
    sim_ipc_t *ipc = (sim_ipc_t *)instance;
    char *p_video_buf = NULL, *p_end = NULL;
    char *tmp = NULL;
    char start_code[3] = { 0x00, 0x00, 0x01 };
    char start_code2[4] = { 0x00, 0x00, 0x00, 0x01 };
    int timestamp = 0, idr = 0;
    FILE *fp = NULL;
    int size = 0;

    if ( !ipc || !ipc->video_file ) {
        LOGE("the h264 file is NULL,should pass h264 file first\n");
        return NULL;
    }

    fp = fopen( ipc->video_file, "r" );
    if ( !fp ) {
        LOGE("open file %s error\n", ipc->video_file );
        return NULL;
    }

    fseek( fp, 0L, SEEK_END );
    size = ftell( fp );
    LOGI("video file size is %d\n", size );
    fseek( fp, 0L, SEEK_SET );
    p_video_buf = (char *)malloc( size );
    if ( !p_video_buf ) {
        LOGE("malloc error\n");
        return NULL;
    }
    fread( p_video_buf, 1, size, fp );
    tmp = p_video_buf;
    p_end = p_video_buf + size;

    while ( ipc->running && p_video_buf < p_end ) {
        int frame_len = 0, type = 0;

        memcpy( &frame_len, p_video_buf, 4 );
        p_video_buf += 4;
        if ( p_video_buf + frame_len < p_end ) {
            if ( memcmp( start_code, p_video_buf, 3 ) == 0 ) {
                type = p_video_buf[3] & 0x1f;
            } else if ( memcmp( start_code2, p_video_buf, 4 ) == 0 ) {
                type = p_video_buf[4] & 0x1f;
            } else {
                LOGE("get nalu start code fail\n");
                return NULL;
            }
            if ( type != 1 ) {
                idr++;
            }
            if ( ipc->video_cb )
                ipc->video_cb( (uint8_t *)p_video_buf, frame_len, !(type == 1), timestamp );
            p_video_buf += frame_len;
            timestamp += 40;
            usleep( 40*1000 );
        } else {
            p_video_buf = tmp;
        }
    }

    free( p_video_buf );
    return NULL;
}

int GetFileSize( char *_pFileName)
{
    FILE *fp = fopen( _pFileName, "r");
    int size = 0;

    if( !fp ) {
        LOGE("fopen file %s error\n", _pFileName );
        return -1;
    }

    fseek(fp, 0L, SEEK_END );
    size = ftell(fp);
    fclose(fp);

    return size;
}

void *sim_ipc_audio_task( void *param )
{
    sim_ipc_t *ipc = (sim_ipc_t *)param;
    if ( !ipc ) {
        LOGE("check param error\n");
        return NULL;
    }

    int len = GetFileSize( ipc->audio_file );
    if ( len <= 0 ) {
        LOGE("GetFileSize error\n");
        exit(1);
    }

    FILE *fp = fopen( ipc->audio_file, "r" );
    if( !fp ) {
        LOGE("open file %s error\n", ipc->audio_file );
        return NULL;
    }

    char *buf_ptr = (char *) malloc ( len );
    if ( !buf_ptr ) {
        LOGE("malloc error\n");
        exit(1);
    }
    memset( buf_ptr, 0, len );
    fread( buf_ptr, 1, len, fp );

    int offset = 0;
    int64_t timeStamp = 0;
    int64_t interval = 0;
    int count = 0;

    while( ipc->running ) {
        LinkADTSFixheader fix;
        LinkADTSVariableHeader var;

        if ( offset + 7 <= len ) {
            LinkParseAdtsfixedHeader( (unsigned char *)( buf_ptr + offset), &fix );
            int size = fix.protection_absent == 1 ? 7 : 9;
            //LOGI("size = %d\n", size );
            LinkParseAdtsVariableHeader( (unsigned char *)( buf_ptr + offset), &var );
            if ( offset + size + var.aac_frame_length <= len ) {
                if ( ipc->audio_cb ) {
                    count++;
                    //LOGI("fix.sampling_frequency_index = %d\n", fix.sampling_frequency_index );
                    interval = ((1024*1000.0)/aacfreq[fix.sampling_frequency_index]);
                    //LOGI("interval = %ld\n", interval );
                    timeStamp = interval * count;
                    ipc->audio_cb( (uint8_t *)buf_ptr + offset, var.aac_frame_length, timeStamp );
                }
            }
            offset += var.aac_frame_length;
            //LOGI("var.aac_frame_length = %d\n", var.aac_frame_length );
            usleep( interval*1000 );
        } else {
            offset = 0;
        }
    }

    return NULL;
}

int sim_ipc_init( struct ipc_dev_t *dev, ipc_param_t *param )
{
    sim_ipc_t *sim = NULL;

    if ( !dev || !param ) {
        LOGE("check param error\n");
        return -1;
    }

    LOGI("ipc init\n");
    if ( !dev ) {
        LOGE("check param error\n");
        return -1;
    }
    sim = (sim_ipc_t *) malloc ( sizeof(sim_ipc_t) );
    if ( !sim ) {
        LOGE("malloc error\n");
        return -1;
    }
    sim->running = 1;
    sim->dev = dev;
    sim->video_file = param->video_file;
    sim->audio_file = param->audio_file;
    sim->pic_file= param->pic_file;
    sim->video_cb = param->video_cb;
    sim->audio_cb = param->audio_cb;
    sim->event_cb = param->event_cb;
    dev->priv = (void*)sim;

    return 0;
}

static void *sim_ipc_motion_detect_task( void *arg )
{
    sim_ipc_t *ipc = (sim_ipc_t *)arg;

    if ( !ipc->event_cb ) {
        LOGE("check event_cb fail\n");
        return NULL;
    }

    for (;;) {
        sleep( 5 );
        ipc->event_cb( EVENT_MOTION_DETECTION, NULL );
        sleep( 8 );
        ipc->event_cb( EVENT_MOTION_DETECTION_DISAPEER, NULL );
    }
    return NULL;
}

static void sim_ipc_run( ipc_dev_t *dev )
{
    pthread_t thread;

    pthread_create( &thread, NULL, sim_ipc_video_task, dev->priv );
    pthread_create( &thread, NULL, sim_ipc_audio_task, dev->priv );
    pthread_create( &thread, NULL, sim_ipc_motion_detect_task, dev->priv );
}

static int sim_ipc_capture_picture( ipc_dev_t *dev, char *file )
{
    sim_ipc_t *sim = (sim_ipc_t *) dev->priv;
    char buf[512] = { 0 };

    if ( !file || !sim ) {
        LOGE("check param error\n");
        return -1;
    }
    if ( !sim->pic_file ) {
        LOGE("check pic_file error\n");
        return -1;
    }
    sprintf( buf, "cp %s %s", sim->pic_file, file );
    LOGI("cmd : %s\n", buf );
    system( buf );
    if ( sim->event_cb ) {
        sim->event_cb( EVENT_CAPTURE_PICTURE_SUCCESS, file );
    }
    return 0;
}

void sim_ipc_deinit(ipc_dev_t *dev)
{
    if ( dev->priv ) {
        free( dev->priv );
    }
}

static ipc_dev_t sim_ipc =
{
    .init = sim_ipc_init,
    .deinit = sim_ipc_deinit,
    .capture_picture = sim_ipc_capture_picture,
    .run = sim_ipc_run,
};

static void __attribute__((constructor)) sim_ipc_register()
{
    ipc_dev_register( &sim_ipc );
}

