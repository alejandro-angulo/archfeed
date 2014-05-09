#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <libxml/xmlreader.h>
#include <sys/ioctl.h>
#include "shared.h"

#define NRM  "\x1B[0m"
#define BLK  "\x1B[30m"
#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define BLU  "\x1B[34m"
#define MAG  "\x1B[35m"
#define CYN  "\x1B[36m"
#define WHT  "\x1B[37m"

static void printHTML    (char *value);
static void processEntry (xmlTextReaderPtr reader);
static void streamFile   ();
static void wrapper      (char *buf, char *text, int *buf_sz, int *buf_len,
                          unsigned short real_sz);
static void makeNull     (char *s, int len);

void parse () {
  streamFile();

  xmlCleanupParser();
  xmlMemoryDump();
}

static void makeNull (char *s, int len) {
  int i;

  for (i = 0; i < len; i++) {
    s[i] = '\0';
  }
}

/* Parse HTML in description */
static void printHTML (char *value) {
  int i;
  int j;
  char str[strlen(value)];
  strcpy(str, value);
  char *tok;
  char *ptr;
  char *link;
  
  struct winsize term;
  ioctl(1, TIOCGWINSZ, &term);    // 1 == stdout
  int  buf_sz = ++term.ws_col;  // '\0' accounted for
  int  buf_len = 0;
  char *buf = (char *) malloc(buf_sz);
  makeNull(buf, buf_sz);
  int  max_word_len = 30;
  char word[max_word_len];
  for (j = 0; j < max_word_len; j++) // null the string to avoid a bug
    word[j] = '\0';

  for (i = 0; i < strlen(value); i++) {
    if (value[i] == '<') {  // Find HTML tags
      tok = strtok(str + i, "<>");
      if (!strcmp(tok, "/p")) {
        printf("%s\n", buf);
        buf_len = 0;
        makeNull(buf, buf_sz);
      }

      else if (!strcmp(tok, "code") && flags.color) {
        if (flags.wrap)
          wrapper(buf, GRN, &buf_sz, &buf_len, term.ws_col);

        else 
          printf("%s", GRN);
      }

      else if (!strcmp(tok, "/code") && flags.color) {
        if (flags.wrap)
          wrapper(buf, NRM, &buf_sz, &buf_len, term.ws_col);

        else
         printf("%s", NRM);
      }
      
      else if (!strcmp(tok, "li"))
        printf("\t");
      
      else if ((ptr = strstr(tok, "/a")) && link) {
        if (flags.wrap && flags.color) {
          wrapper(buf, CYN, &buf_sz, &buf_len, term.ws_col);
          wrapper(buf, " <", &buf_sz, &buf_len, term.ws_col);
          wrapper(buf, link, &buf_sz, &buf_len, term.ws_col);
          wrapper(buf, ">", &buf_sz, &buf_len, term.ws_col);
          wrapper(buf, NRM, &buf_sz, &buf_len, term.ws_col);
        }

        else if (flags.color)
          printf(" %s<%s>%s", CYN, link, NRM);
        
        else
          printf(" <%s> " , link);
      }
      else if ((link = strstr(tok, "http"))) {
        link[strlen(link) - 1] = '\0';  // remove ending quotation mark
      }
      i += strlen(tok) + 1;
    }

    else if (value[i] == '&') { // Find HTML numbers
      if (strstr(value + i, "&gt;"))
        printf(">");

      else if (strstr(value + i, "&lt;"))
        printf("<");

      i += strlen(tok) - 1;
    }

    else {
      if (value[i] == '>') continue;  // ignore end of HTML tag

      if (flags.wrap) { // find word
        while (!ispunct(value[i]) && !isspace(value[i])) {
          word[strlen(word)] = value[i];
          word[strlen(word) + 1] = '\0';
          i++;
        }

        // This if-else prevents accidentally printing HTML tags
        if (value[i] == '<') i--;
        else
          word[strlen(word)] = value[i];

        wrapper(buf, word, &buf_sz, &buf_len, term.ws_col);
        
        // reset word string so it can be reused
        makeNull(word, max_word_len);
      }

      else {
        printf("%c", value[i]);
        if (value[i] == '\n') { // extraneous if?
          buf_len = 0;
          makeNull(buf, buf_sz);
        }
      }
    }
  }

  printf("%s\n", buf);
  free(buf);
}

// REWRITE COMMENT
/* Wraps output without splitting words                                    */
/* buf_sz is the maximum length of buf and buf_len is the current length   */
/* of buf. incr specifies whether buf_len should be incremented (should be */
/* used when text is something that is not printed, such as a color code). */
/* If incr is zero, buf_sz is reallocated to make space for the            */
/* non-printed string. One is returned when the function prints, and zero  */
/* is returned otherwise. If one is returned, buf_sz should be reset.      */
/* IS REALLOCATING NECESSARY? */
/* RESETTING buf_sz SHOULD HAPPEN HERE */
static void wrapper (char *buf, char *text, int *buf_sz, int *buf_len,
                    unsigned short real_sz) {
  int i;
  int j;

  *buf_len += strlen(text);

  if (*buf_len < *buf_sz) {
    for (i = *buf_len - strlen(text), j = 0; i < *buf_len; i++, j++)
      buf[i] = text[j];
    buf[i] = '\0';
  }

  else {
    printf("%s\n", buf);
    *buf_len = strlen(text);
    strcpy(buf, text);
    *buf_sz = *buf_sz;
    buf = (char *) realloc(buf, *buf_sz);
  }
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
      if (!xmlTextReaderRead(reader))
        return;

      name = xmlTextReaderConstName(reader);
    }
    xmlTextReaderRead(reader);
    value = xmlTextReaderConstValue(reader);
    if (!strcmp(nodes[i], nodes[0])) {
      if (flags.color)
        printf(YEL "%s\n", value);
      
      else
        printf("%s\n", value);
    }
    else if (!strcmp(nodes[i], nodes[1])) {
      if (flags.color)
        printf("%s%s%s\n", CYN, value, NRM);
      
      else
        printf("%s\n", value);
    }
    else if (!strcmp(nodes[i], nodes[2])) {
      if (flags.color)
        printf("%s", NRM);

      printHTML((char *) value);
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
    printf("%s\n", flags.outfilename);
    fprintf(stderr, "Unable to open %s\n", flags.outfilename);
  }
}
