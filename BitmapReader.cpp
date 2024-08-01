// Bitmap Reader
//

#include "BitmapReader.h"

// Global Variables for File Data Pointers

BITMAPFILEHEADER *gpCoverFileHdr, *gpStegoFileHdr;
BITMAPINFOHEADER *gpCoverFileInfoHdr, *gpStegoFileInfoHdr;
RGBQUAD *gpCoverPalette, *gpStegoPalette;
unsigned int gCoverFileSize, gMsgFileSize, gStegoFileSize;

// Command Line Global Variables
char gCoverPathFileName[MAX_PATH], *gCoverFileName;
char gMsgPathFileName[MAX_PATH], *gMsgFileName;
char gStegoPathFileName[MAX_PATH], *gStegoFileName;
//char gInputPathFileName[MAX_PATH], *gInputFileName;
//char gOutputPathFileName[MAX_PATH], *gOutputFileName;
char gAction;						// typically hide (1), extract (2), wipe (3), randomize (4), but also specifies custom actions for specific programs
char gNumBits2Hide;

void initGlobals()
{
	gpCoverFileHdr = NULL;
	gpStegoFileHdr = NULL;
	gpCoverFileInfoHdr = NULL;
	gpStegoFileInfoHdr = NULL;
	gpCoverPalette = NULL;
	gpStegoPalette = NULL;
	gCoverFileSize = gMsgFileSize = gStegoFileSize = 0;

	// Command Line Global Variables
	gCoverPathFileName[0] = 0;
	gCoverFileName = NULL;
	gMsgPathFileName[0] = 0;
	gMsgFileName = NULL;
	gStegoPathFileName[0] = 0;
	gStegoFileName = NULL;
	gAction = 0;						// typically hide (1), extract (2)
	gNumBits2Hide = 1;

	return;
} // initGlobals

// this function builds the canonical gray code array variables
//void buildGrayCode()
//{
//	int i, length, posUp, posDw, cnt;
//
//	length = 1;
//	toCGC[0] = 0;
//	toPBC[0] = 0;
//	cnt = 1;
//
//	while(length < 256)
//	{
//		posUp = length - 1;
//		posDw = length;
//		for(i = 0; i < length; i++)
//		{
//			toCGC[i + posDw] = toCGC[posUp - i] + length;
//			toPBC[toCGC[i + posDw]] = cnt++;
//		}
//		length = length * 2;
//	}
//	return;
//} // buildGrayCode

// Show the various bitmap header bytes primarily for debugging
void displayFileInfo(char *pFileName,
					 BITMAPFILEHEADER *pFileHdr, 
					 BITMAPINFOHEADER *pFileInfo,
					 RGBQUAD *ptrPalette,
					 unsigned char *pixelData)
{
	int numColors, i;

	printf("\nFile Information for %s: \n\n", pFileName);
	printf("File Header Info:\n");
	printf("File Type: %c%c\nFile Size:%d\nData Offset:%d\n\n", 
		(pFileHdr->bfType & 0xFF), (pFileHdr->bfType >> 8), pFileHdr->bfSize, pFileHdr->bfOffBits);

	switch(pFileInfo->biBitCount)
	{
	case 1:
		numColors = 2;
		break;
	case 4:
		numColors = 16;
		break;
	case 8:
		numColors = 256;
		break;
	case 16:
		numColors = 65536;
		break;
	case 24:
		numColors = 16777216;
		break;
	default:
		numColors = -1;
	}

	printf("Bit Map Image Info:\n\nSize Info Header:%d\nWidth:%d\nHeight:%d\nPlanes:%d\n"
		"Bits/Pixel:%d ==> %d colors\n"
		"Compression:%d\nImage Size:%d\nRes X:%d\nRes Y:%d\nColors:%d\nImportant Colors:%d\n\n",
		pFileInfo->biSize, 
		pFileInfo->biWidth, 
		pFileInfo->biHeight, 
		pFileInfo->biPlanes, 
		pFileInfo->biBitCount, numColors,
		pFileInfo->biCompression, 
		pFileInfo->biSizeImage, 
		pFileInfo->biXPelsPerMeter,
		pFileInfo->biYPelsPerMeter,
		pFileInfo->biClrUsed,
		pFileInfo->biClrImportant);

	//	There are no palettes
	if(pFileInfo->biBitCount > 16 || numColors == -1)
	{
		printf("\nNo Palette\n\n");
	}
	else
	{
		printf("Palette:\n\n");

		for(i = 0; i < numColors; i++)
		{
			printf("R:%02x   G:%02x   B:%02x\n", ptrPalette->rgbRed, ptrPalette->rgbGreen, ptrPalette->rgbBlue);
			ptrPalette++;
		}
	}

	// print first 24 bytes of pixel data
	printf("\n Pixel Data: \n\n");
	for(int i = 0; i < 24; i++)
	{
		printf("%02X ", *(pixelData + i) );
	}
	printf("\n\n");
	return;
} // displayFileInfo

