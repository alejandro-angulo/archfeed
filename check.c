// Needed for strptime
#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include "shared.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback (void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *) userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  
  // Out of Memory
  if (mem->memory == NULL) {
    fprintf(stderr, "Not enough memory (realloc failed).\n");
    exit(1);
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

/* Compares timestamps of /var/cache/pacman and the news feed    */
/* Returns zero if news feed is older than /var/cache/pacman     */
/* Returns non-zero if /var/cache/pacman is older than news feed */
static int comparison (char *memory) {
  char *mod_str;
  char *ptr;
  struct stat cache;
  struct tm news_mod;
  time_t news_time;

  mod_str = strstr(memory, "Last-Modified");
  ptr = strchr(mod_str, '\n');
  ptr[0] = '\0';
  strptime(mod_str, "Last-Modified: %a, %d %b %Y %H:%M:%S GMT ", &news_mod); 
  if ((news_time = mktime(&news_mod)) == -1) {
    perror("Error");
    exit(1);
  }

  if (stat("/var/cache/pacman", &cache) == -1) {
    perror("Error");
    exit(1);
  }

  if (news_time > cache.st_mtime) {
    return 1;
  }

  return 0;
}

/* Checks if a new entry was added since last update              */
/* Compares lastBuildDate entry to timestamp on /var/cache/pacman */
int check () {
  CURL *curl_handle;
  CURLcode res;

  int update = 0; // Set flag to indicate if an update was found

  struct MemoryStruct chunk;

  chunk.memory = malloc(1); // Will grow later
  chunk.size   = 0;         // No data yet

  curl_global_init(CURL_GLOBAL_ALL);

  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.archlinux.org/feeds/news/");
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) &chunk);
  // Make a header request
  curl_easy_setopt(curl_handle, CURLOPT_HEADER, 1);
  curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1);

  // Get
  res = curl_easy_perform(curl_handle);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed %s\n", curl_easy_strerror(res));
  }
  // Process data
  else {
    update = comparison(chunk.memory);
  }

  curl_easy_cleanup(curl_handle);
  if (chunk.memory) {
    free(chunk.memory);
  }

  return update;
}