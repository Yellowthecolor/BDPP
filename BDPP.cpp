
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "BitmapReader.h"

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
};

struct blockInfo
{
	int blockNumber;
	int matrix[3][3];
	int H = 0;
	int V = 0;
	int D = 0;
	int ratioCheck = 0; // 0 = not passed ratio check, 1 = passed ratio check
	int hvdCheck = 0; // 0 = not passed hvd check, 1 = passed hvd check
	unsigned int middlePixel;
	blockRatios currentBlockRatios;
};

void diagonalPartition(blockInfo* currentBlock, int blockNumber);
void checkBlockRatios(blockRatios* currentBlockRatios, int checkedValue);
void connectivityTest(blockInfo* currentBlock, int blockNumber);
void embedData(blockInfo* currentBlock, int blockNumber, unsigned char* bitsInMsg);

void parsePixelData(BITMAPINFOHEADER* pFileInfo, unsigned char* pixelData,
	unsigned char* msgPixelData, unsigned int* gMsgFileSize)
{
	unsigned int numberOfPixels = pFileInfo->biWidth * pFileInfo->biHeight;
	unsigned int imageSize = pFileInfo->biSizeImage;
	unsigned char* bitsInImage;
	bitsInImage = (unsigned char*)malloc(sizeof(unsigned char) * numberOfPixels);
	unsigned char* bitsInMsg;
	bitsInMsg = (unsigned char*)malloc(sizeof(unsigned char) * numberOfPixels);

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

	int msgBitCounter = 0;
	for (int i = 0; i < *gMsgFileSize; i++)
	{
		for (int j = 7; j >= 0; j--)
		{
			unsigned char currentBit = ((*(msgPixelData + i) >> j) & 1);
			bitsInMsg[msgBitCounter] = currentBit;
			msgBitCounter++;
		}
	}

	printf("\nMessage Bits: \n");
	//for (int i = 0; i < *gMsgFileSize; i++)
	for (int i = 0; i < 1; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			printf("%u", bitsInMsg[i * 8 + j]);
		}
		printf(" ");
	}
	printf(" %d\n", *gMsgFileSize);
	printf("\n\n");

	// printing for debugging
	  // printf("\n");
	  // printf("%d", bitCounter);
	  // printf(" %d", numberOfPixels);
	  // printf("\n");

	  // int j = 2048;
	  // for (int j = 2048; j >= 0; j -= 1024) {
	  // 	for (int i = 0; i < 3/*bitCounter*/; i++)
	  // 	{

	  // 		printf("%u", bitsInImage[i + j]);
	  // 		if ((i + 1) % 8 == 0)
	  // 			printf(" ");
	  // 	}
	  // 	printf("\n");
	  // }
	  // printf("\n\n");

	int blockHeight = floor(pFileInfo->biHeight / 3);
	int blockWidth = floor(pFileInfo->biWidth / 3);
	int totalBlocks = blockWidth * blockHeight;

	printf("%d %d\n%d\n", blockHeight, blockWidth, totalBlocks);

	blockInfo* blockArray;
	blockArray = (blockInfo*)malloc(sizeof(blockInfo) * totalBlocks);

	bitCounter = 0;
	int multiplier = 0;
	int breakingpoint = 0;
	// int bitCTwo = 0;
	// int trash = 0;
	// int finalmult = 0;
	for (int i = 0; i < blockHeight; i++)
	{
		for (int j = 0; j < blockWidth; j++)
		{
			blockInfo currentBlock;
			currentBlock.blockNumber = i * blockWidth + j;
			for (int k = 2; k >= 0; k--)
			{
				for (int l = 0; l < 3; l++)
				{
					if (bitCounter + pFileInfo->biWidth * multiplier >= numberOfPixels)
					{
						breakingpoint = 1;
						break;
						// bitCounter = bitCounter;
						// bitCTwo = bitCounter + pFileInfo->biWidth * multiplier;
						// trash = i * blockWidth + j;
					}

					currentBlock.matrix[k][l] = bitsInImage[bitCounter + pFileInfo->biWidth * multiplier];
					bitCounter++;
				}
				bitCounter = bitCounter - 3;
				multiplier++;
				if (breakingpoint) break;
			}

			blockArray[i * blockWidth + j] = currentBlock;
			bitCounter = bitCounter + 3;

			multiplier = multiplier - 3;
			if (breakingpoint) break;

		}
		multiplier = multiplier + 3;
		if (breakingpoint) break;
	}

	// printf("bitcounter: %d\n", bitCounter);
	// printf("multi bit counter %d\n", bitCTwo);
	// printf("Total Blocks %d\n", totalBlocks);
	// printf("Total pixels in blocks %d\n", totalBlocks * 9);
	// printf("Allocated blocks %d\n", blockArray[trash].blockNumber);
	// printf("Number of Pixels: %d\n\n", numberOfPixels);

	for (int k = 0; k < 4; k++)
	{
		printf("Block Number: %d\n", blockArray[k].blockNumber);
		printf("Block Matrix: \n");
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				printf("%d ", blockArray[k].matrix[i][j]);
			}
			printf("\n");
		}
	}

	for (int i = 0; i < 4; i++)
	{
		diagonalPartition(&blockArray[i], blockArray[i].blockNumber);
		if (!blockArray[i].ratioCheck) continue;
		connectivityTest(&blockArray[i], blockArray[i].blockNumber);
		if (!blockArray[i].ratioCheck) continue;
		embedData(&blockArray[i], blockArray[i].blockNumber, &bitsInMsg[i]);
	}

	for (int k = 0; k < 4; k++)
	{
		printf("Block Number: %d\n", blockArray[k].blockNumber);
		printf("Block Matrix: \n");
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				printf("%d ", blockArray[k].matrix[i][j]);
			}
			printf("\n");
		}
	}

	return;
} // displayFileInfo


