/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Top AVB linux process
 @details Setups linux AVB process and threads for each AVTP stack components.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/ioctl.h>

#include "common/version.h"
#include "common/log.h"
#include "genavb/helpers.h"
#include "genavb/aem_helpers.h"
#include "genavb/sr_class.h"
#include "cfgfile.h"
#include "os/clock.h"

#include "avb.h"
#include "init.h"
#include "log.h"

#if defined(CONFIG_AVDECC)
#include "avdecc/config.h"
#endif

#if defined(CONFIG_AVTP)
void *avtp_thread_main(void *arg);
#endif
#if defined(CONFIG_AVDECC)
void *avdecc_thread_main(void *arg);
#endif
#if defined(CONFIG_MAAP)
void *maap_thread_main(void *arg);
#endif

/* Default configuration files, if none are specified on cmd line */
#define AVB_CONF_FILENAME	"/etc/genavb/genavb-listener.cfg"

#define CHAR_LEN		16

#define AVB_DEFAULT_NET_MODE	"avb"

static int terminate = 0;

static void sigterm_hdlr(int signum)
{
	os_log(LOG_INIT, "signum(%d)\n", signum);

	terminate = 1;
}

void print_version(void)
{
	printf("NXP's GenAVB/TSN stack version %s\n", GENAVB_VERSION);
}

void print_usage (void)
{
	printf("\nUsage:\n avb [options]\n");
	printf("\nOptions:\n"
		"\t-v                    display program version\n"
		"\t-f <config file>      avb configuration filename\n"
		"\t-p <port number>      number of ports to support for the endpoint stack (default: %u, min %u, max %u)\n"
		"\t-s <config file>      srp configuration filename\n"
		"\t-H <hybrid mode>      hybrid mode: (default: %u)\n"
		"\t                           %u: disabled\n"
		"\t                           %u: hybrid with external clock synchronization (no gPTP exchange between endpoint and bridge stacks)\n"
		"\t                           %u: hybrid with gptp clock synchronization (gPTP exchange between endpoint and bridge stacks)\n"
		"\t-h                    print this help text\n",
		CFG_EP_DEFAULT_NUM_PORTS, CFG_EP_DEFAULT_NUM_PORTS, CFG_MAX_ENDPOINTS,
		HYBRID_MODE_DISABLED, HYBRID_MODE_DISABLED, HYBRID_MODE_EXTERNAL_CLOCK_SYNC, HYBRID_MODE_GPTP_CLOCK_SYNC);
}

#if defined(CONFIG_AVDECC)
static unsigned int parse_list_u16(char *str, u16 *list)
{
	unsigned int i = 0;
	char *string;

	string = strtok(str, ",");
	while (string && (i < ACMP_CFG_MAX_UNIQUE_ID)) {
		errno = 0;
		list[i] = strtoul(string, NULL, 0);
		if (errno != 0)
			break;
		i++;
		string = strtok(NULL, ",");
	}

	return i;
}

static unsigned int parse_list_u64(char *str, u64 *list)
{
	unsigned int i = 0;
	char *string;

	string = strtok(str, ",");
	while (string && (i < ACMP_CFG_MAX_UNIQUE_ID)) {
		errno = 0;
		list[i] = strtoull(string, NULL, 0);
		if (errno != 0)
			break;
		i++;
		string = strtok(NULL, ",");
	}

	return i;
}
#endif

static int log_string2level(const char *s)
{
	int level;

	for (level = LOG_CRIT; level <= LOG_DEBUG; level++) {
		if (!strcasecmp(s, log_lvl_string[level]))
			return level;
	}

	return -1;
}

