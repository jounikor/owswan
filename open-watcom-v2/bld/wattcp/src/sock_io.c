/*
 *  SOCK_IO: some sock_xx() functions moved from pctcp.c due
 *           to memory contraints under Turbo-C
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "copyrigh.h"
#include "wattcp.h"
#include "pctcp.h"

/*
 * sock_putc - put a character
 *    - no expansion but flushes on CR/LF
 *    - returns character
 */
BYTE sock_putc (sock_type *sk, BYTE c)
{
#if !defined(USE_UDP_ONLY)
    if (c == '\n' || c == '\r')
        sock_flushnext (sk);
#endif
    sock_write (sk, &c, 1);
    return (c);
}


/*
 * sock_puts - does not append carriage return in binary mode
 *           - returns length
 */
int sock_puts (sock_type *sk, const BYTE *data)
{
    int len;

#if defined(USE_BSD_FUNC)
    if (sk->u.ip_type == IP_TYPE)
        return (0);   /* not supported yet */
#endif

    len = strlen ((const char*)data);

    if (ISON_SOCKMODE(sk->u, TCP_MODE_ASCII)) { /* udp/tcp ASCII mode */
        if (sk->u.ip_type == TCP_PROTO)
            SETON_SOCKMODE(sk->tcp, TCP_MODE_LOCAL);

#if !defined(USE_UDP_ONLY)
        sock_noflush (sk);
#endif

        if (len)
            len = sock_write (sk, data, len);

#if !defined(USE_UDP_ONLY)
        sock_flushnext (sk);
#endif
        sock_write (sk, (const BYTE*)"\r\n", 2);
    } else {
#if !defined(USE_UDP_ONLY)
        sock_flushnext (sk);
#endif
        len = sock_write (sk, data, len);
    }
    return (len);
}


/*
 * sock_gets - read a string from any socket
 *    - return length of returned string
 *    - removes end of line terminator(s)
 *    - Quentin Smart and mark Phillips fixed some problems
 */
int sock_gets (sock_type *sk, BYTE *data, int n)
{
    int   len, rmax, *np, frag = 0;
    int   is_tcp = 0;
    BYTE *src_p, *nl_p, *cr_p;

#if defined(USE_BSD_FUNC)
    if (sk->u.ip_type == IP_TYPE)
        return (0);   /* not supported yet */
#endif

    /* Access the buffer pointer and length.
     */
    if (sk->u.ip_type == UDP_PROTO) {
        src_p = sk->udp.rx_data;
        np    = &sk->udp.rx_datalen;
        rmax  = (int) sk->udp.rx_maxdatalen;
#if !defined(USE_UDP_ONLY)
    } else {
        is_tcp = 1;
        src_p  = sk->tcp.rx_data;
        np     = &sk->tcp.rx_datalen;
        rmax   = (int) sk->tcp.rx_maxdatalen;
        if (sk->tcp.missed_seg[0]) {
            long ldiff = sk->tcp.missed_seg[1] - sk->tcp.acknum;
            frag = abs ((int)ldiff);
        }
#endif
    }

    if (ISON_SOCKMODE(sk->tcp, TCP_MODE_SAWCR)) {
        SETOFF_SOCKMODE(sk->tcp, TCP_MODE_SAWCR);
        if (*np && (*src_p == '\n' || *src_p == '\0')) {
            movmem (src_p + 1, src_p, frag + (*np)--);
        }
    }

    /* Return if there is nothing in the buffer.
     */
    if (*np == 0)
        return (0);

    /* If there is space for all the data, then copy all of it,
     * otherwise, only copy what the space will allow (taking
     * care to reserve space for the null terminator.
     */
    if (--n > *np)
        n = *np;
    memcpy (data, src_p, n);        /* copy everything */
    data[n] = 0;                    /* terminate new string */

    /* At this point, data is a null-terminated string,
     * containing as much of the data as is possible.
     */
    len = n;

    /* Because we are in ASCII mode, we assume that the
     * sender will be consistent in which kind of CRLF is
     * sent (e.g. one and only one of \r\n, \r0, \r, or \n).
     * So if both \r and \n are found, we assume that they
     * are always next to each other, and \n\r is invalid.
     */

    /* Zero the first occurance of \r and \n in data.
     */
    cr_p = memchr (data, '\r', n);
    if (cr_p)
        *cr_p = 0;
    nl_p = memchr (data, '\n', n);
    if (nl_p)
        *nl_p = 0;

    /* Return if we did not find \r or \n yet, but still had room,
     * *and* the connection can get more data!
     */
    if (is_tcp && !cr_p && !nl_p && (n > *np) && (*np < rmax) &&
      sk->tcp.state != tcp_StateLASTACK && sk->tcp.state != tcp_StateCLOSED)
    {
        *data = 0;
        return (0);
    }

    /* If we did find a terminator, then stop there.
    */
    if (cr_p || nl_p) {
        /* Find the length of the first line of data in data.
         */
        len = (int) (nl_p == NULL ? (char*)cr_p - (char*)data :
                 cr_p == NULL ? (char*)nl_p - (char*)data :
                 nl_p < cr_p  ? (char*)nl_p - (char*)data :
                                (char*)cr_p - (char*)data);

        /* We found a terminator character ...
         */
        n = len + 1;

        /* If \r at end of data, might get a \0 or \n in next packet
         */
        if (cr_p && (*np == n))
            SETON_SOCKMODE(sk->tcp, TCP_MODE_SAWCR);

        /* ... and it could have been \r\0 or \r\n.
         */
        if ((*np > n) && (!src_p[n] || (cr_p && src_p[n] == '\n'))) {
            n++;
        }
    }

    /* Remove the first line from the buffer.
     */
    *np -= n;
    if (frag || *np > 0)
        movmem (src_p + n, src_p, frag + *np);

    /* update window if less than MSS/2 free in receive buffer
     */
#if !defined(USE_UDP_ONLY)
    if (sk->u.ip_type == TCP_PROTO && sk->tcp.state != tcp_StateCLOSED) {
        if((sk->tcp.rx_maxdatalen - sk->tcp.rx_datalen) < (sk->tcp.max_seg / 2)) {
            TCP_SENDSOON (sk);
        }
    }
#endif

    return (len);
}

