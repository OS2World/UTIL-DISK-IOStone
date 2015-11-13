/*
*
*   "I/O Stone" Benchmark Program
*
*   Written by: Arvin Park (park@midas.ucdavis.edu) and
*            Jeff Becker (becker@iris.ucdavis.edu)
*            Division of Computer Science
*            University of California, Davis
*            Davis CA 95616
*            (916) 752-5183
*
*   Version C/II
*   Date: 06/27/90
*
*   Defines: If your version of "C" does not include an ftime()
*            function or the C library time.h, define NOTIME.
*            Use a stopwatch to measure elapsed wall time.
*            Divide 2,000,000 by the elapsed time to get
*            the correct number of iostones/second.
*
*   To compile:   cc -O io.c -o io
*
*   Note:    [1] This program should be run without other processes
*            competing for system resources. Run it in the dead of
*            night if you have to.
*
*            [2] This program uses 5 megabytes of disk space. Make
*            sure that at least this much space is available on
*            your file system before you run the program.
*
*   Results: If you get results from a new (machine/operating
*            system/disk controller and drive) combination, please
*            send them to becker@iris.ucdavis.edu. Please include
*            complete information on the machine type, operating
*            system, version, disk controller, and disk drives.
*            Also make a note of any system modifications that
*            have been performed.
*
*-------------------------------------------------------------------------
* 8/26/91 Tin Le
*	Added simple Makefile.
*
*	As far as I can determine from examining the code, iostone is
*	meant for benchmarking file I/O and buffer cache efficiencies.
*
*	It does this by creating NSETS (4) of SET_SIZE (99) files.  Then
*	iostone performs I/O on each file in each set.  The type of I/O is
*	randomly picked (r/w).
*
*--------------------------------------------------------------------------
* 7/21/93 Oddgeir Kvien, kvien@elkraft.unit.no
*
* Slightly modified to compile with Borland C++ for OS/2
*
*--------------------------------------------------------------------------
* 11/17/93 Ketil Kintel, kintel@hsr.no
*
* -Fixed bug with the file name generation for the spacer file.
* -Slightly modified to show some output while running. Users with slow
* systems will shurely appriciate this. (I do ...)
* This may cause IOStone to show a _slightly_ lower value than it would do
* normally. If you don't want it #define NOEXTRAOUTPUT
* -Added code for average read/write troughput.
* -Added code for number of file opens/closes.
* -Modified to compile on a 16bit compiler also (Bolrand C++ 3.11 DOS)
*  An int ain't always an int...
*
*/

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys\stat.h>
#include <fcntl.h>

#define BFLUSH_FILE_SIZE (512L*1024L)   /*size of files used to flush buffers*/
#define NBFLUSH_FILES 8                 /*number of files used to flush buffers*/
#define NBLOCKSIZES 9                   /*number of different block sizes*/
#define SEED 34710373L                  /*random number generator seed*/
#define CONST 500000L                   /*iostone normalization constant*/
#define ITER 4                          /*number of iterations of the code*/
#define BUFFERSIZE (16L*1024L)		/*size of temporary buffer*/

/* #define NOEXTRAOUTPUT 1	/* Define if you dont want the extra    */
							/* time consuming output while running. */

#define CR=13 /* Carriage Return on MS-DOS system */

/* define only one of the next three defines */
/*#define NOTIME		/* Define if no time function in library */
/*#define TIME			/* Use UNIX time function */
#define NON_UNIX_TIME 1        /* Use if a non-unix system */

#define NSETS 4                         /*number of sets of files*/
#define SET_SIZE 99                     /*number of files in each set*/
#define FNAMELEN  8                     /*maximum file name length*/

/* char *malloc(); */
char tmp[FNAMELEN];                     /*a temporary string*/
char *files[NSETS][SET_SIZE];           /*array of file names*/
char *buf_flush_files[NBFLUSH_FILES];   /*array of names of files to flush*/
													 /*system buffers*/
char buffer[BUFFERSIZE];                /*a temporary buffer*/

long int nbytes;                        /*number of bytes transfered*/
long int fd;                                 /*file descriptor*/
long int i,j,k;                              /*counter variables*/
long bsize[NBLOCKSIZES];                /*array for different block sizes*/
long bfreq[NBLOCKSIZES];                 /*number of accesses for each block*/

#ifdef TIME
#include <sys\types.h>
#include <sys\timeb.h>
struct timeb before;
struct timeb after;
long int sec;
long int msec;
#endif

#ifdef NON_UNIX_TIME
long int starttime;
long int totaltime;
#endif

/* PREDEFINISJON */
void init(void);
long int my_rand(int max);
void initfile(char *fname,long fsize);
void readswrites(void);

unsigned long int troughr; /* Counts number read IO bytes */
unsigned long int troughw; /* Counts number write IO bytes */
unsigned long int fileoc; /* Counts number of file opens/closes */

