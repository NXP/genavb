/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FILE_BUFFER_H_
#define _FILE_BUFFER_H_

#define PES_SIZE	188

#define FILE_BUFFER_SIZE (1024 * 10 * PES_SIZE)
#define FILE_BUFFER_READERS	2

#ifdef __cplusplus
extern "C" {
#endif

struct file_buffer {
	unsigned char buf[FILE_BUFFER_SIZE];
	unsigned int size;

	unsigned int w_offset;

	unsigned int r_offset[FILE_BUFFER_READERS];

	unsigned int eof;
	unsigned int readers;
};

/* initialize a buffer that can handle "readers" number of readers */
void file_buffer_init(struct file_buffer *buf, unsigned int readers);

unsigned int file_buffer_available(struct file_buffer *buf, int reader);
unsigned int file_buffer_available_wrap(struct file_buffer *buf, int reader);

/* return true if buffer is empty */
int file_buffer_empty(struct file_buffer *buf, int reader);

unsigned int file_buffer_free(struct file_buffer *buf, int reader);

void *file_buffer_buf(struct file_buffer *buf, int reader);

void file_buffer_read(struct file_buffer *buf, int reader, unsigned int len);

int file_buffer_write(struct file_buffer *buf, unsigned int fd, unsigned int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _FILE_BUFFER_H_ */
