#include "Algebra.h"

#include <cstring>
#include <iostream>

using namespace std;

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE],
                    char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
  int relId = OpenRelTable::openRel(srcRel);

  if (relId < 0) {
    cout << "Relation not open: " << srcRel << endl;
    return relId;
  }

  AttrCatEntry attrCatEntry;
  if (AttrCacheTable::getAttrCatEntry(relId, attr, &attrCatEntry) != SUCCESS) {
    cout << "Attribute not found: " << attr << endl;
    return E_ATTRNOTEXIST;
  }

  Attribute attrVal;

  if (attrCatEntry.attrType == NUMBER) {
    attrVal.nVal = atof(strVal);
  } else {
    strcpy(attrVal.sVal, strVal);
  }

  RelCacheTable::resetSearchIndex(relId);

  RelCatEntry relCatEntry;
  RelCacheTable::getRelCatEntry(relId, &relCatEntry);

  Attribute record[relCatEntry.numAttrs];

  while (BlockAccess::search(relId, record, attr, attrVal, op) == SUCCESS) {
    for (int i = 0; i < relCatEntry.numAttrs; i++) {
      AttrCatEntry attrInfo;
      AttrCacheTable::getAttrCatEntry(relId, i, &attrInfo);

      if (attrInfo.attrType == NUMBER) {
        cout << record[i].nVal << " ";
      } else {
        cout << record[i].sVal << " ";
      }
    }
    cout << endl;
  }

  return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int numberOfAttributes, char record[][ATTR_SIZE]) {
  return FAILURE;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
  return FAILURE;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE],
                     int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
  return FAILURE;
}

int Algebra::join(char srcRelOne[ATTR_SIZE], char srcRelTwo[ATTR_SIZE],
                  char targetRel[ATTR_SIZE], char attrOne[ATTR_SIZE], char attrTwo[ATTR_SIZE]) {
  int relId1 = OpenRelTable::openRel(srcRelOne);
  int relId2 = OpenRelTable::openRel(srcRelTwo);

  if (relId1 < 0) return relId1;
  if (relId2 < 0) return relId2;

  RelCatEntry relCat1, relCat2;
  RelCacheTable::getRelCatEntry(relId1, &relCat1);
  RelCacheTable::getRelCatEntry(relId2, &relCat2);

  AttrCatEntry attrCat1, attrCat2;
  AttrCacheTable::getAttrCatEntry(relId1, attrOne, &attrCat1);
  AttrCacheTable::getAttrCatEntry(relId2, attrTwo, &attrCat2);

  RecBuffer recBuffer(relCat1.firstBlk);

  HeadInfo head;
  recBuffer.getHeader(&head);

  unsigned char slotMap[head.numSlots];
  recBuffer.getSlotMap(slotMap);

  for (int slot = 0; slot < head.numSlots; slot++) {
    if (slotMap[slot] == SLOT_OCCUPIED) {
      Attribute record1[relCat1.numAttrs];
      recBuffer.getRecord(record1, slot);

      Attribute searchVal = record1[attrCat1.offset];

      RelCacheTable::resetSearchIndex(relId2);

      Attribute record2[relCat2.numAttrs];

      while (BlockAccess::search(relId2, record2, attrTwo, searchVal, EQ) == SUCCESS) {
        for (int i = 0; i < relCat1.numAttrs; i++) {
          AttrCatEntry info;
          AttrCacheTable::getAttrCatEntry(relId1, i, &info);

          if (info.attrType == NUMBER)
            cout << record1[i].nVal << " ";
          else
            cout << record1[i].sVal << " ";
        }

        cout << " | ";

        for (int j = 0; j < relCat2.numAttrs; j++) {
          AttrCatEntry info;
          AttrCacheTable::getAttrCatEntry(relId2, j, &info);

          if (info.attrType == NUMBER)
            cout << record2[j].nVal << " ";
          else
            cout << record2[j].sVal << " ";
        }

        cout << endl;
      }
    }
  }

  return SUCCESS;
}