WORD sock_getc (sock_type *sk)
{
    BYTE ch = 0;
    return sock_read (sk, &ch, 1) < 1 ? EOF : ch;
}

/*
 * sock_dataready - returns number of bytes waiting to be read.
 *    - if in ASCII mode, return >0 if a terminated line is present or
 *      the buffer is full or state is LASTACK (tcp only).
 *    - if in binary mode (default), simply return # of bytes in
 *      receive buffer.
 */
WORD sock_dataready (sock_type *sk)
{
    char *p;
    int  len = sk->tcp.rx_datalen;

#if defined(USE_BSD_FUNC)
    if (sk->u.ip_type == IP_TYPE)
        return (0);   /* not supported yet */
#endif

    if (len && ISON_SOCKMODE(sk->tcp, TCP_MODE_ASCII)) {
        p = (char*)sk->tcp.rx_data;

        if (ISON_SOCKMODE(sk->tcp, TCP_MODE_SAWCR)) { /* !! S. Lawson */
            SETOFF_SOCKMODE(sk->tcp, TCP_MODE_SAWCR);
            if (*p == '\n' || *p == '\0') {
                sk->tcp.rx_datalen = --len;
                movmem (p+1, p, sk->tcp.rx_datalen);
                if (!len)
                    return (0);
            }
        }

        if (len == sk->tcp.rx_maxdatalen)
            return (len);

        if (sk->u.ip_type == TCP_PROTO) {        /* GV 99.12.02 */
            if (sk->tcp.state == tcp_StateLASTACK) /* EE 99.07.02 */
                return (len);

            /* S. Lawson - happens if final ACK arrives before app reads data
             */
            if (sk->tcp.state == tcp_StateCLOSED) {
                return (len);
            }
        }

        /* check for terminating `\r' and/or `\n'
         */
        if (memchr(p, '\r', len))
            return (len);
        if (memchr(p, '\n', len))              /* EE added 99/04/30 */
            return (len);
        return (0);
    }
    return (len);
}

