#include "StaticBuffer.h"

#include <cstring>

// Static member definitions
unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
  unsigned char block[BLOCK_SIZE];

  // Read Block Allocation Map from disk blocks 0,1,2,3
  for (int i = 0; i < BLOCK_ALLOCATION_MAP_SIZE; i++) {
    Disk::readBlock(block, i);
    memcpy(blockAllocMap + i * BLOCK_SIZE, block, BLOCK_SIZE);
  }

  // Initialize buffer metadata
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    metainfo[i].free = true;
    metainfo[i].dirty = false;
    metainfo[i].blockNum = -1;
    metainfo[i].timeStamp = -1;
  }
}

StaticBuffer::~StaticBuffer() {
  unsigned char block[BLOCK_SIZE];

  // Write Block Allocation Map back to disk
  for (int i = 0; i < BLOCK_ALLOCATION_MAP_SIZE; i++) {
    memcpy(block, blockAllocMap + i * BLOCK_SIZE, BLOCK_SIZE);
    Disk::writeBlock(block, i);
  }

  // Write dirty buffer blocks back to disk
  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (!metainfo[i].free && metainfo[i].dirty) {
      Disk::writeBlock(blocks[i], metainfo[i].blockNum);
    }
  }
}

int StaticBuffer::getStaticBlockType(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  return (int)blockAllocMap[blockNum];
}

int StaticBuffer::setDirtyBit(int blockNum) {
  int bufferNum = getBufferNum(blockNum);

  if (bufferNum == E_OUTOFBOUND) {
    return E_OUTOFBOUND;
  }

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    return E_BLOCKNOTINBUFFER;
  }

  metainfo[bufferNum].dirty = true;
  return SUCCESS;
}

int StaticBuffer::getBufferNum(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (!metainfo[i].free && metainfo[i].blockNum == blockNum) {
      return i;
    }
  }

  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::getFreeBuffer(int blockNum) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
    return E_OUTOFBOUND;
  }

  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (!metainfo[i].free) {
      metainfo[i].timeStamp++;
    }
  }

  int bufferNum = -1;

  for (int i = 0; i < BUFFER_CAPACITY; i++) {
    if (metainfo[i].free) {
      bufferNum = i;
      break;
    }
  }

  if (bufferNum == -1) {
    int maxTimeStamp = -1;

    for (int i = 0; i < BUFFER_CAPACITY; i++) {
      if (metainfo[i].timeStamp > maxTimeStamp) {
        maxTimeStamp = metainfo[i].timeStamp;
        bufferNum = i;
      }
    }

    if (metainfo[bufferNum].dirty) {
      Disk::writeBlock(blocks[bufferNum], metainfo[bufferNum].blockNum);
    }
  }

  metainfo[bufferNum].free = false;
  metainfo[bufferNum].dirty = false;
  metainfo[bufferNum].blockNum = blockNum;
  metainfo[bufferNum].timeStamp = 0;

  return bufferNum;
}
