#ifndef IRD_H
#define IRD_H

#define BUFF_SIZE 4096
#define PATH_LEN 4096
#define TMP_PATH "/tmp/cIRD"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <stdint.h>
#include <stdio.h>

typedef enum {

    CRD_OK,
    CRD_FAIL,
    CRD_ERROR,

    CRD_ERR_ARGS,
    CRD_ERR_FILE,
    CRD_ERR_GZ,

	CRD_ERR_STD,
	CRD_ERR_VER,

	CRD_ERR_PATH,

} crd_response_t;

typedef struct {
    uint8_t hash            [16];

} region_hash_t;

typedef struct {
    uint8_t sector          [8];
    uint8_t hash            [16];

} file_hash_t;

typedef struct {
	uint8_t magic 		[4];
    uint8_t version 	[1];
    uint8_t title_id 	[9];

} ird_top_t;

typedef struct {
	uint8_t sys_ver 	[4];
	uint8_t disc_ver 	[5];
	uint8_t app_ver 	[5];

} ird_middle_t;

typedef struct {
	uint8_t padding		[4];
	uint8_t pic 		[115];
	uint8_t data1	 	[16];
	uint8_t data2		[16];
	uint8_t uid			[4];
	uint8_t crc			[4];

} ird_bottom_t;

typedef struct {

	uint8_t title_length;
	char *title;

	char title_id[9];
	char sys_ver[4];
	char disc_ver[5];
	char app_ver[5];

	uint8_t pic[0x73];
	uint8_t data1[0x10];
	uint8_t data2[0x10];

	uint32_t uid;
	uint32_t crc;

	uint8_t region_count;
	uint32_t file_count;
	region_hash_t *region_hashes;
	file_hash_t *file_hashes;

	char *header_path;
	char *footer_path;
    FILE *header;
    FILE *footer;

    char *tmp_path;

} ird_t;

#endif
