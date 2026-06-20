#include "Schema.h"

#include <cstring>

int Schema::createRel(char relName[], int numOfAttributes,
                      char attrNames[][ATTR_SIZE], int attrType[]) {
  Attribute relRecord[RELCAT_NO_ATTRS];

  strcpy(relRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
  relRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = numOfAttributes;
  relRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
  relRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
  relRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
  relRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal =
      (BLOCK_SIZE - HEADER_SIZE) / (numOfAttributes * ATTR_SIZE + 1);

  int ret = BlockAccess::insert(RELCAT_RELID, relRecord);
  if (ret != SUCCESS) {
    return ret;
  }

  for (int i = 0; i < numOfAttributes; i++) {
    Attribute attrRecord[ATTRCAT_NO_ATTRS];

    strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
    strcpy(attrRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrNames[i]);
    attrRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrType[i];
    attrRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = 0;
    attrRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
    attrRecord[ATTRCAT_OFFSET_INDEX].nVal = i;

    ret = BlockAccess::insert(ATTRCAT_RELID, attrRecord);
    if (ret != SUCCESS) {
      return ret;
    }
  }

  return SUCCESS;
}

int Schema::deleteRel(char relName[ATTR_SIZE]) {
  return BlockAccess::deleteRelation(relName);
}

int Schema::createIndex(char relName[ATTR_SIZE], char attrName[ATTR_SIZE]) {
  return FAILURE;
}

int Schema::dropIndex(char relName[ATTR_SIZE], char attrName[ATTR_SIZE]) {
  return FAILURE;
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
  return BlockAccess::renameRelation(oldRelName, newRelName);
}

int Schema::renameAttr(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE],
                       char newAttrName[ATTR_SIZE]) {
  return BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
}

int Schema::openRel(char relName[ATTR_SIZE]) {
  return OpenRelTable::openRel(relName);
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  int relId = OpenRelTable::getRelId(relName);
  return OpenRelTable::closeRel(relId);
}