// quick check for bitmap file validity - you may want to expand this or be more specfic for a particular bitmap type
bool isValidBitMap(char *filedata)
{
	if( filedata[0] != 'B' || filedata[1] != 'M') return false;

	return true;
} // isValidBitMap


// reads specified bitmap file from disk
unsigned char *readBitmapFile(char *fileName, unsigned int *fileSize)
{
	FILE *ptrFile;
	unsigned char *pFile;

	ptrFile = fopen(fileName, "rb");	// specify read only and binary (no CR/LF added)

	if(ptrFile == NULL)
	{
		printf("Error in opening file: %s.\n\n", fileName);
		exit(-1);
	}

	fseek(ptrFile, 0, SEEK_END);
	*fileSize = ftell(ptrFile);
	fseek(ptrFile, 0, SEEK_SET);

	// malloc memory to hold the file, include room for the header and color table
	pFile = (unsigned char *) malloc(*fileSize);

	if(pFile == NULL)
	{
		printf("Error - Could not allocate %d bytes of memory for bitmap file.\n\n", *fileSize);
		exit(-1);
	}

	// Read in complete file
	// buffer for data, size of each item, max # items, ptr to the file
	fread(pFile, sizeof(unsigned char), *fileSize, ptrFile);
	fclose(ptrFile);

	return(pFile);
} // readBitmapFile

// writes modified bitmap file to disk
// gMask used to determine the name of the file
int writeFile(char *filename, int fileSize, unsigned char *pFile)
{
	FILE *ptrFile;
	int x;

	// open the new file, MUST set binary format (text format will add line feed characters)
	ptrFile = fopen(filename, "wb+");
	if(ptrFile == NULL)
	{
		printf("Error opening file (%s) for writing.\n\n", filename);
		exit(-1);
	}

	// write the file
	x = (int) fwrite(pFile, sizeof(unsigned char), fileSize, ptrFile);

	// check for success
	if(x != fileSize)
	{
		printf("Error writing file %s.\n\n", filename);
		exit(-1);
	}
	fclose(ptrFile); // close file
	return(SUCCESS);
} // writeFile

// prints help message to the screen
void Usage(char *programName)
{
	char prgname[MAX_PATH];
	int idx;

	idx = strlen(programName);
	while (idx >= 0)
	{
		if(programName[idx] == '\\') break;
		idx--;
	}

	strcpy(prgname, &programName[idx + 1]);
	fprintf(stdout, "\n\n");

	fprintf(stdout, "To print bitmap information:\n\n");
	fprintf(stdout, "%s -c < filename.bmp >\n\n", prgname);

	/*
	fprintf(stdout, "To Hide:\n");
	fprintf(stdout, "\t%s -hide -c <cover file> -m <msg file> [-s <stego file>] [-b <number of bits>]\n\n", prgname);

	fprintf(stdout, "To Extract:\n");
	fprintf(stdout, "\t%s -extract -s <stego file> [-m <message file>] [-b <number of bits>]\n\n", prgname);

	fprintf(stdout, "Parameters:\n");
	fprintf(stdout, "Set number of bits to hide per pixel:	-b ( 1 to 8 def:1 )\n\n");

	fprintf(stdout, "\n\tNOTES:\n\t1.Order of parameters is irrelevant.\n\t2.All selections in \"[]\" are optional.\n\n");
	//*/

	system("pause");
	exit(0);
} // Usage

