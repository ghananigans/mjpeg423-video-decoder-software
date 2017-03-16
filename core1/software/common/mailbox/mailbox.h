#ifndef MAILBOX_H_
#define MAILBOX_H_

#include <stdint.h>

typedef struct mailbox_msg{
	struct {
		enum {
			READ_NEXT_FILE, // Master to Slave: find next file and rd sd and ld Y
			SEEK_VIDEO, // seek to a frame position
			FAST_FORWARD_VIDEO,
			DONE_READ_NEXT_FRAME, // Slave to Master
			OK_TO_READ_NEXT_FRAME, // Master to Slave
			DONE_LD_Y, // Slave to Master
			OK_TO_LD_Y // Master to Slave
		} type;
	} header;

	union {
		struct {
			void * mpegHeader;
			void * mpegTrailer;
			void * bitstream;
			void * yDADC;
		} read_next_file;

		struct {
			void * mpegHeader;
			void * bitstream;
			void * yDADC;
			uint32_t framePosition;
		} seek_video;

		struct {
			uint32_t cbOffset;
			uint32_t crOffset;
			uint8_t frameType;
		} done_read_next_frame;

		struct {
			void * bitstream;
		} ok_to_read_next_frame;

		struct {
			void * yDADC;
		} done_ld_y;

		struct {
			void * mpegHeader;
			void * yBitstream;
			void * yDADC;
		} ok_to_ld_y;

		uint32_t as_uint32[0];
	} type_data;
} mailbox_msg_t;

int init_send_mailbox (char * csr_name);
int init_recv_mailbox (char * csr_name);
mailbox_msg_t * recv_msg (void);
void send_read_next_file (void * mpegHeader, void * mpegTrailer, void * bitstream, void * yDADC);
void send_seek_video (void * mpegHeader, void * bitstream, void * yDADC, uint32_t framePosition);
void send_done_read_next_frame (uint32_t cbOffset, uint32_t crOffset, uint8_t frameType);
void send_ok_to_read_next_frame (void * bitstream);
void send_done_ld_y (void * yDADC);
void send_ok_to_ld_y (void * mpegHeader, void * bitstream, void * yDADC);

#endif // MAILBOX_H_
