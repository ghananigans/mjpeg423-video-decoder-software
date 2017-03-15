#include "mailbox.h"
#include <altera_avalon_mailbox_simple.h>
#include <sys/alt_cache.h>


static altera_avalon_mailbox_dev *sendMailbox = 0;
static altera_avalon_mailbox_dev *recvMailbox = 0;

static mailbox_msg_t volatile * sendBuffer;
static uint8_t counter = 0;

static uint32_t mailboxBuffers[4][2] = {
	{0x1, 0},
	{0x1, 0},
	{0x1, 0},
	{0x1, 0}
};

static void send (void) {
	alt_dcache_flush_all();

	while (altera_avalon_mailbox_send(sendMailbox, (void *) mailboxBuffers[counter], 0, POLL) != 0);

	counter = (counter + 1) & 0x3;
}

int init_send_mailbox (char * csr_name) {
	int i;

	sendBuffer = (mailbox_msg_t volatile *) alt_uncached_malloc(4 * sizeof(mailbox_msg_t));

	for (i = 0; i < 4; ++i) {
		mailboxBuffers[i][1] = (uint32_t) &sendBuffer[i];
	}

	sendMailbox = altera_avalon_mailbox_open(csr_name, 0, 0);

	return 1;
}

int init_recv_mailbox (char * csr_name) {
	recvMailbox = altera_avalon_mailbox_open(csr_name, 0, 0);

	return 1;
}

mailbox_msg_t * recv_msg (void) {
	uint32_t retData[2];

	while(altera_avalon_mailbox_retrieve_poll(recvMailbox, &retData, 0) != 0) {}

	return ((mailbox_msg_t * ) retData[1]);
}

void send_read_next_file (void * mpegHeader, void * mpegTrailer, void * bitstream, void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = READ_NEXT_FILE;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.read_next_file.mpegHeader = mpegHeader;
	sendBuffer[counter].type_data.read_next_file.mpegTrailer = mpegTrailer;
	sendBuffer[counter].type_data.read_next_file.bitstream = bitstream;
	sendBuffer[counter].type_data.read_next_file.yDADC = yDADC;

	send();
}

void send_done_read_next_frame (uint32_t cbOffset, uint32_t crOffset, uint8_t frameType) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = DONE_READ_NEXT_FRAME;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.done_read_next_frame.cbOffset = cbOffset;
	sendBuffer[counter].type_data.done_read_next_frame.crOffset = crOffset;
	sendBuffer[counter].type_data.done_read_next_frame.frameType = frameType;

	send();
}

void send_ok_to_read_next_frame (void * bitstream) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = OK_TO_READ_NEXT_FRAME;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.ok_to_read_next_frame.bitstream = bitstream;

	send();
}

void send_done_ld_y (void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = DONE_LD_Y;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.done_ld_y.yDADC = yDADC;

	send();
}

void send_ok_to_ld_y (void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = OK_TO_LD_Y;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.ok_to_ld_y.yDADC = yDADC;

	send();
}
