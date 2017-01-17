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

  return 0;
}
