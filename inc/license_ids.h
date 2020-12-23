#ifndef __LICENSE_IDS_H
    #define __LICENSE_IDS_H


#define MAX_LICENSE_ID 64
#define MAX_LICENSE_TEXT 1024
#define LICENSE_COUNT 38


typedef struct normalized_license
{
  char spdx_id[MAX_LICENSE_ID];
  char text[MAX_LICENSE_TEXT];
  int ln;
} normalized_license;

normalized_license licenses[LICENSE_COUNT];
int load_licenses(void);

#endif