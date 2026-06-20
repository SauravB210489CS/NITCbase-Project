#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

int compareAttrs(Attribute attr1, Attribute attr2, int attrType) {
  if (attrType == NUMBER) {
    if (attr1.nVal < attr2.nVal) return -1;
    if (attr1.nVal > attr2.nVal) return 1;
    return 0;
  }

  return strcmp(attr1.sVal, attr2.sVal);
}

BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

BlockBuffer::BlockBuffer(char blockType) {
  this->blockNum = getFreeBlock(blockType);
}

int BlockBuffer::getBlockNum() {
  return blockNum;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  int bufferNum = StaticBuffer::getBufferNum(blockNum);

  if (bufferNum == E_OUTOFBOUND) return E_OUTOFBOUND;

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(blockNum);
    if (bufferNum < 0) return bufferNum;

    Disk::readBlock(StaticBuffer::blocks[bufferNum], blockNum);
  }

  *buffPtr = StaticBuffer::blocks[bufferNum];
  return SUCCESS;
}

int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  memcpy(head, buffer, HEADER_SIZE);
  return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  memcpy(buffer, head, HEADER_SIZE);
  return StaticBuffer::setDirtyBit(blockNum);
}

int BlockBuffer::setBlockType(int blockType) {
  if (blockNum < 0 || blockNum >= DISK_BLOCKS) return E_OUTOFBOUND;

  StaticBuffer::blockAllocMap[blockNum] = blockType;
  return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType) {
  for (int i = 0; i < DISK_BLOCKS; i++) {
    if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK) {
      blockNum = i;
      setBlockType(blockType);
      return i;
    }
  }

  return E_DISKFULL;
}

void BlockBuffer::releaseBlock() {
  if (blockNum >= 0 && blockNum < DISK_BLOCKS) {
    StaticBuffer::blockAllocMap[blockNum] = UNUSED_BLK;
  }
}

RecBuffer::RecBuffer() : BlockBuffer(REC) {}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}

int RecBuffer::getSlotMap(unsigned char *slotMap) {
  HeadInfo head;
  int ret = getHeader(&head);
  if (ret != SUCCESS) return ret;

  unsigned char *buffer;
  ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  memcpy(slotMap, buffer + HEADER_SIZE, head.numSlots);
  return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
  HeadInfo head;
  int ret = getHeader(&head);
  if (ret != SUCCESS) return ret;

  unsigned char *buffer;
  ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  memcpy(buffer + HEADER_SIZE, slotMap, head.numSlots);
  return StaticBuffer::setDirtyBit(blockNum);
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  HeadInfo head;
  int ret = getHeader(&head);
  if (ret != SUCCESS) return ret;

  if (slotNum < 0 || slotNum >= head.numSlots) return E_OUTOFBOUND;

  unsigned char *buffer;
  ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  int recordSize = head.numAttrs * ATTR_SIZE;
  int offset = HEADER_SIZE + head.numSlots + slotNum * recordSize;

  memcpy(rec, buffer + offset, recordSize);
  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
  HeadInfo head;
  int ret = getHeader(&head);
  if (ret != SUCCESS) return ret;

  if (slotNum < 0 || slotNum >= head.numSlots) return E_OUTOFBOUND;

  unsigned char *buffer;
  ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  int recordSize = head.numAttrs * ATTR_SIZE;
  int offset = HEADER_SIZE + head.numSlots + slotNum * recordSize;

  memcpy(buffer + offset, rec, recordSize);
  return StaticBuffer::setDirtyBit(blockNum);
}

IndBuffer::IndBuffer(int blockNum) : BlockBuffer(blockNum) {}

IndBuffer::IndBuffer(char blockType) : BlockBuffer(blockType) {}

IndInternal::IndInternal() : IndBuffer(IND_INTERNAL) {}

IndInternal::IndInternal(int blockNum) : IndBuffer(blockNum) {}

int IndInternal::getEntry(void *ptr, int indexNum) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  int offset = HEADER_SIZE + indexNum * INTERNAL_ENTRY_SIZE;
  memcpy(ptr, buffer + offset, INTERNAL_ENTRY_SIZE);
  return SUCCESS;
}

int IndInternal::setEntry(void *ptr, int indexNum) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  int offset = HEADER_SIZE + indexNum * INTERNAL_ENTRY_SIZE;
  memcpy(buffer + offset, ptr, INTERNAL_ENTRY_SIZE);
  return StaticBuffer::setDirtyBit(blockNum);
}

IndLeaf::IndLeaf() : IndBuffer(IND_LEAF) {}

IndLeaf::IndLeaf(int blockNum) : IndBuffer(blockNum) {}

int IndLeaf::getEntry(void *ptr, int indexNum) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  int offset = HEADER_SIZE + indexNum * LEAF_ENTRY_SIZE;
  memcpy(ptr, buffer + offset, LEAF_ENTRY_SIZE);
  return SUCCESS;
}

int IndLeaf::setEntry(void *ptr, int indexNum) {
  unsigned char *buffer;
  int ret = loadBlockAndGetBufferPtr(&buffer);
  if (ret != SUCCESS) return ret;

  int offset = HEADER_SIZE + indexNum * LEAF_ENTRY_SIZE;
  memcpy(buffer + offset, ptr, LEAF_ENTRY_SIZE);
  return StaticBuffer::setDirtyBit(blockNum);
}
