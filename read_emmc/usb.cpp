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

LibSerial::SerialStream s;
static const char *devname = "/dev";
static struct fuse_operations usb_oper;

unsigned int g_MaxMsgLevel = NULL;

//2406399*512 = 1232076288
//dd if=/dev/zero of=cache bs=512 count=2406399
//dd if=/dev/zero of=index bs=1 count=2406399
#define CACHE_FILENAME "cache"
#define INDEXCACHE_FILENAME "index"
static const char *cache_dir = "out";
static int cache_fd = -1;
static int indexcache_fd = -1;

extern int errno;

//static const unsigned int userdata_sector = 1343488; //offset in sectors from flash beginning
static const unsigned int userdata_sector = 6422528; //offset in sectors from flash beginning
//#define USERDATA_PARTITION_SIZE	1232076288 /* hardcoded for now for HTC Desire Z device, so ~1,2GB */
#define USERDATA_PARTITION_SIZE	27917287424 /* hardcoded for now for HTC One device, so ~26GB */
/* 6815744*4096 = 27917287424 */


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
int fastboot_oem_read_emmc(unsigned int startBlock, unsigned int nBlocks, char* outBuf)
{
	char input[256];
	char output[256];
	memset(input, 0, 256);
	memset(output, 0, 256);

	unsigned int totalSize = nBlocks*512*sizeof(char);
	memset(outBuf, 0, totalSize);

	// Prepare temporary buffer
	char* outBufTmp = (char*)malloc(totalSize*10);	//will contains fastboot protocol (INFO...INFO...OKAY)
	if (!outBufTmp) {
		printf("[X] read_emmc: malloc failed");
		return -1;
	}
  memset(outBufTmp, 0, totalSize*10);

	//Prepare command and sent it
	//command format: read_mmc [emmc/sd] [start] [#blocks] [#blocks/read] [show]
	sprintf(input, "oem read_mmc emmc %d %d %d 1", userdata_sector+startBlock, nBlocks, nBlocks);
	printf("[+] read_emmc (%d bytes): \"%s\"\n", strlen(input), input);
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
	hex_dump(outBufTmp, strlen(outBufTmp));

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
				nb = sscanf(pch, "%hhx", &outBuf[count]);
				count++;
				//printf("-> '%s' %d\n", pch, nb);
			} else
				break;
		}
	}
	printf("[+] read_emmc: done.\n");
	hex_dump(outBuf, totalSize);
	//free(outBufTmp);

	return 0;
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
int read_emmc(char* outBuf, unsigned int offset, unsigned int nBytes)
{ 
	//printf("[+] read_emmc start\n");
	const int LINE_SIZE = 256;
	char input[LINE_SIZE];
	char output[LINE_SIZE];

	memset(input, 0, LINE_SIZE);
	memset(output, 0, LINE_SIZE);
	void* res = NULL;

	char* prompt = NULL;
	unsigned int loop = 0;

	char indexcache_filename[256];
	char cache_filename[256];
	sprintf(indexcache_filename, "%s/%s", cache_dir, INDEXCACHE_FILENAME);
	sprintf(cache_filename, "%s/%s", cache_dir, CACHE_FILENAME);

	/* Some check to make it simple. If needed, the caller must ask for more and ignore what it does not want. */
	if ((nBytes < 512) || (offset % 512 != 0) || (nBytes % 512 != 0)) {
		printf("[+] read_emmc: not supporting: offset=%d, nBytes=%d\n", offset, nBytes);
		return -1; //Not supported for now.
	}
	
	/* Determine what sectors to read from all over (cached and HBOOT) */

	unsigned int nBlocks = nBytes / 512;
	unsigned int startBlock = offset / 512;
	printf("[+] read_emmc: blocks to get from everything: [%d, %d]\n", startBlock, startBlock+nBlocks-1);

	// Prepare our cache files
	if ((indexcache_fd = open(indexcache_filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO)) == -1) {
		printf("[x] read_emmc: can not open index cache file: %s\n", indexcache_filename);
		return -1;
	}
	if ((cache_fd = open(cache_filename, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO)) == -1) {
		printf("[x] read_emmc: can not open cache file: %s\n", cache_filename);
		return -1;
	}
	//printf("Opened handles: index=%d, cache=%d\n", indexcache_fd, cache_fd);

	/* Determine what sectors to read from all over from cached */

	// Do we have some data already?
	char* cacheTest = (char*)malloc(nBlocks*sizeof(char));
	if (!cacheTest) {
		printf("[x] read_emmc: malloc failed for reading index cache\n");
		return -1;
	}
	if (lseek(indexcache_fd, startBlock, SEEK_SET) != startBlock) {
		printf("[x] read_emmc: can not seek in index cache file for reading: %s\n", indexcache_filename);
		return -1;
	}
	if (read(indexcache_fd, cacheTest, nBlocks) != (int)nBlocks) {
		printf("[x] read_emmc: read failed for index cache\n");
		return -1;
	}

	unsigned int startBlockCached = startBlock;
	unsigned int nBlocksCached = 0;
	for (unsigned int i=0; i<nBlocks; i++) {
		if (cacheTest[i] == 0x00)
			break;
		nBlocksCached++;
	}
	if (cacheTest) {
		free(cacheTest);
		cacheTest = NULL;
	}

	/* Conclude what sectors to read from all over from HBOOT */

	unsigned int startBlockHBOOT = startBlock+nBlocksCached;
	unsigned int nBlocksHBOOT = nBlocks-nBlocksCached;
	unsigned int nBlocksHBOOTPerRead = nBlocksHBOOT;
	unsigned int nBytesReadHBOOT = 0;
	unsigned int nBytesReadPerLineHBOOT = 0;

	printf("[+] read_emmc: blocks to get from cache: [%d, %d] - %d blocks = %d bytes\n", 
				startBlockCached, startBlockCached+nBlocksCached-1, nBlocksCached, nBlocksCached*512);
	printf("[+] read_emmc: blocks to get from HBOOT: [%d, %d] - %d blocks = %d bytes\n", 
				startBlockHBOOT, startBlockHBOOT+nBlocksHBOOT-1, nBlocksHBOOT, nBlocksHBOOT*512);

	/* Get data from cache */

	if (nBlocksCached > 0)
	{
		printf("[+] read_emmc: getting data from cache\n");

		// Prepare temporary buffer
		char* outBufTmpCached = (char*)malloc(nBlocksCached*512*sizeof(char));
		if (!outBufTmpCached) {
			printf("[x] read_emmc: malloc failed for outBufTmpCached\n");
			return -1;
		}
		off_t res1 = lseek(cache_fd, startBlockCached*512, SEEK_SET);
		if (res1 != startBlockCached*512) { //We support userdata partition size < 2^32 ~ 4GB
			printf("[x] usb_read: can not seek in cache file for outBufTmpCached: %s\n", cache_filename);
			perror("lseek failed: ");
			return -1;
		}
		//printf("lseek success: %d\n", (int)res1);
		ssize_t res = read(cache_fd, outBufTmpCached, nBlocksCached*512);
		if (res != (int)(nBlocksCached*512)) {
			printf("[x] read_emmc: read failed for index cache for outBufTmpCached: %d\n", res);
			perror("read failed: ");
			return -1;
		}
		memcpy(outBuf, outBufTmpCached, nBlocksCached*512); //Assume offset % 512 == 0
	}
	else
		printf("[+] read_emmc: no data to get from cache\n");


	/* Get data from HBOOT */

	if (nBlocksHBOOT > 0) 
	{
		printf("[+] read_emmc: getting data from HBOOT\n");

		printf("[+] read_emmc: userdata start at sector: %d\n", userdata_sector);
		printf("[+] read_emmc: offset in userdata: %d\n", startBlockHBOOT);

		// Prepare temporary buffer
		char* outBufTmpHBOOT = (char*)malloc(nBlocksHBOOT*512*sizeof(char));
		if (!outBufTmpHBOOT) {
			printf("[x] read_emmc: malloc failed\n");
			return -1;
		}

    // Get data from fastboot oem read mmc command
    fastboot_oem_read_emmc(startBlockHBOOT, nBlocksHBOOT, outBufTmpHBOOT);
	
		//memcpy(outBuf, outBufTmpHBOOT + (offset % 512), nBytes);
		printf("[+] read_emmc: copying data from outBufTmpHBOOT=%lx to outBuf=%lx now\n", (unsigned long)outBufTmpHBOOT, (unsigned long)outBuf);
		printf("[+] read_emmc: memcpy(%lx, %lx, %d)\n", (unsigned long)(outBuf + nBlocksCached*512), (unsigned long)outBufTmpHBOOT, nBlocksHBOOT*512);
		memcpy(outBuf + nBlocksCached*512, outBufTmpHBOOT, nBlocksHBOOT*512); //Assume offset % 512 == 0

		//hex_dump(outBuf, 0x500);
	
		// Write all read sectors into cache (do not check if we already saved them, do not worry)
		if (lseek(cache_fd, startBlockHBOOT*512, SEEK_SET) != startBlockHBOOT*512) { //We support userdata partition size < 2^32 ~ 4GB
			printf("[x] usb_read: can not seek in cache file: %s\n", cache_filename);
			return -1;
		}
		if (write(cache_fd, outBufTmpHBOOT, nBlocksHBOOT*512) != (int)(nBlocksHBOOT*512)) {
			printf("[x] read_emmc: write failed for cache\n");
			return -1;
		}
			
		// And update index cache of already saved sectors (same no-check here)
		// For now, a byte holds 1 index value: 0x1 says one sector in cache is saved, 0x0 says it is not saved yet
		if (lseek(indexcache_fd, startBlockHBOOT, SEEK_SET) != startBlockHBOOT) {
			printf("[x] usb_read: can not seek in index cache file for writing: %s\n", indexcache_filename);
			return -1;
		}
		char* ones = (char*)malloc(nBlocksHBOOT*sizeof(char));
		if (!ones) {
			printf("[x] read_emmc: malloc failed for updating index cache\n");
			return -1;
		}
		memset(ones, 0x01, nBlocksHBOOT*sizeof(char));
		if (write(indexcache_fd, ones, nBlocksHBOOT) != (int)nBlocksHBOOT) {
			printf("[x] read_emmc: write failed for index cache\n");
			return -1;
		}
		if (ones) {
			free(ones);
			ones = NULL;
		}
	
		if (outBufTmpHBOOT) {
			free(outBufTmpHBOOT);
			outBufTmpHBOOT = NULL;
		}

	} /* End of getting data from HBOOT */
	else
		printf("[+] read_emmc: no data to get from HBOOT\n");

	// Free resources
	close(cache_fd);
	close(indexcache_fd);

	//printf("[+] read_emmc end: %d\n", nBytes);
	return nBytes;
}

