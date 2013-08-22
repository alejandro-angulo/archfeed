#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "get.h"
#include "parse.h"
#include "check.h"
#include "shared.h"

/* Help Message */
static void print_help (char *program) {
  printf("Usage: %s [OPTION]\n\n", program);
  printf("-b --brief\n");
  printf("\tdo not print news\n\n");
  printf("-c --color\n");
  printf("\tcolorize output\n");
  printf("\t  set when STDOUT detected as being a terminal\n\n");
  printf("-d --display [INTEGER]\n");
  printf("\tspecify number of entries to display (newest first)\n");
  printf("\t  use 0 to display all entries (default)\n");
  printf("\t  use -b if you do not want to display any entries\n\n");
  printf("-f --file [FILE]\n");
  printf("\tspecify download location\n");
  printf("\t  default location is ./news\n\n");
  printf("-l --local\n");
  printf("\tdo not download news (use local copy)\n\n");
  printf("-n --nocolor\n");
  printf("\tdo not colorize output\n");
  printf("\t  set when STDOUT is detected as not being a terminal\n\n");
  printf("-o --force\n");
  printf("\trun program without checking for new entries\n\n");
  printf("-p --poll\n");
  printf("\tissue header request and compare timestamps before proceeding\n");
  printf("\t  program exits if there are no new entries (default)\n\n");
  printf("-u --update\n");
  printf("\tdownload news (default)\n\n");
  printf("-v --verbose\n");
  printf("\tprint news (default)\n\n");
  printf("-h --help\n");
  printf("\tdisplay this message and exit\n\n");
  exit(1);
}

args flags; // initialize global struct to hold flags

int main (int argc, char *argv[]) {
  FILE *fp;
  
  // Assign default values for flags
  flags.entries = 0;
  flags.poll    = 1;
  flags.update  = 1;
  flags.verbose = 1;
  strcpy(flags.outfilename, "news");
  
  /* Command line arguments */
  // Check if output is a terminal
  // Set color flag accordingly
  if (isatty(1)) {  // 1 == stdout
    flags.color = 1;
  }
  else {
    flags.color = 0;
  }

  int opt_index;
  int opt;

  while (1) {
    struct option long_opts[] =
      {
        // {name, has_arg, flag, val};
        {"brief",   0, &flags.verbose,   0},  // -b 
        {"colorize",0, &flags.color  ,   1},  // -c
        {"display", 1, 0             , 'd'},  // -d
        {"file",    1, 0             , 'f'},  // -f
        {"force",   0, &flags.poll   ,   0},  // -o
        {"help",    0, 0             , 'h'},  // -h
        {"local",   0, &flags.update ,   0},  // -l
        {"poll",    0, &flags.poll   ,   1},  // -p
        {"nocolor", 0, &flags.color  ,   0},  // -n
        {"update",  0, &flags.update ,   1},  // -o
        {"verbose", 0, &flags.verbose,   1},  // -v
      };
    
    opt = getopt_long (argc, argv, "bcd:f:lnophvw", long_opts, &opt_index);

    if (opt == -1) {  // Done reading options.
      break;
    }

    switch (opt)
      {
        case 0:
          if (long_opts[opt_index].flag != 0) {
            break;
          }
          else {
            fprintf(stderr, "Failed to set %s flag.\n", long_opts[opt_index].name);
            exit(1);
          }
        case 'b': // --brief
          flags.verbose = 0;
          break;
        case 'c': // --color
          flags.color = 1;
          break;
        case 'd': // --display
          if (!(flags.entries =  (int) strtol(optarg, NULL, 10))) {
            fprintf(stderr, "Failed to read display argument: %s\n", optarg); 
            exit(1);
          }
          break;
        case 'f': // --file
          strcpy(flags.outfilename, optarg);
          break;
        case 'h': // --help
          print_help(argv[0]);
          break;
        case 'l': // --local
          flags.poll   = 0;
          flags.update = 0;
          break;
        case 'n': // --nocolor
          flags.color = 0;
          break;
        case 'o': // --force
          flags.poll = 0;
          break;
        case 'p': // --poll
          flags.poll = 1;
          break;
        case 'u': // --update
          flags.update = 1;
          break;
        case 'v': // --verbose
          flags.verbose = 1;
          break;
        case '?':
          print_help(argv[0]);
          break;
        default:
          abort();
      }
  }

  /* Run */
  if (flags.poll) {
    if(!check()) {
      printf("No new entries.\n");
      return 0;
    }
  }

  if (flags.update) {  
    fp = fopen(flags.outfilename, "w+b");
    if (fp == NULL) {
      perror("Error");
      printf("Failed to open <%s>.\n", flags.outfilename);
      exit(1);
    }
    if (access(flags.outfilename, W_OK) == -1) {
      perror("Error");
      printf("Writing to <%s> will fail.\n", flags.outfilename);
      exit(1);
    }
    download(fp);
    fclose(fp);
    printf("\n");
  }

  if (flags.verbose) {
    parse();
  }

  return 0;
}
