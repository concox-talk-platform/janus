#include <stdio.h>
#include <string.h>
#include "mp3_encoder.h"

int convert_single_file(int argc, const char * argv[]) {
    if (2 > argc) {
        printf("Usage: %s pcm [mp3]\n", argv[0]);
        return -1;
    }

    char buf[128] = {0};

    if (3 == argc) {
        strcpy(buf, argv[2]);
    } else {
        get_mp3_file_name(argv[1], buf, sizeof(buf));
    }

    int ret = pcm2mp3(argv[1], buf, NULL);
    if (0 != ret) {
        printf("convert pcm(%s) to mp3(%s) fail, code: %d\n", argv[1], buf, ret);
        return -1;
    }

    return 0;
}

/**
 * 多个文件测试，底层是否会缓存上一个文件的数据？
 * 如缓存影响了下一个文件的内容，尽量每次重建encoder
 */
int convert_multi_files(int argc, const char * argv[]) {
    if (1 >= argc) {
        printf("Usage: %s pcm [,pcm,...]\n", argv[0]);
        return -1;
    }

    char buf[128] = {0};

    encoder_t enc = get_mp3_encoder();
    if (!enc) {
        printf("encoder is null\n");
        return -1;
    }

    if (0 != set_mp3_encoder(enc, 1, 16000, 8, 2)) {
        printf("encoder init fail\n");
        return -1;
    }

    int i = 1;
    for (; i < argc; ++i) {
        get_mp3_file_name(argv[i], buf, sizeof(buf));
        int code = do_mp3_encode(enc, argv[i], buf);
        if (0 != code) {
            printf("format to %s fail. code: %d\n", buf, code);
            return -1;
        } else {
            printf("format to %s ok.\n", buf);
        }
    }

    return 0;
}


int main(int argc, char * argv[]) {
    
    // convert_single_file(argc, argv);
    convert_multi_files(argc, argv);
    printf("Done.\n");
    return 0;
}