/**
 * getattr FUSE function
 * @param 
 * @see fuse_operations structure from fuse.h
 * @return
 */
static int usb_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if(strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if(strcmp(path, devname) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		//stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = USERDATA_PARTITION_SIZE; //hardcoded for now
	}
	else
		res = -ENOENT;

	//printf("[+] usb_getattr end: %d\n", res);
	return res;
}

/**
 * readdir FUSE function
 * @param 
 * @see fuse_operations structure from fuse.h
 * @return
 */
static int usb_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			             off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if(strcmp(path, "/") != 0) {
		//printf("[+] usb_readdir error: %s\n", path);
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, devname + 1, NULL, 0);

	//printf("[+] usb_readdir end\n");
	return 0;
}

/**
 * open FUSE function
 * @param 
 * @see fuse_operations structure from fuse.h
 * @return
 */
static int usb_open(const char *path, struct fuse_file_info *fi)
{
	//printf("[+] usb_open start\n");

	if(strcmp(path, devname) != 0) {
		//printf("[x] usb_open path error: %s\n", path);
		return -ENOENT;
	}

	/*if((fi->flags & 3) != O_RDONLY) {
		//printf("[x] usb_open flags error: %x\n", fi->flags);
		return -EACCES;
	}*/

	s.Open("/dev/ttyUSB0");

	//printf("[+] usb_open end\n");
	return 0;
}

