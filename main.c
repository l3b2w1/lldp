#include <unistd.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <net/if.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "tlv.h"
#include "lldp_port.h"
#include "lldp_neighbor.h"
#include "lldp_debug.h"
#include "lldp_neighbor.h"
#include "lldp_linux_framer.h"
#include "rx_sm.h"
#include "tx_sm.h"
#include "tlv.h"
#include "tlv_common.h"

// This is set to argv[0] on startup.
char *program;

#define LLDP_IF_NAMESIZE    32
char iface_list[IF_NAMESIZE];
int  iface_filter = 0;   /* boolean */

struct lldp_port *lldp_ports = NULL;

static void usage();
int initialize_lldp();
void handle_segfault();
void handle_hup();



void walk_port_list() {
    struct lldp_port *lldp_port = lldp_ports;
#if 1
    while(lldp_port != NULL) {
        lldp_printf(MSG_DEBUG, "Interface structure @ %X\n", lldp_port);
        lldp_printf(MSG_DEBUG, "\tName: %s\n", lldp_port->if_name);
        lldp_printf(MSG_DEBUG, "\tIndex: %d\n", lldp_port->if_index);
        lldp_printf(MSG_DEBUG, "\tMTU: %d\n", lldp_port->mtu);
        lldp_printf(MSG_DEBUG, "\tMAC: %X:%X:%X:%X:%X:%X\n", lldp_port->source_mac[0]
                                                            , lldp_port->source_mac[1]
                                                            , lldp_port->source_mac[2] , lldp_port->source_mac[3]
                                                            , lldp_port->source_mac[4]
                                                            , lldp_port->source_mac[5]); 
        lldp_printf(MSG_DEBUG, "\tIP: %d.%d.%d.%d\n", lldp_port->source_ipaddr[0]
                                                     , lldp_port->source_ipaddr[1]
                                                     , lldp_port->source_ipaddr[2]
                                                     , lldp_port->source_ipaddr[3]);
        lldp_port = lldp_port->next;
    }
#endif
}


void parse_args(int argc, char **argv)
{
	int exitcode = 0;
	int fork = 0;
	const char *log_file = NULL;
	char c;

	for (;;) {
		c = getopt(argc, argv, "dhqf:stxi:");
		if (c < 0)
		  break;
		switch (c) {
			case 'i':
				iface_filter = 1;
				memcpy(iface_list, optarg, strlen(optarg));
				iface_list[LLDP_IF_NAMESIZE - 1] = '\0';
				lldp_printf(MSG_INFO, "Using interface %s\n", iface_list);
				break;
			case 'x':
				fork = 0;
				break;
			case 'h':
				usage();
				exitcode = -1;
				break;
			case 'd':
				if (lldp_debug_level > 0)
				  lldp_debug_level--;
				break;
			case 'f':
				log_file = optarg;
				break;
			case 'q':
				lldp_debug_level++;
				break;
#ifdef CONFIG_DEBUG_SYSLOG
			case 's':
				lldp_debug_open_syslog();
				break;
#endif /* CONFIG_DEBUG_SYSLOG */
			case 't':
				lldp_debug_timestamp++;
				break;
			default:
				usage();
				exitcode = -1;
				break;
		}
	}

	if (log_file)
	  lldp_debug_open_file(log_file);
	else
	  lldp_debug_setup_stdout();

	if (exitcode == -1)
		exit(EXIT_SUCCESS);
	else
		return;
	
}