static int process_section_general(struct _SECTIONENTRY *configtree, struct avb_ctx *avb)
{
	int level;
	int i, rc = 0;
	int nb_cmp;
	char log_item[max_COMPONENT_ID][CFG_STRING_LIST_MAX_LEN];
	char stringvalue[CFG_STRING_MAX_LEN] = "";


	/* loglevel */
	if (cfg_get_string(configtree, "AVB_GENERAL", "log_level", "info", stringvalue)) {
		rc = -1;
		goto exit;
	}

	level = log_string2level(stringvalue);
	if (level < 0) {
		printf("Error setting log level (%s)\n", stringvalue);
		rc = -1;
		goto exit;
	}

	printf("AVB cfg file: setting log level to %s (%d)\n", stringvalue, level);

	log_level_set(os_COMPONENT_ID, level);
	log_level_set(common_COMPONENT_ID, level);

	avb->avtp_cfg.log_level = level;
	avb->avdecc_cfg.log_level = level;
	avb->maap_cfg.log_level = level;

	/* log_monotonic */
	if (cfg_get_string(configtree, "AVB_GENERAL", "log_monotonic", "disabled", stringvalue)) {
		rc = -1;
		goto exit;
	}

	if (!strcmp(stringvalue, "enabled"))
		log_enable_monotonic();


	/* disable log for some components */
	nb_cmp = cfg_get_string_list(configtree, "AVB_GENERAL", "disable_component_log", "none", log_item, max_COMPONENT_ID);
	if (nb_cmp < 0) {
		rc = -1;
		goto exit;
	}

	/* parse list of components to disable trace for ... */
	for (i = 0; i < nb_cmp; i++) {
		if (strcasestr(log_item[i], "avtp"))
			avb->avtp_cfg.log_level = LOG_CRIT;
		else if (strcasestr(log_item[i], "avdecc"))
			avb->avdecc_cfg.log_level = LOG_CRIT;
		else if (strcasestr(log_item[i], "common"))
			log_level_set(common_COMPONENT_ID, LOG_CRIT);
		else if (strcasestr(log_item[i], "os"))
			log_level_set(os_COMPONENT_ID, LOG_CRIT);
	}

exit:
	return rc;
}


static int process_section_avtp(struct _SECTIONENTRY *configtree, struct avtp_config *avtp_cfg)
{
#if 0
	char stringvalue[CFG_STRING_MAX_LEN] = "";

	if (cfg_get_string(configtree, "AVB_AVTP", "string_option", "0", stringvalue))
		goto exit;

	return 0;

exit:
	return -1;
#endif
	return 0;
}


