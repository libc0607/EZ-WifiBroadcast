
// Usage: ./tx_rc_sbus config.ini
/*

[tx_rc_sbus]
mode=0	# 0-air, 1-udp, 2-both
nic=wlan0				// optional, when mode set to 0or2
udp_ip=127.0.0.1		// optional, when mode set to 1or2
udp_port=30302			// optional, when mode set to 1or2
udp_bind_port=30300		// optional, when mode set to 1or2
uart=/dev/ttyUSB0
bitrate=6				// Mbit
retrans=2

*/
//


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
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include "lib.h"

int sockfd;
int uartfd;
int udpfd;

struct framedata_s 
{
	// header 16 bytes
    uint8_t rt1;
    uint8_t rt2;
    uint8_t rt3;
    uint8_t rt4;
    uint8_t rt5;
    uint8_t rt6;
    uint8_t rt7;
    uint8_t rt8;
    uint8_t rt9;
    uint8_t rt10;
    uint8_t rt11;
    uint8_t rt12;
    uint8_t fc1;
    uint8_t fc2;
    uint8_t dur1;
    uint8_t dur2;
	
	// body
	uint32_t crc;				// crc of full body; 4+1+4+4+25=38bytes now
								// little endian, htonl() & ntohl()
								// fill crc with crc_magic first, 
								// calculate crc of body, then fill back in crc;
	uint8_t length;				// body max 255 bytes
    uint32_t seqnumber;
	uint8_t info[4];			// for future use, now fill with 0
    uint8_t sbus_data[25];

} __attribute__ ((__packed__));
/* 	
framedata.rt1 = 0; // <-- radiotap version
framedata.rt2 = 0; // <-- radiotap version
framedata.rt3 = 12; // <- radiotap header length
framedata.rt4 = 0; // <- radiotap header length
framedata.rt5 = 4; // <-- radiotap present flags 00000100 10000000 00000000 00000000
framedata.rt6 = 128; // <-- radiotap present flags
framedata.rt7 = 0; // <-- radiotap present flags
framedata.rt8 = 0; // <-- radiotap present flags
framedata.rt9 = 24; // <-- radiotap rate
framedata.rt10 = 0; // <-- radiotap stuff
framedata.rt11 = 0; // <-- radiotap stuff
framedata.rt12 = 0; // <-- radiotap stuff
framedata.fc1 = 180; // <-- frame control field (0xb4)	// fc1 & fc2
framedata.fc2 = 191; // <-- frame control field (0xbf)	// used in pcap filter
framedata.dur1 = 0; // <-- duration
framedata.dur2 = 0; // <-- duration 
*/
struct framedata_s framedata;
static const unsigned int crc32_table[] =
{
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
  0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
  0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
  0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
  0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
  0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
  0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
  0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
  0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
  0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
  0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
  0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
  0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
  0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
  0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
  0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
  0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
  0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
  0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
  0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
  0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
  0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
  0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
  0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
  0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
  0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
  0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
  0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
  0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
  0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
  0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
  0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
  0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
  0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
  0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};
static const uint32_t crc_magic = 0x11451419;	// 810!
static const uint8_t framedata_body_length = sizeof(framedata) -16;	// now is 38, const
static const uint8_t framedata_radiotap_header[16] = {
	0, 0, 12, 0, 4, 128, 0, 0, 
	24, 0, 0, 0, 180, 191, 0, 0
};
static const uint32_t crc32_startvalue = 0xffffffff;

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

