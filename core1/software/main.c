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
#include "common/config.h"
#include "common/utils.h"
#include "libs/ece423_sd/ece423_sd.h"
#include "common/lib_exts/mpeg423/mpeg423_decoder_ext.h"
#include "common/mailbox/mailbox.h"
#include <system.h>
#include <sys/alt_cache.h>

FILE_CONTEXT fileContext;
FAT_FILE_HANDLE hFile;
FAT_HANDLE hFAT;
FAT_BROWSE_HANDLE FatBrowseHandle;
uint8_t lastFrameType;
int counter = 0;

typedef struct FRAME_OFFSETS_STRUCT {
	uint32_t cbOffset;
	uint32_t crOffset;
}FRAME_OFFSETS;

static void seekFrame (uint32_t framePosition) {
	bool error;

	DBG_PRINT("Move to seel position: %d\n", framePosition);

	//
	// Move file to frame
	//
	error = Fat_FileSeek(hFile, FILE_SEEK_BEGIN,
			framePosition);
	assert(error, "Failed to seek file\n");
}
int load_mpeg_header (FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader) {
	//
	// Seek to the beginning of the file
	//
	if (!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, 0)) {
		printf("Failed to seek file\n");
		return 0;
	}

	//
	// Read MPEG423 File header data
	//
	if (!Fat_FileRead(hFile, (void*)mpegHeader, sizeof(MPEG_FILE_HEADER))) {
		printf("Failed to read the file.\n");
		return 0;
	}

	printf("MPG File header read: \n");
	printf("   Num frames %u\n", mpegHeader->num_frames);
	printf("   Width %u\n", mpegHeader->w_size);
	printf("   Height %u\n", mpegHeader->h_size);
	printf("   Num i frames %u\n", mpegHeader->num_iframes);
	printf("   Payload size %u\n", mpegHeader->payload_size);

	return 1;
}

int load_mpeg_trailer (FAT_FILE_HANDLE hFile, MPEG_FILE_HEADER* mpegHeader, MPEG_FILE_TRAILER* mpegTrailer) {
	mpegTrailer->num_iframes = mpegHeader->num_iframes;
	mpegTrailer->iframe_info = malloc(mpegHeader->num_iframes * sizeof(MPEG_IFRAME_INFO));

	//
	// Move to the trailer
	//
	if (!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, mpegHeader->payload_size + sizeof(MPEG_FILE_HEADER))) {
		printf("Failed to seek file\n");
		return 0;
	}

	//
	// Read trailer data
	//
	if (!Fat_FileRead(hFile, (void*)mpegTrailer->iframe_info, mpegHeader->num_iframes * sizeof(MPEG_IFRAME_INFO))) {
		printf("Failed to read the trailer.\n");
		return 0;
	}

	for (int i = 0; i < mpegTrailer->num_iframes; i++) {
		DBG_PRINT("Iframe #%d\n", i);
		DBG_PRINT("    Frame Number: %d\n", mpegTrailer->iframe_info[i].frame_index);
		DBG_PRINT("    Frame payload offset: %d\n", mpegTrailer->iframe_info[i].frame_position);
	}

	//
	// Move file pointer to the beginning of the paylod
	// (Right after the header)
	//
	if (!Fat_FileSeek(hFile, FILE_SEEK_BEGIN, sizeof(MPEG_FILE_HEADER))) {
		printf("Failed to seek back to beginning of payload\n");
		return 0;
	}

	return 1;
}

void unload_mpeg_trailer (MPEG_FILE_TRAILER* mpegTrailer) {
	free(mpegTrailer->iframe_info);
	mpegTrailer->iframe_info = NULL;
}

void loadVideo (FAT_HANDLE hFat, char* filename, MPEG_FILE_HEADER* mpegHeader, MPEG_FILE_TRAILER* mpegTrailer) {
	DBG_PRINT("Loading next video...\n");

	// opening the file
	hFile = Fat_FileOpen(hFat, filename);
	assert(hFile, "Error in opening file!\n")

	load_mpeg_header(hFile, mpegHeader);
	assert(mpegHeader->w_size == DISPLAY_WIDTH, "Video file width unrecognized\n");
	assert(mpegHeader->h_size == DISPLAY_HEIGHT, "Video file height unrecognized\n");

	unload_mpeg_trailer(mpegTrailer); //unload the previous trailer
	load_mpeg_trailer(hFile, mpegHeader, mpegTrailer);
}

