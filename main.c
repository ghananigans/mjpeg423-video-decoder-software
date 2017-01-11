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
#include "libs/ece423_sd/ece423_sd.h"

void print_result(bool result, char* system_initing){
	if (result)
	  {
		printf("%s successful :D\n");
	  }
	  else
	  {
		printf("%s failed D:\n");
		while(1){}
	  }
}

int main()
{
  printf("Application Starting...\n");

  FAT_HANDLE hFAT;
  FAT_BROWSE_HANDLE FatBrowseHandle;
  FILE_CONTEXT fileContext;

  print_result(SDLIB_Init(SD_CONT_BASE), "SDLIB_Init");

  hFAT = Fat_Mount();
  print_result(hFAT, "Fat_Mount");

  print_result(Fat_FileBrowseBegin(hFAT, &FatBrowseHandle), "Fat_FileBrowseBegin");

  print_result(Fat_FileBrowseNext(&FatBrowseHandle, &fileContext), "Fat_FileBrowseNext");
  printf("File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext), fileContext.FileSize);

  print_result(Fat_FileBrowseNext(&FatBrowseHandle, &fileContext), "Fat_FileBrowseNext");
  printf("File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext), fileContext.FileSize);


  return 0;
}
