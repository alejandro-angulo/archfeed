#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include "get.h"
#include "parse.h"
#include "shared.h"

static void print_help (char *program) {
  printf("Usage: %s [OPTION]\n", program);
  printf("-b --brief\n");
  printf("\tdo not print news\n\n");
  printf("-d --display [INTEGER]\n");
  printf("\tspecify number of entries to display (newest first)\n");
  printf("\t  use 0 to display all entries (default)\n");
  printf("\t  use -b if you do not want to display any entries\n\n");
  printf("-f --file [FILE]\n");
  printf("\tspecify download location\n");
  printf("\t  default location is ./news\n\n");
  printf("-l --local\n");
  printf("\tdo not download news (use local copy)\n\n");
  printf("-o --online\n");
  printf("\tdownload news (default)\n\n");
  printf("-v --verbose\n");
  printf("\tprint news (default)\n\n");
  printf("-h --help\n");
  printf("\tdisplay this message and exit\n\n");
  exit(1);
}

// Terminal dimensions
struct winsize term;

int main (int argc, char *argv[]) {
  FILE *fp;
  char outfilename[255] = "news";
  
  // Command line arguments
  int verbose = 1;
  int online  = 1;
  unsigned short entries = 0;

  int opt_index;
  int opt;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &term);

  while (1) {
    struct option long_opts[] =
      {
        // {name, has_arg, flag, val};
        {"brief",   0, &verbose,   0},  // -b 
        {"display", 1, 0,        'd'},  // -d
        {"file",    1, 0       , 'f'},  // -f
        {"help",    0, 0       , 'h'},  // -h
        {"local",   0, &online ,   0},  // -l
        {"online",  0, &online ,   1},  // -o
        {"verbose", 0, &verbose,   1},  // -v
      };
    
    opt = getopt_long (argc, argv, "bd:f:lnohvw", long_opts, &opt_index);

    if (opt == -1) {  // Done reading options.
      break;
    }

    switch (opt)
      {
        case 0: // flag
          if (long_opts[opt_index].flag != 0) {
            break;
          }
          else {
            fprintf(stderr, "Failed to set %s flag.\n", long_opts[opt_index].name);
            exit(1);
          }
        case 'b': // --brief
          verbose = 0;
          break;
        case 'd': // --display
          entries = (unsigned short) strtoul(optarg, NULL, 0);
          break;
        case 'f': // --file
          strcpy(outfilename, optarg);
          break;
        case 'h': // --help
          print_help(argv[0]);
          break;
        case 'l': // --local
          online = 0;
          break;
        case 'o': // --online
          online = 1;
          break;
        case 'v': // --verbose
          verbose = 1;
          break;
        case '?':
          print_help(argv[0]);
          break;
        default:
          abort();
      }
  }

  // Run
  if (online) {  
    fp = fopen(outfilename, "w+b");
    if (fp == NULL) {
      perror("Error");
      printf("Failed to open <%s>.\n", outfilename);
      exit(1);
    }
    if (access(outfilename, W_OK) == -1) {
      perror("Error");
      printf("Writing to <%s> will fail.\n", outfilename);
      exit(1);
    }

    download(fp, outfilename);
    fclose(fp);
    printf("\n");
  }
  if (verbose) {
    parse(outfilename, entries);
  }
  return 0;
}