/**
 * write FUSE function
 * @param 
 * @see fuse_operations structure from fuse.h
 * @return
 */
/*static int usb_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("[+] usb_write(%s, %x, %d, %x, %x)\n", path, (unsigned int)buf, size, (unsigned int)offset, (unsigned int)fi);

	return size; //fake write function
}*/

unsigned int bFirst = 1;

/**
 * read FUSE function
 * @param 
 * @see fuse_operations structure from fuse.h
 * @return
 */
static int usb_read(const char *path, char *buf, size_t size, off_t offset,
			          struct fuse_file_info *fi)
{
	printf("[+] usb_read(%s, %lx, %d, %lx, %lx)\n", path, (unsigned long)buf, size, (unsigned long)offset, (unsigned long)fi);

	(void) fi;
	size_t out_size = 0;

	// Read from our fake device only interests us here
	if(strcmp(path, devname) != 0) {
		//printf("[+] usb_read path error: %s\n", path);
		return -ENOENT;
	}

	// On first try, get the minimal bloc 
	if (bFirst == 1) {
		char buf2[512*4+1];
		printf("[+] First shot, initializing blocs 0-95\n");
		for (int bloc=0; bloc<96/4; bloc++) {
			printf("[+] read_emmc(%lx, %lx, %d, %d)\n", (unsigned long)&s, (unsigned long)buf2, bloc*512*4, 512*4);
			if (read_emmc(buf2, bloc*512*4, 512*4) == -1) {
				printf("[x] -> failed\n");
		  	return 0;
			}
		}
		bFirst = 0;
	}

