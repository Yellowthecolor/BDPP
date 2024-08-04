// LSB_Steg Header File
// Authors: John A. Ortiz
// Modified by: Roberto Delgado, Mark Solis, Daniel Zartuche
//

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define SUCCESS 0
#define FAILURE -1

#define ACTION_HIDE		1
#define ACTION_EXTRACT	2

#define VERSION "1.0"

void parsePixelData(BITMAPINFOHEADER* pFileInfo, unsigned char* pixelData,
    unsigned char* msgPixelData, unsigned int* gMsgFileSize, unsigned char* extractBytes, int gKey, int action);

/*
* Function: printMatrix
* Usage: printMatrix(&blockArray[i]);
* ------------------------------------------------------
* This function is used to print the 3x3 matrix of a
* block mostly for debugging.
*/
void printMatrix(blockInfo* block);

/*
* Function: diagonalPartition
* Usage: diagonalPartition(&blockArray[i], int blockArray[i].blockNumber);
* ------------------------------------------------------
* This function splits each 3x3 block diagonally into 4
* sections upper left, upper right, lower left, and lower
* right. It then calls checkBlockRatios to check the ratio
* of 0s to 1s. If the block has 2 or more unique ratios
* it passes the ratio check (ratioCheck = 1).
*/
void diagonalPartition(blockInfo* currentBlock, int blockNumber);
void checkBlockRatios(blockRatios* currentBlockRatios, int checkedValue);
void connectivityTest(blockInfo* currentBlock, int blockNumber);
void embedData(blockInfo* currentBlock, int blockNumber);
// the following structure information is taken from wingdi.h

/* constants for the biCompression field
#define BI_RGB        0L	// (no compression)
#define BI_RLE8       1L	// (run-length encoding, 8 bits)
#define BI_RLE4       2L	// (run-length encoding, 4 bits)
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

// this structure defines the file header format for a bitmap file
typedef struct tagBITMAPFILEHEADER // (14 bytes)
{
        WORD    bfType;					// ASCII "BM"
        unsigned int   bfSize;					// total length of bitmap file
        WORD    bfReserved1;			// reserved
        WORD    bfReserved2;			// reserved
        unsigned int   bfOffBits;				// offset to start of actual pixel data
} BITMAPFILEHEADER;

// this structure defines the header which describes the bitmap itself
typedef struct tagBITMAPINFOHEADER // (40 bytes)
{
        unsigned int      biSize;				// size of BITMAPINFOHEADER
        LONG       biWidth;				// width in pixels
        LONG       biHeight;			// height in pixels
        WORD       biPlanes;			// always 1
        WORD       biBitCount;			// color bits per pixel
        unsigned int      biCompression;		// BI_RGB, BI_RLE8, BI_RLE4
        unsigned int      biSizeImage;			// total bytes in image
        LONG       biXPelsPerMeter;		// 0, or optional horizontal resolution
        LONG       biYPelsPerMeter;		// 0, or optional vertical resolution
        unsigned int      biClrUsed;			// colors actually used (normally zero, can be lower than biBitCount)
        unsigned int      biClrImportant;		// important colors actualy used (normally zero)
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD
{
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;

//*/

