
// Usage: ./tx_rc_sbus config.ini
/*

[tx_rc_sbus]
mode=0	# 0-send packet to air, 1-send to udp, 2-both
nic=wlan0				// optional, when mode set to 0or2
udp_ip=127.0.0.1		// optional, when mode set to 1or2
udp_port=30302			// optional, when mode set to 1or2
udp_bind_port=30300		// optional, when mode set to 1or2
uart=/dev/ttyUSB0
wifimode=0				// 0-b/g 1-n
rate=6					// Mbit(802.11b/g) / mcs index(802.11n/ac)
ldpc=0					// 802.11n/ac only
retrans=2
encrypt=0
password=1145141919810

*/
//


#include "lib.h"
#include "xxtea.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <iniparser.h>
#include <linux/serial.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stropts.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Note: redefinition issue -- use setsbus
//#include <asm/termios.h>
#include <sys/ioctl.h>

int sockfd;
int uartfd;
int udpfd;

static uint8_t u8aRadiotapHeader[] = {
	0x00, 0x00, // <-- radiotap version
	0x0c, 0x00, // <- radiotap header length
	0x04, 0x80, 0x00, 0x00, // <-- radiotap present flags
	0x00, // datarate (will be overwritten later in packet_header_init)
	0x00,
	0x00, 0x00
};

static uint8_t u8aRadiotapHeader80211n[] = {
	0x00, 0x00, // <-- radiotap version
	0x0d, 0x00, // <- radiotap header length
	0x00, 0x80, 0x08, 0x00, // <-- radiotap present flags (tx flags, mcs)
	0x00, 0x00, 	// tx-flag
	0x07, 			// mcs have: bw, gi, fec: 					 8'b00010111
	0x00,			// mcs: 20MHz bw, long guard interval, ldpc, 8'b00010000
	0x02,			// mcs index 2 (speed level, will be overwritten later)
};

static uint8_t u8aIeeeHeader_rts[] = {
        180, 191, 0, 0, // frame control field (2 bytes), duration (2 bytes)
        0xff, // 1st byte of IEEE802.11 RA (mac) must be 0xff or something odd (wifi hardware determines broadcast/multicast through odd/even check)
};

struct framedata_s {
	// 16 bytes info
	uint32_t crc;					// wip, 0x00000000 now
	uint32_t length;				// body length
    uint32_t seqnumber;
	uint8_t info[4];				// for future use, now fill with 0
	
	// payload
    uint8_t data[25];

} __attribute__ ((__packed__));
#define FRAME_DATA_OFFSET 16

void usage(void)
{
    printf(
        "tx_rc_sbus by libc0607. GPL2\n"
        "\n"
        "Usage: tx_rc_sbus config.ini \n"
        "\n"
        "\n");
    exit(1);
}

int open_sock (char *ifname) 
{
    struct sockaddr_ll ll_addr;
    struct ifreq ifr;
	int sock;
	
    sock = socket (AF_PACKET, SOCK_RAW, 0);
    if (sock == -1) {
		fprintf(stderr, "Error:\tSocket failed\n");
		exit(1);
    }
    ll_addr.sll_family = AF_PACKET;
    ll_addr.sll_protocol = 0;
    ll_addr.sll_halen = ETH_ALEN;
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		fprintf(stderr, "Error:\tioctl(SIOCGIFINDEX) failed\n");
		exit(1);
    }

    ll_addr.sll_ifindex = ifr.ifr_ifindex;
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		fprintf(stderr, "Error:\tioctl(SIOCGIFHWADDR) failed\n");
		exit(1);
    }
    memcpy(ll_addr.sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    if (bind (sock, (struct sockaddr *)&ll_addr, sizeof(ll_addr)) == -1) {
		fprintf(stderr, "Error:\tbind failed\n");
		close(sock);
		exit(1);
    }
    if (sock == -1 ) {
        fprintf(stderr,
        "Error:\tCannot open socket\n"
        "Info:\tMust be root with an 802.11 card with RFMON enabled\n");
        exit(1);
    }
    return sock;
}