FRAME_OFFSETS readFrameData (void * buffer) {
	static uint32_t frame_header[4];
    static uint32_t Ysize, Cbsize, frame_size, frame_type;
    FRAME_OFFSETS retStruct;

	//read frame payload
	bool read = Fat_FileRead(hFile, frame_header, 4*sizeof(uint32_t));
	assert(read, "cannot read frame header");

	frame_size  = frame_header[0];
	frame_type  = frame_header[1];
	Ysize       = frame_header[2];
	Cbsize      = frame_header[3];

	DBG_PRINT("MPG Frame #: %d\n", counter++);
	DBG_PRINT("    MPG Frame size: %d: \n", frame_size);
	DBG_PRINT("    MPG Frame type: 0x%x: \n", frame_type);

	read = Fat_FileRead(hFile, buffer, frame_size - 4 * sizeof(uint32_t));
	assert_persistent(read, "cannot read frame data");

	// Keep track of last frame type
	lastFrameType = frame_type;

	//set the Cb and Cr bitstreams to point to the right location
	retStruct.cbOffset = Ysize;
	retStruct.crOffset = Ysize + Cbsize;

	return retStruct;
}

static void findNextVideo (void) {
	int retVal;
	bool fileFound;

	DBG_PRINT("Finding next video...\n");

	//
	// Main loop
	//
	// Steps:
	//   1) Find next file
	//   2) Handle button presses and play video
	//   3) Close video if needed
	//   4) Repeat (back to 1)
	//

	//
	// Reset fileFound flag to 0 (false -- not found)
	//
	fileFound = 0;

	while(1) {
		//
		// Try to find the next available MPG file
		//
		while (Fat_FileBrowseNext(&FatBrowseHandle, &fileContext)) {
			//
			// Check if the file is a .MPG file
			//
			if (Fat_CheckExtension(&fileContext, ".MPG")) {
				DBG_PRINT("Found an MPG files!\n");
				fileFound = 1;
				break;
			}
		}

		// Break out of while loop when we've found a MPEG
		if (fileFound) {
			break;
		}

		// Assume that if file not found
		// we are at the end of the directory
		//
		// TODO: Don't make this assumption
		// End of FAT system, so loop back to beginning
		DBG_PRINT("Reached end of file list; Re-Begin File browse\n");
		retVal = Fat_FileBrowseBegin(hFAT, &FatBrowseHandle);
		assert(retVal, "Fat_FileBrowseBegin failed!");
	}

	DBG_PRINT("File Found; File Name is: %s, file size %d\n", Fat_GetFileName(&fileContext),
					fileContext.FileSize);
}

static void findLoadNextVideo (MPEG_FILE_HEADER* mpegHeader, MPEG_FILE_TRAILER* mpegTrailer) {
	findNextVideo();
	loadVideo(hFAT, Fat_GetFileName(&fileContext), mpegHeader, mpegTrailer);
	counter =  0;
}

