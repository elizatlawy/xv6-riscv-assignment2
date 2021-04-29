#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  int i;

  if(argc < 3){
    fprintf(2,  "usage: kill<pid><signum>\n");
    exit(1);
  }
  for(i=1; i<argc; i = i + 2){
      kill(atoi(argv[i]),atoi(argv[i+1]));
  }
  exit(0);
}
