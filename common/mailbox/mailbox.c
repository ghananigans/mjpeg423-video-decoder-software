#include "mailbox.h"

extern alt_mailbox_dev;
extern altera_avalon_mailbox_open(char *);
extern altera_avalon_mailbox_post(alt_mailbox_dev*);
extern altera_avalon_mailbox_pend(alt_mailbox_dev*);

static alt_mailbox_dev *sendMailbox = 0;
static alt_mailbox_dev *recvMailbox = 0;

static mailbox_msg_t sendBuffer[4];
static uint8_t counter = 0;

static void send (mailbox_msg_t * msg) {
	while (altera_avalon_mailbox_post(sendMailbox, (alt_u32) msg) != 0);

	counter = (counter + 1) & 0x3;
}

int init_send_mailbox (char * csr_name) {
	sendMailbox = altera_avalon_mailbox_open(csr_name);

	return 1;
}

int init_recv_mailbox (char * csr_name) {
	recvMailbox = altera_avalon_mailbox_open(csr_name);

	return 1;
}

mailbox_msg_t * recv_msg (void) {
	return altera_avalon_mailbox_pend(recvMailbox);
}

void send_read_next_file (void * bitstream, void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = READ_NEXT_FILE;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.read_next_file.bitstream = bitstream;
	sendBuffer[counter].type_data.read_next_file.yDADC = yDADC;

	send(&sendBuffer[counter]);
}

void send_done_read_next_frame (uint32_t cbOffset, uint32_t crOffset) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = DONE_READ_NEXT_FRAME;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.done_read_next_frame.cbOffset = cbOffset;
	sendBuffer[counter].type_data.done_read_next_frame.crOffset = crOffset;

	send(&sendBuffer[counter]);
}

void send_ok_to_read_next_frame (void * bitstream) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = OK_TO_READ_NEXT_FRAME;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.ok_to_read_next_frame.bitstream = bitstream;

	send(&sendBuffer[counter]);
}

void send_done_ld_y (void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = DONE_LD_Y;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.done_ld_y.yDADC = yDADC;

	send(&sendBuffer[counter]);
}

void send_ok_to_ld_y (void * yDADC) {
	// Set mailbox msg type
	sendBuffer[counter].header.type = OK_TO_LD_Y;

	// Mailbox msg type specific data
	sendBuffer[counter].type_data.ok_to_ld_y.yDADC = yDADC;

	send(&sendBuffer[counter]);
}
