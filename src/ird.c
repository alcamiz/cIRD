#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#include <string.h>
#include <zlib.h>

#include "ird.h"

/**** Little-Endian Parsers ****/

static
uint16_t LE_int16(uint8_t *iso_num) {
    return (((uint16_t) (iso_num[0] & 0xff))
         | (((uint16_t) (iso_num[1] & 0xff)) << 8));
}

static
uint32_t LE_int32(uint8_t *iso_num) {
    return (((uint32_t) (LE_int16(iso_num)))
         | (((uint32_t) (LE_int16(iso_num + 2))) << 16));
}

static
uint64_t LE_int64(uint8_t *iso_num) {
    return (((uint64_t) (LE_int32(iso_num)))
         | (((uint64_t) (LE_int32(iso_num + 4))) << 32));
}


/**** Wrapper Functions ****/

static
crd_response_t handle_alloc(void **mem, size_t count, size_t size, bool clear) {

    crd_response_t ret_val;
    void *in_mem;

    if (clear) {
        in_mem = calloc(count, size);
    } else {
        in_mem = malloc(count * size);
    }

    if (in_mem == NULL) {
        ret_val = CRD_ERROR;
        goto exit_normal;
    }

    *mem = in_mem;
    ret_val = CRD_OK;

    exit_normal:
        return ret_val;
}

