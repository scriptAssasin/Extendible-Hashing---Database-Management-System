#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20
// #define NUM_OF_RECORDS ((BF_BLOCK_SIZE - 2 * sizeof(int)) / sizeof(Record)) //Number of records per Block.
#define NUM_OF_RECORDS 3 // Number of records per Block.

static int counter = 0;

FileIndex array[MAX_OPEN_FILES];

#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HT_ERROR;        \
    }                         \
  }

HT_ErrorCode HT_Init()
{
  // insert code here
  for (int i = 0; i < MAX_OPEN_FILES; i++)
  {
    array[i].isOpen = 0;
    array[i].fileDesc = 0;
    array[i].buckets = 0;
    array[i].depth = 0;
    strcpy(array[i].fileName, "0");
  }
  return HT_OK;
}

int Position_In_Array()
{

  for (int i = 0; i < MAX_OPEN_FILES; i++)

    if (array[i].isOpen == 0)
      return i;
}

int binaryTodecimal(int binaryNumber)
{
  int decimal_num = 0, temp = 0, rem;
  while (binaryNumber != 0)
  {
    rem = binaryNumber % 10;
    binaryNumber = binaryNumber / 10;
    decimal_num = decimal_num + rem * pow(2, temp);
    temp++;
  }
  return decimal_num;
}

int hashFunc(int key, int depth)
{
  // if (key == 0)
  //   return 0;
  // if (key == 1)
  //   return 1;
  // if (key == 2)
  //   return 2;
  // if (key == 3)
  //   return 3;
  // array to store binary number
  int binaryNum[32];
  for (int i = 0; i < 32; i++)
  {
    binaryNum[i] = 0;
  }

  // counter for binary array
  int i = 0;
  // printf("%d-", key);
  while (key > 0)
  {

    // storing remainder in binary array
    binaryNum[i] = key % 2;
    key = key / 2;
    i++;
  }
  // int final = 0;
  char *myStr = malloc(100 * sizeof(char));
  // printing binary array in reverse order
  for (int k = depth - 1; k >= 0; k--)
  {
    char *temp = malloc(2 * sizeof(char));
    sprintf(temp, "%d", binaryNum[k]);
    strcat(myStr, temp);
  }

  for (int k = 0; k < strlen(myStr) - depth; k++)
    if (myStr[k] != '0' && myStr[k] != '1')
      myStr[k] = '0';

  // printf("%s", myStr);
  // printf("\n");
  // printf("%s",myStr);

  return binaryTodecimal(atoi(myStr));
}