#if defined(CONFIG_AVDECC)
static int process_section_avdecc(struct _SECTIONENTRY *configtree, struct avdecc_config *avdecc_cfg)
{
	char stringvalue[CFG_STRING_MAX_LEN] = "";
	char section_name[CFG_STRING_MAX_LEN] = "";
	int nb_entity = 0;
	int entity_index = 0;
	int rc = 0;
	unsigned int i;
	u64 association_id = 0;
	struct avdecc_entity_config *entity_cfg;

	entity_cfg = &avdecc_cfg->entity_cfg[0];

	if (cfg_get_string(configtree, "AVB_AVDECC", "enabled", "1", stringvalue))
		goto exit;

	if (strcmp(stringvalue, "1")) {
		avdecc_cfg->enabled = false;
		goto out;
	}

	avdecc_cfg->enabled = true;

	if (cfg_get_string(configtree, "AVB_AVDECC", "milan_mode", "0", stringvalue))
		goto exit;

	if (!strcmp(stringvalue, "0"))
		avdecc_cfg->milan_mode = false;
	else
		avdecc_cfg->milan_mode = true;

	if (cfg_get_uint(configtree, "AVB_AVDECC", "max_entities_discovery",
				CFG_ADP_DEFAULT_NUM_ENTITIES_DISCOVERY, CFG_ADP_MIN_NUM_ENTITIES_DISCOVERY, CFG_ADP_MAX_NUM_ENTITIES_DISCOVERY, &avdecc_cfg->max_entities_discovery))
		goto exit;

	if (avdecc_cfg->milan_mode) {
		/* Milan mode forces SRP enabled */
		avdecc_cfg->srp_enabled = true;
	} else {
		/* Configuration options available only in non-milan mode */

		if (cfg_get_string(configtree, "AVB_AVDECC", "srp_enabled", "1", stringvalue))
			goto exit;

		if (!strcmp(stringvalue, "0"))
			avdecc_cfg->srp_enabled = false;
		else
			avdecc_cfg->srp_enabled = true;

		/* fast_connect_btb */
		if (cfg_get_string(configtree, "AVB_AVDECC", "btb_demo_mode", "0", stringvalue))
			goto exit;

		if (!strcmp(stringvalue, "1")) {
			entity_cfg->flags |= AVDECC_FAST_CONNECT_BTB;
			entity_cfg->flags |= AVDECC_FAST_CONNECT_MODE; /* btb demo mode relies on fast connect, so enable it too */
		} else if (strcmp(stringvalue, "0"))
			printf("AVB cfg file: Warning, unknow value for btb_demo_mode (%s). Ignoring.\n", stringvalue);


		/* enable fast-connect mode */
		if (cfg_get_string(configtree, "AVB_AVDECC", "fast_connect", "0", stringvalue))
			goto exit;

		if (!strcmp(stringvalue, "1"))
			entity_cfg->flags |= AVDECC_FAST_CONNECT_MODE;
		else if (strcmp(stringvalue, "0"))
			printf("AVB cfg file: Warning, unknow value for avdecc_fast_connect (%s). Ignoring.\n", stringvalue);
	}

	for (entity_index = 0; entity_index < CFG_AVDECC_NUM_ENTITIES; entity_index++) {
		snprintf(section_name, 256, "AVB_AVDECC_ENTITY_%d", entity_index + 1);
		//printf("Processing section %s\n", section_name);

		rc = cfg_get_string(configtree, section_name, "entity_file", "none", stringvalue);

		if (rc == 0 && strcmp(stringvalue, "none")) {
			/* aem file name - load aem file */
			entity_cfg = &avdecc_cfg->entity_cfg[nb_entity];

			entity_cfg->aem = aem_entity_load_from_file(stringvalue);
			if (!entity_cfg->aem)
				goto err;

			nb_entity++;

			if (cfg_get_u64(configtree, section_name, "entity_id", 0, 0, UINT64_MAX, &entity_cfg->entity_id))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_listener_streams", CFG_ACMP_DEFAULT_NUM_LISTENER_STREAMS, CFG_ACMP_MIN_NUM_LISTENER_STREAMS, CFG_ACMP_MAX_NUM_LISTENER_STREAMS, &entity_cfg->max_listener_streams))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_talker_streams", CFG_ACMP_DEFAULT_NUM_TALKER_STREAMS, CFG_ACMP_MIN_NUM_TALKER_STREAMS, CFG_ACMP_MAX_NUM_TALKER_STREAMS, &entity_cfg->max_talker_streams))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_inflights", CFG_AVDECC_DEFAULT_NUM_INFLIGHTS, CFG_AVDECC_MIN_NUM_INFLIGHTS, CFG_AVDECC_MAX_NUM_INFLIGHTS, &entity_cfg->max_inflights))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_unsolicited_registratons", CFG_AECP_DEFAULT_NUM_UNSOLICITED, CFG_AECP_MIN_NUM_UNSOLICITED, CFG_AECP_MAX_NUM_UNSOLICITED, &entity_cfg->max_unsolicited_registrations))
				goto err;

			if (cfg_get_uint(configtree, section_name, "max_ptlv_entries", CFG_AEM_DEFAULT_NUM_PTLV_ENTRIES, CFG_AEM_MIN_NUM_PTLV_ENTRIES, CFG_AEM_MAX_NUM_PTLV_ENTRIES, &entity_cfg->max_ptlv_entries))
				goto exit;

			if (cfg_get_uint(configtree, section_name, "channel_waitmask", 0, 0, 7, &entity_cfg->channel_waitmask))
				goto err;

			if (avdecc_cfg->milan_mode) {
				/* Force milan fixed valid time */
				entity_cfg->valid_time = CFG_ADP_MILAN_VALID_TIME;
			} else {
				/* Configuration options available only in non-milan mode */

				if (cfg_get_uint(configtree, section_name, "valid_time", CFG_ADP_DEFAULT_VALID_TIME, CFG_ADP_MIN_VALID_TIME, CFG_ADP_MAX_VALID_TIME, &entity_cfg->valid_time))
					goto err;

				if (cfg_get_uint(configtree, section_name, "max_listener_pairs", CFG_ACMP_DEFAULT_NUM_LISTENER_PAIRS, CFG_ACMP_MIN_NUM_LISTENER_PAIRS, CFG_ACMP_MAX_NUM_LISTENER_PAIRS, &entity_cfg->max_listener_pairs))
					goto err;

				/* talker entity ID list */
				if (cfg_get_string(configtree, section_name, "talker_entity_id_list", "none", stringvalue))
					goto err;

				if (strcmp(stringvalue, "none"))
					entity_cfg->talker_entity_id_n = parse_list_u64(stringvalue, entity_cfg->talker_entity_id);

				/* talker unique ID list */
				if (cfg_get_string(configtree, section_name, "talker_unique_id_list", "none", stringvalue))
					goto err;

				if (strcmp(stringvalue, "none"))
					entity_cfg->talker_unique_id_n = parse_list_u16(stringvalue, entity_cfg->talker_unique_id);

				/* listener unique ID list */
				if (cfg_get_string(configtree, section_name, "listener_unique_id_list", "none", stringvalue))
					goto err;

				if (strcmp(stringvalue, "none"))
					entity_cfg->listener_unique_id_n = parse_list_u16(stringvalue, entity_cfg->listener_unique_id);
			}
		}
	}


	avdecc_cfg->num_entities = nb_entity;

	printf("AVB cfg file: loaded %d AEM entities.\n", nb_entity);


	/* association ID */
	if (cfg_get_u64(configtree, "AVB_AVDECC", "association_id", 0, 0, UINT64_MAX, &association_id))
		goto err;

	printf("Advertising association ID 0x%" PRIx64 " for all entities.\n", association_id);

	for (i = 0; i < avdecc_cfg->num_entities; i++)
		avdecc_cfg->entity_cfg[i].association_id = association_id;