void thread_rx_sm(void *ptr)
{
	int socket_width = 0;
    struct timeval timeout;
    struct timeval un_timeout;
    time_t current_time = 0;
    time_t last_check = 0;
    int result = 0;
	struct lldp_port *lldp_port = NULL;
	pthread_t thread_tx_id;

	struct eth_hdr expect_hdr, *ether_hdr;

	fd_set readfds;

	expect_hdr.ethertype = htons(0x88cc);
	while (1) {
		FD_ZERO(&readfds);	/* setup select() */

		lldp_port = lldp_ports;

		while (lldp_port) {

			/* This is not the interface you are looking for ... */
			if (lldp_port->if_name == NULL) {
				lldp_printf(MSG_ERROR, "[%s %d][ERROR] Interface index %d with name  is NULL \n", __FUNCTION__, __LINE__, lldp_port->if_index);
				continue; /* Error, interface index %d with name %s is NULL */
			}

			FD_SET(lldp_port->socket, &readfds);

			if (lldp_port->socket > socket_width)
			  socket_width = lldp_port->socket;

			lldp_port = lldp_port->next;
		}

		time(&current_time);

		/* tell select how long to wait for ... */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		/* Timeout after 1 second if nothing is ready */
		result = select(socket_width + 1, &readfds, NULL, NULL, &timeout);

		/* process the sockets */
		lldp_port = lldp_ports;

		while (lldp_port != NULL) {
			/* This is not the interface you are looking for ... */
			if (lldp_port->if_name == NULL) {
				lldp_printf(MSG_ERROR, "[%s %d][ERROR] Interface index %d with name  is NULL \n", 
							__FUNCTION__, __LINE__, lldp_port->if_index);
				continue; /* Error, interface index %d with name %s is NULL */
			}

			//lldp_printf(MSG_DEBUG, "[%s %d] ifname %s, result %u\n", 
			//			__FUNCTION__, __LINE__, lldp_port->if_name, result);
			if (result > 0) {
				if (FD_ISSET(lldp_port->socket, &readfds)) {
					//lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] %s is readable, recvsize %d\n", 
					//			__FUNCTION__, __LINE__, lldp_port->if_name, result);

					lldp_read(lldp_port);

					ether_hdr = (struct eth_hdr *)lldp_port->rx.frame;

					if (ether_hdr->ethertype != expect_hdr.ethertype)
						continue;

					if (lldp_port->rx.recvsize <= 0) {
						if (errno != EAGAIN && errno != ENETDOWN)
						  printf("Error: (%d): %s (%s:%d)\n", 
									  errno, strerror(errno),
									  __FUNCTION__, __LINE__);
					} else {
						/* Got an LLDP Frame %d bytes  long on %s */
						//lldp_printf(MSG_INFO, "[%s %d][INFO] Got an LLDP frame %d bytes long on %s\n", 
						//			__FUNCTION__, __LINE__, lldp_port->rx.recvsize, lldp_port->if_name);

						//lldp_debug_hex_dump(MSG_DEBUG, lldp_port->rx.frame, lldp_port->rx.recvsize);

						/* Mark that we received a frame so the rx state machine can process it. */
						lldp_port->rx.rcvFrame = 1;

						show_lldp_pdu(lldp_port->rx.frame, lldp_port->rx.recvsize);
						//rxStatemachineRun(lldp_port);
					}
				}
			} /* end result > 0 */


			if ((result == 0) || (current_time > last_check)) {
				lldp_port->tick = 1;

				//txStatemachineRun(lldp_port); 
				//rxStatemachineRun(lldp_port);

				lldp_port->tick = 0;
			}


			if(result < 0) {
				if(errno != EINTR) {
					lldp_printf(MSG_ERROR, "[%s %d][ERROR] %s\n", __FUNCTION__, __LINE__, strerror(errno));
				}
			}

			lldp_port = lldp_port->next;
		}	/* end while(lldp_port != NULL) */

		time(&last_check);

	}
}

void thread_tx_sm(void *ptr)
{
	struct lldp_port  *lldp_port;

	while (1) {

		printf("Thread TX SM run...\n");
		lldp_port = lldp_ports;

		while (lldp_port) {
			txStatemachineRun(lldp_port);
			lldp_port = lldp_port->next;
		}

		sleep(1);
	}
}

