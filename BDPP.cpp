// Block Diagonal Partition Pattern (BDPP) Algorithm
// Authors: John A. Ortiz
// Modified by: Roberto Delgado, Mark Solis, Daniel Zartuche
//
// This program uses the Block-Diagonal Partition Pattern (BDPP) algorithm to hide and extract data from a bitmap image.
// This method of hiding was developed by Gyankamal J.Chhajed and Bindu Garg in 2023
// 
// This program's implementation of the BDPP algorithm was designed by Roberto Delgado
// The algorithm is as follows:
// 1. The image is divided into 3x3 blocks.
// 2. Each block is then diagonally partitioned into 4 sections. Pixels split in half are counted as whole.
// 3. Each section is checked for a ratio of 0s to 1s in the block's matrix. The ratios are 6:0, 5:1, 4:2, 3:3, 2:4, 1:5, 0:6.
// 4. If a block has 2 or more unique ratios, it passes the ratio check.
// 5. The block is then checked for horizontal, vertical, and diagonal connectivity of 0s.
//    this means that there must be at least 2 zeros connected horizontally, vertically, and diagonally.
//    if this is true then the block is said to be HVD connected.
// 6. If the block passes both the ratio check and the HVD check, then the block is embeddable.
// 7. The middle pixel of the block is then flipped steps 2-5 are repeated for all blocks.
// 8. If the block still passes both checks, then the middle pixel is flipped back to its original value.
//    and the block is marked as embeddable.
// 9. The program then embeds the message data into the embeddable blocks or if extracting, extracts the data
//    from the embeddable blocks based on the key value provided.
//

#include "BitmapReader.h"


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
	unsigned char* msgPixelData, unsigned int* gMsgFileSize, unsigned char* extractedBits, int gKey, int action)
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
	int blockHeight = floor(pFileInfo->biHeight / 3);
	int blockWidth = floor(pFileInfo->biWidth / 3);
	int totalPossibleBlocks = blockWidth * blockHeight;

	blockInfo* blockArray;
	blockArray = (blockInfo*)malloc(sizeof(blockInfo) * totalPossibleBlocks);


	bitCounter = 0;
	int multiplier = 0;
	int breakingpoint = 0;
	size_t rowSize = ((pFileInfo->biWidth + 7) / 8 + 3) & ~3;
	for (int i = 0; i < blockHeight; i++)
	{
		for (int j = 0; j < blockWidth; j++)
		{
			blockInfo currentBlock;
			currentBlock.blockNumber = i * blockWidth + j;
			int blockX = j * 3;
			int blockY = i * 3;
			bitCounter = blockY * pFileInfo->biWidth;

			for (int k = 2; k >= 0; k--)
			{
				for (int l = 0; l < 3; l++)
				{
					int pixelX = blockX + l;
					int pixelY = blockY + k;
					size_t byteIndex = (pixelY * rowSize) + (pixelX / 8);
					size_t bitIndex = 7 - (pixelX % 8);
					if (bitCounter + pFileInfo->biWidth * multiplier >= numberOfPixels)
					{
						breakingpoint = 1;
						break;
					}

					currentBlock.matrix[k][l] = (pixelData[byteIndex] & (1 << bitIndex)) ? 1 : 0;
					bitCounter++;
				}
				if (breakingpoint) break;
				bitCounter = bitCounter - 3;
				multiplier++;

			}
			if (breakingpoint) break;
			blockArray[i * blockWidth + j] = currentBlock;
			bitCounter = (i * 3 + 3) * pFileInfo->biWidth;
			multiplier = 0;


		}
		if (breakingpoint) break;
	}

	int embeddableBlocks = 0;
	for (int i = 0; i < totalPossibleBlocks; i++)
	{
		diagonalPartition(&blockArray[i], blockArray[i].blockNumber);
		if (!blockArray[i].ratioCheck) continue;
		connectivityTest(&blockArray[i], blockArray[i].blockNumber);
		if (!blockArray[i].hvdCheck) continue;
		embedData(&blockArray[i], blockArray[i].blockNumber);
		if (!blockArray[i].isEmbeddable) continue;
		embeddableBlocks++;
	}
	unsigned int msgBitCounter = 0;
	unsigned int totalMsgBits = 0;
	unsigned char currentBit = 0;
	size_t bitIndex = 0;
	size_t byteIndex = 0;
	switch (action)
	{
	case ACTION_HIDE:
	{
		msgBitCounter = 0;
		for (int i = 0; i < *gMsgFileSize; i++)
		{
			for (int j = 7; j >= 0; j--)
			{
				unsigned char currentBit = ((*(msgPixelData + i) >> j) & 1);
				bitsInMsg[msgBitCounter] = currentBit;
				msgBitCounter++;
			}
		}
		totalMsgBits = msgBitCounter;

		for (int i = 0; i < totalPossibleBlocks; i++)
		{
			blockInfo* currentBlock = &blockArray[i];
			if (bitIndex >= totalMsgBits) break;
			if (!currentBlock->isEmbeddable) continue;

			int blockNumber = currentBlock->blockNumber;
			currentBit = bitsInMsg[bitIndex++]; // use bit before incrementing
			currentBlock->matrix[1][1] = currentBit;
		}
		gKey = bitIndex;
		if (bitIndex < totalMsgBits)
		{
			printf("Error - Message too large to embed in image, possible message Data loss.\n\n");
		}
		else
		{
			printf("\n\nMessage Embedded Successfully\n");
		}
		printf("Key Value (Used when extracting): %d\n\n", gKey);
		printf("Message Size: %d bits\n", totalMsgBits);
		printf("Total Blocks: %d\n", totalPossibleBlocks);
		printf("Embeddable Blocks: %d\n", embeddableBlocks);
		printf("Embedded Blocks Used: %d\n", bitIndex);
		printf("Hiding Capacity: %d bits | %d bytes\n", embeddableBlocks, embeddableBlocks / 8);
		printf("Percentage Capacity Used: %.2f%%\n", (float)bitIndex / (float)embeddableBlocks * 100);

		// Modify pixelData using totalPossibleBlocks.
		int randomCounter = 0;
		rowSize = ((pFileInfo->biWidth + 7) / 8 + 3) & ~3;
		for (int k = 0; k < totalPossibleBlocks; k++)
		{
			int blockX = (k % blockWidth) * 3;
			int blockY = (k / blockWidth) * 3;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					int px = blockX + j;
					int py = blockY + i;
					size_t byteIndex = (py * rowSize) + (px / 8);
					size_t bitIndex = 7 - (px % 8);
					if (blockArray[k].matrix[i][j] == 1)
					{
						pixelData[byteIndex] |= (1 << bitIndex);
					}
					else
					{
						pixelData[byteIndex] &= ~(1 << bitIndex);
					}
				}
			}
		}
		break;
	}
	case ACTION_EXTRACT:
	{
		for (int i = 0; i < totalPossibleBlocks; i++)
		{
			blockInfo* currentBlock = &blockArray[i];
			if (bitIndex > gKey) break;
			if (!currentBlock->isEmbeddable) continue;

			currentBit = currentBlock->matrix[1][1] ^ 1;
			extractedBits[bitIndex++] = currentBit;
		}
		if (bitIndex < gKey)
		{
			printf("Error - Extracted data doesnot match key, possible data loss or incorrect key.\n");
		}
		else
		{
			printf("Message Extracted Successfully\n\n");
		}
		break;
	}
	default:
		printf("Error - Invalid action\n");
		break;
	}

	return;
} // parsePixelData


