// Last Update:2019-04-03 21:06:37
/**
 * @file sample.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-04-03
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "ipc.h"
#include "ts.h"

typedef struct {
    ts_stream_t *ts;
    int running;
    FILE *fp;
} app_t;

static app_t app;

int VideoFrameCallBack ( uint8_t *frame, int len, int iskey, int64_t timestamp )
{
    frame_info_t frame_info;

    frame_info.frame = frame;
    frame_info.timestamp = timestamp;
    frame_info.len = len;
    frame_info.frame_type = FRAME_TYPE_VIDEO;
    ts_write_frame( app.ts, &frame_info );

    return 0;
}

int AudioFrameCallBack ( uint8_t *frame, int len, int64_t timestamp )
{
    frame_info_t frame_info;

    frame_info.frame = frame;
    frame_info.timestamp = timestamp;
    frame_info.len = len;
    frame_info.frame_type = FRAME_TYPE_AUDIO;
    ts_write_frame( app.ts, &frame_info );
    return 0;
}

static int packet_output( uint8_t *buf, uint32_t len )
{
    if ( !buf )
        return -1;

    if ( !app.fp ) {
        app.fp = fopen( "./output.ts", "w+");
        if ( !app.fp )
            return -1;
    }

    fwrite( buf, 1, len, app.fp );

    return 0;
}

int main()
{
    ipc_param_t param =
    {
        .audio_type = AUDIO_AAC,
        .video_file = "./video.h264",
        .audio_file = "./audio.aac",
        .pic_file = NULL,
        .video_cb = VideoFrameCallBack,
        .audio_cb = AudioFrameCallBack,
        .event_cb = NULL,
    };

    ts_param_t ts_param =
    {
        .audio_stream_id = 0xc0,
        .video_stream_id = 0xe0,
        .audio_pid = 0x101,
        .video_pid = 0x100,
        .pmt_pid = 0x1000,
        .pcr = 10,
        .output = packet_output,
        .program_count = 1,
        .program_list =
        {
            { 0x01, 0x1000 },
        }
    };

    app.ts = new_ts_stream( &ts_param );
    if ( !app.ts )
        return 0;
    ts_write_pat( app.ts );
    ts_write_pmt( app.ts );

    ipc_init( &param );
    ipc_run();

    while( app.running ) {
        sleep( 2 );
    }

    return 0;
}

