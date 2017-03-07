//
//  mjpeg423_encoder.c
//  mjpeg423app
//
//  Created by Rodolfo Pellizzoni on 12/23/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/mjpeg423_types.h"
#include "mjpeg423_encoder.h"
#include "../common/util.h"

void decode_bmp(rgb_pixel_t* rgbblock, uint32_t w_size, uint32_t h_size, char *filename);

void mjpeg423_encode(uint32_t num_frames, int first, double stride, uint32_t max_I_interval, uint32_t w_size, uint32_t h_size, const char* filenamebase_in, const char* filename_out)
{
    
    int hCb_size = h_size/8;           //number of chrominance blocks
    int wCb_size = w_size/8;
    int hYb_size = h_size/8;           //number of luminance blocks
    int wYb_size = w_size/8;
    
    //main data structures. See Part II, Chapter 4
    //TODO: check that malloc does not return null
    rgb_pixel_t* rgbblock = malloc(w_size*h_size*sizeof(rgb_pixel_t));
    color_block_t* Yblock = malloc(hYb_size * wYb_size * 64);
    color_block_t* Cbblock = malloc(hCb_size * wCb_size * 64);
    color_block_t* Crblock = malloc(hCb_size * wCb_size * 64);
    dct_block_t* YDCAC = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CbDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CrDCAC = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* YDCACq_next = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CbDCACq_next = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CrDCACq_next = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* YDCACq_prev = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CbDCACq_prev = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CrDCACq_prev = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* YDCACqI = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CbDCACqI = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CrDCACqI = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* YDCACqP = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CbDCACqP = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    dct_block_t* CrDCACqP = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    uint8_t* YbitstreamI = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    uint8_t* CbbitstreamI = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    uint8_t* CrbitstreamI = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    uint8_t* YbitstreamP = malloc(hYb_size * wYb_size * 64 * sizeof(DCTELEM));
    uint8_t* CbbitstreamP = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    uint8_t* CrbitstreamP = malloc(hCb_size * wCb_size * 64 * sizeof(DCTELEM));
    uint8_t* Ybitstream;
    uint8_t* Cbbitstream;
    uint8_t* Crbitstream;
    
    //other payload and trailer fields
    uint32_t Ysize, YsizeI, YsizeP;
    uint32_t Cbsize, CbsizeI, CbsizeP;
    uint32_t Crsize, CrsizeP, CrsizeI;
    uint32_t frame_size;
    uint32_t frame_type = 0;
    char pad_str[4] = {0,0,0,0};
    uint32_t pad_num;
    
    iframe_trailer_t* trailer = malloc(sizeof(iframe_trailer_t)*num_frames);
    uint32_t file_position = 0;
    uint32_t num_iframes = 0;
    
    uint32_t last_iframe = 0;
    double bmp_number = first;
    
    DEBUG_PRINT("Encoder start\n")
    
    //file stream
    FILE* file_out;
    if((file_out = fopen(filename_out, "w")) == NULL) error_and_exit("cannot open output file");
    char* filename_in = malloc(strlen(filenamebase_in)+1);
    strcpy(filename_in, filenamebase_in);
    
    //write header
    if(fwrite(&num_frames, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    if(fwrite(&w_size, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");    
    if(fwrite(&h_size, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    //the next two fields will be overwritten after the trailer is computed
    if(fwrite(&num_iframes, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    if(fwrite(&file_position, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    file_position += sizeof(uint32_t)*5;
    
    //encode and write frames
    for(int frame_index = 0; frame_index < num_frames; frame_index ++){
        DEBUG_PRINT_ARG("\nFrame #%u\n",frame_index)
        
        //open and read bmp file
        long pos = strlen(filename_in) - 8;      //this assumes the namebase is in the format name0000.bmp
        filename_in[pos] = (char)(((uint32_t)(bmp_number))/1000) + '0';
        filename_in[pos+1] = (char)(((uint32_t)(bmp_number))/100%10) + '0';
        filename_in[pos+2] = (char)(((uint32_t)(bmp_number))/10%10) + '0';
        filename_in[pos+3] = (char)(((uint32_t)(bmp_number))%10) + '0';
        bmp_number += stride;
        DEBUG_PRINT_ARG("Filename: %s\n",filename_in)
        decode_bmp(rgbblock, w_size, h_size, filename_in);
        
        //rgb to ybcbr conversion
        for(int b = 0; b < hCb_size*wCb_size; b++) {
            rgb_to_ycbcr(b/wCb_size*8, b%wCb_size*8, w_size, rgbblock, Yblock[b], Cbblock[b], Crblock[b]);
        }

        //fdct
        for(int b = 0; b < hYb_size*wYb_size; b++){
            fdct(Yblock[b], YDCAC[b]);
        }
        for(int b = 0; b < hCb_size*wCb_size; b++) {
            fdct(Cbblock[b], CbDCAC[b]);
        }
        for(int b = 0; b < hCb_size*wCb_size; b++) {
            fdct(Crblock[b], CrDCAC[b]);
        }
        
        //I-frame
        //quantize I
        DCTELEM DCprev = 0;
        for(int b = 0; b < hYb_size*wYb_size; b++) 
            quantize_I(&DCprev, Yquant, YDCAC[b], YDCACqI[b], YDCACq_next[b]);
        DCprev = 0;
        for(int b = 0; b < hCb_size*wCb_size; b++) 
            quantize_I(&DCprev, Cquant, CbDCAC[b], CbDCACqI[b], CbDCACq_next[b]);
        DCprev = 0;
        for(int b = 0; b < hCb_size*wCb_size; b++) 
            quantize_I(&DCprev, Cquant, CrDCAC[b], CrDCACqI[b], CrDCACq_next[b]);
        //lossless
        YsizeI = lossless_encode(hYb_size*wYb_size, YDCACqI, YbitstreamI);
        CbsizeI = lossless_encode(hCb_size*wCb_size, CbDCACqI, CbbitstreamI);
        CrsizeI = lossless_encode(hCb_size*wCb_size, CrDCACqI, CrbitstreamI);
        DEBUG_PRINT_ARG("I-frame bitstream size: %u\n", YsizeI + CbsizeI + CrsizeI)
        
        //P-frame
        if(frame_index > 0){
            //quantize P
            for(int b = 0; b < hYb_size*wYb_size; b++) 
                quantize_P(Yquant, YDCACq_prev[b], YDCAC[b], YDCACqP[b]);
            for(int b = 0; b < hCb_size*wCb_size; b++) 
                quantize_P(Cquant, CbDCACq_prev[b], CbDCAC[b], CbDCACqP[b]);
            for(int b = 0; b < hCb_size*wCb_size; b++) 
                quantize_P(Cquant, CrDCACq_prev[b], CrDCAC[b], CrDCACqP[b]);
            
            //lossless
            YsizeP = lossless_encode(hYb_size*wYb_size, YDCACqP, YbitstreamP);
            CbsizeP = lossless_encode(hCb_size*wCb_size, CbDCACqP, CbbitstreamP);
            CrsizeP = lossless_encode(hCb_size*wCb_size, CrDCACqP, CrbitstreamP);
            DEBUG_PRINT_ARG("P-frame bitstream size: %u\n", YsizeP + CbsizeP + CrsizeP)
        }
        
        dct_block_t* tempp;
        if( (frame_index == 0) || 
            (YsizeI + CbsizeI + CrsizeI <= YsizeP + CbsizeP + CrsizeP) || 
            (frame_index - last_iframe >= max_I_interval) ) {
            //I frame
            DEBUG_PRINT("Selecting I-frame\n")
            frame_type = 0;
            last_iframe = frame_index;
            
            Ysize = YsizeI;
            Cbsize = CbsizeI;
            Crsize = CrsizeI;
            Ybitstream = YbitstreamI;
            Cbbitstream = CbbitstreamI;
            Crbitstream = CrbitstreamI;
        
            tempp = YDCACq_prev; YDCACq_prev = YDCACq_next; YDCACq_next = tempp;
            tempp = CbDCACq_prev; CbDCACq_prev = CbDCACq_next; CbDCACq_next = tempp;
            tempp = CrDCACq_prev; CrDCACq_prev = CrDCACq_next; CrDCACq_next = tempp;
        }
        else{
            //P frame
            DEBUG_PRINT("Selecting P-frame\n")
            frame_type = 1;
            
            Ysize = YsizeP;
            Cbsize = CbsizeP;
            Crsize = CrsizeP;
            Ybitstream = YbitstreamP;
            Cbbitstream = CbbitstreamP;
            Crbitstream = CrbitstreamP;
        }
        
        //write frame payload
        frame_size = Ysize + Cbsize + Crsize + sizeof(uint32_t)*4;
        frame_size = (frame_size%4 == 0)? frame_size : (frame_size + 4 - frame_size%4); //align to 4 bytes
        pad_num = frame_size - Ysize - Cbsize - Crsize - sizeof(uint32_t)*4;
        DEBUG_PRINT_ARG("Frame size: %u\n", frame_size)
        if(fwrite(&frame_size, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
        if(fwrite(&frame_type, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
        if(fwrite(&Ysize, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
        if(fwrite(&Cbsize, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
        if(fwrite(Ybitstream, 1, Ysize, file_out) != Ysize) error_and_exit("cannot write to output file");
        if(fwrite(Cbbitstream, 1, Cbsize, file_out) != Cbsize) error_and_exit("cannot write to output file");
        if(fwrite(Crbitstream, 1, Crsize, file_out) != Crsize) error_and_exit("cannot write to output file");
        
        if(fwrite(&pad_str, 1, pad_num, file_out) != pad_num)      //align to 32bits by padding
            error_and_exit("cannot write to output file");      
        
        //if I-frame, add to trailer data structure
        if(frame_type == 0){
            iframe_trailer_t iframe = {frame_index, file_position};
            trailer[num_iframes++] = iframe;
        }
        
        file_position += frame_size;
        
    } //end frame iteration
    
    //write trailer
    for(int count = 0; count < num_iframes; count++){
        if(fwrite(&(trailer[count].frame_index), sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
        if(fwrite(&(trailer[count].frame_position), sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    }
    //ToDo check: write an additional 512 bytes after the trailer to ensure proper SD card reading
    uint8_t pad512[512];
    if(fwrite(pad512, 1, 512, file_out) != 512) error_and_exit("cannot write to output file");
    //overwrite header info
    file_position -= 5 * sizeof(uint32_t); //set it to payload length
    if(fseek(file_out,sizeof(uint32_t)*3,SEEK_SET) != 0) error_and_exit("cannot seek into file");
    if(fwrite(&num_iframes, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    if(fwrite(&file_position, sizeof(uint32_t), 1, file_out) != 1) error_and_exit("cannot write to output file");
    DEBUG_PRINT_ARG("\nNum Iframes: #%u. Encoder done.\n\n\n", num_iframes)

    
    fclose(file_out);
    //TODO: free on malloced data
}