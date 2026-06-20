#include "RelCacheTable.h"

#include <cstring>

RelCacheEntry *RelCacheTable::relCache[MAX_OPEN];

void RelCacheTable::recordToRelCatEntry(Attribute record[RELCAT_NO_ATTRS],
                                        RelCatEntry *relCatEntry) {
  strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
  relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
  relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
  relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
  relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
  relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}

void RelCacheTable::relCatEntryToRecord(RelCatEntry *relCatEntry,
                                        Attribute record[RELCAT_NO_ATTRS]) {
  strcpy(record[RELCAT_REL_NAME_INDEX].sVal, relCatEntry->relName);
  record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = relCatEntry->numAttrs;
  record[RELCAT_NO_RECORDS_INDEX].nVal = relCatEntry->numRecs;
  record[RELCAT_FIRST_BLOCK_INDEX].nVal = relCatEntry->firstBlk;
  record[RELCAT_LAST_BLOCK_INDEX].nVal = relCatEntry->lastBlk;
  record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = relCatEntry->numSlotsPerBlk;
}

int RelCacheTable::getRelCatEntry(int relId, RelCatEntry *relCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN || relCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  *relCatBuf = relCache[relId]->relCatEntry;
  return SUCCESS;
}

int RelCacheTable::setRelCatEntry(int relId, RelCatEntry *relCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN || relCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  relCache[relId]->relCatEntry = *relCatBuf;
  relCache[relId]->dirty = true;

  return SUCCESS;
}

int RelCacheTable::getSearchIndex(int relId, RecId *searchIndex) {
  if (relId < 0 || relId >= MAX_OPEN || relCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  *searchIndex = relCache[relId]->searchIndex;
  return SUCCESS;
}

int RelCacheTable::setSearchIndex(int relId, RecId *searchIndex) {
  if (relId < 0 || relId >= MAX_OPEN || relCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  relCache[relId]->searchIndex = *searchIndex;
  return SUCCESS;
}

int RelCacheTable::resetSearchIndex(int relId) {
  if (relId < 0 || relId >= MAX_OPEN || relCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  relCache[relId]->searchIndex.block = -1;
  relCache[relId]->searchIndex.slot = -1;

  return SUCCESS;
}
