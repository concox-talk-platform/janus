/**
 * Copyright (c) 2019. All rights reserved.
 * LAME Wrapper, for more information, see https://github.com/gypified/libmp3lame
 * Author: tesion
 * Date: April 29th 2019
 */
#ifndef CODEC_MP3_ENCODER_H_
#define CODEC_MP3_ENCODER_H_
#include "lame/lame.h"

typedef lame_t encoder_t;
typedef struct encoder_cfg {
    int channels;
    int sample;
    int bit_rate;
    int quality;
    int mode;
} encoder_cfg_t;

/**
 * @brief get the handle of lame with default setting
 * @return
 *  if success, return handle; otherwise NULL
 */
encoder_t get_mp3_encoder();

/**
 * @brief change the default setting of lame
 * @param encoder IN the handle of lame
 * @param channels IN the number of channel
 * @param sample IN sample rate
 * @param bitrate IN bitrate, default: 11
 * @param quality IN the encode quality: 0..9, 0=best (very slow).  9=worst.
 * recommended:     2     near-best quality, not too slow
 *                  5     good quality, fast
 *                  7     ok quality, really fast
 * @param mode IN mode = 0,1,2,3 
 * @return 
 *  0 OK, !=0 Fail
 */
int set_mp3_encoder_internal(encoder_t encoder, int channels, int sample, int bitrate, int quality, int mode);

int set_mp3_encoder(encoder_t encoder, int channels, int sample, int bitrate, int quality);

/**
 * @brief convert pcm to mp3 format
 * @param encoder IN the handle of lame
 * @param pcm_file IN pcm file path
 * @param mp3_file IN mp3 file path
 */
int do_mp3_encode(encoder_t encoder, const char * pcm_file, const char * mp3_file);

/**
 * @brief convert pcm to mp3
 * @param pcm_path IN pcm file path
 * @param mp3_path IN mp3 file path
 * @return
 *      0 OK, !=0 Fail
 */
int pcm2mp3(const char * pcm_path, const char * mp3_path, encoder_cfg_t * cfg);

/**
 * @brief get mp3 file name from pcm file name
 * @param pcm_path IN pcm file path
 * @param buf OUT buffer to store mp3 file name
 * @param len IN the length of buffer
 */
void get_mp3_file_name(const char * pcm_path, char * buf, size_t len);

#endif  // CODDEC_MP3_ENCODER_H_