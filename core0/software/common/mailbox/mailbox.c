#include "mailbox.h"
#include "altera_avalon_mailbox_simple.h"


static altera_avalon_mailbox_dev *sendMailbox = 0;
static altera_avalon_mailbox_dev *recvMailbox = 0;

static mailbox_msg_t sendBuffer[4];
static uint8_t counter = 0;

static uint32_t mailboxBuffers[4][2] = {
	0x1, (uint32_t) &sendBuffer[0],
	0x1, (uint32_t) &sendBuffer[1],
	0x1, (uint32_t) &sendBuffer[2],
	0x1, (uint32_t) &sendBuffer[3]
};

static void send (void) {
	altera_avalon_mailbox_send(sendMailbox, (void *) mailboxBuffers[counter], 0, POLL);

	counter = (counter + 1) & 0x3;
}

int init_send_mailbox (char * csr_name) {
	sendMailbox = altera_avalon_mailbox_open(csr_name, 0, 0);

	return 1;
}

int init_recv_mailbox (char * csr_name) {
	recvMailbox = altera_avalon_mailbox_open(csr_name, 0, 0);

	return 1;
}

mailbox_msg_t * recv_msg (void) {
	uint32_t retData[2];
	altera_avalon_mailbox_retrieve_poll(recvMailbox, &retData, 0);

	return ((mailbox_msg_t * ) retData[1]);
}

void send_read_next_file (void * bitstream, void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = READ_NEXT_FILE;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.read_next_file.bitstream = bitstream;
	sendBuffer[counter].type_data.read_next_file.yDADC = yDADC;

	send();
}

void send_done_read_next_frame (uint32_t cbOffset, uint32_t crOffset) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = DONE_READ_NEXT_FRAME;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.done_read_next_frame.cbOffset = cbOffset;
	sendBuffer[counter].type_data.done_read_next_frame.crOffset = crOffset;

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