out:
	return 0;

err:
	/* free memory allocated from aem_entity_load_from_file() */
	for (i = 0; i < avdecc_cfg->num_entities; i++) {
		if (avdecc_cfg->entity_cfg[i].aem)
			free(avdecc_cfg->entity_cfg[i].aem);
	}

exit:
	return -1;
}
#endif

static int process_avb_config(struct avb_ctx *avb, const char *filename)
{
	struct _SECTIONENTRY *configtree;

	/**************************************************************/
	/* open cfg file, store inputs in a tree, then process inputs */
	/**************************************************************/
	printf("AVB: Using configuration file: %s\n", filename);

	/* read all sections and all key/value pairs from config file, and store them in chained list */
	configtree = cfg_read(filename);
	if (!configtree)
		goto err_open;

	/********************************/
	/* fetch values from configtree */
	/********************************/

	if (process_section_general(configtree, avb))
		goto err_parse;

	if (process_section_avtp(configtree, &avb->avtp_cfg))
		goto err_parse;

#if defined(CONFIG_AVDECC)
	if (process_section_avdecc(configtree, &avb->avdecc_cfg))
		goto err_parse;
#else
	avb->avdecc_cfg.enabled = false;
#endif

	/* finished parsing the configuration tree, so free memory */
	cfg_free_configtree(configtree);

	return 0;

err_parse:
	printf("AVB cfg file: Error while parsing config file\n");

	cfg_free_configtree(configtree);
err_open:
	return -1;
}

static int process_srp_config(struct avb_ctx *avb, const char *filename)
{
	struct _SECTIONENTRY *configtree;

	printf("SRP: Using configuration file: %s\n", filename);

	/* read all sections and all key/value pairs from config file, and store them in chained list */
	configtree = cfg_read(filename);
	if (!configtree)
		goto err_open;

	/********************************/
	/* fetch values from configtree */
	/********************************/

	/* enable sr_class */
	if (cfg_get_sr_class_list(configtree, avb->sr_class) < 0)
		goto err_parse;

	/* finished parsing the configuration tree, so free memory */
	cfg_free_configtree(configtree);

	return 0;

err_parse:
	printf("SRP cfg file: Error while parsing config file\n");

	cfg_free_configtree(configtree);

err_open:
	return -1;
}