void main(void) {

  printf("\nWait - IOSTONE is setting up test files:\t");
  init();
  printf("\rWait - IOSTONE is performing disk IO with varying blocksizes.\t\n");
							/*start timing*/
#ifdef NOTIME
  printf("start timing\n");
#endif
#ifdef TIME
  ftime(&before);
#endif
#ifdef NON_UNIX_TIME
  starttime = time(0);
#endif

  for(k=0; k<ITER; k++)			/*perform string of file operations*/
  {
#ifndef NOEXTRAOUTPUT
	printf("\r\t\t\t\t\t\t\rPass %d:\t",(k+1));
#endif
	readswrites();
  }
					/*stop timimg*/
#ifdef NOTIME
  printf("stop timing\n");
#endif
#ifdef TIME
  ftime(&after);
  sec = after.time - before.time;
  msec = after.millitm - before.millitm;
  if (msec < 0) {			/*adjust if fractional time < 0*/
	sec -= 1;
	 msec += 1000;
  }
  printf("\r\t\t\t\t\t\t\nTotal elapsed time is %d seconds and %d milliseconds.\n",sec,msec);
  printf("Files were opened %u times. %u kb read. %u kb written. \n",
	fileoc,((troughr+512)/1024),((troughw+512)/1024));
  if (sec!=0 || msec!=0)
  {
	printf("Average read/write troughput was %.0f kb/sec, including open/close overhead.\n",
	  (((troughr+troughw)/((float) sec + (float) msec/1000)) + 0.5)/1024);

	 printf("\nThis machine benchmarks at %.0f iostones/sec.\n\n",
		((float)(CONST*ITER)/((float) sec + (float) msec/1000)) + 0.5);
  }

#endif
#ifdef NON_UNIX_TIME
  totaltime = time(0) - starttime;
  printf("\r\t\t\t\t\t\t\nTotal elapsed time is %ld sec.\n",totaltime); /* Uses multipple lines */
  printf("Files were opened %u times.",fileoc);             /* because of bug in BorlandC */
  printf(" %u kb was read, and",((troughr+512)/1024));
  printf(" %u kb written.\n",((troughw+512)/1024));
  if (totaltime!=0)
  {
	printf("Average read/write troughput was %.0f kb/sec, including open/close overhead.\n",
	  (((troughr+troughw)/((float) totaltime)) + 0.5)/1024);

	printf("\nThis machine benchmarks at %.0f iostones/sec.\n\n",
		((float)(CONST*ITER)/((float) totaltime)) + 0.5);
  }

#endif
  for (i=0;i<NSETS;i++)
	for (j=0;j<SET_SIZE;j++)
		unlink(files[i][j]);              /*remove files*/
  for (k=0;k<NBFLUSH_FILES;k++)
	unlink(buf_flush_files[k]);
}

void init(void) {

  long int this_set;                         /*mark the file set (0..NSETS-1)*/
  long int bcount;                           /*counter to track #spacer files*/
  long int fcount;                           /*a counter to tell where to create*/
										/*files to flush buffer cache and*/
										/*spread other files across disk*/
  bsize[0]=256; bfreq[0]=128;
  bsize[1]=512; bfreq[1]=64;
  bsize[2]=1024; bfreq[2]=64;
  bsize[3]=2048; bfreq[3]=64;
  bsize[4]=4096; bfreq[4]=32;           /*set file block sizes and*/
  bsize[5]=8192; bfreq[5]=32;           /*access frequencies*/
  bsize[6]=16384; bfreq[6]=8;
  bsize[7]=32768; bfreq[7]=2;
  bsize[8]=65536; bfreq[8]=2;

  troughr=0;
  troughw=0;
  fileoc=0;

  k=0;                                  /*set up files*/
  bcount=0;
  fcount=0;
  for(i=0;i<NBLOCKSIZES;i++) {
	printf(".");
	for(j=0;j<bfreq[i];j++) {
	  if (i<NBLOCKSIZES-1)
	this_set = j%NSETS;
	  else
	this_set = (j+2)%NSETS;
		sprintf(tmp,"%0d_%0d",i,j);       /*create filename*/
		files[this_set][k] = malloc(1+strlen(tmp));
	  if (!files[this_set][k]) {
	printf("Could not allocate string for filename\n");
	exit(1);
	  }
		strcpy(files[this_set][k],tmp);
	  initfile(tmp,bsize[i]);
		if (i < NBLOCKSIZES-1 && this_set == NSETS-1) k++;
		if (bcount < NBFLUSH_FILES && fcount%44 == 0) {
	sprintf(tmp,"s_%0d",bcount);       /*create spacer file*/
	buf_flush_files[bcount] = malloc(1+strlen(tmp));
	if (!buf_flush_files[bcount]) {
	  printf("Could not allocate string for filename\n");
	  exit(1);
	}
	strcpy(buf_flush_files[bcount],tmp);
	initfile(tmp,BFLUSH_FILE_SIZE);
	bcount++;
		}
		fcount++;
	 }
  }

  for(i=0;i<NBFLUSH_FILES;i++) {        /*read spacer files to flush buffers*/
	 if ((fd = open(buf_flush_files[i],O_RDWR))<0) /* UNIX 2 */ {
		printf("\nError opening buffer flush file %d of %d.\n",i+1,NBFLUSH_FILES);
	  exit(1);
	 }
	 lseek(fd,0L,0);
	k = BFLUSH_FILE_SIZE/BUFFERSIZE;
	 for(j=0;j<k;j++) {
	  if((nbytes = read(fd,buffer,BUFFERSIZE))<0) {
	printf("\nError reading buffer flush file %d of %d.\n",j+1,k);
	exit(1);
	  }
	}
	 close(fd);
  }

#ifdef NON_UNIX_TIME
  srand(SEED);
#else
  srandom(SEED);			/*initialize random number generator*/
#endif					/*and order files in a random*/
  for(i=0;i<NSETS;i++) {		/*permutation for reading/writing*/
	 for(j=SET_SIZE;j>0;j--) {
		k=my_rand(j);
		strcpy(tmp,files[i][j-1]);
		strcpy(files[i][j-1],files[i][k]);
		strcpy(files[i][k],tmp);
	 }
  }
}