void diagonalPartition(blockInfo* currentBlock, int blockNumber)
{

	currentBlock->middlePixel = currentBlock->matrix[1][1];

	printf("Block Number: %d\n", blockNumber);
	printf("Middle Pixel: %d\n", currentBlock->middlePixel);

	//partition ratio counters zeroes to ones
	struct blockRatios* currentBlockRatios = &currentBlock->currentBlockRatios;

	// reset ratios to zero
	currentBlock->currentBlockRatios.sixToZero = 0;
	currentBlock->currentBlockRatios.fiveToOne = 0;
	currentBlock->currentBlockRatios.fourToTwo = 0;
	currentBlock->currentBlockRatios.threeToThree = 0;
	currentBlock->currentBlockRatios.twoToFour = 0;
	currentBlock->currentBlockRatios.oneToFive = 0;
	currentBlock->currentBlockRatios.zeroToSix = 0;

	//upper left block
	int checkedValue = 0;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (i == 1 && j == 2) continue;
			else if (i == 2 && j == 1) continue;
			else if (i == 2 && j == 2) continue;

			if (currentBlock->matrix[i][j] == 0)
			{
				checkedValue++;
			}
		}
	}
	checkBlockRatios(currentBlockRatios, checkedValue);

	//lower right block
	checkedValue = 0;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (i == 0 && j == 0) continue;
			else if (i == 0 && j == 1) continue;
			else if (i == 1 && j == 0) continue;

			if (currentBlock->matrix[i][j] == 0)
			{
				checkedValue++;
			}
		}
	}
	checkBlockRatios(currentBlockRatios, checkedValue);

	//upper right block
	checkedValue = 0;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (i == 1 && j == 0) continue;
			else if (i == 2 && j == 0) continue;
			else if (i == 2 && j == 1) continue;

			if (currentBlock->matrix[i][j] == 0)
			{
				checkedValue++;
			}
		}
	}
	checkBlockRatios(currentBlockRatios, checkedValue);

	//lower left block
	checkedValue = 0;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (i == 0 && j == 1) continue;
			else if (i == 0 && j == 2) continue;
			else if (i == 1 && j == 2) continue;

			if (currentBlock->matrix[i][j] == 0)
			{
				checkedValue++;
			}
		}
	}
	checkBlockRatios(currentBlockRatios, checkedValue);

	//check for unique ratios
	if (currentBlockRatios->sixToZero > 0)
		currentBlockRatios->totalUniqueRatios++;
	if (currentBlockRatios->fiveToOne > 0)
		currentBlockRatios->totalUniqueRatios++;
	if (currentBlockRatios->fourToTwo > 0)
		currentBlockRatios->totalUniqueRatios++;
	if (currentBlockRatios->threeToThree > 0)
		currentBlockRatios->totalUniqueRatios++;
	if (currentBlockRatios->twoToFour > 0)
		currentBlockRatios->totalUniqueRatios++;
	if (currentBlockRatios->oneToFive > 0)
		currentBlockRatios->totalUniqueRatios++;
	if (currentBlockRatios->zeroToSix > 0)
		currentBlockRatios->totalUniqueRatios++;

	//check if ratio check passed
	if (currentBlockRatios->totalUniqueRatios >= 2)
	{
		currentBlock->ratioCheck = 1;
	}
	else
	{
		currentBlock->ratioCheck = 0;
		printf("Ratio Check Failed for Block %d\n", blockNumber);
	}

	// prints used for debugging
	printf("Unique Ratios: %d\n", currentBlockRatios->totalUniqueRatios);
	printf("6:0 = %d\n", currentBlockRatios->sixToZero);
	printf("5:1 = %d\n", currentBlockRatios->fiveToOne);
	printf("4:2 = %d\n", currentBlockRatios->fourToTwo);
	printf("3:3 = %d\n", currentBlockRatios->threeToThree);
	printf("2:4 = %d\n", currentBlockRatios->twoToFour);
	printf("1:5 = %d\n", currentBlockRatios->oneToFive);
	printf("0:6 = %d\n", currentBlockRatios->zeroToSix);
	printf("Ratio Check: %d\n", currentBlock->ratioCheck);
	printf("\n");
	return;
}

