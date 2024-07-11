/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AECP common code
 @details Handles AECP stack
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdarg.h>
#include <getopt.h>

#include "genavb/aem.h"
#include "genavb/helpers.h"

#include "aem_manager_helpers.h"

static struct aem_desc_handler desc_handler[AEM_NUM_DESC_TYPES] = {
	[AEM_DESC_TYPE_ENTITY] = {
		.fixup = aem_entity_desc_fixup,
		.check = entity_desc_check,
		.print = entity_desc_print,
		.update_name = entity_desc_update_name,
	},

	[AEM_DESC_TYPE_CONFIGURATION] = {
		.fixup = aem_configuration_desc_fixup,
		.check = configuration_desc_check,
		.print = configuration_desc_print,
	},

	[AEM_DESC_TYPE_AUDIO_UNIT] = {
		.print = audio_unit_desc_print,
	},

	[AEM_DESC_TYPE_VIDEO_UNIT] = {
		.print = video_unit_desc_print,
	},

	[AEM_DESC_TYPE_STREAM_INPUT] = {
		.print = stream_input_desc_print,
		.update_name = stream_input_desc_update_name,
	},

	[AEM_DESC_TYPE_STREAM_OUTPUT] = {
		.print = stream_output_desc_print,
		.update_name = stream_output_desc_update_name,
	},

	[AEM_DESC_TYPE_VIDEO_CLUSTER] = {
		.fixup = aem_video_cluster_desc_fixup,
	},

	[AEM_DESC_TYPE_CONTROL] = {
		.print = control_desc_print,
	},

};

void aem_print_level(int level, const char *format, ...)
{
	va_list ap;
	int i;

	for (i = 0; i < level; i++)
		printf("\t");

	va_start(ap, format);

	vprintf(format, ap);

	va_end(ap);
}

static int aem_entity_dump_to_file(const char *name, struct aem_desc_hdr *aem_desc, unsigned int overwrite)
{
	int i;
	int fd;

	if (overwrite)
		fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	else
		fd = open(name, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	if (fd < 0) {
		printf("open(%s) failed: %s\n", name, strerror(errno));
		goto err;
	}

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		if (write(fd, &aem_desc[i].total, sizeof(avb_u16)) != sizeof(avb_u16))
			goto err_write;

		if (write(fd, &aem_desc[i].size, sizeof(avb_u16)) != sizeof(avb_u16))
			goto err_write;
	}

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		int size = aem_desc[i].size * aem_desc[i].total;

		if (write(fd, aem_desc[i].ptr, size) != size)
			goto err_write;
	}

	close(fd);

	return 0;

err_write:
	close(fd);
err:
	return -1;
}


static void __aem_entity_print(struct aem_desc_hdr *aem_desc)
{
	desc_handler[AEM_DESC_TYPE_ENTITY].print(desc_handler, aem_desc, 0, 0, 0xffff);
}

/* Return value: < 0 when error, 0 otherwise. */
static int aem_entity_print(const char *filename)
{
	struct aem_desc_hdr *aem_desc;

	aem_desc = aem_entity_load_from_file(filename);
	if (!aem_desc)
		return -1;

	__aem_entity_print(aem_desc);
	return 0;
}

int aem_entity_create(const char *name, void (*entity_init)(struct aem_desc_hdr *aem_desc))
{
	struct aem_desc_hdr aem_desc[AEM_NUM_DESC_TYPES] = {{0, }};
	int rc = 0;

	entity_init(aem_desc);

	aem_entity_fixup(desc_handler, aem_desc);

	if (aem_entity_check(desc_handler, aem_desc) < 0) {
		printf("aem_desc(%p) failed to create entity %s\n", aem_desc, name);
		rc = -1;
		goto err;
	}

	__aem_entity_print(aem_desc);

	aem_entity_dump_to_file(name, aem_desc, 0);

err:
	return rc;
}

/* Return value: < 0 when error, 0 otherwise. */
static int aem_entity_update_name(const char *filename, int type, char *names)
{
	struct aem_desc_hdr *aem_desc;

	aem_desc = aem_entity_load_from_file(filename);
	if (!aem_desc)
		return -1;

	if (!desc_handler[type].update_name)
		return -1;

	desc_handler[type].update_name(aem_desc, names);

	__aem_entity_print(aem_desc);

	aem_entity_dump_to_file(filename, aem_desc, 1);
	return 0;
}

