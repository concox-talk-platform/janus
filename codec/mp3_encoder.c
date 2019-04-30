/**
 * Copyright (c) 2019. All rights reserved.
 * LAME Wrapper, for more information, see https://github.com/gypified/libmp3lame
 * Author: tesion
 * Date: April 29th 2019
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lame/lame.h"
#include "mp3_encoder.h"

#define INPUT_BUF_SIZE 4096
#define OUTPUT_UBF_SIZE (5120 + 7200)   // OUTPUT = (1.24 * INPUT) + 7200
#define PCM_SIZE 640
#define MP3_SIZE 8192
#define DEF_CHANNEL_NUM 1
#define DEF_SAMPLE 16000
#define DEF_BIT_RATE 8
#define DEF_QUALITY 5
#define DEF_MODE -1

#define LDEBUG(format, ...) printf("[LAME][%s:%s:%d]" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

encoder_t get_mp3_encoder() {
    return lame_init();
}

int set_mp3_encoder_internal(encoder_t encoder, int channels, int sample, int bitrate, int quality, int mode) {
    if (!encoder) return 1;

    lame_set_num_channels(encoder, channels);
    lame_set_in_samplerate(encoder, sample);
    lame_set_brate(encoder, bitrate);

    if (0 <= mode && 3 >= mode) {
        lame_set_mode(encoder, mode);
    }
    
    if (0 <= quality && 9 >= quality) {
        lame_set_quality(encoder, quality);  // 2=high 5=medium 7=low
    }
    
    return lame_init_params(encoder);
}

int set_mp3_encoder(encoder_t encoder, int channels, int sample, int bitrate, int quality)
{
    return set_mp3_encoder_internal(encoder, channels, sample, bitrate, quality, -1);
}

int do_mp3_encode(encoder_t encoder, const char * pcm_file, const char * mp3_file) {
    FILE * pcm_fp = NULL, * mp3_fp = NULL;
    int ret = 0;

    if (!encoder) {
        ret = -1;
        goto ERROR;
    }

    if (NULL == pcm_file || NULL == mp3_file) {
#ifdef LAME_DEBUG
        LDEBUG("param input or out is null\n");
#endif
        ret = -2;
        goto ERROR;
    }

    pcm_fp = fopen(pcm_file, "rb");
    if (!pcm_fp) {
#ifdef LAME_DEBUG
        LDEBUG("open %s error: %s\n", pcm_file, strerror(errno));
#endif
        ret = -3;
        goto ERROR;
    }

    mp3_fp = fopen(mp3_file, "wb");
    if (!mp3_fp) {
#ifdef LAME_DEBUG
        LDEBUG("open %s error: %s\n", mp3_file, strerror(errno));
#endif
        ret = -4;
        goto ERROR;
    }

    short int pcm_buf[PCM_SIZE] = {0};
    unsigned char mp3_buf[MP3_SIZE] = {0};
    size_t read_bytes = 0, write_bytes = 0;

    do {
        memset(pcm_buf, 0, sizeof(pcm_buf));
        memset(mp3_buf, 0, sizeof(mp3_buf));
        read_bytes = fread(pcm_buf, sizeof(short int), PCM_SIZE, pcm_fp);
        if (read_bytes == 0) {
            write_bytes = lame_encode_flush(encoder, mp3_buf, MP3_SIZE);
        }
        else {
            write_bytes = lame_encode_buffer(encoder, pcm_buf, NULL, read_bytes, mp3_buf, MP3_SIZE);
        }

        fwrite(mp3_buf, write_bytes, 1, mp3_fp);

#ifdef LAME_DEBUG
        LDEBUG("write mp3 buffer: %lu byte(s)\n", write_bytes);
#endif
    } while (0 != read_bytes);

ERROR:
    fclose(pcm_fp);
    fclose(mp3_fp);

    return ret;
}

int pcm2mp3(const char * pcm_path, const char * mp3_path, encoder_cfg_t * cfg) {
    int ret = 0, stat = 0;
    encoder_t lame = get_mp3_encoder();
    if (NULL == lame) {
        stat = 1;
        goto END;
    }

    int channels = DEF_CHANNEL_NUM;
    int sample = DEF_SAMPLE;
    int bit_rate = DEF_BIT_RATE;
    int quality = DEF_QUALITY;
    int mode = DEF_QUALITY;

    if (cfg) {
        channels = cfg->channels;
        sample = cfg->sample;
        bit_rate = cfg->bit_rate;
        quality = cfg->quality;
        mode = cfg->mode;
    }

    ret = set_mp3_encoder_internal(lame, channels, sample, bit_rate, quality, mode);
    if (0 > ret) {
        stat = 2;
        goto END;
    }

    ret = do_mp3_encode(lame, pcm_path, mp3_path);
    printf("encode ret: %d\n", ret);
    if (0 != ret) {
        stat = 3;
        goto END;
    }

END:
    if(lame) {
        lame_close(lame);
    }

    return stat;
}

void get_mp3_file_name(const char * pcm_path, char * buf, size_t len) {
    if (!pcm_path) return;

    int sz = strlen(pcm_path);
    
    if (sz > len) return;

    memset(buf, 0, sizeof(char) * len);
    strcpy(buf, pcm_path);

    if (sz >= 4) {
        // x.wav, 替换后缀为 x.mp3
        char * p = buf + (sz - 4);
        if (0 == strcasecmp(p, ".wav")) {
            strcpy(p, ".mp3");
        } else {
            strcat(buf, ".mp3");
        }
    } else {
        strcat(buf, ".mp3");
    }
}