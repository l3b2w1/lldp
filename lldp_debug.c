#include "lldp_debug.h"


#ifdef CONFIG_DEBUG_SYSLOG
#include <syslog.h>
static int lldp_debug_syslog = 0;
#endif /* CONFIG_DEBUG_SYSLOG */

#ifdef CONFIG_DEBUG_LINUX_TRACING

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

static FILE *lldp_debug_tracing_file = NULL;

#define WPAS_TRACE_PFX "wpas <%d>: "
#endif /* CONFIG_DEBUG_LINUX_TRACING */

int lldp_debug_level = MSG_DEBUG;
int lldp_debug_show_keys = 0;
int lldp_debug_timestamp = 0;

#ifndef CONFIG_NO_STDOUT_DEBUG
#ifdef CONFIG_DEBUG_FILE
static FILE *out_file = NULL;
#endif /* CONFIG_DEBUG_FILE */

void lldp_debug_print_timestamp(void)
{
	struct timeval tv;
	if (!lldp_debug_timestamp)
		return;
	gettimeofday(&tv, NULL);
#ifdef CONFIG_DEBUG_FILE
	if (out_file) {
		fprintf(out_file, "%ld.%06u: ", (long) tv.tv_sec,
			(unsigned int) tv.tv_usec);
	} else
#endif /* CONFIG_DEBUG_FILE */
	printf("%ld.%06u: ", (long) tv.tv_sec, (unsigned int) tv.tv_usec);
}


#ifdef CONFIG_DEBUG_SYSLOG
#ifndef LOG_HOSTAPD
#define LOG_HOSTAPD LOG_DAEMON
#endif /* LOG_HOSTAPD */


void lldp_debug_open_syslog(void)
{
	openlog("lldp_supplicant", LOG_PID | LOG_NDELAY, LOG_HOSTAPD);
	lldp_debug_syslog++;
}


static int syslog_priority(int level)
{
	switch (level) {
	case MSG_DEBUG:
		return LOG_DEBUG;
	case MSG_INFO:
		return LOG_NOTICE;
	case MSG_WARNING:
		return LOG_WARNING;
	case MSG_ERROR:
		return LOG_ERR;
	}
	return LOG_INFO;
}

#endif

#ifdef CONFIG_DEBUG_FILE
static char *last_path = NULL;
#endif /* CONFIG_DEBUG_FILE */

int lldp_debug_open_file(const char *path)
{
#ifdef CONFIG_DEBUG_FILE
	if (!path)
		return 0;

	out_file = fopen(path, "a");
	if (out_file == NULL) {
		lldp_printf(MSG_ERROR, "lldp_debug_open_file: Failed to open output file, using standard output");
		return -1;
	}
	setvbuf(out_file, NULL, _IOLBF, 0);
#endif /* CONFIG_DEBUG_FILE */
	return 0;
}

void lldp_debug_close_file(void)
{
#ifdef CONFIG_DEBUG_FILE
	if (!out_file)
		return;
	fclose(out_file);
	out_file = NULL;
#endif /* CONFIG_DEBUG_FILE */
}

void lldp_printf(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (level >= lldp_debug_level) {
#ifdef CONFIG_DEBUG_SYSLOG
		if (lldp_debug_syslog) {
			vsyslog(syslog_priority(level), fmt, ap);
		} else {
#endif /* CONFIG_DEBUG_SYSLOG */
		lldp_debug_print_timestamp();
#ifdef CONFIG_DEBUG_FILE
		if (out_file) {
			vfprintf(out_file, fmt, ap);
			fprintf(out_file, "\n");
		} else {
#endif /* CONFIG_DEBUG_FILE */
		vprintf(fmt, ap);
		//printf("\n");
#ifdef CONFIG_DEBUG_FILE
		}
#endif /* CONFIG_DEBUG_FILE */
#ifdef CONFIG_DEBUG_SYSLOG
		}
#endif /* CONFIG_DEBUG_SYSLOG */
	}
	va_end(ap);

#ifdef CONFIG_DEBUG_LINUX_TRACING
	if (lldp_debug_tracing_file != NULL) {
		va_start(ap, fmt);
		fprintf(lldp_debug_tracing_file, WPAS_TRACE_PFX, level);
		vfprintf(lldp_debug_tracing_file, fmt, ap);
		fprintf(lldp_debug_tracing_file, "\n");
		fflush(lldp_debug_tracing_file);
		va_end(ap);
	}
#endif /* CONFIG_DEBUG_LINUX_TRACING */
}

void lldp_debug_setup_stdout(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);
}

#endif
