#include <math.h>
#include <unistd.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <sys/ioctl.h>
#include "shared.h"

#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL 3

struct myprogress {
  double lastruntime;
  CURL *curl;
};

static int xferinfo       (void *ptr, curl_off_t dltotal, curl_off_t dlnow,
                           curl_off_t ultotal, curl_off_t ulnow);
static int older_progress (void *ptr, double dltotal, double dlnow,
                           double ultotal, double ulnow);
static size_t write_data  (void *ptr, size_t size, size_t nmemb, FILE *stream);

/* Initializes libcurl settings and calls functions neccesary to download file */
void download (const FILE *fp) {
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct myprogress prog;
  char *url = "https://www.archlinux.org/feeds/news/";
  curl = curl_easy_init();
  
  if (curl) {
    fputs("Retreiving news...\n", stdout);

    prog.lastruntime = 0;
    prog.curl = curl;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &prog);
    if (isatty(1)) {  // Only show progress if stdout is a terminal
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, older_progress);

      #if LIBCURL_VERSION_NUM >= 0x072000
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
      #endif

      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
    curl_easy_setopt(curl ,CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    
    if ((res = curl_easy_perform(curl)))
      printf("%s\n", curl_easy_strerror(res));
    else
      curl_easy_cleanup(curl);

  }
  else {
    printf("Failed to begin libcurl session.\n");
  }
}

/* Transeferinfo  function for libcurl */
/* Used for progress bar               */
static int xferinfo (void *ptr,
                     curl_off_t dltotal, curl_off_t dlnow,
                     curl_off_t ultotal, curl_off_t ulnow) {
  // Get terminal length
  struct winsize term;
  ioctl(1, TIOCGWINSZ, &term);  // 1 == stdout

  // Subtract 7 from term.ws_col to account for
  // the space surrounding the loading bar ("xxx% [" and the ending "]")
  int    bar_len = term.ws_col - 7;
  int    bar_full;
  int    i;
  double frac_down;


  if (dlnow == 0) {
    frac_down = 0;
    bar_full  = 0;
  }
  else {
    frac_down = (double) dlnow / (double) dltotal;
    bar_full = round(frac_down * bar_len);
  }

  printf("%3.0f%% [", frac_down * 100);

  for (i = 0; i < bar_full; i++)
    printf("=");

  for ( ; i < bar_len; i++)
    printf(" ");

  printf("]\r");
  fflush(stdout);

  return 0;
}

/* Transeferinfo function for older versions of libcurl */
/* Casts arguments to correct type for xferinfo (above) */
static int older_progress (void *ptr,
                           double dltotal, double dlnow,
                           double ultotal, double ulnow) {
  return xferinfo(ptr,
                  (curl_off_t) dltotal,
                  (curl_off_t) dlnow,
                  (curl_off_t) ultotal,
                  (curl_off_t) ulnow);
}

static size_t write_data (void *ptr, size_t size, size_t nmemb, FILE *stream) {
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
}

