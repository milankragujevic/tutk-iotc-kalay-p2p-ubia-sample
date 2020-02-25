#include <stdio.h>

#include "audioalarm_common.h"

void audio_file_init(char *path)
{

        FILE *fp = fopen(path, "w");
        if(fp == NULL){
                printf("open failed\n");
                return;
        }

        struct wav_header hdr;
        hdr.riff_id = ID_RIFF;
        hdr.riff_fmt = ID_WAVE;
        hdr.fmt_id = ID_FMT;
        hdr.audio_format = FORMAT_PCM;
        hdr.fmt_sz = 16;
        hdr.bits_per_sample = 16;
        hdr.num_channels = 1;
        hdr.data_sz = 0; /* TODO before record over */
        hdr.sample_rate = 8000;
        hdr.data_id = ID_DATA;
        hdr.byte_rate = hdr.sample_rate * hdr.num_channels * hdr.bits_per_sample / 8;
        hdr.block_align = hdr.num_channels * hdr.bits_per_sample / 8;

        fwrite(&hdr, 40, 1, fp);
        fclose(fp);
}

void audio_file_write_data(char *path, char *pdata)
{
        FILE *fp = fopen(path, "a+");
        if (fp == NULL ) {
                printf("open failed\n");
                return;
        }
        fwrite(pdata, 8192, 1, fp);
        fclose(fp);
}



