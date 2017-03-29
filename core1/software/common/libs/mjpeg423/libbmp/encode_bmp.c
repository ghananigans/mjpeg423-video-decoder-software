#include <stdio.h>
#include "bmpfile.h"
#include "../common/mjpeg423_types.h"

void encode_bmp(rgb_pixel_t* rgbbblock, uint32_t w_size, uint32_t h_size, const char* filename);

void encode_bmp(rgb_pixel_t* rgbbblock, uint32_t w_size, uint32_t h_size, const char* filename)
{
  bmpfile_t *bmp;
  int row, column;

  if ((bmp = bmp_create_e(w_size, h_size, 32)) == NULL) {
    printf("Cannot create bmp\n");
    exit(-1);
  }

    for (row = 0; row < h_size; row++)
        for(column = 0; column < w_size; column++){
            bmp_set_pixel(bmp, column, row, rgbbblock[row*w_size+column]);
        }

  bmp_save(bmp, filename);
  bmp_destroy(bmp);

}
