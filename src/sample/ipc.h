// Last Update:2019-03-21 20:32:13
/**
 * @file ipc.h
 * @brief  ipc interface, support multi manufacturer ipc
 * @author felix
 * @version 0.1.00
 * @date 2019-03-15
 */

#ifndef IPC_H
#define IPC_H

#include <stdint.h>

enum {
    AUDIO_AAC,
    AUDIO_G711,
};

enum {
    EVENT_CAPTURE_PICTURE_SUCCESS,
    EVENT_CAPTURE_PICTURE_FAIL,
    EVENT_MOTION_DETECTION,
    EVENT_MOTION_DETECTION_DISAPEER,
};

typedef struct {
    int audio_type;
    char *video_file;
    char *pic_file;
    char *audio_file;
    int (*video_cb) ( uint8_t *frame, int len, int iskey, int64_t timestamp );
    int (*audio_cb) ( uint8_t *frame, int len, int64_t timestamp );
    int (*event_cb)( int event, void *data );
} ipc_param_t;

typedef struct ipc_dev_t {
    int (*init)( struct ipc_dev_t *ipc, ipc_param_t *param );
    void (*run)( struct ipc_dev_t *ipc );
    int (*capture_picture)( struct ipc_dev_t *ipc, char *file );
    void (*deinit)( struct ipc_dev_t *ipc );
    void *priv;
} ipc_dev_t;

extern int ipc_init( ipc_param_t *param );
extern void ipc_run();
extern int ipc_dev_register( ipc_dev_t *dev );
extern int ipc_capture_picture( char *file );

#endif  /*IPC_H*/

