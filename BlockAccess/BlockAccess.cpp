#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char *attrName, Attribute attrVal, int op) {
  RelCatEntry relCatEntry;
  AttrCatEntry attrCatEntry;

  if (RelCacheTable::getRelCatEntry(relId, &relCatEntry) != SUCCESS) return {-1, -1};
  if (AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry) != SUCCESS) return {-1, -1};

  RecId searchIndex;
  RelCacheTable::getSearchIndex(relId, &searchIndex);

  int blockNum = searchIndex.block;
  int slotNum = searchIndex.slot + 1;

  if (blockNum == -1) {
    blockNum = relCatEntry.firstBlk;
    slotNum = 0;
  }

  while (blockNum != -1) {
    RecBuffer recBuffer(blockNum);
    HeadInfo head;
    recBuffer.getHeader(&head);

    unsigned char slotMap[head.numSlots];
    recBuffer.getSlotMap(slotMap);

    for (int slot = slotNum; slot < head.numSlots; slot++) {
      if (slotMap[slot] == SLOT_OCCUPIED) {
        Attribute record[head.numAttrs];
        recBuffer.getRecord(record, slot);

        int cmp = compareAttrs(record[attrCatEntry.offset], attrVal, attrCatEntry.attrType);

        bool match = false;
        if (op == EQ && cmp == 0) match = true;
        if (op == NE && cmp != 0) match = true;
        if (op == LT && cmp < 0) match = true;
        if (op == LE && cmp <= 0) match = true;
        if (op == GT && cmp > 0) match = true;
        if (op == GE && cmp >= 0) match = true;

        if (match) {
          RecId found = {blockNum, slot};
          RelCacheTable::setSearchIndex(relId, &found);
          return found;
        }
      }
    }

    blockNum = head.rblock;
    slotNum = 0;
  }

  RelCacheTable::resetSearchIndex(relId);
  return {-1, -1};
}

int BlockAccess::search(int relId, Attribute *record, char *attrName, Attribute attrVal, int op) {
  RecId recId = linearSearch(relId, attrName, attrVal, op);

  if (recId.block == -1 || recId.slot == -1) {
    return E_NOTFOUND;
  }

  RecBuffer recBuffer(recId.block);
  return recBuffer.getRecord(record, recId.slot);
}

int BlockAccess::insert(int relId, union Attribute *record) {
  RelCatEntry relCatEntry;

  if (RelCacheTable::getRelCatEntry(relId, &relCatEntry) != SUCCESS) {
    return E_RELNOTOPEN;
  }

  int blockNum = relCatEntry.firstBlk;

  if (blockNum == -1) {
    blockNum = relId == RELCAT_RELID ? RELCAT_BLOCK : ATTRCAT_BLOCK;

    RecBuffer newBlock(blockNum);

    HeadInfo head;
    head.blockType = REC;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = relCatEntry.numAttrs;
    head.numSlots = relCatEntry.numSlotsPerBlk;

    newBlock.setHeader(&head);

    unsigned char slotMap[head.numSlots];
    for (int i = 0; i < head.numSlots; i++) {
      slotMap[i] = SLOT_UNOCCUPIED;
    }

    newBlock.setSlotMap(slotMap);

    relCatEntry.firstBlk = blockNum;
    relCatEntry.lastBlk = blockNum;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);
  }

  RecBuffer recBuffer(blockNum);

  HeadInfo head;
  recBuffer.getHeader(&head);

  unsigned char slotMap[head.numSlots];
  recBuffer.getSlotMap(slotMap);

  for (int slot = 0; slot < head.numSlots; slot++) {
    if (slotMap[slot] == SLOT_UNOCCUPIED) {
      recBuffer.setRecord(record, slot);

      slotMap[slot] = SLOT_OCCUPIED;
      recBuffer.setSlotMap(slotMap);

      head.numEntries++;
      recBuffer.setHeader(&head);

      relCatEntry.numRecs++;
      RelCacheTable::setRelCatEntry(relId, &relCatEntry);

      return SUCCESS;
    }
  }

  return E_DISKFULL;
}