/*
* Function: printMatrix
* Usage: printMatrix(&blockArray[i]);
* ------------------------------------------------------
* This function is used to print the 3x3 matrix of a
* block mostly for debugging.
*/
void printMatrix(blockInfo* block) {
	printf("Block Number: %d\n", block->blockNumber);
	printf("Block Matrix: \n");
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			printf("%d ", block->matrix[i][j]);
		}
		printf("\n");
	}
}  // printMatrix


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
void diagonalPartition(blockInfo* currentBlock, int blockNumber)
{

	currentBlock->middlePixel = currentBlock->matrix[1][1];
	currentBlock->ratioCheck = 0;

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
	}

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

	// Reset H, D, V, hvdCheck upon second call for embedding test.
	currentBlock->H = 0;
	currentBlock->D = 0;
	currentBlock->V = 0;
	currentBlock->hvdCheck = 0;

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
	}
	return;
}

void embedData(blockInfo* currentBlock, int blockNumber)
{
	int temp = currentBlock->matrix[1][1];
	if (currentBlock->matrix[1][1] == 1)
		currentBlock->matrix[1][1] = 0;
	else if (currentBlock->matrix[1][1] == 0)
		currentBlock->matrix[1][1] = 1;
	else
	{
		printf("Error - %d is not a bit. Data may not be binary.\n\n", currentBlock->matrix[1][1]);
		exit(-1);
	}
	diagonalPartition(currentBlock, blockNumber);
	connectivityTest(currentBlock, blockNumber);
	if (!currentBlock->ratioCheck || !currentBlock->hvdCheck)
	{
		currentBlock->matrix[1][1] = temp;
		currentBlock->isEmbeddable = 0;
		return;
	}
	currentBlock->isEmbeddable = 1;
}