extern void listener_audio_single_init(struct aem_desc_hdr *aem_desc);
extern void listener_audio_single_milan_init(struct aem_desc_hdr *aem_desc);
extern void talker_audio_single_init(struct aem_desc_hdr *aem_desc);
extern void talker_audio_single_milan_init(struct aem_desc_hdr *aem_desc);
extern void listener_talker_audio_single_init(struct aem_desc_hdr *aem_desc);
extern void listener_talker_audio_single_milan_init(struct aem_desc_hdr *aem_desc);
extern void listener_video_single_init(struct aem_desc_hdr *aem_desc);
extern void listener_video_multi_init(struct aem_desc_hdr *aem_desc);
extern void talker_video_single_init(struct aem_desc_hdr *aem_desc);
extern void talker_video_multi_init(struct aem_desc_hdr *aem_desc);
extern void talker_audio_video_init(struct aem_desc_hdr *aem_desc);
extern void talker_listener_audio_multi_init(struct aem_desc_hdr *aem_desc);
extern void talker_listener_audio_multi_aaf_init(struct aem_desc_hdr *aem_desc);
extern void talker_listener_audio_multi_format_init(struct aem_desc_hdr *aem_desc);
extern void controller_init(struct aem_desc_hdr *aem_desc);
extern void avnu_certification_init(struct aem_desc_hdr *aem_desc);


const char *filename = "listener_talker_audio_single.aem";

void print_usage (void)
{
	printf("\nUsage:\n aem-manager [options]\n");
	printf("\nOptions:\n"
		"\t-h                    prints this help text\n"
		"\t-c                    create binary file for each defined entity\n"
		"\t-f filename           name of the entity binary file to use (use with p/e/i/o options)\n"
		"\t-p                    print entity to stdout\n"
		"\t-e name               modifies the entity name in the binary file\n"
		"\t-i name1;name2        modifies the input stream names in the binary file\n"
		"\t-o name1;name2        modifies the output stream names in the binary file\n\n"
		"Examples:         ./aem-manager -c\n"
		"                  ./aem-manager -f talker_listener_audio_multi.aem -p\n"
		"                  ./aem-manager -f talker_listener_audio_multi.aem -e \"New entity name\"\n");
}

int main(int argc, char *argv[])
{
	int option, nb_options = 0;

	entity_desc_handler_init(desc_handler);

	while ((option = getopt(argc, argv,"f:pce:i:o:h")) != -1) {

		nb_options += 1;

		switch (option) {
		case 'f':
			filename = optarg;
			break;

		case 'p':
			if (aem_entity_print(filename) < 0)
				goto err;

			break;

		case 'e':
			if (aem_entity_update_name(filename, AEM_DESC_TYPE_ENTITY, optarg) < 0)
				goto err;

			break;

		case 'i':
			if (aem_entity_update_name(filename, AEM_DESC_TYPE_STREAM_INPUT, optarg) < 0)
				goto err;

			break;

		case 'o':
			if (aem_entity_update_name(filename, AEM_DESC_TYPE_STREAM_OUTPUT, optarg) < 0)
				goto err;

			break;

		case 'c':
			if ((aem_entity_create("listener_audio_single.aem", listener_audio_single_init) < 0)
			    || (aem_entity_create("listener_audio_single_milan.aem", listener_audio_single_milan_init) < 0)
			    || (aem_entity_create("talker_audio_single.aem", talker_audio_single_init) < 0)
			    || (aem_entity_create("talker_audio_single_milan.aem", talker_audio_single_milan_init) < 0)
			    || (aem_entity_create("listener_talker_audio_single.aem", listener_talker_audio_single_init) < 0)
			    || (aem_entity_create("listener_talker_audio_single_milan.aem", listener_talker_audio_single_milan_init) < 0)
			    || (aem_entity_create("listener_video_single.aem", listener_video_single_init) < 0)
			    || (aem_entity_create("listener_video_multi.aem", listener_video_multi_init) < 0)
			    || (aem_entity_create("talker_video_single.aem", talker_video_single_init) < 0)
			    || (aem_entity_create("talker_video_multi.aem", talker_video_multi_init) < 0)
			    || (aem_entity_create("talker_audio_video.aem", talker_audio_video_init) < 0)
			    || (aem_entity_create("talker_listener_audio_multi.aem", talker_listener_audio_multi_init) < 0)
			    || (aem_entity_create("talker_listener_audio_multi_aaf.aem", talker_listener_audio_multi_aaf_init) < 0)
			    || (aem_entity_create("talker_listener_audio_multi_format.aem", talker_listener_audio_multi_format_init) < 0)
			    || (aem_entity_create("controller.aem", controller_init) < 0)
			    || (aem_entity_create("avnu_certification.aem", avnu_certification_init) < 0)) {

				fprintf(stderr, "%s: entity generation failed\n", argv[0]);
				goto err;
			}

			break;

		case 'h':
		default:
			print_usage();
			break;
		}
	}

	if (!nb_options)
		print_usage();

	return 0;

err:
	fprintf(stderr, "%s: error!\n", argv[0]);
	return 1;
}
