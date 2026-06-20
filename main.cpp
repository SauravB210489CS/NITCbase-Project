#include <iostream>
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "Algebra/Algebra.h"

int main() {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  int status = Algebra::join(
      (char*)"RELATIONCAT",
      (char*)"ATTRIBUTECAT",
      (char*)"temp",
      (char*)"RelName",
      (char*)"RelName"
  );

  std::cout << "Join Status: " << status << std::endl;

  return 0;
}