int main(int argc, char **argv)
{
    uid_t uid;
    struct timeval timeout;
    struct timeval un_timeout;
   	int fork = 0;
	const char *log_file = NULL;
	char c;

	int op = 0;
    int socket_width = 0;
    time_t current_time = 0;
    time_t last_check = 0;
    int result = 0;
	struct lldp_port *lldp_port = NULL;
	pthread_t thread_tx_id, thread_rx_id;

	fd_set readfds;
    fd_set unixfds;
	
	program = argv[0];

	parse_args(argc, argv);

	get_sys_desc();
	get_sys_fqdn();

	// get uid of user executing program. 
    uid = getuid();
	if (uid != 0) {
        lldp_printf(MSG_ERROR, "You must be running as root to run %s!\n", program);
        exit(0);
    }
	lldp_printf(MSG_INFO, "[%s %d] Program %s uid %d\n", __FUNCTION__, __LINE__, argv[0], uid);
	
	if (initialize_lldp() == 0)
		lldp_printf(MSG_WARNING, "[%s %d] no interface found to listen on\n", __FUNCTION__, __LINE__);
	
	if (fork) {
		if (daemon(0, 0) != 0)
			/* unable to daemonize */
			lldp_printf(MSG_WARNING, "[%s %d] Unable to daemonize\n", __FUNCTION__, __LINE__);
	}
	
	walk_port_list();

	pthread_create(&thread_tx_id, NULL, thread_tx_sm, NULL);
	pthread_create(&thread_rx_id, NULL, thread_rx_sm, NULL);
	pthread_join(thread_tx_id, NULL);
	pthread_join(thread_rx_id, NULL);

#if 0
	while (1) {
		FD_ZERO(&readfds);	/* setup select() */

		lldp_port = lldp_ports;
		
		while (lldp_port) {

			/* This is not the interface you are looking for ... */
			if (lldp_port->if_name == NULL) {
				lldp_printf(MSG_ERROR, "[%s %d][ERROR] Interface index %d with name  is NULL \n", __FUNCTION__, __LINE__, lldp_port->if_index);
				continue; /* Error, interface index %d with name %s is NULL */
			}
			
			FD_SET(lldp_port->socket, &readfds);

			if (lldp_port->socket > socket_width)
				 socket_width = lldp_port->socket;
			
			lldp_port = lldp_port->next;
		}
		
		time(&current_time);
		
		/* tell select how long to wait for ... */
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		
		/* Timeout after 1 second if nothing is ready */
		result = select(socket_width + 1, &readfds, NULL, NULL, &timeout);
		
		/* process the sockets */
		lldp_port = lldp_ports;
		
		while (lldp_port != NULL) {
			/* This is not the interface you are looking for ... */
			if (lldp_port->if_name == NULL) {
				lldp_printf(MSG_ERROR, "[%s %d][ERROR] Interface index %d with name  is NULL \n", 
							__FUNCTION__, __LINE__, lldp_port->if_index);
				continue; /* Error, interface index %d with name %s is NULL */
			}
			
			lldp_printf(MSG_DEBUG, "[%s %d] ifname %s, result %u\n", 
						__FUNCTION__, __LINE__, lldp_port->if_name, result);
			if (result > 0) {
				if (FD_ISSET(lldp_port->socket, &readfds)) {
                    			lldp_printf(MSG_DEBUG, "[%s %d][DEBUG] %s is readable!\n", 
									__FUNCTION__, __LINE__, lldp_port->if_name);

					lldp_read(lldp_port);
					
					if (lldp_port->rx.recvsize <= 0) {
						if (errno != EAGAIN && errno != ENETDOWN)
							printf("Error: (%d): %s (%s:%d)\n", 
								errno, strerror(errno),
								__FUNCTION__, __LINE__);
					} else {
						/* Got an LLDP Frame %d bytes  long on %s */
						lldp_printf(MSG_INFO, "[%s %d][INFO] Got an LLDP frame %d bytes long on %s\n", 
									__FUNCTION__, __LINE__, lldp_port->rx.recvsize, lldp_port->if_name);

					 //lldp_debug_hex_dump(MSG_DEBUG, lldp_port->rx.frame, lldp_port->rx.recvsize);

					/* Mark that we received a frame so the rx state machine can process it. */
						lldp_port->rx.rcvFrame = 1;
						
						//rxStatemachineRun(lldp_port);
					}
				}
			} /* end result > 0 */
			

			if ((result == 0) || (current_time > last_check)) {
				lldp_port->tick = 1;
				
				txStatemachineRun(lldp_port); 
                //rxStatemachineRun(lldp_port);

				lldp_port->tick = 0;
			}


			if(result < 0) {
				if(errno != EINTR) {
					lldp_printf(MSG_ERROR, "[%s %d][ERROR] %s\n", __FUNCTION__, __LINE__, strerror(errno));
				}
			}

			lldp_port = lldp_port->next;
		}	/* end while(lldp_port != NULL) */

		time(&last_check);
	
	}	/* end while(1) */
	
#endif
out:	

	return 0;	
}

