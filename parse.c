#include <string.h>
#include <sys/ioctl.h>
#include <libxml/xmlreader.h>
#include "shared.h"

#define MAX_LEN 40

#define NRM  "\x1B[0m"
#define BLK  "\x1B[30m"
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define BLU  "\x1B[34m"
#define MAG  "\x1B[35m"
#define CYN  "\x1B[36m"
#define WHT  "\x1B[37m"


/* Parse HTML in description */
static void parseHTML(char *value) {
  int i;
  char str[strlen(value)];
  strcpy(str, value);
  char *tok;
  char *ptr;
  char *link;

  for (i = 0; i < strlen(value); i++) {
    if (value[i] == '<') {  // Find HTML tags
      tok = strtok(str + i, "<>");
      if (!strcmp(tok, "/p")) {
        printf("\n");
      }
      else if (!strcmp(tok, "code") && flags.color) {
        fputs(GRN, stdout);
      }
      else if (!strcmp(tok, "/code") && flags.color) {
        fputs(NRM, stdout);
      }
      else if (!strcmp(tok, "li")) {
        fputs("\t", stdout);
      }
      else if ((ptr = strstr(tok, "/a"))) {
        if (link) {
          link[strlen(link) - 1] = '\0';
          if (flags.color) {
            printf(CYN " <%s> " NRM, link);
          }
          else {
            printf(" <%s> " , link);
          }
        }
      }
      else if ((link = strstr(tok, "http"))) {
        // Do nothing.
        // Only want to test for a link if the other tests failed
        // hence the "else if"
      }
      i += strlen(tok) + 1;
    }

    else if (value[i] == '&') { // Find HTML numbers
      if (strstr(value + i, "&gt;")) {
        printf(">");
      }
      else if (strstr(value + i, "&lt;")) {
        printf("<");
      }
      i += strlen(tok) - 1;
    }
    else {
      printf("%c", value[i]);
    }
  }
  fputs("\n", stdout);
}

/* Get information about the current entry. */
static void processEntry (xmlTextReaderPtr reader) {
  const xmlChar *name, *value;
  int  i;
  int  numNodes = 3;
  char nodes[numNodes][25];
  strcpy(nodes[0], "title");
  strcpy(nodes[1], "link");
  strcpy(nodes[2], "description");

  name = xmlTextReaderConstName(reader);
  if (name == NULL) {
    name = BAD_CAST "--";
  }

  for (i = 0; i < numNodes; i++) {
    while (strcmp((char *) name, nodes[i]) != 0) {
      if (!xmlTextReaderRead(reader)) {
        return;
      };
      name = xmlTextReaderConstName(reader);
    }
    xmlTextReaderRead(reader);
    value = xmlTextReaderConstValue(reader);
    if (!strcmp(nodes[i], nodes[0])) {
      if (flags.color) { 
        printf(YEL "%s\n", value);
      }
      else {
        printf("%s\n", value);
      }
    }
    else if (!strcmp(nodes[i], nodes[1])) {
      if (flags.color) {
      }
      else {
        printf("%s\n", value);
      }
    }
    else if (!strcmp(nodes[i], nodes[2])) {
      if (flags.color) {
        printf(NRM);
      }
      parseHTML((char *) value);
    } 
    xmlTextReaderRead(reader);

  }
}

/* Parse and print information about an XML file. */
static void streamFile () {
  xmlTextReaderPtr reader;
  int ret;
  unsigned short i = 0;  // entries counter

  reader = xmlReaderForFile(flags.outfilename, NULL, 0);
  if (reader != NULL) {
    ret = xmlTextReaderRead(reader);
    
    if (flags.entries) {  // there's probably a better way
      while (ret == 1 && i < flags.entries) {
        processEntry(reader);
        ret = xmlTextReaderRead(reader);
        i++;
      }
    }
    else {
      while (ret == 1) {
        processEntry(reader);
        ret = xmlTextReaderRead(reader);
      }
    }
    
    xmlFreeTextReader(reader);
    
    if (ret == -1) {  // if error encountered
      printf("%d\n", ret);
      fprintf(stderr, "%s : failed to parse\n", flags.outfilename);
    }
  }
  else {
    fprintf(stderr, "Unable to open %s\n", flags.outfilename);
  }
}

void parse () {
  streamFile();

  xmlCleanupParser();
  xmlMemoryDump();
}
