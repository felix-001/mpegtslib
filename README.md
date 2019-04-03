# tslib
嵌入式的是被对内存和flash占用都比较敏感，如果移植ffmpeg，占用空间大，占用内存也大。tslib是一个轻量级的ts封装库，适用于嵌入式的设备。主要用于将一帧帧的h264/h265和aac/g711打包成ts文件。对于直播延迟要求不高的情况，可以使用此库打包ts。
## 支持的feature
- [x] 低内存占用
- [x] 占用空间小
- [x] 支持h264
- [x] 支持h265
- [x] 支持aac
- [x] 支持g711
- [x] 跨平台，方便移植


# API说明
## 1.创建推流实例
```
ts_stream_t *new_ts_stream( ts_param_t *param  )
```
### 1.1 ts_param_t说明

成员 | 说明 
---|---
audio_stream_id | 音频流id 
video_stream_id | 视频流id
pmt_pid | pmt pid 
audio_pid | 音频pid
video_pid | 视频pid
pcr | pcr
program_count | 节目数
program_list | 节目列表
output | ts包输出回调函数


## 2. 写入pat
```
int ts_write_pat( ts_stream_t *ts )
```
## 2.1 参数说明
ts - ts封装实例

## 3.写入pmt
```
int ts_write_pmt( ts_stream_t *ts )
```
### 3.1 参数说明
ts - ts封装实例

## 4.写入pes
```
int ts_write_frame( ts_stream_t *ts, frame_info_t *frame )
```
### 4.1 frame_info_t说明

成员 | 说明
---|---
frame_type | 帧类型：视频或者音频
frame | 视频或者音频数据
len | 帧长度
timestamp | 时间戳

```