void cleanupLLDP(struct lldp_port *lldp_port)
{
	lldp_port = lldp_ports;
	lldp_printf(MSG_INFO, "[%s %d][INFO] Recv signal, cleanup lldp resourses\n", __FUNCTION__, __LINE__);
	while (lldp_port != NULL) {
		if (lldp_port->if_name != NULL) {
			tlvCleanupLLDP(lldp_port);
			socketCleanupLLDP(lldp_port);
		} else {
			lldp_printf(MSG_INFO, "[%s %d][ERROR] Interface with name is NULL\n", __FUNCTION__, __LINE__);
			/* Error interface with name %s is NULL */
		}
		
		lldp_port = lldp_port->next;
		
		/* clean the previous node and move up */
		free(lldp_ports);
		lldp_ports = lldp_port;
	}
	
	exit(0);
}

int initialize_lldp()
{
	int if_index = 0;
	char if_name[LLDP_IF_NAMESIZE];
	struct lldp_port *lldp_port = NULL;
	int nb_ifaces = 0;
	
	/*
	 * we need to initialize an LLDP port-per interface
	 * "lldp_port" will be changed to point at the interface currently being
	 * serviced
	 */
	for (if_index = MIN_INTERFACES; if_index < MAX_INTERFACES; if_index++) {
		if (if_indextoname(if_index, if_name) == NULL)
			continue;
		
		/* keep only the interface specified by -i option */
		if (iface_filter) {
			if (strncmp(if_name, (const char *)iface_list, LLDP_IF_NAMESIZE) != 0) {
				lldp_printf(MSG_INFO, "[%s %d]Skipping interface %s (not %s)\n",
							__FUNCTION__, __LINE__, if_name, iface_list);
				continue;	/* skipping interface */
			}
		}
		lldp_printf(MSG_INFO, "[%s %d] interface[%d] name %s\n", __FUNCTION__, __LINE__, if_index, if_name);

#if 1
		/* we do not process the lo interface */
		if (strstr(if_name, "lo") != NULL)
			continue;

		if (strncmp(if_name, "vmnet", 6) == 0)
			continue;
#endif
		/* create new interface struct */
		lldp_port = malloc(sizeof(struct lldp_port));
		memset(lldp_port, 0x0, sizeof(struct lldp_port));
		
		/* add it to the global list */
		lldp_port->next = lldp_ports;
		lldp_printf(MSG_DEBUG, "[%s %d] add this interface %s to the global port list \n", __FUNCTION__, __LINE__, if_name);
		
		lldp_port->if_index = if_index;
		lldp_port->if_name = malloc(LLDP_IF_NAMESIZE);
		if (lldp_port->if_name == NULL) {
			free(lldp_port);
			lldp_port = NULL;
			continue;
		}
		
		memcpy(lldp_port->if_name, if_name, LLDP_IF_NAMESIZE);
		
		lldp_printf(MSG_INFO, "[%s %d] %s (index %d) found. Initializing...\n",
					__FUNCTION__, __LINE__, 
					lldp_port->if_name, lldp_port->if_index);

		lldp_port->portEnabled = 1;
		
		/* initialize the socket for this interface */
		if (lldp_init_socket(lldp_port) != 0) {
			lldp_printf(MSG_ERROR, "[%s %d][ERROR] Problem initialize socket for this interface\n", __FUNCTION__, __LINE__);
			free(lldp_port->if_name);
			lldp_port->if_name = NULL;
			free(lldp_port);
			lldp_port = NULL;
			continue;
		} else {
			lldp_printf(MSG_INFO, "[%s %d]Finished initializing socket for index %d with name %s\n", 
						__FUNCTION__, __LINE__, 
						lldp_port->if_index, lldp_port->if_name);
		}
		
		nb_ifaces++;

		lldp_printf(MSG_INFO, "[%s %d]Initializing TX SM for index %d with name %s\n", 
					__FUNCTION__, __LINE__, 
					lldp_port->if_index, lldp_port->if_name);
		lldp_port->tx.state = TX_LLDP_INITIALIZE;
		txInitializeLLDP(lldp_port);

#if 0
		lldp_printf(MSG_INFO, "[%s %d]Initializing RX SM for index %d with name %s\n", 
					__FUNCTION__, __LINE__, 
					lldp_port->if_index, lldp_port->if_name);
		lldp_port->rx.state = LLDP_WAIT_PORT_OPERATIONAL;
		rxInitializeLLDP(lldp_port);
#endif
		lldp_port->portEnabled = 0;
		lldp_port->adminStatus = enabledRxTx;
		
		lldp_printf(MSG_INFO, "[%s %d]Initializing TLV subsystem for index %d with name %s\n", 
					__FUNCTION__, __LINE__, 
					lldp_port->if_index, lldp_port->if_name);
		/* Initialize the TLV subsystem for this interface */
		tlvInitializeLLDP(lldp_port);
		
		lldp_printf(MSG_DEBUG, "[%s %d] To send out the first lldp frame\n", __FUNCTION__, __LINE__);
		/*  send out the first lldp frame
		 *	This allows other devices to see us right when we come up,
		 *  rather than having to wait for a full timer cycle.
		 */
		//walk_port_list();
		txChangeToState(lldp_port, TX_IDLE);
		mibConstrInfoLLDPDU(lldp_port);
		txFrame(lldp_port);
		lldp_ports = lldp_port;
	}
	
	/* Don't forget to initialize the TLV validators... */
    //initializeTLVFunctionValidators();

	signal(SIGTERM, cleanupLLDP);
	signal(SIGINT, cleanupLLDP);
	signal(SIGQUIT, cleanupLLDP);
	signal(SIGSEGV, handle_segfault);
	signal(SIGHUP, handle_hup);

	return nb_ifaces;
}

/***************************************
 *
 * Trap a segfault, and exit cleanly.
 *
 ***************************************/
void handle_segfault()
{
    fprintf(stderr, "[FATAL] SIGSEGV  (Segmentation Fault)!\n");

    fflush(stderr); fflush(stdout);
    exit(-1);
}

/***************************************
 *
 * Trap a HUP, and read location config file new.
 *
 ***************************************/
void handle_hup()
{
#ifdef USE_CONFUSE
  lldp_printf(DEBUG_NORMAL, "[INFO] SIGHUP-> read config file again!\n");
  lci_config();
#endif // USE_CONFUSE
}


static void usage(void)
{
	printf("options:\n"
			"  -d increase debugging verbosity (-dd even more)\n"
			"  -q decreate debugging verbosity (-qq even less)\n"
			"  -h show this help text\n"
			"  -t include timestamp in debug messages\n"
			"  -s log output to syslog instead of stdout\n"
			"  -f log output to debug file instead of stdout\n");
}