void parseCommandLine(int argc, char *argv[])
{
	int cnt;

	if(argc < 2) Usage(argv[0]);

	// RESET Parameters to make error checking easier
	gAction = 0;
	gCoverPathFileName[0] = 0;
	gMsgPathFileName[0] = 0;
	gStegoPathFileName[0] = 0;
	cnt = 1;
	while(cnt < argc)	// argv[0] = program name
	{
		if(_stricmp(argv[cnt], "-c") == 0)	// cover file
		{
			cnt++;
			if(cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following <%s> parameter.\n\n", argv[cnt-1]);
				exit(-1);
			}

			if(gCoverPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - cover file <%s> already specified.\n\n", gCoverPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gCoverPathFileName, &gCoverFileName);
		}

		/*
		else if(_stricmp(argv[cnt], "-m") == 0)	// msg file
		{
			cnt++;
			if(cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following <%s> parameter.\n\n", argv[cnt-1]);
				exit(-1);
			}

			if(gMsgPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - message file <%s> already specified.\n\n", gMsgPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gMsgPathFileName, &gMsgFileName);
		}
		else if(_stricmp(argv[cnt], "-s") == 0) // stego file
		{
			cnt++;
			if(cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following '%s' parameter.\n\n", argv[cnt-1]);
				exit(-1);
			}

			if(gStegoPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - stego file <%s> already specified.\n\n", gStegoPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gStegoPathFileName, &gStegoFileName);
		}
		else if(_stricmp(argv[cnt], "-b") == 0)	// set number of bits to hide per data block
		{
			cnt++;
			if(cnt == argc)
			{
				fprintf(stderr, "\n\nInput Error - no number following '%s' parameter.\n\n", argv[cnt-1]);
				exit(-1);
			}

			// the range for gNumBits2Hide is 1 - 8
			gNumBits2Hide = *(argv[cnt]) - 48;

			if(gNumBits2Hide < 1 || gNumBits2Hide > 8)	// for this program, 8 is the max
			{
				fprintf(stderr, "\n\nThe number of bits to hide is out of range (1 - 8).\n\n");
				exit(-1);
			}
		}
		else if(_stricmp(argv[cnt], "-hide") == 0)	// hide
		{
			if(gAction)
			{
				fprintf(stderr, "\n\nError, an action has already been specified.\n\n");
				exit(-1);
			}

			gAction = ACTION_HIDE;
		}
		else if(_stricmp(argv[cnt], "-extract") == 0)	// extract
		{
			if(gAction)
			{
				fprintf(stderr, "\n\nError, an action has already been specified.\n\n");
				exit(-1);
			}

			gAction = ACTION_EXTRACT;
		}
		//*/

		cnt++;
	} // end while loop

	return;
} // parseCommandLine

// Main function
// Parameters are used to indicate the input file and available options
int main(int argc, char *argv[])
{
	unsigned char *coverData, *pixelData;

	// get the number of bits to use for data hiding or data extracting
	// if not specified, default to one
	initGlobals();

	parseCommandLine(argc, argv);

// take appropriate actions based on user inputs
// example opening cover bitmap

	if(gCoverPathFileName[0] != 0)
	{
		coverData = readBitmapFile(gCoverPathFileName, &gCoverFileSize);

		gpCoverFileHdr = (BITMAPFILEHEADER *) coverData;

		gpCoverFileInfoHdr = (BITMAPINFOHEADER *) (coverData + sizeof(BITMAPFILEHEADER) );

		// there might not exist a palette - I don't check here, but you can see how in display info
		gpCoverPalette = (RGBQUAD *) ( (char *) coverData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) );

		pixelData = coverData + gpCoverFileHdr->bfOffBits;

		displayFileInfo(gCoverPathFileName, gpCoverFileHdr, gpCoverFileInfoHdr, gpCoverPalette, pixelData);
	}
	return 0;
} // main


