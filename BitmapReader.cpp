// Bitmap Reader
// Authors: John A. Ortiz
// Modified by: Roberto Delgado, Mark Solis, Daniel Zartuche
//
// This program uses the Block-Diagonal Partition Pattern (BDPP) algorithm to hide and extract data from a bitmap image.
// This method of hiding was developed by Gyankamal J.Chhajed and Bindu Garg in 2023
//

#include "BitmapReader.h"

// Global Variables for File Data Pointers

BITMAPFILEHEADER* gpCoverFileHdr, * gpStegoFileHdr, * gpMsgFileHdr, * gpTypeFileHdr;
BITMAPINFOHEADER* gpCoverFileInfoHdr, * gpStegoFileInfoHdr, * gpMsgFileInfoHdr, * gpTypeFileInfoHdr;
RGBQUAD* gpCoverPalette, * gpStegoPalette;
unsigned int gCoverFileSize, gMsgFileSize, gStegoFileSize;

// Command Line Global Variables
char gCoverPathFileName[MAX_PATH], * gCoverFileName;
char gMsgPathFileName[MAX_PATH], * gMsgFileName;
char gStegoPathFileName[MAX_PATH], * gStegoFileName;
char gOutputPathFileName[MAX_PATH], * gOutputFileName;
//char gInputPathFileName[MAX_PATH], *gInputFileName;
char gAction;						// typically hide (1), extract (2), wipe (3), randomize (4), but also specifies custom actions for specific programs
char gNumBits2Hide;
int gKey;
int gInfo;

void initGlobals()
{
	gpCoverFileHdr = NULL;
	gpStegoFileHdr = NULL;
	gpMsgFileHdr = NULL;
	gpCoverFileInfoHdr = NULL;
	gpStegoFileInfoHdr = NULL;
	gpMsgFileInfoHdr = NULL;
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
	gKey = -1;
	gInfo = 0;

	return;
} // initGlobals

// Show the various bitmap header bytes primarily for debugging
void displayFileInfo(char* pFileName,
	BITMAPFILEHEADER* pFileHdr,
	BITMAPINFOHEADER* pFileInfo,
	RGBQUAD* ptrPalette,
	unsigned char* pixelData)
{
	int numColors, i;

	printf("\nFile Information for %s: \n\n", pFileName);
	printf("File Header Info:\n");
	printf("File Type: %c%c\nFile Size:%d\nData Offset:%d\n\n",
		(pFileHdr->bfType & 0xFF), (pFileHdr->bfType >> 8), pFileHdr->bfSize, pFileHdr->bfOffBits);

	switch (pFileInfo->biBitCount)
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
	if (pFileInfo->biBitCount > 16 || numColors == -1)
	{
		printf("\nNo Palette\n\n");
	}
	else
	{
		printf("Palette:\n\n");

		for (i = 0; i < numColors; i++)
		{
			printf("R:%02x   G:%02x   B:%02x\n", ptrPalette->rgbRed, ptrPalette->rgbGreen, ptrPalette->rgbBlue);
			ptrPalette++;
		}
	}

	// print first 24 bytes of pixel data
	printf("\n Pixel Data: \n\n");
	for (int i = 0; i < 24; i++)
	{
		printf("%02X ", *(pixelData + i));
	}
	printf("\n\n");
	return;
} // displayFileInfo

// quick check for bitmap file validity - you may want to expand this or be more specfic for a particular bitmap type
bool isValidBitMap(unsigned char* filedata)
{
	if (filedata[0] != 'B' || filedata[1] != 'M') return false;

	return true;
} // isValidBitMap


