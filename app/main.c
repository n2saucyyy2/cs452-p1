#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <bits/getopt_core.h>
#include "../src/lab.h"

int main(int argc, char *argv[])
{
  //printf("hello world\n");

  int c;

  while((c = getopt(argc, argv, "abc:v")) != -1){
    switch(c)
{
    case 'v':
      printf("Version %02d.%02d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
      exit(0);
    case 'a':
     printf("get a here");
      break;

    case 'b':
      printf("get b here");
      break;

    case 'c':
      printf("get c here");
      break;

    // case '?':
    //   if(isprint(getopt))
    //     fprintf(stderr, "Unknown Option", optopt);
    //   else
    //     fprintf(stderr, "Unkown Option", optopt);
    //   return 1;

    default:
        exit(1);
   }
  }
}