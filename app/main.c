#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <bits/getopt_core.h>
#include <lab.h>

int main(int argc, char *argv[])
{
  printf("hello world\n");

  int c;

  while((c = getopt(argc, argv, "abc:")) != -1)
    switch(c)
  {
    case 'a':
     printf("get a here");
      break;

    case 'b':
      printf("get b here");
      break;

    case 'c':
      printf("get c here");
      break;

    case '?':
      if(isprint(getopt))
        fprintf(stderr, "Unknown Option", optopt);
      else
        fprintf(stderr, "Unkown Option", optopt);
      return 1;

    default:
      abort();
  }
}