static
crd_response_t handle_fopen(FILE **file, char *filename, char *mode) {

    crd_response_t ret_val;

    *file = fopen(filename, mode);

    if (file == NULL) {
        ret_val = CRD_ERR_FILE;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_fclose(FILE *file) {

    crd_response_t ret_val;
    int close_ret;

    close_ret = fclose(file);
    if (close_ret != 0) {
        ret_val = CRD_ERR_FILE;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_fseek(FILE *file, off_t loc, int whence) {

    crd_response_t ret_val;
    int seek_ret;

    seek_ret = fseeko(file, loc, whence);
    if (seek_ret != 0) {
        ret_val = CRD_ERR_FILE;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_fread(void *buf, size_t size, size_t count, FILE *file) {

    crd_response_t ret_val;
    size_t read_ret;

    read_ret = fread(buf, size, count, file);
    if (read_ret < 0) {
        ret_val = CRD_ERR_FILE;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_fwrite(void *buf, size_t size, size_t count, FILE *file) {

    crd_response_t ret_val;
    size_t read_ret;

    read_ret = fwrite(buf, size, count, file);
    if (read_ret < 0) {
        ret_val = CRD_ERR_FILE;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_gzopen(gzFile *file, char *filename, char *mode) {

    crd_response_t ret_val;

    *file = gzopen(filename, mode);

    if (file == NULL) {
        ret_val = CRD_ERR_GZ;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_gzclose(gzFile file) {

    crd_response_t ret_val;
    int close_ret;

    close_ret = gzclose(file);
    if (close_ret != 0) {
        ret_val = CRD_ERR_GZ;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_gzseek(gzFile file, long loc, int whence) {

    crd_response_t ret_val;
    long seek_ret;

    seek_ret = gzseek(file, loc, whence);
    if (seek_ret != 0) {
        ret_val = CRD_ERR_GZ;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t handle_gzread(void *buf, size_t size, size_t count, gzFile *file) {

    crd_response_t ret_val;
    int read_ret;

    read_ret = gzread(file, buf, size * count);
    if (read_ret < 1) {
        ret_val = CRD_ERR_GZ;
        goto exit_normal;
    }

    ret_val = CRD_OK;
    exit_normal:
        return ret_val;
}

static
crd_response_t inflate_to_file(gzFile input, size_t in_size, char *out_path) {

    crd_response_t ret_val;
    z_stream strm;
    FILE *out_file;

    int gz_ret, window_bits;
    void *in_buf, *out_buf;
    size_t read_size, write_size;

    ret_val = handle_fopen(&out_file, out_path, "w");
    if (ret_val != CRD_OK) {
        goto exit_early;
    }

    ret_val = handle_alloc(&in_buf, BUFF_SIZE, 1, false);
    if (ret_val != CRD_ERR_GZ) {
        goto exit_file;
    }

    ret_val = handle_alloc(&out_buf, BUFF_SIZE, 1, false);
    if (ret_val != CRD_ERR_GZ) {
        goto exit_in;
    }

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = in_buf;

    window_bits = 0xF + 0x20;
    gz_ret = inflateInit2(&strm, window_bits);
    if (gz_ret != Z_OK) {
        ret_val = CRD_ERR_GZ;
        goto exit_out;
    }

    gz_ret = Z_OK;
    while (in_size > 0) {

        if (gz_ret != Z_OK && gz_ret != Z_STREAM_END) {
            ret_val = CRD_ERR_GZ;
            goto exit_normal;
        }

        if (strm.avail_out == 0 || gz_ret == Z_STREAM_END) {
            write_size = (gz_ret == Z_STREAM_END) ? BUFF_SIZE - strm.avail_out : BUFF_SIZE;
            ret_val = handle_fwrite(out_buf, read_size, 1, out_file);
            if (ret_val != Z_OK) {
                goto exit_normal;
            }
            strm.avail_out = BUFF_SIZE;
            in_size -= write_size;

        } else if (strm.avail_in == 0) {
            read_size = MIN(BUFF_SIZE, in_size - read_size);
            ret_val = handle_gzread(in_buf, read_size, 1, input);
            if (ret_val != Z_OK) {
                goto exit_normal;
            }
            strm.avail_in = read_size;
        }

        gz_ret = inflate(&strm, Z_NO_FLUSH);
    }

    ret_val = CRD_OK;
    exit_normal:
        inflateEnd(&strm);
    exit_out:
        free(out_buf);
    exit_in:
        free(in_buf);
    exit_file:
        handle_fclose(out_file);
    exit_early:
        return ret_val;
}


/**** API Functions ****/

crd_response_t crd_open_ird(ird_t *ird, char *path, char *t_path) {

    crd_response_t ret_val;
    gzFile ird_file;
    int path_len;

    uint8_t title_len, reg_count;
    uint8_t header_len[4], footer_len[4], file_count[4];

    char *title, *header_path, *footer_path;
    FILE *header, *footer;
    region_hash_t *reg_hashes;
    file_hash_t *file_hashes;

    ird_top_t ird_top;
    ird_middle_t ird_middle;
    ird_bottom_t ird_bottom;

    if (ird == NULL || path == NULL) {
        ret_val = CRD_ERR_ARGS;
        goto exit_early;
    }

    ret_val = handle_gzopen(&ird_file, path, "r");
    if (ret_val != CRD_OK) {
        goto exit_early;
    }

    ret_val = handle_gzread(&ird_top, sizeof(ird_top_t), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_normal;
    }

    if (memcmp(ird_top.magic, "3IRD", sizeof(ird_top.magic))) {
        ret_val = CRD_ERR_STD;
        goto exit_normal; 
    }

    if (ird_top.version[0] != 9) {
        ret_val = CRD_ERR_STD;
        goto exit_normal;
    }

    ret_val = handle_gzread(&title_len, sizeof(uint8_t), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_normal;
    }

    ret_val = handle_alloc(&title, title_len + 1, 1, false);
    if (ret_val != CRD_OK) {
        goto exit_normal;
    }

    ret_val = handle_gzread(title, sizeof(char), title_len, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_title;
    }
    title[title_len] = '\0';

    ret_val = handle_gzread(&ird_middle, sizeof(ird_middle_t), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_title;
    }

    ret_val = handle_gzread(header_len, sizeof(header_len), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_title;
    }

    ret_val = handle_alloc(&header_path, BUFF_SIZE, 1, false);
    if (ret_val != CRD_OK) {
        goto exit_title;
    }

    path_len = snprintf(header_path, PATH_LEN, "%s/%s.bin", t_path, "header");
    if (path_len >= PATH_LEN) {
        ret_val = CRD_ERR_PATH;
        goto exit_header;
    }

    ret_val = inflate_to_file(ird_file, LE_int32(header_len), header_path);
    if (ret_val != CRD_OK) {
        goto exit_header;
    }

    ret_val = handle_gzread(footer_len, sizeof(footer_len), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_title;
    }

    ret_val = handle_alloc(&footer_path, BUFF_SIZE, 1, false);
    if (ret_val != CRD_OK) {
        goto exit_title;
    }

    path_len = snprintf(footer_path, PATH_LEN, "%s/%s.bin", t_path, "footer");
    if (path_len >= PATH_LEN) {
        ret_val = CRD_ERR_PATH;
        goto exit_header;
    }

    ret_val = inflate_to_file(ird_file, LE_int32(footer_len), footer_path);
    if (ret_val != CRD_OK) {
        goto exit_header;
    }

    ret_val = handle_gzread(&reg_count, sizeof(reg_count), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_header;
    }

    ret_val = handle_alloc(&reg_hashes, reg_count, sizeof(region_hash_t), false);
    if (ret_val != CRD_OK) {
        goto exit_header;
    }

    ret_val = handle_gzread(reg_hashes, sizeof(region_hash_t), reg_count, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_region;
    }

    ret_val = handle_gzread(file_count, sizeof(file_count), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_region;
    }

    ret_val = handle_alloc(&file_hashes, LE_int32(file_count), sizeof(file_hash_t), false);
    if (ret_val != CRD_OK) {
        goto exit_region;
    }

    ret_val = handle_gzread(file_hashes, sizeof(file_hash_t), LE_int32(file_count), ird_file);
    if (ret_val != CRD_OK) {
        goto exit_files;
    }

    ret_val = handle_gzread(&ird_bottom, sizeof(ird_bottom_t), 1, ird_file);
    if (ret_val != CRD_OK) {
        goto exit_files;
    }

    ird->title_length = title_len;
    ird->title = title;

    memcpy(ird->title_id, ird_top.title_id, sizeof(ird_top.title_id));
    memcpy(ird->sys_ver, ird_middle.sys_ver, sizeof(ird_middle.sys_ver));
    memcpy(ird->disc_ver, ird_middle.disc_ver, sizeof(ird_middle.disc_ver));
    memcpy(ird->app_ver, ird_middle.app_ver, sizeof(ird_middle.app_ver));

    memcpy(ird->pic, ird_bottom.pic, sizeof(ird_bottom.pic));
    memcpy(ird->data1, ird_bottom.data1, sizeof(ird_bottom.data1));
    memcpy(ird->data2, ird_bottom.data2, sizeof(ird_bottom.data2));

    ird->uid = LE_int32(ird_bottom.uid);
    ird->crc = LE_int32(ird_bottom.crc);

    ird->region_count = reg_count;
    ird->file_count = LE_int32(file_count);
    ird->region_hashes = reg_hashes;
    ird->file_hashes = file_hashes;

    ird->header_path = header_path;
    ird->footer_path = footer_path;
    ird->header = header;
    ird->footer = footer;

    ret_val = CRD_OK;
    goto exit_normal;

    exit_files:
        free(file_hashes);
    exit_region:
        free(reg_hashes);
    exit_header:
        free(header_path);
    exit_title:
        free(title);
    exit_normal:
        handle_gzclose(ird);
    exit_early:
        return ret_val;
}