	// Process HBOOT communication
	int res = read_emmc(buf, offset, size);
	if (res == -1)
		out_size = 0;
	else
		out_size = res;

/* 
  There is a bug in Linux: https://groups.google.com/group/ext3grep/browse_thread/thread/b3b30260742950a5
  We can NOT mount an ext3 partition in read only (ro) if the needs_recovery flag is set.
 
  The offset and values are set here: http://jessland.net/JISK/Forensics/Areas/Disks_and_Filesystems/ext.php
  Boot block is 1024 bytes
  Then follows superblock and we have offsets/values as following:
 	Offset(decimal)	Description
	92   [4]  	Compatible   feature flags
	96   [4]  	Incompatible feature flags
	100  [4]  	Read only    feature flags

  Features:
		   * Compatible::
			    0x0001  Preallocate dir blocks to reduce fragmentation
			    0x0002  AFS server inodes exist
			    0x0004  File system has a journal (Ext3)
			    0x0008  Inodes have extended attributes
			    0x0010  File system can resize itself for larger partitions
			    0x0020  Dirs use hash index
		   * Incompatible:
			    0x0001  Compression
			    0x0002  Dir. entries contain a file type field
			    0x0004  File system needs recovery
			    0x0008  File system uses a journal device
		   * Read only compatible:
			    0x0001  Sparse Super-Block and GDTs
			              Enabled by default
			              Copies typically in: 1, 3, 7, 9, 25, 27
			    0x0002  File system contains a large file
			    0x0004  Dirs. use B-Trees (not implemented)

  So we fake it so we are able to mount it: $ mount -o loop,ro -t ext2 mnt/dev mnt2
 */
#define OFFSET_COMPATIBLE_FLAGS 0x45C
#define OFFSET_INCOMPATIBLE_FLAGS 0x460
	/*if (out_size > 0) {
		if (offset <= OFFSET_INCOMPATIBLE_FLAGS && OFFSET_INCOMPATIBLE_FLAGS <= (offset+size)) {
			if (offset == 0) {
				printf("[+] FAKE INCOMPATIBLE flag was used, old=0x%x", buf[OFFSET_INCOMPATIBLE_FLAGS]);
				buf[OFFSET_INCOMPATIBLE_FLAGS] = 0x2;
				printf(", new=0x%x\n", buf[OFFSET_INCOMPATIBLE_FLAGS]);
			} else {
				printf("[+] usb_read: offset=0x%x, size=0x%x not supported yet\n", (unsigned int)offset, size);
			}
		}
	}*/

	//printf("[+] usb_read end -> %d bytes\n", out_size);
	return out_size;
}

/**
 * release FUSE function
 * @param 
 * @see fuse_operations structure from fuse.h
 * @return
 */
static int usb_release(const char *path, struct fuse_file_info *fi)
{
	//printf("[+] usb_release start\n");

	(void) fi;
	if(strcmp(path, devname) != 0)
		return -ENOENT;

	s.Close();

	//printf("[+] usb_release end\n");
	return 0;
}

int main(int argc, char *argv[])
{
  printf("[+] Starting...\n");
	memset(&usb_oper, 0, sizeof(struct fuse_operations));
	usb_oper.getattr = usb_getattr;
	usb_oper.readdir = usb_readdir;
	usb_oper.open = usb_open;
	//usb_oper.write = usb_write;
	usb_oper.read = usb_read;
	usb_oper.release = usb_release;

	return fuse_main(argc, argv, &usb_oper);
}