wifibroadcast_rx_status_t *telemetry_wbc_status_memory_open (void) 
{
    int fd = 0;

    while(1) {
        fd = shm_open("/wifibroadcast_rx_status_0", O_RDONLY, S_IRUSR | S_IWUSR);
	    if(fd < 0) {
			fprintf(stderr, "Could not open wifibroadcast rx status - will try again ...\n");
	    } else {
			break;
	    }
	    usleep(500000);
    }

	void *retval = mmap(NULL, sizeof(wifibroadcast_rx_status_t), PROT_READ, MAP_SHARED, fd, 0);
	if (retval == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	return (wifibroadcast_rx_status_t*)retval;
}

void telemetry_init(telemetry_data_t *td) 
{
    td->rx_status = telemetry_wbc_status_memory_open();
}

void set_baud_custom (int fd, int speed)
{
	struct serial_struct ss;
	if (ioctl(fd, TIOCGSERIAL, &ss) < 0) {
		perror("TIOCGSERIAL failed");
		exit(1);
	}

	ss.flags = (ss.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
	ss.custom_divisor = (ss.baud_base + (speed/2)) / speed;
	int closest_speed = ss.baud_base / ss.custom_divisor;

	if (closest_speed < speed * 98 / 100 || closest_speed > speed * 102 / 100) {
		fprintf(stderr, "Cannot set speed to %d, closest is %d\n", speed, closest_speed);
		exit(1);
	}

	fprintf(stderr, "closest baud = %i, base = %i, divisor = %i\n", closest_speed, ss.baud_base,
			ss.custom_divisor);

	if (ioctl(fd, TIOCSSERIAL, &ss) < 0) {
		perror("TIOCSSERIAL failed");
		exit(1);
	}
	return;
}

uint8_t bitrate_to_rtap8 (int bitrate) 
{
	uint8_t ret;
	switch (bitrate) {
		case  1: ret=0x02; break;
		case  2: ret=0x04; break;
		case  5: ret=0x0b; break;
		case  6: ret=0x0c; break;
		case 11: ret=0x16; break;
		case 12: ret=0x18; break;
		case 18: ret=0x24; break;
		case 24: ret=0x30; break;
		case 36: ret=0x48; break;
		case 48: ret=0x60; break;
		default: fprintf(stderr, "ERROR: Wrong or no data rate specified\n"); exit(1); break;
	}
	return ret;
}

void dump_memory(void* p, int length, char * tag)
{
	int i, j;
	unsigned char *addr = (unsigned char *)p;

	fprintf(stderr, "\n");
	fprintf(stderr, "===== Memory dump at %s, length=%d =====", tag, length);
	fprintf(stderr, "\n");

	for(i = 0; i < 16; i++)
		fprintf(stderr, "%2x ", i);
	fprintf(stderr, "\n");
	for(i = 0; i < 16; i++)
		fprintf(stderr, "---");
	fprintf(stderr, "\n");

	for(i = 0; i < (length/16) + 1; i++) {
		for(j = 0; j < 16; j++) {
			if (i * 16 + j >= length)
				break;
			fprintf(stderr, "%2x ", *(addr + i * 16 + j));
		}
		fprintf(stderr, "\n");
	}
	for(i = 0; i < 16; i++)
		fprintf(stderr, "---");
	fprintf(stderr, "\n\n");
}

int uart_sbus_init(int fd) 
{
	// Note: CH340 only works with Linux kernel 4.10+
	
	// redefinition issue
#ifndef PARODD
	fprintf(stderr, "redefinition issue -- please call setsbus first to set baudrate\n");
	return 0;
#else
	int rate = 100000;
	struct termios2 tio;
	ioctl(fd, TCGETS2, &tio);
	// 8bit
	tio.c_cflag &= ~CSIZE;   
	tio.c_cflag |= CS8;
	// even
	tio.c_cflag &= ~(PARODD | CMSPAR);
	tio.c_cflag |= PARENB;
	// 2 stop bits
	tio.c_cflag |= CSTOPB;	 
	// baud rate
	tio.c_ispeed = rate;
	tio.c_ospeed = rate;
	// other
	tio.c_iflag |= (INPCK|IGNBRK|IGNCR|ISTRIP);
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= (BOTHER|CREAD|CLOCAL);
	// apply
	return ioctl(fd, TCSETS2, &tio);
#endif
}

// return: rtheader_length
int packet_rtheader_init (int offset, uint8_t *buf, dictionary *ini) 
{
	int param_bitrate = iniparser_getint(ini, "tx_rc_sbus:rate", 0);
	int param_wifimode = iniparser_getint(ini, "tx_rc_sbus:wifimode", 0);
	int param_ldpc = (param_wifimode == 1)? iniparser_getint(ini, "tx_rc_sbus:ldpc", 0): 0;
	uint8_t * p_rtheader = (param_wifimode == 1)? u8aRadiotapHeader80211n: u8aRadiotapHeader;
	size_t rtheader_length = (param_wifimode == 1)? sizeof(u8aRadiotapHeader80211n): sizeof(u8aRadiotapHeader);

	// set args
	if (param_wifimode == 0) {	// 802.11g
		u8aRadiotapHeader[8] = bitrate_to_rtap8(param_bitrate);
	} else if (param_wifimode == 1) {					// 802.11n
		if (param_ldpc == 0) {
			u8aRadiotapHeader80211n[10] &= (~0x10);
			u8aRadiotapHeader80211n[11] &= (~0x10);
			u8aRadiotapHeader80211n[12] = (uint8_t)param_bitrate;
		} else {
			u8aRadiotapHeader80211n[10] |= 0x10;
			u8aRadiotapHeader80211n[11] |= 0x10;
			u8aRadiotapHeader80211n[12] = (uint8_t)param_bitrate;
		}
		
	}
	// copy radiotap header
	memcpy(buf+offset, p_rtheader, rtheader_length);
	return rtheader_length;
}

// return: ieeeheader_length
int packet_ieeeheader_init (int offset, uint8_t * buf, dictionary *ini)
{
	// default rts frame
	memcpy(buf+offset, u8aIeeeHeader_rts, sizeof(u8aIeeeHeader_rts));
	return sizeof(u8aIeeeHeader_rts);
}

int main (int argc, char *argv[]) 
{
	struct framedata_s framedata;
	uint8_t framedata_body_length = sizeof(framedata);
	uint8_t buf[512];
	bzero(buf, sizeof(buf));
	
	// Get ini from file
	char *file = argv[1];
	dictionary *ini = iniparser_load(file);
	if (!ini) {
		fprintf(stderr,"iniparser: failed to load %s.\n", file);
		exit(1);
	}
	if (argc != 2) {
		usage();
	}
	int param_mode = iniparser_getint(ini, "tx_rc_sbus:mode", 0);
	int param_retrans = iniparser_getint(ini, "tx_rc_sbus:retrans", 0);
	int param_debug = iniparser_getint(ini, "tx_rc_sbus:debug", 0); 
	int param_encrypt = iniparser_getint(ini, "tx_rc_sbus:encrypt", 0);
	char * param_password = (param_encrypt == 1)? (char *)iniparser_getstring(ini, "tx_rc_sbus:password", NULL): NULL;
	// init wifi raw socket
	if (param_mode == 0 || param_mode == 2) {
		char * nic_name = (char *)iniparser_getstring(ini, "tx_rc_sbus:nic", NULL);
		sockfd = open_sock(nic_name);
		usleep(20000); // wait a bit 
	}
	
	// init udp socket & bind
	int16_t port;
	struct sockaddr_in send_addr;
	struct sockaddr_in source_addr;	
	int slen = sizeof(send_addr);
	if (param_mode == 1 || param_mode == 2) {
		port = atoi(iniparser_getstring(ini, "tx_rc_sbus:udp_port", NULL));
		bzero(&send_addr, sizeof(send_addr));
		send_addr.sin_family = AF_INET;
		send_addr.sin_port = htons(port);
		send_addr.sin_addr.s_addr = inet_addr(iniparser_getstring(ini, "tx_rc_sbus:udp_ip", NULL));
		bzero(&source_addr, sizeof(source_addr));
		source_addr.sin_family = AF_INET;
		source_addr.sin_port = htons(atoi(iniparser_getstring(ini, "tx_rc_sbus:udp_bind_port", NULL)));
		source_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if ((udpfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) 
			printf("ERROR: Could not create UDP socket!");
		if (-1 == bind(udpfd, (struct sockaddr*)&source_addr, sizeof(source_addr))) {
			fprintf(stderr, "Bind UDP port failed.\n");
			exit(0);
		}
	}
	
	// init uart to b100000,8E2
	uartfd = open( (char *)iniparser_getstring(ini, "tx_rc_sbus:uart", NULL),
					O_RDWR|O_NOCTTY|O_NDELAY);	// not using |O_NONBLOCK now
	if (uartfd < 0) {
		fprintf(stderr, "open uart failed!\n"); 
	}
	int r = uart_sbus_init(uartfd);
	if (r == 0) {
        fprintf(stderr, "Set UART to S.BUS port successfully.\n");
    } else {
        perror("uart ioctl");
    }
	
	// init RSSI shared memory
	telemetry_data_t td;
	telemetry_init(&td);
	
	// init radiotap header to buf
	int rtheader_length = packet_rtheader_init(0, buf, ini);
	
	// init ieee header
	int ieeeheader_length = packet_ieeeheader_init(rtheader_length, buf, ini);
	
	// set length
	framedata.length = framedata_body_length;
	
	// Main loop
	uint32_t seqno = 0;
	int k = 0;
	int uart_read_len = 0;
	int full_packet_size = rtheader_length + ieeeheader_length + sizeof(framedata);
	int full_header_length = rtheader_length + ieeeheader_length;
	
	while (1) {
		// 1. Get data from UART
		uart_read_len = read(uartfd, buf +full_header_length +FRAME_DATA_OFFSET, 25);
		if (param_debug) {
			fprintf(stderr, "uart read() got %d bytes\n", uart_read_len);
			dump_memory(buf +full_header_length +FRAME_DATA_OFFSET, uart_read_len, "uart read");
		}
		if (uart_read_len == 0)
			continue;
		if (uart_read_len < 0) {
			fprintf(stderr, "uart read() got an error\n");
			continue;
		}
		
		// 2. fill frame seqno
		framedata.seqnumber = htonl(seqno);
		
		// 3. calculate crc
		// pass
		
		// 4.1 Send frame to air
		if (param_mode == 0 || param_mode == 2) {
			for (k = 0; k < param_retrans; k++) {
				if ( write(sockfd, buf, full_packet_size) < 0 ) {
					fprintf(stderr, "injection failed, seqno=%d\n", seqno);	// send failed
				}
				usleep(2000);
			}
		}
		
		// 4.2 Send frame to UDP
		if (param_mode == 1 || param_mode == 2) {
			for (k = 0; k < param_retrans; k++) {
				if (sendto(udpfd, buf +full_header_length, framedata_body_length, 0, 
									(struct sockaddr*)&send_addr, slen) == -1) {
					fprintf(stderr, "ERROR: UDP Could not send data! seqno=%d\n", seqno);
				} 
				usleep(2000); 
			}
		}
		
		// 
		seqno++;
//		usleep(3000);	// wait a bit (if in nonblock mode)
	}
	
	close(sockfd);
	if (param_mode == 0 || param_mode == 2)
		close(uartfd);
	if (param_mode == 1 || param_mode == 2)
		close(udpfd);
	iniparser_freedict(ini);
	return EXIT_SUCCESS;
}