unsigned int xcrc32 (const unsigned char *buf, int len, unsigned int init)
{
	unsigned int crc = init;
	while (len--) {
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}

static int open_sock (char *ifname) 
{
    struct sockaddr_ll ll_addr;
    struct ifreq ifr;

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

void fill_frame(uint32_t seqno) {
	framedata.seqnumber = htonl(seqno);
	// Calculate CRC32 
	framedata.crc = htonl(crc_magic);
	framedata.crc = htonl( xcrc32( (&framedata)+16, (int)framedata_body_length, crc32_startvalue) );
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
	switch (bitrate) {
		case  1: framedata[8]=0x02; break;
		case  2: framedata[8]=0x04; break;
		case  5: framedata[8]=0x0b; break;
		case  6: framedata[8]=0x0c; break;
		case 11: framedata[8]=0x16; break;
		case 12: framedata[8]=0x18; break;
		case 18: framedata[8]=0x24; break;
		case 24: framedata[8]=0x30; break;
		case 36: framedata[8]=0x48; break;
		case 48: framedata[8]=0x60; break;
		default: fprintf(stderr, "ERROR: Wrong or no data rate specified\n"); exit(1); break;
	}

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

int main (int argc, char *argv[]) 
{
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
	int param_bitrate = iniparser_getint(ini, "tx_rc_sbus:bitrate", 0);
	int param_debug = iniparser_getint(ini, "tx_rc_sbus:debug", 0); 
	// init wifi raw socket
	if (param_mode == 0 || paran_mode == 2) {
		sockfd = open_sock(iniparser_getstring(ini, "tx_rc_sbus:nic", NULL));
		usleep(20000); // wait a bit 
	}
	
	// init udp socket & bind
	int16_t port;
	struct sockaddr_in send_addr;
	struct sockaddr_in source_addr;	
	int slen = sizeof(send_addr);
	if (param_mode == 1 || paran_mode == 2) {
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
	// set uart to b100000: See https://github.com/cbrake/linux-serial-test
	uartfd = open( iniparser_getstring(ini, "tx_rc_sbus:uart", NULL),
					O_RDWR|O_NOCTTY|O_NDELAY);						// not using |O_NONBLOCK now
	if (uartfd < 0) {
		fprintf(stderr, "open uart failed!\n"); 
	}
	if (fcntl(uartfd, F_SETFL, 0) < 0) {
		fprintf(stderr, "fcntl F_SETFL failed!\n"); 
	} else {
		fprintf(stderr, "fcntl=%d\n", fcntl(uartfd, F_SETFL, 0)); 
	}
	if (isatty(uartfd) == 0) {
		fprintf(stderr, "Not a terminal device. exit\n"); 
		exit(0);
	}
	struct termios termios_conf;
	bzero(&termios_conf, sizeof(termios_conf));
	termios_conf.c_cflag |= CS8;			//8bit
	termios_conf.c_iflag |= (INPCK | ISTRIP); 
	termios_conf.c_cflag |= PARENB; 
	termios_conf.c_cflag &= ~PARODD; 		// odd
	termios_conf.c_cflag |= CSTOPB;			// 2 stop bits
	set_baud_custom(uartfd, 100000);
	if ( (tcsetattr(uartfd, TCSANOW, &termios_conf))!= 0 ) { 
		fprintf(stderr, "uart setting error.\n"); 
		exit(0); 
	} 
 
	// init RSSI shared memory
	telemetry_data_t td;
	telemetry_init(&td);
	
	// Copy header to framedata
	memcpy(&framedata, framedata_radiotap_header, 16);
	// Set bitrate
	framedata[8] = bitrate_to_rtap8(param_bitrate);
	// set length
	framedata.length = framedata_body_length;
	
	// Main loop
	uint32_t seqno = 0;
	int k = 0;
	int uart_read_len = 0;
	while (1) {
		// Get data from UART
		uart_read_len = read(uartfd, (&framedata)+16+13, 25);
		if (param_debug) {
			fprintf(stderr, "uart read() got %d bytes\n");
			dump_memory((&framedata)+16+13, uart_read_len, "uart read");
		}
		if (uart_read_len == 0)
			continue;
		if (uart_read_len < 0) {
			fprintf(stderr, "uart read() got an error\n");
			continue;
		}
		// fill frame crc & seqno
		fill_frame(seqno);
		// Send frame to air
		if (param_mode == 0 || param_mode == 2) {
			for (k = 0; k < param_retrans; k++) {
				if ( write(sockfd, &framedata, sizeof(framedata)) < 0 ) {
					fprintf(stderr, "injection failed, seqno=%d\n", seqno);	// send failed
				}
				usleep(2000); // wait 2ms between sending multiple frames to lower collision probability
			}
		}
		
		// Send frame to UDP
		if (param_mode == 1 || param_mode == 2) {
			for (k = 0; k < param_retrans; k++) {
				if (sendto(udpfd, (&framedata)+16, framedata_body_length, 0, 
									(struct sockaddr*)&send_addr, slen) == -1) {
					fprintf(stderr, "ERROR: UDP Could not send data! seqno=%d\n", seqno);
				} 
				usleep(2000); // wait 2ms between sending multiple frames to lower collision probability
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
