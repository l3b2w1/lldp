#ifndef _LLDP_DEBUG_H_
#define _LLDP_DEBUG_H_

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
extern int lldp_debug_level;
extern int lldp_debug_show_keys;
extern int lldp_debug_timestamp;

#define CONFIG_DEBUG_SYSLOG


enum {
	MSG_DEBUG, 
	MSG_INFO, 
	MSG_WARNING, 
	MSG_ERROR
};

#ifdef CONFIG_NO_STDOUT_DEBUG

#define lldp_debug_print_timestamp() do { } while (0)
#define lldp_printf(args...) do { } while (0)
#define lldp_hexdump(l,t,b,le) do { } while (0)
#define lldp_hexdump_buf(l,t,b) do { } while (0)
#define lldp_hexdump_key(l,t,b,le) do { } while (0)
#define lldp_hexdump_buf_key(l,t,b) do { } while (0)
#define lldp_hexdump_ascii(l,t,b,le) do { } while (0)
#define lldp_hexdump_ascii_key(l,t,b,le) do { } while (0)
#define lldp_debug_open_file(p) do { } while (0)
#define lldp_debug_close_file() do { } while (0)
#define lldp_debug_setup_stdout() do { } while (0)
#define lldp_dbg(args...) do { } while (0)

#else /* CONFIG_NO_STDOUT_DEBUG */

int lldp_debug_open_file(const char *path);
int lldp_debug_reopen_file(void);
void lldp_debug_close_file(void);
void lldp_debug_setup_stdout(void);


/**
 * lldp_debug_printf_timestamp - Print timestamp for debug output
 *
 * This function prints a timestamp in seconds_from_1970.microsoconds
 * format if debug output has been configured to include timestamps in debug
 * messages.
 */
void lldp_debug_print_timestamp(void);

/**
 * lldp_printf - conditional printf
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration.
 *
 * Note: New line '\n' is added to the end of the text when printing to stdout.
 */
void lldp_printf(int level, const char *fmt, ...);


#ifdef CONFIG_DEBUG_SYSLOG

void lldp_debug_open_syslog(void);
void lldp_debug_close_syslog(void);

#else /* CONFIG_DEBUG_SYSLOG */

static inline void lldp_debug_open_syslog(void)
{
}

static inline void lldp_debug_close_syslog(void)
{
}

#endif /* CONFIG_DEBUG_SYSLOG */


#endif /* CONFIG_NO_STDOUT_DEBUG */


#ifdef CONFIG_DEBUG_LINUX_TRACING

int lldp_debug_open_linux_tracing(void);
void lldp_debug_close_linux_tracing(void);

#else /* CONFIG_DEBUG_LINUX_TRACING */

static inline int lldp_debug_open_linux_tracing(void)
{
	return 0;
}

static inline void lldp_debug_close_linux_tracing(void)
{
}

#endif /* CONFIG_DEBUG_LINUX_TRACING */

#ifdef LLDP_TEST
#define LLDP_ASSERT(a)						       \
	do {							       \
		if (!(a)) {					       \
			printf("WPA_ASSERT FAILED '" #a "' "	       \
			       "%s %s:%d\n",			       \
			       __FUNCTION__, __FILE__, __LINE__);      \
			exit(1);				       \
		}						       \
	} while (0)
#else
#define LLDP_ASSERT(a) do { } while (0)
#endif




#endif	/* __DEBUG_H_ */
