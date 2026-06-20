#include "AttrCacheTable.h"

#include <cstring>

AttrCacheEntry *AttrCacheTable::attrCache[MAX_OPEN];

void AttrCacheTable::recordToAttrCatEntry(Attribute record[ATTRCAT_NO_ATTRS],
                                          AttrCatEntry *attrCatEntry) {
  strcpy(attrCatEntry->relName, record[ATTRCAT_REL_NAME_INDEX].sVal);
  strcpy(attrCatEntry->attrName, record[ATTRCAT_ATTR_NAME_INDEX].sVal);
  attrCatEntry->attrType = (int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;
  attrCatEntry->primaryFlag = (bool)record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal;
  attrCatEntry->rootBlock = (int)record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
  attrCatEntry->offset = (int)record[ATTRCAT_OFFSET_INDEX].nVal;
}

void AttrCacheTable::attrCatEntryToRecord(AttrCatEntry *attrCatEntry,
                                          Attribute record[ATTRCAT_NO_ATTRS]) {
  strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, attrCatEntry->relName);
  strcpy(record[ATTRCAT_ATTR_NAME_INDEX].sVal, attrCatEntry->attrName);
  record[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrCatEntry->attrType;
  record[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = attrCatEntry->primaryFlag;
  record[ATTRCAT_ROOT_BLOCK_INDEX].nVal = attrCatEntry->rootBlock;
  record[ATTRCAT_OFFSET_INDEX].nVal = attrCatEntry->offset;
}

int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE],
                                    AttrCatEntry *attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN || attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (strcmp(entry->attrCatEntry.attrName, attrName) == 0) {
      *attrCatBuf = entry->attrCatEntry;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::getAttrCatEntry(int relId, int attrOffset,
                                    AttrCatEntry *attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN || attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (entry->attrCatEntry.offset == attrOffset) {
      *attrCatBuf = entry->attrCatEntry;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, char attrName[ATTR_SIZE],
                                    AttrCatEntry *attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN || attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (strcmp(entry->attrCatEntry.attrName, attrName) == 0) {
      entry->attrCatEntry = *attrCatBuf;
      entry->dirty = true;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setAttrCatEntry(int relId, int attrOffset,
                                    AttrCatEntry *attrCatBuf) {
  if (relId < 0 || relId >= MAX_OPEN || attrCache[relId] == nullptr) {
    return E_RELNOTOPEN;
  }

  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (entry->attrCatEntry.offset == attrOffset) {
      entry->attrCatEntry = *attrCatBuf;
      entry->dirty = true;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, char attrName[ATTR_SIZE],
                                   IndexId *searchIndex) {
  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (strcmp(entry->attrCatEntry.attrName, attrName) == 0) {
      *searchIndex = entry->searchIndex;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::getSearchIndex(int relId, int attrOffset,
                                   IndexId *searchIndex) {
  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (entry->attrCatEntry.offset == attrOffset) {
      *searchIndex = entry->searchIndex;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, char attrName[ATTR_SIZE],
                                   IndexId *searchIndex) {
  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (strcmp(entry->attrCatEntry.attrName, attrName) == 0) {
      entry->searchIndex = *searchIndex;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::setSearchIndex(int relId, int attrOffset,
                                   IndexId *searchIndex) {
  AttrCacheEntry *entry = attrCache[relId];

  while (entry != nullptr) {
    if (entry->attrCatEntry.offset == attrOffset) {
      entry->searchIndex = *searchIndex;
      return SUCCESS;
    }
    entry = entry->next;
  }

  return E_ATTRNOTEXIST;
}

int AttrCacheTable::resetSearchIndex(int relId, char attrName[ATTR_SIZE]) {
  IndexId searchIndex;
  searchIndex.block = -1;
  searchIndex.index = -1;

  return setSearchIndex(relId, attrName, &searchIndex);
}

int AttrCacheTable::resetSearchIndex(int relId, int attrOffset) {
  IndexId searchIndex;
  searchIndex.block = -1;
  searchIndex.index = -1;

  return setSearchIndex(relId, attrOffset, &searchIndex);
}