static void doWork (void) {
	mailbox_msg_t volatile * msg;
	FRAME_OFFSETS frameOffsets;
	uint32_t val;
	MPEG_FILE_HEADER * mpegHeader;
	int prevType = 0;

	while (1) {
		DBG_PRINT("Waiting for request\n");
		msg = recv_msg();
		DBG_PRINT("Got msg type: %d\n", msg->header.type);

		switch(msg->header.type) {
			case READ_NEXT_FILE:
				DBG_PRINT("READ_NEXT_FILE msg received\n");
				DBG_PRINT(" bistream: %p\n", msg->type_data.read_next_file.bitstream);
				DBG_PRINT(" yDADC: %p\n", msg->type_data.read_next_file.yDADC);
				DBG_PRINT(" mpegHeader: %p\n", msg->type_data.read_next_file.mpegHeader);
				DBG_PRINT(" mpegTrailer: %p\n", msg->type_data.read_next_file.mpegTrailer);

				findLoadNextVideo(msg->type_data.read_next_file.mpegHeader,
						msg->type_data.read_next_file.mpegTrailer);

				frameOffsets = readFrameData(msg->type_data.read_next_file.bitstream);
				alt_dcache_flush(msg->type_data.read_next_file.bitstream, YBISTREAM_BYTES);
				send_done_read_next_frame(frameOffsets.cbOffset, frameOffsets.crOffset, lastFrameType);
				DBG_PRINT("DONE_READ_NEXT_FRAME msg sent\n");

				mpegHeader = (MPEG_FILE_HEADER *) msg->type_data.read_next_file.mpegHeader;
				val = mpegHeader->h_size * mpegHeader->w_size / 64;
				lossless_decode(val, msg->type_data.read_next_file.bitstream,
						msg->type_data.read_next_file.yDADC, Yquant, lastFrameType);
				alt_dcache_flush(msg->type_data.read_next_file.yDADC, NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));
				send_done_ld_y(msg->type_data.read_next_file.yDADC);
				DBG_PRINT("DONE_LD_Y msg sent\n");

				prevType = OK_TO_LD_Y;
				break;

			case SEEK_VIDEO:
				DBG_PRINT("SEEK_VIDEO msg received\n");
				DBG_PRINT(" bistream: %p\n", msg->type_data.seek_video.bitstream);
				DBG_PRINT(" yDADC: %p\n", msg->type_data.seek_video.yDADC);
				DBG_PRINT(" framePosition: %p\n", msg->type_data.seek_video.framePosition);

				seekFrame(msg->type_data.seek_video.framePosition);
				DBG_PRINT("Rewinding complete\n");

				frameOffsets = readFrameData(msg->type_data.seek_video.bitstream);
				alt_dcache_flush(msg->type_data.seek_video.bitstream, YBISTREAM_BYTES);
				send_done_read_next_frame(frameOffsets.cbOffset, frameOffsets.crOffset, lastFrameType);
				DBG_PRINT("DONE_READ_NEXT_FRAME msg sent\n");

				mpegHeader = (MPEG_FILE_HEADER *) msg->type_data.seek_video.mpegHeader;
				val = mpegHeader->h_size * mpegHeader->w_size / 64;
				lossless_decode(val, msg->type_data.seek_video.bitstream,
						msg->type_data.seek_video.yDADC, Yquant, lastFrameType);
				alt_dcache_flush(msg->type_data.seek_video.yDADC, NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));
				send_done_ld_y(msg->type_data.seek_video.yDADC);
				DBG_PRINT("DONE_LD_Y msg sent\n");

				prevType = OK_TO_LD_Y;

				break;

			case OK_TO_READ_NEXT_FRAME:
				DBG_PRINT("OK_TO_READ_NEXT_FRAME msg received\n");
				DBG_PRINT(" bistream: %p\n", msg->type_data.ok_to_read_next_frame.bitstream);

				assert(prevType == OK_TO_LD_Y, "Prev wasn't OK_TO_LD_Y: %d", prevType);

				frameOffsets = readFrameData(msg->type_data.ok_to_read_next_frame.bitstream);
				alt_dcache_flush(msg->type_data.ok_to_read_next_frame.bitstream, YBISTREAM_BYTES);
				DBG_PRINT("  OK_TO_READ_NEXT_FRAME msg processed\n");

				send_done_read_next_frame(frameOffsets.cbOffset, frameOffsets.crOffset, lastFrameType);
				DBG_PRINT("  DONE_READ_NEXT_FRAME msg sent\n");

				prevType = OK_TO_READ_NEXT_FRAME;

				break;

			case OK_TO_LD_Y:
				DBG_PRINT("OK_TO_LD_Y msg received\n");
				DBG_PRINT(" ybistream: %p\n", msg->type_data.ok_to_ld_y.yBitstream);
				DBG_PRINT(" yDADC: %p\n", msg->type_data.ok_to_ld_y.yDADC);
				DBG_PRINT(" mpegHeader: %p\n", msg->type_data.ok_to_ld_y.mpegHeader);

				assert(prevType == OK_TO_READ_NEXT_FRAME, "Prev wasn't OK_TO_LD_Y: %d", prevType);

				mpegHeader = (MPEG_FILE_HEADER *) msg->type_data.ok_to_ld_y.mpegHeader;
				val = mpegHeader->h_size * mpegHeader->w_size / 64;
				lossless_decode(val, msg->type_data.ok_to_ld_y.yBitstream,
						msg->type_data.ok_to_ld_y.yDADC, Yquant, lastFrameType);
				alt_dcache_flush(msg->type_data.ok_to_ld_y.yDADC, NUM_Y_DCT_BLOCKS * sizeof(dct_block_t));
				DBG_PRINT("OK_TO_LD_Y msg processed\n");

				send_done_ld_y(msg->type_data.ok_to_ld_y.yDADC);
				DBG_PRINT("DONE_LD_Y msg sent\n");

				prevType = OK_TO_LD_Y;

				break;

			default:
				assert(0, "INVALID MSG TO SLAVE: %d", msg->header.type);
		}
	}
}

int main()
{
	int retVal;

	printf("Application Starting (Slave Core - Core 1)...\n");

	//
	// Init the SD
	//
	retVal = SDLIB_Init(SD_CONT_BASE);
	assert(retVal, "SDLIB_Init failed!");

	//
	// Mount the FAT file system
	//
	hFAT = Fat_Mount();
	assert(hFAT, "Fat_Mount failed!");

	//
	// Get handle to browse FAT file system
	//
	retVal = Fat_FileBrowseBegin(hFAT, &FatBrowseHandle);
	assert(retVal, "Fat_FileBrowseBegin failed!");

	//
	// Init Mailbox
	//
	retVal = init_send_mailbox(MAILBOX_SIMPLE_CPU1_TO_CPU0_NAME);
	assert(retVal, "Failed to init send mailbox!");

	retVal = init_recv_mailbox(MAILBOX_SIMPLE_CPU0_TO_CPU1_NAME);
	assert(retVal, "Failed to init recv mailbox!");

	DBG_PRINT("Initialization complete on Core 1 (Slave Core)!\n");

	doWork();

  return 0;
}