void checkBlockRatios(blockRatios* currentBlockRatios, int checkedValue)
{
	for (int i = 6; i >= 0; i--)
	{
		if (checkedValue == i)
		{
			switch (i)
			{
			case 0:
				currentBlockRatios->zeroToSix++;
				break;
			case 1:
				currentBlockRatios->oneToFive++;
				break;
			case 2:
				currentBlockRatios->twoToFour++;
				break;
			case 3:
				currentBlockRatios->threeToThree++;
				break;
			case 4:
				currentBlockRatios->fourToTwo++;
				break;
			case 5:
				currentBlockRatios->fiveToOne++;
				break;
			case 6:
				currentBlockRatios->sixToZero++;
				break;
			default:
				printf("Error counting ratios\n");
				break;
			}
		}
	}
	return;
}

// Test connectivitity of consecutive 0s in the matrix
void connectivityTest(blockInfo* currentBlock, int blockNumber)
{
	int i, j;

	// Horizontal and Diagonal Connectivity Check
	j = 1;
	for (i = 0; i < 3; i++)
	{
		if (currentBlock->matrix[i][j] == 0)
		{
			// Horizontal connectivity check
			if (currentBlock->matrix[i][j - 1] == 0 || currentBlock->matrix[i][j + 1] == 0)
			{
				currentBlock->H = 1;
			}

			// Diagonal connectivity check
			if (i == 1 || i == 0)
			{
				if (currentBlock->matrix[i + 1][j - 1] == 0 || currentBlock->matrix[i + 1][j + 1] == 0)
				{
					currentBlock->D = 1;
				}
			}
			if (i == 1 || i == 2)
			{
				if (currentBlock->matrix[i - 1][j - 1] == 0 || currentBlock->matrix[i - 1][j + 1] == 0)
				{
					currentBlock->D = 1;
				}
			}
		}
	}

	// Vertical Connectivity Check
	i = 1;
	for (j = 0; j < 3; j++)
	{
		if (currentBlock->matrix[i][j] == 0)
		{
			// Vertical connectivity check
			if (currentBlock->matrix[i + 1][j] == 0 || currentBlock->matrix[i - 1][j] == 0)
			{
				currentBlock->V = 1;
			}
		}
	}

	if (currentBlock->H && currentBlock->V && currentBlock->D)
	{
		currentBlock->hvdCheck = 1;
	}
	else
	{
		currentBlock->hvdCheck = 0;
		printf("HVD Check Failed for Block %d\n", blockNumber);
	}

	// prints used for debugging
	printf("Block Number: %d\n", blockNumber);
	printf("Horizontal Connectivity: %d\n", currentBlock->H);
	printf("Vertical Connectivity: %d\n", currentBlock->V);
	printf("Diagonal Connectivity: %d\n", currentBlock->D);
	printf("HVD Check: %d\n", currentBlock->hvdCheck);
	printf("\n");
	return;
}

void embedData(blockInfo* currentBlock, int blockNumber, unsigned char* bitsInMsg)
{
	int temp = currentBlock->matrix[1][1];
	printf("matrix[1][1] = %d\n\n\n", currentBlock->matrix[1][1]);
	if (currentBlock->matrix[1][1] == 1)
		currentBlock->matrix[1][1] = 0;
	else if (currentBlock->matrix[1][1] == 0)
		currentBlock->matrix[1][1] = 1;
	else
	{
		printf("Error - %d is not a bit. Data may not be binary.\n\n", currentBlock->matrix[1][1]);
		exit(-1);
	}
	printf("matrix[1][1] = %d\n\n\n", currentBlock->matrix[1][1]);
	diagonalPartition(currentBlock, blockNumber);
	connectivityTest(currentBlock, blockNumber);
	if (!currentBlock->ratioCheck || !currentBlock->hvdCheck)
	{
		currentBlock->matrix[1][1] = temp;
		printf("Data Embedding Failed for Block %d\n", blockNumber);
    return;
	}

	currentBlock->matrix[1][1] = *bitsInMsg;

}