int BlockAccess::renameRelation(char *oldName, char *newName) {
  Attribute oldVal;
  strcpy(oldVal.sVal, oldName);

  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  RecId recId = linearSearch(RELCAT_RELID,
                             (char *)RELCAT_ATTR_RELNAME,
                             oldVal,
                             EQ);

  if (recId.block == -1) {
    return E_RELNOTEXIST;
  }

  RecBuffer recBuffer(recId.block);
  Attribute record[RELCAT_NO_ATTRS];
  recBuffer.getRecord(record, recId.slot);

  strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
  recBuffer.setRecord(record, recId.slot);

  return SUCCESS;
}

int BlockAccess::renameAttribute(char *relName, char *oldName, char *newName) {
  Attribute relVal;
  strcpy(relVal.sVal, relName);

  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  while (true) {
    RecId recId = linearSearch(
        ATTRCAT_RELID,
        (char *)ATTRCAT_ATTR_RELNAME,
        relVal,
        EQ);

    if (recId.block == -1) {
      break;
    }

    RecBuffer recBuffer(recId.block);
    Attribute record[ATTRCAT_NO_ATTRS];
    recBuffer.getRecord(record, recId.slot);

    if (strcmp(record[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0) {
      strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
      recBuffer.setRecord(record, recId.slot);
      return SUCCESS;
    }
  }

  return E_ATTRNOTEXIST;
}

int BlockAccess::deleteRelation(char *relName) {
  Attribute attrVal;
  strcpy(attrVal.sVal, relName);

  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  RecId recId = linearSearch(RELCAT_RELID,
                             (char *)RELCAT_ATTR_RELNAME,
                             attrVal,
                             EQ);

  if (recId.block == -1) {
    return E_RELNOTEXIST;
  }

  RecBuffer relCatBuffer(recId.block);

  HeadInfo head;
  relCatBuffer.getHeader(&head);

  unsigned char slotMap[head.numSlots];
  relCatBuffer.getSlotMap(slotMap);

  // Delete relation entry from RELATIONCAT
  slotMap[recId.slot] = SLOT_UNOCCUPIED;
  relCatBuffer.setSlotMap(slotMap);

  // Delete all attributes from ATTRIBUTECAT
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  while (true) {
    RecId attrRecId = linearSearch(
        ATTRCAT_RELID,
        (char *)ATTRCAT_ATTR_RELNAME,
        attrVal,
        EQ);

    if (attrRecId.block == -1) {
      break;
    }

    RecBuffer attrCatBuffer(attrRecId.block);

    HeadInfo attrHead;
    attrCatBuffer.getHeader(&attrHead);

    unsigned char attrSlotMap[attrHead.numSlots];
    attrCatBuffer.getSlotMap(attrSlotMap);

    attrSlotMap[attrRecId.slot] = SLOT_UNOCCUPIED;
    attrCatBuffer.setSlotMap(attrSlotMap);
  }

  return SUCCESS;
}

int BlockAccess::project(int relId, Attribute *record) {
  RelCatEntry relCatEntry;

  if (RelCacheTable::getRelCatEntry(relId, &relCatEntry) != SUCCESS) {
    return E_RELNOTOPEN;
  }

  if (relCatEntry.firstBlk == -1) {
    return E_NOTFOUND;
  }

  RecBuffer recBuffer(relCatEntry.firstBlk);

  HeadInfo head;
  recBuffer.getHeader(&head);

  unsigned char slotMap[head.numSlots];
  recBuffer.getSlotMap(slotMap);

  for (int slot = 0; slot < head.numSlots; slot++) {
    if (slotMap[slot] == SLOT_OCCUPIED) {
      return recBuffer.getRecord(record, slot);
    }
  }

  return E_NOTFOUND;
}
