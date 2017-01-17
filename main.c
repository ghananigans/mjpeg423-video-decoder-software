/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include <system.h>
#include "libs/ece423_sd/ece423_sd.h"
#include "lib_exts/mpeg423/mpeg423_decoder_ext.h"
#include "libs/ece423_vid_ctl/ece423_vid_ctl.h"

void print_result(bool result, char* string){
	if (result)
	  {
		printf("%s successful :D\n", string);
	  }
	  else
	  {
		printf("%s failed D:\n", string);
		while(1){}
	  }
}

int main()
{
  printf("Application Starting...\n");

  // File System

  FAT_HANDLE hFAT;
  FAT_BROWSE_HANDLE FatBrowseHandle;
  FILE_CONTEXT fileContext;

  print_result(SDLIB_Init(SD_CONT_BASE), "SDLIB_Init");

  hFAT = Fat_Mount();
  print_result(hFAT, "Fat_Mount");

  print_result(Fat_FileBrowseBegin(hFAT, &FatBrowseHandle), "Fat_FileBrowseBegin");

  bool fileFound = 0;

  while(Fat_FileBrowseNext(&FatBrowseHandle, &fileContext))
  {
	  if(Fat_CheckExtension(&fileContext, ".MPG"))
	  {
		  fileFound = 1;
		  break;
	  }
  }

  if(!fileFound)
  {
	  printf("No MPEG file found\n");
	  while(1){}
  }

  printf("File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext), fileContext.FileSize);

  // opening the file
  FAT_FILE_HANDLE hFile = Fat_FileOpen(hFAT, Fat_GetFileName(&fileContext));
  if (!hFile) {
	  printf("Error in opening file!\n");
	  while(1){}
  }

  // read the file header
  MPEG_FILE_HEADER mpegHeader;
  read_mpeg_header(hFile, &mpegHeader);

  // init video display
  ece423_video_display* display = ece423_video_display_init(VIDEO_DMA_CSR_NAME, mpegHeader.w_size, mpegHeader.h_size, FRAME_BUFFER_SIZE);
  if(!display)
  {
	  printf("Failed to init video display\n");
	  while(1){}
  }
  printf("Video DMA initialized!\n");

  // set up frame buffers
  MPEG_WORKING_BUFFER mpegFrameBuffer;
  if (!allocate_frame_buffer(&mpegHeader, &mpegFrameBuffer))
  {
	  printf("Failed to allocate working buffers\n");
	  while(1){}
  }

  if (!read_next_frame(hFile, &mpegHeader, &mpegFrameBuffer, display->buffer_ptrs[1]->buffer))
  {
	  printf("Failed to read next frame\n");
	  while(1){}
  }

  printf("Generated first buffer\n");

  // Wait until buffer is available
  while(!ece423_video_display_buffer_is_available(display)) {}

  ece423_video_display_register_written_buffer(display);

  printf("Switching frames\n");
  ece423_video_display_switch_frames(display);
  printf("Done Switching frames\n");

  deallocate_frame_buffer(&mpegFrameBuffer);
  return 0;
}
