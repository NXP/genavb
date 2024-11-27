/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "file_buffer.h"
#include "common.h"

void file_buffer_init(struct file_buffer *buf, unsigned int readers)
{
	unsigned int i;

	buf->w_offset = 0;

	if (readers > FILE_BUFFER_READERS)
		readers = FILE_BUFFER_READERS;

	for (i = 0; i < readers; i++)
		buf->r_offset[i] = 0;

	buf->readers = readers;
	buf->eof = 0;
	buf->size = FILE_BUFFER_SIZE;
}


unsigned int file_buffer_available(struct file_buffer *buf, int reader)
{
	if (buf->w_offset >= buf->r_offset[reader])
		return buf->w_offset - buf->r_offset[reader];
	else
		return (buf->w_offset + buf->size) - buf->r_offset[reader];
}

void file_buffer_read(struct file_buffer *buf, int reader, unsigned int len)
{
	buf->r_offset[reader] += len;
	if (buf->r_offset[reader] >= buf->size)
		buf->r_offset[reader] -= buf->size;
}

void *file_buffer_buf(struct file_buffer *buf, int reader)
{
	return buf->buf + buf->r_offset[reader];
}

unsigned int file_buffer_available_wrap(struct file_buffer *buf, int reader)
{
	if (buf->w_offset >= buf->r_offset[reader])
		return buf->w_offset - buf->r_offset[reader];
	else
		return buf->size - buf->r_offset[reader];
}


int file_buffer_empty(struct file_buffer *buf, int reader)
{
	return !file_buffer_available(buf, reader);
}

unsigned int file_buffer_free(struct file_buffer *buf, int reader)
{
	return buf->size - file_buffer_available(buf, reader) - 1;
}

int file_buffer_write(struct file_buffer *buf, unsigned int fd, unsigned int timeout)
{
	unsigned int len, len_now;
	unsigned int written = 0;
	int rc;

	if (buf->eof)
		return 0;

//	printf("%s: %u %u %u %u\n", __func__, buf->w_offset, buf->r_offset[0], buf->r_offset[1], buf->eof);

	len = file_buffer_free(buf, 0);
	if (len < buf->size / 16)
		return 0;

	if (buf->readers > 1) {
		if (len > file_buffer_free(buf, 1))
			len = file_buffer_free(buf, 1);
	}

	len_now = len;
	if ((buf->w_offset + len_now) > buf->size) {
		len_now = buf->size - buf->w_offset;

		rc = file_read(fd, buf->buf + buf->w_offset, len_now, timeout);
		if (rc <= 0)
			return rc;

		buf->w_offset += rc;
		if (buf->w_offset >= buf->size)
			buf->w_offset = 0;

		written += rc;

		if (rc < (int)len_now)
			return written;

		len_now = len - rc;
	}

	if (!len_now)
		return written;

	rc = file_read(fd, buf->buf + buf->w_offset, len_now, timeout);
	if (rc <= 0) {
		if (!rc && written)
			return written;

		return rc;
	}

	written += rc;

	buf->w_offset += rc;
	if (buf->w_offset >= buf->size)
		buf->w_offset = 0;

	return written;
}
