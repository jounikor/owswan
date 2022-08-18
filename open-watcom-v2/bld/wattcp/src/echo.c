/*
 * echo.c : A simple echo/discard daemon. Listens for traffic on
 *          udp/tcp ports 7 and 9.
 *
 * G. Vanem, Oct-2000
 */

#include <stdio.h>
#include <stdlib.h>

#include "wattcp.h"
#include "wattcpd.h"
#include "strings.h"
#include "pcconfig.h"
#include "pctcp.h"
#include "pcbsd.h"
#include "pcdbug.h"
#include "echo.h"

#if defined(USE_ECHO_DISC)

static udp_Socket udp_echo_sk, udp_disc_sk;
static tcp_Socket tcp_echo_sk, tcp_disc_sk;

static BOOL  do_echo   = 0;
static WORD  echo_port = 7;
static DWORD echo_host = 0;

static BOOL  do_disc   = 0;
static WORD  disc_port = 9;
static DWORD disc_host = 0;

static void echo_discard_daemon (void);

static void (*prev_hook) (const char *, const char *) = NULL;

/*
 * Parse and match "echo.daemon = 0/1", "echo.port = <n>" etc.
 */
static void echo_config (const char *name, const char *value)
{
    static struct config_table echo_cfg[] = {
            { "DAEMON", ARG_ATOI,    &do_echo   },
            { "HOST",   ARG_RESOLVE, &echo_host },
            { "PORT",   ARG_ATOI,    &echo_port },
            { NULL }
        };
    static struct config_table disc_cfg[] = {
            { "DAEMON", ARG_ATOI,    &do_disc   },
            { "HOST",   ARG_RESOLVE, &disc_host },
            { "PORT",   ARG_ATOI,    &disc_port },
            { NULL }
        };

    if (!parse_config_table(&echo_cfg[0], "ECHO.", name, value) &&
      !parse_config_table(&disc_cfg[0], "DISCARD.", name, value))
    {
        if (prev_hook) {
            (*prev_hook) (name, value);
        }
    }
}

/*
 * callback handler for echo + discard UDP sockets.
 */
static int udp_handler (sock_type *sk, BYTE *data, int len, tcp_PseudoHeader *tcp_phdr, udp_Header *udp_hdr)
{
    ARGSUSED (tcp_phdr); ARGSUSED (udp_hdr);

    if (&sk->udp == &udp_echo_sk) {
        if (!sock_enqueue (sk, data, len)) {
            sock_close (sk);
        }
    } else {
        /* discard packet */
    }
    return (1);
}

/*
 * Called from sock_init(): Setup config-file parser for "echo..."
 * and "discard.." keywords.
 */
void echo_discard_init (void)
{
    prev_hook = usr_init;
    usr_init  = echo_config;
}

void echo_discard_start (void)
{
#if defined(USE_DEBUG)
    char buf[100], ip1[20], ip2[20];
    sprintf (buf, "echo-daemon %s, %s/%u, discard-daemon %s, %s/%u\r\n\r\n",
           do_echo ? "yes" : "no", _inet_ntoa(ip1, echo_host), echo_port,
           do_disc ? "yes" : "no", _inet_ntoa(ip2, disc_host), disc_port);
    dbug_write_raw (buf);
#endif

    if (do_echo) {
        udp_listen (&udp_echo_sk, echo_host, echo_port, 0, udp_handler);
        SETON_SOCKMODE(udp_echo_sk, UDP_MODE_NOCHK);
        tcp_listen (&tcp_echo_sk, echo_host, echo_port, 0, NULL, 0);
    }

    if (do_disc) {
        udp_listen (&udp_disc_sk, disc_host, disc_port, 0, udp_handler);
        SETON_SOCKMODE(udp_disc_sk, UDP_MODE_NOCHK);
        tcp_listen (&tcp_disc_sk, disc_host, disc_port, 0, NULL, 0);
    }

    if (do_echo || do_disc) {
        addwattcpd (echo_discard_daemon);
    }
}

/*
 * "background" process handling echo + discard TCP sockets.
 */
static void echo_discard_daemon (void)
{
    sock_type *sk = (sock_type*) &tcp_echo_sk;

    if (sock_dataready (sk)) {
        BYTE buf[ETH_MAX_DATA];
        int  len = sock_read (sk, buf, sizeof(buf));
        sock_write (sk, buf, len);
    }
}
#endif

