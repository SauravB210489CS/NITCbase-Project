#include "OpenRelTable.h"

#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {
  for (int i = 0; i < MAX_OPEN; i++) {
    tableMetaInfo[i].free = true;
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);

  tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);

  RecBuffer relCatBuffer(RELCAT_BLOCK);

  Attribute relRecord[RELCAT_NO_ATTRS];

  RelCacheTable::relCache[RELCAT_RELID] = new RelCacheEntry;
  relCatBuffer.getRecord(relRecord, RELCAT_SLOTNUM_FOR_RELCAT);
  RelCacheTable::recordToRelCatEntry(relRecord, &RelCacheTable::relCache[RELCAT_RELID]->relCatEntry);
  RelCacheTable::relCache[RELCAT_RELID]->dirty = false;
  RelCacheTable::relCache[RELCAT_RELID]->recId.block = RELCAT_BLOCK;
  RelCacheTable::relCache[RELCAT_RELID]->recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
  RelCacheTable::relCache[RELCAT_RELID]->searchIndex.block = -1;
  RelCacheTable::relCache[RELCAT_RELID]->searchIndex.slot = -1;

  RelCacheTable::relCache[ATTRCAT_RELID] = new RelCacheEntry;
  relCatBuffer.getRecord(relRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relRecord, &RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry);
  RelCacheTable::relCache[ATTRCAT_RELID]->dirty = false;
  RelCacheTable::relCache[ATTRCAT_RELID]->recId.block = RELCAT_BLOCK;
  RelCacheTable::relCache[ATTRCAT_RELID]->recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
  RelCacheTable::relCache[ATTRCAT_RELID]->searchIndex.block = -1;
  RelCacheTable::relCache[ATTRCAT_RELID]->searchIndex.slot = -1;

  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
  Attribute attrRecord[ATTRCAT_NO_ATTRS];

  for (int slot = 0; slot < 12; slot++) {
    attrCatBuffer.getRecord(attrRecord, slot);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::recordToAttrCatEntry(attrRecord, &attrCatEntry);

    int relId = -1;

    if (strcmp(attrCatEntry.relName, RELCAT_RELNAME) == 0) {
      relId = RELCAT_RELID;
    } else if (strcmp(attrCatEntry.relName, ATTRCAT_RELNAME) == 0) {
      relId = ATTRCAT_RELID;
    }

    if (relId != -1) {
      AttrCacheEntry *newEntry = new AttrCacheEntry;
      newEntry->attrCatEntry = attrCatEntry;
      newEntry->dirty = false;
      newEntry->recId.block = ATTRCAT_BLOCK;
      newEntry->recId.slot = slot;
      newEntry->searchIndex.block = -1;
      newEntry->searchIndex.index = -1;
      newEntry->next = AttrCacheTable::attrCache[relId];

      AttrCacheTable::attrCache[relId] = newEntry;
    }
  }
}

OpenRelTable::~OpenRelTable() {
  for (int relId = 0; relId < MAX_OPEN; relId++) {
    if (RelCacheTable::relCache[relId] != nullptr) {
      if (RelCacheTable::relCache[relId]->dirty) {
        Attribute record[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[relId]->relCatEntry, record);

        RecBuffer recBuffer(RelCacheTable::relCache[relId]->recId.block);
        recBuffer.setRecord(record, RelCacheTable::relCache[relId]->recId.slot);
      }

      delete RelCacheTable::relCache[relId];
      RelCacheTable::relCache[relId] = nullptr;
    }

    AttrCacheEntry *entry = AttrCacheTable::attrCache[relId];

    while (entry != nullptr) {
      if (entry->dirty) {
        Attribute record[ATTRCAT_NO_ATTRS];
        AttrCacheTable::attrCatEntryToRecord(&entry->attrCatEntry, record);

        RecBuffer recBuffer(entry->recId.block);
        recBuffer.setRecord(record, entry->recId.slot);
      }

      AttrCacheEntry *next = entry->next;
      delete entry;
      entry = next;
    }

    AttrCacheTable::attrCache[relId] = nullptr;
  }
}

int OpenRelTable::getFreeOpenRelTableEntry() {
  for (int i = 0; i < MAX_OPEN; i++) {
    if (tableMetaInfo[i].free) {
      return i;
    }
  }

  return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
  for (int i = 0; i < MAX_OPEN; i++) {
    if (!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName) == 0) {
      return i;
    }
  }

  return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
  int relId = getRelId(relName);

  if (relId >= 0) {
    return relId;
  }

  int freeRelId = getFreeOpenRelTableEntry();
  if (freeRelId < 0) {
    return freeRelId;
  }

  int relCatRelId = RELCAT_RELID;

  Attribute record[RELCAT_NO_ATTRS];
  Attribute attrVal;
  strcpy(attrVal.sVal, relName);

  RelCacheTable::resetSearchIndex(relCatRelId);

  int ret = BlockAccess::search(relCatRelId, record,
                                (char *)RELCAT_ATTR_RELNAME,
                                attrVal, EQ);

  if (ret != SUCCESS) {
    return E_RELNOTEXIST;
  }

  tableMetaInfo[freeRelId].free = false;
  strcpy(tableMetaInfo[freeRelId].relName, relName);

  RelCacheTable::relCache[freeRelId] = new RelCacheEntry;
  RelCacheTable::recordToRelCatEntry(
      record,
      &RelCacheTable::relCache[freeRelId]->relCatEntry);

  RelCacheTable::relCache[freeRelId]->dirty = false;

  return freeRelId;
}

int OpenRelTable::closeRel(int relId) {
  if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >= MAX_OPEN || tableMetaInfo[relId].free) {
    return E_RELNOTOPEN;
  }

  delete RelCacheTable::relCache[relId];
  RelCacheTable::relCache[relId] = nullptr;

  tableMetaInfo[relId].free = true;

  return SUCCESS;
}