long int my_rand(int max)
{
#ifdef NON_UNIX_TIME
  return rand()%max;
#else
  return random()%max;
#endif
}


void initfile(char *fname,long fsize)
{                                       /*create a temporary file*/
  FILE *fs;
  long int block, num_blocks;

  if((fs=fopen(fname,"w"))==NULL){
	 printf("init: Cannot create temporary file\n");
	 exit(1);
  }
  rewind(fs);				/*write initial portion of file*/
  if (fsize > BUFFERSIZE) {
	 num_blocks=fsize/BUFFERSIZE;
	 for(block=0;block<num_blocks;block++) {
		if ((nbytes=fwrite(buffer,1,BUFFERSIZE,fs))<0) {
	printf("init: error writing block\n");
	exit(1);
		}
	 }
  }
  else {
	 if ((nbytes=fwrite(buffer,1,fsize,fs))<0) {
		printf("init: error writing block\n");
		exit(1);
	 }
  }
  fclose(fs);
}

void readswrites(void){

  long int xfer, num_xfer;                   /*to access buffer correct # times*/
  long int xfer_amt;                        /*amount to transfer to/from buffer*/
  long int fsize_index;			/*file size index (0..8)*/
  long int rnum;				/*rand. num to choose read or write*/
  long int rep1,rep2;			/*indices to loop through each file*/
					/*set twice, and all sets three times*/
  for(rep1=0;rep1<3;rep1++) {		/*in order to achieve locality which*/
	 for(i=0;i<NSETS;i++) {		/*is consistent with buffer cache data*/
		for(rep2=0;rep2<2;rep2++) {	/*of Ousterhout et al (1985)*/
#ifndef NOEXTRAOUTPUT
		  printf(".");
#endif
	for(j=0;j<SET_SIZE;j++) {
	  if ((fd = open(files[i][j],O_RDWR))<0) /* UNIX 2 */ {
		 printf("readswrites: cannot open file[%d][%d]\n",i,j);
		 exit(1);
	  }
	  fileoc++; /* Counts number of file opens/closes */
	  fsize_index = *(files[i][j]) -'0';     /*max xfer_amt = BUFFERSIZE*/
	  if (bsize[fsize_index] >= BUFFERSIZE) {
		 num_xfer = bsize[fsize_index]/BUFFERSIZE;
		 xfer_amt = BUFFERSIZE;}
	  else {
		 num_xfer = 1;
		 xfer_amt = bsize[fsize_index];}
	  rnum = my_rand(3);
	  if (rnum < 2) {		/*read:write = 2:1*/
		 lseek(fd,0L,0);
		 for (xfer=0; xfer<num_xfer; xfer++) {
			troughr+=xfer_amt; /* Counts total read IO bytes*/
			if((nbytes=read(fd,buffer,xfer_amt))<0) {
				printf ("readswrites: read error[%d][%d]\n",i,j);
				exit(1);
			}
		 }
	  }
	  else {
		 lseek(fd,0L,0);
		 for (xfer=0; xfer<num_xfer; xfer++) {
			troughw+=xfer_amt; /* Counts total write IO bytes*/
			if((nbytes=write(fd,buffer,xfer_amt))<0) {
				printf ("readswrites: write error[%d][%d]\n",i,j);
				exit(1);
			}
		 }
	  }
	  close(fd);
	}
		}
	 }
  }
}
