
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BitmapReader.h"

struct blockInfo
{
  int blockNumber;
  int matrix[3][3];
  int H = 0;
  int V = 0;
  int D = 0;
  unsigned int middlePixel;
}

void parsePixelData(BITMAPINFOHEADER *pFileInfo, unsigned char *pixelData)
{
	unsigned int numberOfPixels = pFileInfo->biWidth * pFileInfo->biHeight;
	unsigned int imageSize = pFileInfo->biSizeImage;
	unsigned char* bitsInImage;
	bitsInImage = (unsigned char*)malloc(sizeof(unsigned char) * numberOfPixels);

	int bitCounter = 0;
	for (int i = 0; i < imageSize; i++)
	{
		for (int j = 7; j >= 0; j--)
		{
			unsigned char currentBit = ((*(pixelData + i) >> j) & 1);
			bitsInImage[bitCounter] = currentBit;
			bitCounter++;
		}
	}

  int blockHeight = floor(pFileInfo->biHeight / 3);
  int blockWidth = floor(pFileInfo->biWidth / 3);
  int totalBlocks = blockHeight * blockWidth;

  blockInfo *blockArray;
  blockArray = (blockInfo *)malloc(sizeof(blockInfo) * blockHeight * blockWidth);

  bitCounter = 0;
  int multiplier = 0;
  for(int i = 0; i < blockHeight; i++)
  {
    for(int j = 0; j < blockWidth; j++)
    {
      blockInfo currentBlock;
      currentBlock.blockNumber = i * blockWidth + j;
      for(int k = 2; k >= 0; k--)
      {
        for(int l = 0; l < 3; l++)
        {
          currentBlock.matrix[k][l] = bitsInImage[bitCounter + pFileInfo->biWidth * multiplier];
          bitCounter++;
        }
        bitCounter = bitCounter - 3;
        multiplier++;
      }
      bitCounter = bitCounter + 3;
      multiplier = multiplier - 3;
    }
    multiplier = multiplier + 3;
  }


  printf("Block Number: %d\n", blockArray[0].blockNumber);
  printf("Block Matrix: \n");
  for(int i = 0; i < 3; i++)
  {
    for(int j = 0; j < 3; j++)
    {
      printf("%d ", blockArray[0].matrix[i][j]);
    }
    printf("\n");
  }
  

	int k = 0;
	// print first 24 bytes of pixel data
	printf("\n Pixel Data: \n\n");
	for(int i = 0; i < 24; i++)
	{
		unsigned int testBit;
		//printf("%02X ", *(pixelData + i) );
		for (int j = 7; j >= 0; j--)
		{
			testBit = ((*(pixelData + i) >> j) & 1);
			bitsInImage[k] = testBit;
			k++;
			//printf("%d", testBit);
		}
		printf(" ");
	}

	for (int i = 0; i < 192; i++)
	{
		printf("%u", testBitArr[i]);
		if ((i + 1) % 8 == 0)
			printf(" ");
	}
}
