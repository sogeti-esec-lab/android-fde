#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <fstream>

//http://libserial.sourceforge.net/doxygen/
#include <iostream>
#include <SerialStream.h>

// /usr/include/fuse/fuse.h
#include <fuse.h>

using namespace std;

//http://www.cplusplus.com/reference/istream/basic_iostream/
LibSerial::SerialStream s;

unsigned int g_MaxMsgLevel = NULL;

extern int errno;

//static const unsigned int userdata_sector = 6422528; //offset in sectors from flash beginning
static const unsigned int userdata_sector = 0; //debug
#define USERDATA_PARTITION_SIZE	1232076288 /* hardcoded for now for HTC Desire Z device, so ~1,2GB */

// http://sws.dett.de/mini/hexdump-c/
static void hex_dump(void *data, int size)
{
	/* dumps size bytes of *data to stdout. Looks like:
	 * [0000] 75 6E 6B 6E 6F 77 6E 20
	 *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
	 * (in a single line of course)
	 */

	unsigned char *p = (unsigned char*)data;
	unsigned char c;
	int n;
	char bytestr[4] = {0};
	char addrstr[10] = {0};
	char hexstr[ 16*3 + 5] = {0};
	char charstr[16*1 + 5] = {0};
	for(n=1;n<=size;n++) {
		if (n%16 == 1) {
			/* store address for this line */
			snprintf(addrstr, sizeof(addrstr), "%.4lx", ((uintptr_t)p-(uintptr_t)data) );
		}
			
		c = *p;
		if (isalnum(c) == 0) {
			c = '.';
		}

		/* store hex str (for left side) */
		snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
		strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

		/* store char str (for right side) */
		snprintf(bytestr, sizeof(bytestr), "%c", c);
		strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

		if(n%16 == 0) { 
			/* line completed */
			printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
			hexstr[0] = 0;
			charstr[0] = 0;
		} else if(n%8 == 0) {
			/* half line: add whitespaces */
			strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
			strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
		}
		p++; /* next byte */
	}

	if (strlen(hexstr) > 0) {
		/* print rest of buffer if not empty */
		printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
	}
}

/**
 * Reads data from an opened serial line over USB from HBOOT/fastboot supporting read_emmc function
 * @param s				an opened serial-over-USB device
 * @param outBuf	a buffer to save read data
 * @param	offset	the offset in bytes from the start of /data partition
 * @param	nBytes	the nb of bytes to read
 * @see None
 * @return 				the nb of read bytes of -1 if error
 */
int read_emmc(unsigned int startBlock, unsigned int nBlocks, char* outfile)
{
	char input[256];
	char output[256];
	memset(input, 0, 256);
	memset(output, 0, 256);

	unsigned int totalSize = nBlocks*512*sizeof(char);

	// Prepare temporary buffer
	char* outBufTmp = (char*)malloc(totalSize*10);	//will contains fastboot protocol (INFO...INFO...OKAY)
	if (!outBufTmp) {
		printf("[X] read_emmc: malloc failed");
		return -1;
	}
	char* outBufTmp2 = (char*)malloc(totalSize);
	if (!outBufTmp2) {
		printf("[X] read_emmc: malloc2 failed");
		return -1;
	}
	memset(outBufTmp2, 0, totalSize+1);

	//Prepare command and sent it
	//command format: read_mmc [emmc/sd] [start] [#blocks] [#blocks/read] [show]
	sprintf(input, "oem read_mmc emmc %d %d %d 1", userdata_sector+startBlock, nBlocks, nBlocks);
	printf("[+] read_emmc: writing (%d): \"%s\"\n", strlen(input), input);
	s.write(input, strlen(input));
	sleep(2);
	//printf("[+] read_emmc: reading \n");
	char* ptr = outBufTmp;
	while (1) {
		memset(output, 0, 256);
		s.readsome(output, 256);	//in practice, read one char at a time...
		//printf(".");
		//hex_dump(output, strlen(output));
		memcpy(ptr, output, strlen(output));
		ptr += strlen(output);
		if (strstr(outBufTmp, "OKAY"))
			break;
	}

	printf("[+] read_emmc: finished reading\n");
	//hex_dump(outBufTmp, strlen(outBufTmp));

	memset(output, 0, 256);
	char* pch = strtok(outBufTmp, "I");
	pch = strtok(NULL, "O"); //skip header
	char* end = NULL;
	unsigned int count=0;
	int nb = 0;
	while (1) {
		//printf("%08X %s\n", pch, pch);
		pch = strtok(NULL, "O"); //"O" in INFO or OKAY
		if (pch) {
			//INFO or OKAY found
			end = strstr(pch, " ");
			if (end) {
				end[0] = 0;
				nb = sscanf(pch, "%hhx", &outBufTmp2[count]);
				count++;
				//printf("-> '%s' %d\n", pch, nb);
			} else
				break;
		}
	}
	printf("[+] done.\n");
	//hex_dump(outBufTmp2, totalSize);

	ofstream myfile;
	myfile.open(outfile);
	myfile.write(outBufTmp2, totalSize);
	myfile.close();

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
		printf("Usage: %s <start> <num> <hboot_output_filename>\n", argv[0]);
		exit(1);
	}
	s.Open("/dev/ttyUSB0");
	read_emmc(atoi(argv[1]),atoi(argv[2]),argv[3]);
	s.Close();
}

