/***************************************************************
 extract_video.c
Compile:
./cc
Test:
./extract_video -i /media/ipcam11/datadir0/ -o ./output -l -s 2014-10-13 -v
****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#define PROGRAM_VERSION "0.2"

struct FILE_IDX_HEADER
{	
	uint64_t modifyTimes;
  uint32_t version;
  uint32_t avFiles;
  uint32_t nextFileRecNo;
  uint32_t lastFileRecNo;
  uint8_t currFileRec[1176];
  uint8_t res3[76];
  uint32_t checksum;
};

struct FILE_IDX_RECORD	
{	
	uint32_t fileNo;
  uint16_t chan;
  uint16_t segRecNums;
  time_t startTime;
  time_t endTime;
  uint8_t status;
  uint8_t res1;
  uint16_t lockedSegNum;
  uint8_t res2[4];
  uint8_t infoTypes[8];
};

struct SEGMENT_IDX_RECORD
{
	uint8_t type;
	uint8_t status;
	uint8_t res1[2];
	uint8_t resolution[4];
  uint64_t startTime;
  uint64_t endTime;
  uint64_t firstKeyFrame_absTime;
  uint32_t firstKeyFrame_stdTime;
  uint32_t lastFrame_stdTime;
  uint32_t startOffset;
  uint32_t endOffset;
  uint8_t res2[4];
  uint8_t infoNum[4];
  uint8_t infoTypes[8];
  uint8_t infoStartTime[4];
  uint8_t infoEndTime[4];
  uint8_t infoStartOffset[4];
  uint8_t infoEndOffset[4];
};

typedef enum log_event
{	LOG_DEBUG=1,
	LOG_WARNING=2,
	LOG_ERROR=4,
	LOG_INFO=8
} log_event;

FILE *_log_debug=NULL,*_log_warning=NULL,*_log_error=NULL,*_log_info=NULL;
void logger(log_event event,char *function,char *msg,...);
char *timeformat(time_t time);
char *makefilename(char *path, char *name);
char *timefilename(char* prefix, char* postfix, time_t start, time_t end);


int main(int argc, char **argv)
{
	struct FILE_IDX_HEADER header;
	struct FILE_IDX_RECORD file;
	struct SEGMENT_IDX_RECORD segment;

	char *inputPath=NULL, *outputPath=NULL, *matchString=NULL, buffer[8192];
	FILE *idxFile, *ivFile, *ovFile;
	
	unsigned int skip_existing=0, verbose=0, list_only=0, totals_only=0;
	
	unsigned long total_files=0;
	unsigned long long total_filesize=0;
	unsigned long total_time=0;

	int err=0,c,i,j,l,help=0;

	_log_error=stderr;
	_log_info=stderr;
	
// Parse options

	while (err==0 && (c=getopt(argc,argv,"tvlkhi:o:s:"))!=-1)
	{
		switch (c)
		{
			case 'i': // Format output
				logger(LOG_DEBUG,"main","Format output using '%s'",optarg);
				inputPath=optarg;
				break;

			case 'o': // Server URL
				logger(LOG_DEBUG,"main","Server URL set to '%s'",optarg);
				outputPath=optarg;
				break;

			case 's': // Regex
				logger(LOG_DEBUG,"main","Match regex set to '%s'",optarg);
				matchString=optarg;
				break;

			case 'k': // Skip existing files in output dir
				skip_existing=1;
				break;

			case 'l': // List only, don't extract
				list_only=1;
				break;

			case 't': // Only show totals
				totals_only=1;
				break;

			case 'v': // Verbose mode
				verbose=1;
				break;

			case 'h':
		  case '?':
				help=1;
				printf("Read/Extract Hikvision Video File Storage\n");
				printf("Alexey Ozerov (c) 2014 - ver. %s\n\n", PROGRAM_VERSION);
				printf("Options\n");
				printf(" -? -h            Display this help\n");
				printf(" -i <path>        Input directory path\n");
				printf(" -o <path>        Output directory path\n");
				printf(" -s	<string>      List/extract only file names including string\n");
				printf(" -k				        Don't overwrite existing output files\n");
				printf(" -l				        List only, don't extract data\n");
			  printf(" -t				        Only calculate and show totals\n");
				printf(" -v				        Verbose mode\n");
		}
	}

// Open index file

  if (!err && !help && inputPath!=NULL)
  {
  	idxFile=fopen(makefilename(inputPath,"index00.bin"),"rb");
  	if (!idxFile)
  	{	logger(LOG_ERROR,"main","Can't open file %s",makefilename(inputPath,"index00.bin"));
  		err++;
  		return 1;
  	}
  	else
  	{

// Read header

  		if (fread(&header,sizeof(header),1,idxFile)!=1)
  		{ logger(LOG_ERROR,"main","Can't read %d header bytes",sizeof(header));
  			err++;
  			return 1;
  		}

			if (verbose)
			{	printf("HEADER =================\n");
				printf("version: %u\n",header.version);
				printf("avFiles: %u\n",header.avFiles);
				printf("nextFileRecNo: %u\n",header.nextFileRecNo);
				printf("lastFileRecNo: %u\n",header.lastFileRecNo);
			}

// Read files

			for (i=0;i<header.avFiles;i++)
			{

    		if (fread(&file,sizeof(file),1,idxFile)!=1)
    		{ logger(LOG_ERROR,"main","Can't read %d bytes for file index",sizeof(file));
  	  		err++;
  		  	return 1;
  		  }

        if (file.chan != 0xFFFF) // Skip empty file records
        {
    			if (verbose)
    			{	printf("FILE %u =================\n",i);
		   	  	printf("fileNo: %u\n",file.fileNo);
		       	printf("chan: %u\n",file.chan);
		  	  	printf("startTime: %s\n",timeformat(file.startTime));
			  	  printf("endTime: %s\n",timeformat(file.endTime));
			  	 }
		  	}
				
			}

// Read segments
// Motion >> type: 3 status: 0

			for (i=0;i<header.avFiles;i++)
			{ 
				for (j=0;j<256;j++)
				{
      		if (fread(&segment,sizeof(segment),1,idxFile)!=1)
      		{ logger(LOG_ERROR,"main","Can't read %d bytes for segment index",sizeof(segment));
  	    		err++;
  		    	return 1;
  		    }

          if (segment.type != 0x00 && segment.startTime != 0 && segment.endTime != 0) // Skip empty segment records
          {
    			  if (verbose)
    			  { printf("FILE %u SEG %u ============\n",i,j);
		      		printf("type: %u\n",segment.type);
	  		    	printf("status: %u\n",segment.status);
		    	  	printf("startTime: %s\n",timeformat(segment.startTime));
		  	  	  printf("endTime: %s\n",timeformat(segment.endTime));
		  	  	  printf("firstKeyFrame_absTime: %s\n",timeformat(segment.firstKeyFrame_absTime));
		  	  	  printf("firstKeyFrame_stdTime: %u\n",segment.firstKeyFrame_stdTime);
		  	  	  printf("lastFrame_stdTime: %u\n",segment.lastFrame_stdTime);
	    	  	  printf("startOffset: %u\n",segment.startOffset);
		  	  	  printf("endOffset: %u\n",segment.endOffset);
		  	  	}

// Extract from video file

  					if (outputPath!=NULL) //if (i==7) // Extract only from one file pls
  					{
    					size_t filesize = segment.endOffset - segment.startOffset;
    					size_t l1,l2;  			
    					int file_exists=0;
  						char *ovFilename = makefilename(outputPath,timefilename("hikvideo", ".mp4", segment.startTime, segment.endTime));

							if (matchString!=NULL && strstr(ovFilename,matchString)==NULL)
								continue;

							if (!totals_only)
						  {	printf("File name: %s\n",ovFilename);
								printf("File size: %u bytes\n",filesize);
								printf("Play time: %u sec\n",segment.endTime-segment.startTime);
							}
							
							total_files++;
							total_filesize+=filesize;
							total_time+=(segment.endTime-segment.startTime);
							
// Check output file duplicates
							
							if (!list_only && !totals_only)
							{ ovFile = fopen(ovFilename,"r");
								if (ovFile)
								{ if (skip_existing)
									  printf("File exists... Skipped!\n");
									else 
									  printf("File exists... Overwriting...\n");
									file_exists=1;
									fclose(ovFile);
								}
							}
  						
// Open input file and copy data
  						
  						if ((!file_exists || !skip_existing) && !list_only && !totals_only)
  						{
	  						ovFile = fopen(ovFilename,"wb");
	  						if (!ovFile)
	    	    		{	logger(LOG_ERROR,"main","Can't open file %s",ovFilename);
	    						err++;
	    						return 1;
	    					}

	  						sprintf(buffer, "hiv%05u.mp4", i);
	  						ivFile = fopen(makefilename(inputPath,buffer),"rb");
	  				  	if (!ivFile)
	    	    		{	logger(LOG_ERROR,"main","Can't open file %s",makefilename(inputPath,buffer));
	    						err++;
	    						return 1;
	    					}
	  						if (fseek(ivFile,segment.startOffset,SEEK_SET) != 0)
	  						{	logger(LOG_ERROR,"main","Can't seek in file %s to position %u",buffer,segment.startOffset);
	    						err++;
	    						return 1;
	    					}
				
	              while((l1 = fread(buffer, 1, filesize > sizeof(buffer) ? sizeof(buffer) : filesize , ivFile)) > 0 && filesize > 0) 
	              {
	                if (l1 < (filesize > sizeof(buffer) ? sizeof(buffer) : filesize))
	                {	logger(LOG_ERROR,"main","Input file failure");
	    						  err++;
	    						  return 1;
	    					  }
	  
	                l2 = fwrite(buffer, 1, l1, ovFile);
	                if(l2 < l1)
	                {	logger(LOG_ERROR,"main","Output file failure");
	    						  err++;
	    						  return 1;
	    					  }
	    					  
	    					  filesize-=l1;
	              }
	              if (filesize > 0)
	              {	logger(LOG_ERROR,"main","Input file truncated?");
	    						err++;
	    						return 1;
	    					}
	  						
	  						fclose(ovFile);
	  						fclose(ivFile);
	  					}
  					}
  					else
  					{	logger(LOG_ERROR,"main","Output directory path not specified");
	    				err++;
	    				return 1;
	    			}
  		  	}
				}
				
			}
			
			printf("\nTotal files: %lu\n",total_files);
			printf("Total file size: %llu bytes (=%llu MB)\n",total_filesize,total_filesize/1024/1024);
			printf("Total play time: %lu sec (=%lu min)\n",total_time,total_time/60);

  		fclose(idxFile);
  	}
  }

}

//***************************************************************
// Common functions
//***************************************************************

char timebuffer[100];

char *timeformat(time_t time)
{
	 struct tm *tmptr = gmtime(&time);
	 if (tmptr!=NULL && strftime(timebuffer, sizeof(timebuffer), "%Y-%m-%d %H:%M:%S", tmptr)) 
	   return timebuffer;
   else
   	return "Error converting time";
}

char *timefilename(char* prefix, char* postfix, time_t start, time_t end)
{
	char buffer1[20],buffer2[20];
	struct tm *tmptr;
	
	tmptr = gmtime(&start);
	if (tmptr==NULL || !strftime(buffer1, sizeof(buffer1), "%Y-%m-%d_%H.%M.%S", tmptr))
		return "Error converting time";
  tmptr = gmtime(&end);
  if (tmptr==NULL || !strftime(buffer2, sizeof(buffer2), "%H.%M.%S", tmptr))
		return "Error converting time";

  strcpy(timebuffer,prefix);
  strcat(timebuffer,"_");
  strcat(timebuffer,buffer1);
  strcat(timebuffer,"_to_");
  strcat(timebuffer,buffer2);
  strcat(timebuffer,postfix);
  return timebuffer;
}


//***************************************************************

char *filebuffer=NULL;
char *makefilename(char *path, char *name)
{
	if (filebuffer!=NULL) 
  { free(filebuffer);
  	filebuffer=NULL;
  }
	
	filebuffer = malloc (strlen(path)+strlen(name)+2);
	if (filebuffer)
	{	
		strcpy (filebuffer,path);
		if (strlen(filebuffer)>0 && filebuffer[strlen(filebuffer)-1]!='/')
		  strcat (filebuffer,"/");
		strcat (filebuffer,name);
	}
	return filebuffer;

}

//***************************************************************

void logger(log_event event,char *function,char *msg,...)
{
	va_list args;

	va_start(args,msg);
	switch (event)
	{
		case LOG_DEBUG:
			if (_log_debug)
			{
				fprintf(_log_debug,"message: logger.%s - ",function);
				vfprintf(_log_debug,msg,args);
				fprintf(_log_debug,"\n");
			}
			break;

		case LOG_WARNING:
			if (_log_warning)
			{
				fprintf(_log_warning,"warning: logger.%s - ",function);
				vfprintf(_log_warning,msg,args);
				fprintf(_log_warning,"\n");
			}
			break;

		case LOG_ERROR:
			if (_log_error)
			{
				fprintf(_log_error,"error: logger.%s - ",function);
				vfprintf(_log_error,msg,args);
				fprintf(_log_error,"\n");

  		}
			break;

		case LOG_INFO:
			if (_log_info)
			{
				fprintf(_log_info,"info: logger.%s - ",function);
				vfprintf(_log_info,msg,args);
				fprintf(_log_info,"\n");
			}
			break;
	}
	va_end(args);
}
