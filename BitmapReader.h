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

/*
* Structure: blockRatios
* Usage: blockRatios currentBlockRatios;
* ------------------------------------------------------
* This structure is used to store the ratios for the
* ratio check. The ratio is 0s and 1s in a 3x3 block
* matrix.
*/
struct blockRatios
{
    int sixToZero = 0;  //  6:0
    int fiveToOne = 0;   //  5:1
    int fourToTwo = 0;   //  4:2
    int threeToThree = 0; // 3:3
    int twoToFour = 0;   //  2:4
    int oneToFive = 0;   //  1:5
    int zeroToSix = 0;   //  0:6
    int totalUniqueRatios = 0;
};  // blockRatios

/*
* Structure: blockInfo
* Usage: blockInfo currentBlock;
* ------------------------------------------------------
* This structure is used to store the information for
* each 3x3 block in the image. This includes the block
* number, the 3x3 matrix, the middle pixel, the HVD
* check, the ratio check, and whether the block is
* embeddable. It also store the blockRatios structure
* for the individual blocks.
*/
struct blockInfo
{
    int blockNumber;  //  used to identify the block, primarily for debugging
    int matrix[3][3]; // 3x3 matrix of the block, stores the pixel values
    int H = 0;
    int V = 0; // Horizontal, Vertical, Diagonal connectivity,
    int D = 0; // 0 = not connected, 1 = connected
    int ratioCheck = 0; // 0 = not passed ratio check, 1 = passed ratio check
    int hvdCheck = 0; // 0 = not passed hvd check, 1 = passed hvd check
    int isEmbeddable = 0; // 0 = not embeddable, 1 = embeddable
    unsigned int middlePixel;
    blockRatios currentBlockRatios;  // structure to store the ratios for the ratio check
};  // blockInfo

/*
* Function: parsePixelData
* Usage: parsePixelData(pFileInfo, pixelData, msgPixelData, gMsgFileSize, extractedBits, gKey, action);
* ------------------------------------------------------
* This function is used to parse the pixel data from the
* image run the BDPP algorithm to embed or extract data
* and then reassamble the pixel data to be written back,
* or to be used for extraction.
*/
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

/*
* Function: checkBlockRatios
* Usage: checkBlockRatios(&currentBlockRatios, checkedValue);
* ------------------------------------------------------
* This function is used to check the ratio of 0s to 1s
* in a block matrix. The ratios are 6:0, 5:1, 4:2, 3:3,
* 2:4, 1:5, 0:6. This is called by diagonalPartition
*/
void checkBlockRatios(blockRatios* currentBlockRatios, int checkedValue);

/*
* Function: connectivityTest
* Usage: connectivityTest(&blockArray[i], blockArray[i].blockNumber);
* ------------------------------------------------------
* This function is used to check the connectivity of 0s
* in a block. The block must have at least 2 zeros
* connected horizontally, vertically, and diagonally to
* pass the HVD check. This is done after the ratio check.
* If the block is HVD connected (hvdCheck = 1).
*/
void connectivityTest(blockInfo* currentBlock, int blockNumber);

/*
* Function: embedData
* Usage: embedData(&blockArray[i], blockArray[i].blockNumber);
* ------------------------------------------------------
* This function is used to "embed" data into the
* embeddable blocks. It first flips the middle pixel
* then calls diagonalPartition, and connectivityTest
* to check if the block is embeddable. If the block is
* embeddable (isEmbeddable = 1) then the middle pixel
* is flipped back to its original value.
*/
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