// reads specified bitmap file from disk
unsigned char* readBitmapFile(char* fileName, unsigned int* fileSize)
{
	FILE* ptrFile;
	unsigned char* pFile;

	ptrFile = fopen(fileName, "rb");	// specify read only and binary (no CR/LF added)

	if (ptrFile == NULL)
	{
		printf("Error in opening file: %s.\n\n", fileName);
		exit(-1);
	}

	fseek(ptrFile, 0, SEEK_END);
	*fileSize = ftell(ptrFile);
	fseek(ptrFile, 0, SEEK_SET);

	// malloc memory to hold the file, include room for the header and color table
	pFile = (unsigned char*)malloc(*fileSize);

	if (pFile == NULL)
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
int writeFile(char* filename, int fileSize, unsigned char* pFile)
{
	FILE* ptrFile;
	int x;

	// open the new file, MUST set binary format (text format will add line feed characters)
	ptrFile = fopen(filename, "wb+");
	if (ptrFile == NULL)
	{
		printf("Error opening file (%s) for writing.\n\n", filename);
		exit(-1);
	}

	// write the file
	x = (int)fwrite(pFile, sizeof(unsigned char), fileSize, ptrFile);

	// check for success
	if (x != fileSize)
	{
		printf("Error writing file %s.\n\n", filename);
		exit(-1);
	}
	fclose(ptrFile); // close file
	return(SUCCESS);
} // writeFile

// prints help message to the screen
void Usage(char* programName)
{
	char prgname[MAX_PATH];
	int idx;

	idx = strlen(programName);
	while (idx >= 0)
	{
		if (programName[idx] == '\\') break;
		idx--;
	}

	strcpy(prgname, &programName[idx + 1]);
	fprintf(stdout, "\n\n");
	fprintf(stdout, " ***** %s Version: %s ***** \n       ... by John A. Ortiz, Roberto Delgado, Mark Solis, Daniel Zartuche\n\n", prgname, VERSION);

	fprintf(stdout, "BDPP ALGORITHM INFORMATION\n");
	fprintf(stdout, "This program uses the Block-Diagonal Partition Pattern (BDPP) algorithm to hide and extract data from a bitmap image.\n");
	fprintf(stdout, "This method of hiding was developed by Gyankamal J.Chhajed and Bindu Garg in 2023\n");
	fprintf(stdout, "This program's implementation of the BDPP algorithm was designed by Roberto Delgado\n\n");

	fprintf(stdout, "The algorithm is as follows:\n");
	fprintf(stdout, "\t1. The image is divided into 3x3 blocks.\n");
	fprintf(stdout, "\t2. Each block is then diagonally partitioned into 4 sections. Pixels split in half are counted as whole.\n");
	fprintf(stdout, "\t3. Each section is checked for a ratio of 0s to 1s in the block's matrix. The ratios are 6:0, 5:1, 4:2, 3:3, 2:4, 1:5, 0:6.\n");
	fprintf(stdout, "\t4. If a block has 2 or more unique ratios, it passes the ratio check.\n");
	fprintf(stdout, "\t5. The block is then checked for horizontal, vertical, and diagonal connectivity of 0s.\n");
	fprintf(stdout, "\t   this means that there must be at least 2 zeros connected horizontally, vertically, and diagonally.\n");
	fprintf(stdout, "   if this is true then the block is said to be HVD connected.\n");
	fprintf(stdout, "\t6. If the block passes both the ratio check and the HVD check, then the block is embeddable.\n");
	fprintf(stdout, "\t7. The middle pixel of the block is then flipped steps 2-5 are repeated for all blocks.\n");
	fprintf(stdout, "\t8. If the block still passes both checks, then the middle pixel is flipped back to its original value.\n");
	fprintf(stdout, "\t   and the block is marked as embeddable.\n");
	fprintf(stdout, "\t9. The program then embeds the message data into the embeddable blocks or if extracting, extracts the data\n");

	fprintf(stdout, "USAGE INFORMATION\n\n");

	fprintf(stdout, "Print Bitmap Information:\n");
	fprintf(stdout, "%s -i < filename.bmp >\n", prgname);
	fprintf(stdout, "  *This parameter supercedes all other options, and exits\n\n");


	fprintf(stdout, "Hide:\n");
	fprintf(stdout, "%s -hide -c <cover file> -m <msg file> [-o <stego file>]\n", prgname);
	fprintf(stdout, "Hide with random message data:\n");
	fprintf(stdout, "%s -hide -c <cover file> -m random [-o <stego file>]\n", prgname);
	fprintf(stdout, "  *Cover file should be a bitmap file\n");
	fprintf(stdout, "  *Hiding capacity may vary between files, therefore we recommend\n");
	fprintf(stdout, "   the message be at most half the size of the cover file.\n");
	fprintf(stdout, "  *Random message data is generated based on the cover image width value.\n");
	fprintf(stdout, "  *If no output file is specified, the default is hiding_output.bmp.\n\n");

	fprintf(stdout, "Extract:\n");
	fprintf(stdout, "%s -extract -s <stego file> -k <key value> [-o <message file>] \n", prgname);
	fprintf(stdout, "  *Stego file should be a bitmap file\n");
	fprintf(stdout, "  *Key value is generated when an image is hidden\n");
	fprintf(stdout, "  *Key value is equal to the hidden message bits\n");
	fprintf(stdout, "  *Using an incorrect key may extract incorrect information\n");
	fprintf(stdout, "  *If no output file is specified, the default is extraction_output.bin.\n\n");

	fprintf(stdout, "Help:\n");
	fprintf(stdout, "%s -h\n", prgname);
	fprintf(stdout, "  *Displays this help screen\n----------------------------------------------\n");

	fprintf(stdout, "Parameters:\n");
	fprintf(stdout, "Display this usage help scree: ..... -h\n");
	fprintf(stdout, "Display Bitmap Information: ........ -i < filename.bmp >\n");
	fprintf(stdout, "Hide Data: ......................... -hide\n");
	fprintf(stdout, "Extract Data: ...................... -extract\n");
	fprintf(stdout, "Set the value of the cover file: ... -c < filename.bmp >\n");
	fprintf(stdout, "Set the value of the stego file: ... -s < filename.bmp >\n");
	fprintf(stdout, "Set the value of the extraction key:	-k ( Positive Integer )\n");
	fprintf(stdout, "Set the value of the message file: . -m < filename.bmp >\n");
	fprintf(stdout, "Set the value of the output file: .. -o < filename.bmp >\n");


	fprintf(stdout, "\n\tNOTES:\n\t1. Order of parameters is irrelevant.\n\t2. All selections in \"[]\" are optional.\n\n");

	system("pause");
	exit(0);
} // Usage

void parseCommandLine(int argc, char* argv[])
{
	int cnt;

	if (argc < 2) Usage(argv[0]);

	// RESET Parameters to make error checking easier
	gAction = 0;
	gCoverPathFileName[0] = 0;
	gMsgPathFileName[0] = 0;
	gStegoPathFileName[0] = 0;
	cnt = 1;
	while (cnt < argc)	// argv[0] = program name
	{
		if (_stricmp(argv[cnt], "-c") == 0)	// cover file
		{
			cnt++;
			if (cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following <%s> parameter.\n\n", argv[cnt - 1]);
				exit(-1);
			}

			if (gCoverPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - cover file <%s> already specified.\n\n", gCoverPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gCoverPathFileName, &gCoverFileName);
		}
		else if (_stricmp(argv[cnt], "-m") == 0)	// msg file
		{
			cnt++;
			if (cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following <%s> parameter.\n\n", argv[cnt - 1]);
				exit(-1);
			}

			if (gMsgPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - message file <%s> already specified.\n\n", gMsgPathFileName);
				exit(-2);
			}
			if (_stricmp(argv[cnt], "random") == 0)
			{
				gMsgFileName = (char*)"random";
			}
			{
				GetFullPathName(argv[cnt], MAX_PATH, gMsgPathFileName, &gMsgFileName);
			}
		}
		else if (_stricmp(argv[cnt], "-s") == 0) // stego file
		{
			cnt++;
			if (cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following '%s' parameter.\n\n", argv[cnt - 1]);
				exit(-1);
			}

			if (gStegoPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - stego file <%s> already specified.\n\n", gStegoPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gStegoPathFileName, &gStegoFileName);
		}
		else if (_stricmp(argv[cnt], "-o") == 0)	// output file
		{
			cnt++;
			if (cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following '%s' parameter.\n\n", argv[cnt - 1]);
				exit(-1);
			}

			if (gOutputPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - output file <%s> already specified.\n\n", gOutputPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gOutputPathFileName, &gOutputFileName);
		}
		else if (_stricmp(argv[cnt], "-k") == 0)	// key value
		{
			cnt++;
			if (cnt == argc)
			{
				fprintf(stderr, "\n\nError - no key value following '%s' parameter.\n\n", argv[cnt - 1]);
				exit(-1);
			}

			gKey = atoi(argv[cnt]);
		}
		else if (_stricmp(argv[cnt], "-h") == 0)	// help
		{
			Usage(argv[0]);
		}
		else if (_stricmp(argv[cnt], "-i") == 0)  // info
		{
			cnt++;
			if (cnt == argc)
			{
				fprintf(stderr, "\n\nError - no file name following '%s' parameter.\n\n", argv[cnt - 1]);
				exit(-1);
			}

			if (gCoverPathFileName[0] != 0)
			{
				fprintf(stderr, "\n\nError - input file <%s> already specified.\n\n", gCoverPathFileName);
				exit(-2);
			}

			GetFullPathName(argv[cnt], MAX_PATH, gCoverPathFileName, &gCoverFileName);
			gInfo = 1;
		}
		else if (_stricmp(argv[cnt], "-hide") == 0)	// hide
		{
			if (gAction)
			{
				fprintf(stderr, "\n\nError, an action has already been specified.\n\n");
				exit(-1);
			}

			gAction = ACTION_HIDE;
		}
		else if (_stricmp(argv[cnt], "-extract") == 0)	// extract
		{
			if (gAction)
			{
				fprintf(stderr, "\n\nError, an action has already been specified.\n\n");
				exit(-1);
			}
			gAction = ACTION_EXTRACT;

		}
		else
		{
			fprintf(stderr, "\n\nError - unknown parameter <%s>.\n\n", argv[cnt]);
			exit(-1);
		}

		cnt++;

		// error checking for parameters
		if (gInfo == 0)
		{
			if (cnt == argc && gAction == 0)
			{
				fprintf(stderr, "\n\nError - no action specified.\n\n");
				exit(-1);
			}
			if (cnt == argc && gAction == 0)
			{
				fprintf(stderr, "\n\nError - no action specified.\n\n");
				exit(-1);
			}
			if (cnt == argc && gAction == ACTION_EXTRACT && gKey == -1)
			{
				fprintf(stderr, "\n\nError - no key specified.\n\n");
				exit(-1);
			}
			if (cnt == argc && gOutputPathFileName[0] == 0)
			{
				if (gAction == ACTION_HIDE)
				{
					GetFullPathName("hiding_output.bmp", MAX_PATH, gOutputPathFileName, &gOutputFileName);
				}
				else if (gAction == ACTION_EXTRACT)
				{
					GetFullPathName("extraction_output.bin", MAX_PATH, gOutputPathFileName, &gOutputFileName);
				}
			}
			if (cnt == argc && gMsgPathFileName[0] == 0 && gAction == ACTION_HIDE)
			{
				fprintf(stderr, "\n\nError - no message file specified.\n\n");
				exit(-1);
			}
			if (cnt == argc && gStegoPathFileName[0] == 0 && gAction == ACTION_EXTRACT)
			{
				fprintf(stderr, "\n\nError - no stego file specified.\n\n");
				exit(-1);
			}
			if (cnt == argc && gCoverPathFileName[0] == 0 && gAction == ACTION_HIDE)
			{
				fprintf(stderr, "\n\nError - no cover file specified.\n\n");
				exit(-1);
			}
		}
		else
		{
			if (cnt == argc && gCoverPathFileName[0] == 0)
			{
				fprintf(stderr, "\n\nError - no cover file specified.\n\n");
				exit(-1);
			}
		}

	} // end while loop

	return;
} // parseCommandLine

// Main function
// Parameters are used to indicate the input file and available options
int main(int argc, char* argv[])
{
	unsigned char* coverData, * pixelData, * messageData, * msgPixelData;  // used to store file data for cover and message
	unsigned char* extractBits;  // used to store extracted bits


	// get the number of bits to use for data hiding or data extracting
	// if not specified, default to one
	initGlobals();

	parseCommandLine(argc, argv);

	// take appropriate actions based on user inputs
	// example opening cover bitmap

	// Necessary to display information about the bitmap file and exit before any other action
	if (gInfo == 1)
	{
		if (gCoverPathFileName[0] != 0)
		{
			coverData = readBitmapFile(gCoverPathFileName, &gCoverFileSize);

			if (!isValidBitMap(coverData))
			{
				printf("Error - %s is not a valid bitmap file.\n\n", gCoverPathFileName);
				exit(-1);
			}

			gpTypeFileHdr = (BITMAPFILEHEADER*)coverData;

			gpTypeFileInfoHdr = (BITMAPINFOHEADER*)(coverData + sizeof(BITMAPFILEHEADER));

			// there might not exist a palette - I don't check here, but you can see how in display info
			gpCoverPalette = (RGBQUAD*)((char*)coverData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

			pixelData = coverData + gpTypeFileHdr->bfOffBits;

			displayFileInfo(gCoverPathFileName, gpTypeFileHdr, gpTypeFileInfoHdr, gpCoverPalette, pixelData);
			exit(0);
		}
	} // end if gInfo

	// hide or extract data
	if (gAction == ACTION_HIDE)
	{
		if (gCoverPathFileName[0] != 0)
		{
			coverData = readBitmapFile(gCoverPathFileName, &gCoverFileSize);

			if (!isValidBitMap(coverData))  //  check if cover is usable for hiding, should be bmp
			{
				printf("Error - %s is not a valid bitmap file.\n\n", gCoverPathFileName);
				exit(-1);
			}

			gpTypeFileHdr = (BITMAPFILEHEADER*)coverData;

			gpTypeFileInfoHdr = (BITMAPINFOHEADER*)(coverData + sizeof(BITMAPFILEHEADER));

			// there might not exist a palette - I don't check here, but you can see how in display info
			gpCoverPalette = (RGBQUAD*)((char*)coverData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

			pixelData = coverData + gpTypeFileHdr->bfOffBits;

			//  check if message is random, if so generate random data based on the cover image width 
			// (almost guarantees data will be small enough to hide)
			if (_stricmp(gMsgFileName, "random") == 0)
			{
				// generate random data
				srand((unsigned int)time(NULL));
				unsigned int maxMsgFilesSize = gpTypeFileInfoHdr->biWidth;
				gMsgFileSize = rand() % maxMsgFilesSize;
				messageData = (unsigned char*)malloc(sizeof(unsigned char) * gMsgFileSize);
				printf("\nRandom message data size: %d", gMsgFileSize);
				for (int i = 0; i < gMsgFileSize; i++)
				{
					messageData[i] = rand() % 256;
				}
				msgPixelData = messageData;  //  no header data
			}
			else  //  read the file and check if it is a bitmap or not
			{
				messageData = readBitmapFile(gMsgPathFileName, &gMsgFileSize);
				if (isValidBitMap(messageData))
				{
					gpMsgFileHdr = (BITMAPFILEHEADER*)messageData;
					gpMsgFileInfoHdr = (BITMAPINFOHEADER*)(messageData + sizeof(BITMAPFILEHEADER));
					msgPixelData = messageData + gpTypeFileHdr->bfOffBits;
				}
				else
				{
					msgPixelData = messageData;
					gMsgFileSize = strlen((char*)messageData);
				}
			}
			// extract bits needs to bee set to be able to enter parsePixelData, so we allocate the bare minimum
			extractBits = (unsigned char*)malloc(sizeof(unsigned char) * 1);
			printf("\nAttempting to hide in %s\n", gCoverPathFileName);
		}
		else
		{
			printf("Error - no cover file specified.\n\n");
			exit(-1);
		}
	}
	else if (gAction == ACTION_EXTRACT)
	{
		// basic setting to prep the data for parsing (reading checking)
		if (gStegoPathFileName[0] != 0)
		{
			//have to allocate msgPixelData to be able to enter parsePixelData
			//we allocate the expected size of the hidden message plust a little extra just in case
			msgPixelData = (unsigned char*)malloc(sizeof(unsigned char) * gKey * 2);
			coverData = readBitmapFile(gStegoPathFileName, &gStegoFileSize);

			if (!isValidBitMap(coverData))
			{
				printf("Error - %s is not a valid bitmap file.\n\n", gStegoPathFileName);
				exit(-1);
			}

			gpTypeFileHdr = (BITMAPFILEHEADER*)coverData;

			gpTypeFileInfoHdr = (BITMAPINFOHEADER*)(coverData + sizeof(BITMAPFILEHEADER));

			pixelData = coverData + gpTypeFileHdr->bfOffBits;

			// there might not exist a palette - I don't check here, but you can see how in display info
			gpStegoPalette = (RGBQUAD*)((char*)coverData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));

			// extractBits is allocated to the size of the key since the key is the size of the hidden message
			extractBits = (unsigned char*)malloc(sizeof(unsigned char) * gKey);
			printf("\nAttempting to extract from %s\n", gStegoPathFileName);

		}
		else
		{
			printf("Error - no stego file specified.\n\n");
			exit(-1);
		}
	}

	// parse the pixel data, hides, extracts, and performs all checks for the BDPP algorithm
	parsePixelData(gpTypeFileInfoHdr, pixelData, msgPixelData, &gMsgFileSize, extractBits, gKey, gAction);

	unsigned char headerData[14];
	unsigned char headerDataInfo[40];
	memcpy(headerData, coverData, 14);
	memcpy(headerDataInfo, coverData + 14, 40);
	FILE* f1;
	size_t count = gpTypeFileHdr->bfSize - gpTypeFileHdr->bfOffBits;
	switch (gAction)
	{
	case ACTION_HIDE:
	{
		f1 = fopen(gOutputFileName, "wb");
		fwrite(headerData, 1, 14, f1);
		fwrite(headerDataInfo, 1, 40, f1);
		fwrite(gpCoverPalette, 1, 8, f1);
		fwrite(pixelData, 1, count, f1);
		fclose(f1);

		printf("Message hidden in %s\n", gOutputPathFileName);
		break;
	}
	case ACTION_EXTRACT:
	{
		f1 = fopen(gOutputFileName, "wb");
		fwrite(extractBits, 1, gKey, f1);
		fclose(f1);
		printf("Message extracted to %s\n", gOutputPathFileName);
		break;
	}
	}
	return 0;
} // main