static int process_system_config(struct os_net_config *net_config, const char *filename)
{
	struct _SECTIONENTRY *configtree;

	printf("System: Using configuration file: %s\n", filename);

	/* read all sections and all key/value pairs from config file, and store them in chained list */
	configtree = cfg_read(filename);
	if (!configtree)
		goto err_open;

	if(cfg_get_net_mode(configtree, "avdecc_net_mode", AVB_DEFAULT_NET_MODE, &net_config->avdecc_net_mode, &net_config->enabled_modes_flag))
		goto err_parse;

	if(cfg_get_net_mode(configtree, "avtp_net_mode", AVB_DEFAULT_NET_MODE, &net_config->avtp_net_mode, &net_config->enabled_modes_flag))
		goto err_parse;

	/* finished parsing the configuration tree, so free memory */
	cfg_free_configtree(configtree);

	return 0;

err_parse:
	printf("System cfg file: Error while parsing config file\n");

	cfg_free_configtree(configtree);

err_open:
	return -1;
}

static int endpoint_init(struct avb_ctx *avb)
{
#if defined(CONFIG_AVTP) || defined(CONFIG_MAAP) || defined(CONFIG_AVDECC)
	int rc;
#endif

#if defined(CONFIG_AVTP)
	pthread_cond_init(&avb->avtp_cond, NULL);
#endif
#if defined(CONFIG_MAAP)
	pthread_cond_init(&avb->maap_cond, NULL);
#endif
#if defined(CONFIG_AVDECC)
	if (avb->avdecc_cfg.enabled)
		pthread_cond_init(&avb->avdecc_cond, NULL);
#endif

#if defined(CONFIG_AVTP)
	rc = pthread_create(&avb->avtp_thread, NULL, avtp_thread_main, avb);
	if (rc) {
		os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
		goto err_pthread_create_avtp;
	}
#endif
#if defined(CONFIG_MAAP)
	rc = pthread_create(&avb->maap_thread, NULL, maap_thread_main, avb);
	if (rc) {
		os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
		goto err_pthread_create_maap;
	}
#endif

	pthread_mutex_lock(&avb->status_mutex);
#if defined(CONFIG_AVTP)
	while (!avb->avtp_status) pthread_cond_wait(&avb->avtp_cond, &avb->status_mutex);
#endif
#if defined(CONFIG_MAAP)
	while (!avb->maap_status) pthread_cond_wait(&avb->maap_cond, &avb->status_mutex);
#endif
	pthread_mutex_unlock(&avb->status_mutex);

#if defined(CONFIG_AVTP)
	if (avb->avtp_status < 0)
		goto err_thread_init;
#endif

#if defined(CONFIG_MAAP)
	if (avb->maap_status < 0)
		goto err_thread_init;
#endif

#if defined(CONFIG_AVDECC)
	if (avb->avdecc_cfg.enabled) {
		rc = pthread_create(&avb->avdecc_thread, NULL, avdecc_thread_main, avb);
		if (rc) {
			os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
			goto err_pthread_create_avdecc;
		}

		pthread_mutex_lock(&avb->status_mutex);
		while (!avb->avdecc_status) pthread_cond_wait(&avb->avdecc_cond, &avb->status_mutex);
		pthread_mutex_unlock(&avb->status_mutex);

		if (avb->avdecc_status < 0)
			goto err_avdecc_init;
	}
#endif

	return 0;

#if defined(CONFIG_AVDECC)
err_avdecc_init:
	if (avb->avdecc_cfg.enabled) {
		pthread_cancel(avb->avdecc_thread);
		pthread_join(avb->avdecc_thread, NULL);
	}

err_pthread_create_avdecc:
#endif
#if defined(CONFIG_AVTP) || defined(CONFIG_MAAP)
err_thread_init:
#endif
#if defined(CONFIG_MAAP) && defined(CONFIG_AVDECC)
	pthread_cancel(avb->maap_thread);
	pthread_join(avb->maap_thread, NULL);
#endif
#if defined(CONFIG_MAAP)
err_pthread_create_maap:
#endif
#if defined(CONFIG_AVTP) && (defined(CONFIG_MAAP) || defined(CONFIG_AVDECC))
	pthread_cancel(avb->avtp_thread);
	pthread_join(avb->avtp_thread, NULL);
#endif
#if defined(CONFIG_AVTP)
err_pthread_create_avtp:
#endif
	return -1;
}