int localdepthFunc(int key, int depth)
{
  // array to store binary number
  int binaryNum[32];
  for (int i = 0; i < 32; i++)
  {
    binaryNum[i] = 0;
  }

  // counter for binary array
  int i = 0;
  // printf("%d-", key);
  while (key > 0)
  {

    // storing remainder in binary array
    binaryNum[i] = key % 2;
    key = key / 2;
    i++;
  }
  // int final = 0;
  char *myStr = malloc(100 * sizeof(char));
  // printing binary array in reverse order
  for (int k = depth - 1; k >= 0; k--)
  {
    char *temp = malloc(2 * sizeof(char));
    sprintf(temp, "%d", binaryNum[k]);
    strcat(myStr, temp);
  }

  for (int k = 0; k < strlen(myStr) - depth; k++)
    if (myStr[k] != '0' && myStr[k] != '1')
      myStr[k] = '0';

  // printf("%s", myStr);
  // printf("\n");
  // printf("%s",myStr);

  return binaryTodecimal(atoi(myStr));
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth)
{
  // insert code here

  // Create a file with name filename.
  CALL_BF(BF_CreateFile(filename));

  int fileDesc;

  CALL_BF(BF_OpenFile(filename, &fileDesc));

  // Unique signature to the first block of the file to make it a HASH_FILE
  BF_Block *block;
  BF_Block_Init(&block);

  // Allocate memory for the first
  CALL_BF(BF_AllocateBlock(fileDesc, block));

  // Keep block data
  char *blockData;
  blockData = BF_Block_GetData(block);

  // Integer signature chosen for the first block
  int sign = 100;
  // Number of Directories = 2^Global Depth
  int directoriesNum = (int)pow(2.0, (double)depth);

  memcpy(blockData, &sign, sizeof(int));
  memcpy(blockData + sizeof(int), &directoriesNum, sizeof(int));

  // Because we want to change the data of the first block.
  BF_Block_SetDirty(block);
  // I unpin the block because I don't want it anymore.
  CALL_BF(BF_UnpinBlock(block));

  int num;
  num = (directoriesNum * sizeof(int)) / BF_BLOCK_SIZE + 1;

  for (int i = 0; i < num; i++)
  {
    CALL_BF(BF_AllocateBlock(fileDesc, block));
    CALL_BF(BF_UnpinBlock(block));
  }

  // Close the file.
  CALL_BF(BF_CloseFile(fileDesc));
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc)
{
  // printf("--%ld--\n", NUM_OF_RECORDS);
  // insert code here
  int fileDesc;
  // Open file
  CALL_BF(BF_OpenFile(fileName, &fileDesc));

  // Find index in array
  int pos = Position_In_Array();
  // Change isOpen boolean for this specific file, fileDesc and filename
  array[pos].isOpen = 1;
  array[pos].fileDesc = fileDesc;
  strcpy(array[pos].fileName, fileName);
  *indexDesc = pos;
  printf("Hash file named %s is stored in position %d of fileindex array\n", fileName, pos);

  BF_Block *block;
  BF_Block_Init(&block);

  int signCheck, num_of_buckets;
  CALL_BF(BF_GetBlock(fileDesc, 0, block));
  char *data = BF_Block_GetData(block);
  memcpy(&signCheck, data, sizeof(int));

  if (signCheck != 100)
    return HT_ERROR;

  memcpy(&num_of_buckets, data + sizeof(int), sizeof(int));
  array[pos].buckets = num_of_buckets;
  array[pos].depth = (int)sqrt((double)num_of_buckets);

  for (int i = 0; i < 1000; i++)
  {
    array[pos].directories[i] = INT_MAX;
    array[pos].bucketslocaldepth[i] = INT_MAX;
  }

  for (int i = 0; i < array[pos].buckets; i++)
  {
    array[pos].directories[i] = i;
    array[pos].bucketslocaldepth[i] = array[pos].depth;
  }

  CALL_BF(BF_UnpinBlock(block));

  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc)
{
  // insert code here
  CALL_BF(BF_CloseFile(array[indexDesc].fileDesc));

  printf("Hash file named %s which is stored in position %d of fileindex array has just closed\n", array[indexDesc].fileName, indexDesc);

  array[indexDesc].isOpen = 0;
  array[indexDesc].fileDesc = 0;
  array[indexDesc].buckets = 0;
  array[indexDesc].depth = 0;
  strcpy(array[indexDesc].fileName, "0");

  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record)
{

  int num_of_block, hash, temp;
  int file_desc = array[indexDesc].fileDesc;
  int buckets = array[indexDesc].buckets;
  int depth = array[indexDesc].depth;
  int eight_bytes = 2 * sizeof(int); // 4 bytes for count_records, 4 bytes for next_block.
  int blocks_num;

  // DEBUG START
  // CALL_BF(BF_GetBlockCounter(array[indexDesc].fileDesc, &blocks_num));
  // if (blocks_num > 4)
  // {

  //   BF_Block *yyy;
  //   BF_Block_Init(&yyy);
  //   int signCheck;
  //   CALL_BF(BF_GetBlock(array[indexDesc].fileDesc, 3, yyy));
  //   char *dataa = BF_Block_GetData(yyy);

  //   // memcpy(&tels, data + eight_bytes + 2 * sizeof(Record), sizeof(Record));
  //   memcpy(&signCheck, dataa, sizeof(int));

  //   // printf("%d\n", signCheck);
  //   Record mytemparrr[3];
  //   Record empty;
  //   for (int i = 0; i < 3; i++)
  //   {
  //     memcpy(&mytemparrr[i], dataa + eight_bytes + i * sizeof(Record), sizeof(Record));
  //     printf("--%d->%d--,",mytemparrr[i].id,signCheck);

  //     // int asdssa = array[indexDesc].directories[hashFunc(mytemparr[i].id, depth)];
  //     // printf("%d - %d\n",mytemparr[i].id, asdssa);
  //   }
  // }
  // printf("\n");

  // DEBUG END

  BF_Block *block;
  BF_Block_Init(&block);
  hash = hashFunc(record.id, depth);                       // Hash function.
  num_of_block = (array[indexDesc].directories[hash]) + 1; // Find in which block is the willing bucket.
  // if(depth == 2)
  // printf("%d\n",hash);
  CALL_BF(BF_GetBlock(file_desc, num_of_block, block));
  char *data = BF_Block_GetData(block);
  memcpy(&temp, data + array[indexDesc].directories[hash] * sizeof(int), sizeof(int)); // Take the key of the bucket.

  // If it is the first time that we visit this bucket.
  // We allocate the first block of a specific bucket.
  if (temp == 0)
  {
    // printf("%d,%d\n", record.id, num_of_block);
    // for(int telis =0; telis<10;telis++)
    //   printf("%d\n",array[indexDesc].directories[telis]);
    // printf("\n");
    int blocks_num;
    CALL_BF(BF_GetBlockCounter(file_desc, &blocks_num));
    memcpy(data + array[indexDesc].directories[hash] * sizeof(int), &blocks_num, sizeof(int));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    CALL_BF(BF_AllocateBlock(file_desc, block));

    data = BF_Block_GetData(block);
    int count_record = 1;
    memcpy(data, &count_record, sizeof(int));
    memcpy(data + eight_bytes, &record, sizeof(Record)); // First 8 bytes are for count_record and next_block, others are for records.
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
  }
  else // We have already visited this bucket at least once.
  {
    // printf("1");
    CALL_BF(BF_UnpinBlock(block));
    int count_record;
    // Find the last block of the chain.
    while (1)
    {
      CALL_BF(BF_GetBlock(file_desc, temp, block));
      data = BF_Block_GetData(block);
      memcpy(&temp, data + sizeof(int), sizeof(int));
      if (temp != 0)
      {
        CALL_BF(BF_UnpinBlock(block));
      }
      else
        break;
    }
    memcpy(&count_record, data, sizeof(int));
    // Case 1: If there is freespace for the record.
    if (count_record < NUM_OF_RECORDS)
    {

      // printf("%d,", record.id);
      memcpy(data + eight_bytes + count_record * sizeof(Record), &record, sizeof(Record));
      count_record++;
      memcpy(data, &count_record, sizeof(int));
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
    }
    // Case 2: If there isn't any freespace for new Records, allocate a new block for this record.
    else
    {
      // printf("%d\n", record.id);
      int overflow_bucket = num_of_block - 1;
      int overflow_bucket2;
      int overflow_bucket_local_depth = array[indexDesc].bucketslocaldepth[overflow_bucket];

      // Case 1
      // global depth == local depth
      if (depth == overflow_bucket_local_depth)
      {
        printf("aa%d\n",record.id);

        int temparr[1000];
        for (int i = 0; i < 1000; i++)
        {
          temparr[i] = INT_MAX;
        }

        array[indexDesc].depth += 1;
        array[indexDesc].buckets += 1;

        // local_depth = array[indexDesc].bucketslocaldepth[overflow_bucket] + 1;

        for (int i = 0; i < pow(2, depth); i++)
        {
          if (hashFunc(i, depth) == overflow_bucket)
            array[indexDesc].bucketslocaldepth[i] += 1;
        }

        depth = array[indexDesc].depth;
        buckets = array[indexDesc].buckets;

        for (int i = 0; i < pow(2, depth); i++)
        {
          int bucket_local_depth = localdepthFunc(i, depth - 1);
          array[indexDesc].bucketslocaldepth[i] = array[indexDesc].bucketslocaldepth[bucket_local_depth];
        }

        for (int i = 0; i < pow(2, depth); i++)
        {
          if (i < pow(2, depth - 1))
            temparr[i] = array[indexDesc].directories[i];

          else
          {
            int idealHash = hashFunc(i, depth - 1);
            int diff = depth - array[indexDesc].bucketslocaldepth[idealHash];
            int acceptedPointers = pow(2, diff);

            int counter = 0;

            for (int j = 0; j < 1000; j++)
            {
              if (temparr[j] == idealHash)
                counter++;
            }

            if (counter < acceptedPointers)
              temparr[i] = idealHash;
            else
            {
              temparr[i] = i;
              overflow_bucket2 = i;
            }
          }
        }

        for (int i = 0; i < 1000; i++)
          array[indexDesc].directories[i] = temparr[i];

        // int array[] = {16,4,6,22,24,10,31,7,9,20,26};

        // for(int i=0;i<4;i++)
        //   printf("%d\n",hashFunc(array[i], depth));

        BF_Block *Firstblock;
        BF_Block_Init(&Firstblock);

        CALL_BF(BF_GetBlock(file_desc, 0, Firstblock));
        char *firstBlockdata = BF_Block_GetData(Firstblock);

        memcpy(firstBlockdata + sizeof(int), &buckets, sizeof(int));

        BF_Block_SetDirty(Firstblock);
        CALL_BF(BF_UnpinBlock(Firstblock));

        CALL_BF(BF_AllocateBlock(file_desc, Firstblock));
        CALL_BF(BF_UnpinBlock(Firstblock));

        // Record tels;

        // memcpy(&tels, data + eight_bytes + 2 * sizeof(Record), sizeof(Record));

        int previous_count_record = count_record;
        count_record = 0;

        memcpy(data, &count_record, sizeof(int));
        BF_Block_SetDirty(block);
        CALL_BF(BF_UnpinBlock(block));

        Record mytemparr[previous_count_record];
        Record empty;
        for (int i = 0; i < previous_count_record; i++)
        {
          memcpy(&mytemparr[i], data + eight_bytes + i * sizeof(Record), sizeof(Record));
          memcpy(data + eight_bytes + i * sizeof(Record), &empty, sizeof(Record));

          // int asdssa = array[indexDesc].directories[hashFunc(mytemparr[i].id, depth)];
          // printf("%d - %d\n",mytemparr[i].id, asdssa);
        }

        for (int i = 0; i < previous_count_record; i++)
        {
          HT_InsertEntry(indexDesc, mytemparr[i]);
        }

        HT_InsertEntry(indexDesc, record);
      }
      // printf("%d", array[indexDesc].bucketslocaldepth[num_of_block - 1]);
    }
  }
  BF_Block_Destroy(&block);

  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id)
{
  // insert code here
  return HT_OK;
}
