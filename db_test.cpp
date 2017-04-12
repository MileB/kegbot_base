#include <iostream>
#include "db_access.h"

int main(int argc, char** argv)
{
  printf("Starting..\n");
  db_access db("./kegbot.cfg");
  db.print();
  db.archive();
  return 0;
}