static void endpoint_exit(struct avb_ctx *avb)
{
#if defined(CONFIG_AVDECC)
	if (avb->avdecc_cfg.enabled) {
		pthread_cancel(avb->avdecc_thread);
		pthread_join(avb->avdecc_thread, NULL);
	}
#endif

#if defined(CONFIG_MAAP)
	pthread_cancel(avb->maap_thread);
#endif
#if defined(CONFIG_AVTP)
	pthread_cancel(avb->avtp_thread);
#endif
#if defined(CONFIG_MAAP)
	pthread_join(avb->maap_thread, NULL);
#endif
#if defined(CONFIG_AVTP)
	pthread_join(avb->avtp_thread, NULL);
#endif
}

int main(int argc, char *argv[])
{
	struct avb_ctx *avb;
	struct os_net_config net_config = { 0 };
	sigset_t set;
	struct sigaction action;
	int option;
	int i;
	const char *avb_conf_filename;
	const char *srp_conf_filename;
	unsigned int endpoint_logical_port_list[CFG_MULTI_EP_DEFAULT_NUM_PORTS] = CFG_MULTI_EP_LOGICAL_PORT_LIST;
	unsigned int *logical_port_list;
	long hybrid_mode = HYBRID_MODE_DISABLED;
	unsigned long num_ports;
	os_clock_id_t clock_log;
	int fd;
	int rc;

	/* Setup standard output in append mode so that log file truncate works correctly */
	fd = fileno(stdout);
	if (fd < 0)
		goto err_fileno;

	if (fcntl(fd, F_SETFL, O_APPEND) < 0)
		goto err_fcntl;

	print_version();

	avb = malloc(sizeof(struct avb_ctx));
	if (!avb) {
		printf("malloc(): failed, aborting.\n");
		goto err_malloc;
	}

	memset(avb, 0, sizeof(struct avb_ctx));

	avb_conf_filename = AVB_CONF_FILENAME;
	srp_conf_filename = SRP_CONF_FILENAME;

	num_ports = CFG_EP_DEFAULT_NUM_PORTS;

	while ((option = getopt(argc, argv,"vhH:f:p:s:")) != -1) {
		switch (option) {
		case 'v':
			print_version();
			goto exit;
			break;

		case 'f':
			avb_conf_filename = optarg;
			break;

		case 's':
			srp_conf_filename = optarg;
			break;

		case 'H':
			if ((h_strtol(&hybrid_mode, optarg, NULL, 0) < 0)
				|| ((hybrid_mode < HYBRID_MODE_MIN) || (hybrid_mode > HYBRID_MODE_MAX))) {
				printf("invalid -H %s option\n", optarg);
				print_usage();
				goto exit;
			}
			break;
		case 'p':
			if ((h_strtoul(&num_ports, optarg, NULL, 0) < 0) || (num_ports < CFG_EP_DEFAULT_NUM_PORTS) || (num_ports > CFG_MULTI_EP_DEFAULT_NUM_PORTS)) {
				printf("invalid -p %s option\n", optarg);
				print_usage();
				goto exit;
			}
			break;

		case 'h':
		default:
			print_usage();
			goto exit;
		}
	}

	if (process_avb_config(avb, avb_conf_filename) < 0) /* AVB cfg file processing failed, exit */
		goto err_config;

	if (process_srp_config(avb, srp_conf_filename) < 0) /* SRP cfg file processing failed, exit */
		goto err_config;

	if (process_system_config(&net_config, SYSTEM_CONF_FILENAME) < 0) /* System cfg file processing failed, exit */
		goto err_config;

	if (os_init(&net_config) < 0)
		goto err_osal;

	if (sr_class_config(avb->sr_class) < 0) {
		os_log(LOG_ERR, "sr_class_config failed\n");
		goto err_osal;
	}

	logical_port_list = endpoint_logical_port_list;
	clock_log = OS_CLOCK_GPTP_EP_0_0;

#ifdef CONFIG_MANAGEMENT
	avb->avdecc_cfg.management_enabled = true;
	avb->maap_cfg.management_enabled = true;
#else
	avb->avdecc_cfg.management_enabled = false;
	avb->maap_cfg.management_enabled = false;
#endif

	/* Hybrid mode with external clock synchronization means there is no gptp endpoint running:
	 * Use the gptp bridge stack to get gptp informations in avdecc stack.
	 */
	avb->avdecc_cfg.use_gptp_bridge_stack = (hybrid_mode == HYBRID_MODE_EXTERNAL_CLOCK_SYNC) ? true : false;

	avb->maap_cfg.port_max = num_ports;
	avb->avdecc_cfg.port_max = num_ports;
	avb->avtp_cfg.port_max = num_ports;

	for (i = 0; i < num_ports; i++) {
		avb->avtp_cfg.logical_port_list[i] = logical_port_list[i];
		avb->avtp_cfg.clock_gptp_list[i] = logical_port_to_gptp_clock(logical_port_list[i], CFG_DEFAULT_GPTP_DOMAIN);

		avb->maap_cfg.logical_port_list[i] = logical_port_list[i];
		avb->avdecc_cfg.logical_port_list[i] = logical_port_list[i];
	}

	/* Block most signals for all threads */
	if (sigfillset(&set) < 0)
		os_log(LOG_ERR, "sigfillset(): %s\n", strerror(errno));

	if (sigdelset(&set, SIGILL) < 0)
		os_log(LOG_ERR, "sigdelset(): %s\n", strerror(errno));

	if (sigdelset(&set, SIGFPE) < 0)
		os_log(LOG_ERR, "sigdelset(): %s\n", strerror(errno));

	if (sigdelset(&set, SIGSEGV) < 0)
		os_log(LOG_ERR, "sigdelset(): %s\n", strerror(errno));

	rc = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (rc)
		os_log(LOG_ERR, "pthread_sigmask(): %s\n", strerror(rc));

	if (endpoint_init(avb) < 0)
		goto err_endpoint_init;

	action.sa_handler = sigterm_hdlr;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		os_log(LOG_ERR, "sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGTERM, &action, NULL) < 0)
		os_log(LOG_ERR, "sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGQUIT, &action, NULL) < 0)
		os_log(LOG_ERR, "sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGINT, &action, NULL) < 0)
		os_log(LOG_ERR, "sigaction(): %s\n", strerror(errno));

	/* Unblock normal process control signals for the main thread only */
	if (sigemptyset(&set) < 0)
		os_log(LOG_ERR, "sigfillset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGINT) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGTERM) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGQUIT) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	if (sigaddset(&set, SIGTSTP) < 0)
		os_log(LOG_ERR, "sigaddset(): %s\n", strerror(errno));

	rc = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
	if (rc)
		os_log(LOG_ERR, "sigprocmask(): %s\n", strerror(rc));

	while (1) {
		sleep(1);

		log_update_time(clock_log);
		log_update_monotonic();

		if (terminate)
			break;
	}

	endpoint_exit(avb);

err_endpoint_init:

	os_exit();

	if (avb->avdecc_cfg.enabled) {
		for (i = 0; i< avb->avdecc_cfg.num_entities; i++) {
			if (avb->avdecc_cfg.entity_cfg[i].aem)
				free(avb->avdecc_cfg.entity_cfg[i].aem);
		}
	}

exit:
	free(avb);

	return 0;

err_osal:
err_config:
	free(avb);

err_malloc:
err_fcntl:
err_fileno:
	return -1;
}
