/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2002-2021 The Open Watcom Contributors. All Rights Reserved.
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description: TCP/IP transport link for trap files.
*
****************************************************************************/


#define LIST_INTERFACES

#if defined( __OS2__ ) && defined( _M_I86 )
#define OS2
#define _TCP_ENTRY __cdecl __far
#define BSD_SELECT
#endif

#if defined ( __NETWARE__ )
#define __FUNCTION_DATA_ACCESS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if defined ( __RDOS__ )
    #include "rdos.h"
#elif defined( __NT__ ) || defined( __WINDOWS__ )
    #include <winsock.h>
#elif defined ( __NETWARE__ )
    #include <sys/types.h>
    #include <unistd.h>
#else
  #if defined( __OS2__ )
    #include <types.h>
  #else
    #include <sys/types.h>
  #endif
    #include <unistd.h>
    #include <sys/socket.h>
  #if !defined ( __LINUX__ )
    #include <sys/select.h>
  #endif
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netdb.h>
  #if defined( __OS2__ )
    #if defined( _M_I86 )
        #include <netlib.h>
    #else
        #include <arpa/inet.h>
    #endif
    #include <sys/ioctl.h>
    #include <net/if.h>
  #elif defined( __DOS__ )
    #include <arpa/inet.h>
    #define BSD         /* To get the right macros from wattcp headers. */
    #include <sys/ioctl.h>
    #undef BSD
    #include <net/if.h>
    #include <tcp.h>
  #elif defined( __UNIX__ )
    #include <arpa/inet.h>
  #endif
#endif

#include "watcom.h"
#if defined ( __NETWARE__ )
    #include "novhax.h"
#endif

#if defined ( __NETWARE__ )
    #include "debugme.h"
#endif

#include "trptypes.h"
#if !defined( SERVER ) && defined( __NT__ )
    #include "trpimp.h"
#endif
#include "trperr.h"
#include "packet.h"
#ifdef LIST_INTERFACES
// TODO: need rework to POSIX if_nameindex in <net/if.h>
#include "ifi.h"
#endif
#if defined( GUISERVER )
    #include <wwindows.h>
    #include "servio.h"
    #include "options.h"
#endif


#if !defined ( __NETWARE__ )
    #define _DBG_THREAD( x )
    #define _DBG_DR( x )
    #define _DBG_EVENT( x )
    #define _DBG_IO( x )
    #define _DBG_MISC( x )
    #define _DBG_IPX( x )
    #define _DBG_NET( x )
    #define _DBG_REQ( x )
    #define _DBG_ERROR( x )
#endif

#define DEFAULT_PORT    0x0DEB  /* 3563 */

#if defined( __RDOS__ )
    #define INVALID_SOCKET          0
    #define IS_VALID_SOCKET(x)      (x!=INVALID_SOCKET && !RdosIsTcpConnectionClosed(x))
    #define IS_RET_OK(x)            (x!=0)
    #define trp_socket              int
    #define soclose( s )            RdosCloseTcpConnection( s )
#elif defined( __NT__ ) || defined( __WINDOWS__ )
    #define IS_VALID_SOCKET(x)      (x!=INVALID_SOCKET)
    #define IS_RET_OK(x)            (x!=SOCKET_ERROR)
    #define trp_socket              SOCKET
    #define trp_socklen             int
    #define soclose( s )            closesocket( s )
#elif defined( __DOS__ )
    #define IS_VALID_SOCKET(x)      (x>=0)
    #define IS_RET_OK(x)            (x!=-1)
    #define trp_socket              int
    #define trp_socklen             int
#elif defined( __OS2__ )
    #define INVALID_SOCKET          -1
    #define IS_VALID_SOCKET(x)      (x>=0)
    #define IS_RET_OK(x)            (x!=-1)
    #define trp_socket              int
    #define trp_socklen             int
#else   /* POSIX */
    #define INVALID_SOCKET          -1
    #define IS_VALID_SOCKET(x)      (x>=0)
    #define IS_RET_OK(x)            (x!=-1)
    #define trp_socket              int
    #define trp_socklen             socklen_t
    #define soclose( s )            close( s )
#endif
#if !defined( __NT__ ) && !defined( __WINDOWS__ )
    typedef struct sockaddr         *LPSOCKADDR;
#endif

#ifdef __RDOS__

    #define SOCKET_BUFFER   0x7000

static int recv( int handle, void *buf, int size, int timeout )
{
    int count;

    /* unused parameters */ (void)timeout;

    count = 0;
    while( !RdosIsTcpConnectionClosed( handle ) && count == 0 ) {
        count = RdosReadTcpConnection( handle, buf, size );
    }
    return( count );
}

static int send( int handle, const void *buf, int size, int timeout )
{
    int count;

    /* unused parameters */ (void)timeout;

    count = RdosWriteTcpConnection( handle, buf, size);
    return( count );
}

#else

  #ifndef IPPROTO_TCP
    #define IPPROTO_TCP 6
  #endif

    static struct sockaddr_in   socket_address;

#endif

#ifdef SERVER
  #ifdef __RDOS__
    static int                  listen_handle = 0;
    static int                  wait_handle = 0;
  #else
    static trp_socket           control_socket;
  #endif
#ifdef LIST_INTERFACES
// TODO: need rework to POSIX if_nameindex in <net/if.h>
    static struct ifi_info      *get_ifi_info( int, int );
    static void                 free_ifi_info( struct ifi_info * );
#endif
#endif

static trp_socket           data_socket = INVALID_SOCKET;

#if defined( SERVER )
extern void     ServMessage( const char * );
#endif

#if !defined( SERVER )
static bool terminate_connection( void )
{
#ifdef __RDOS__
    RdosPushTcpConnection( data_socket );
#else
    struct linger       linger;

    linger.l_onoff = 1;
    linger.l_linger = 0;
    setsockopt( data_socket, (int)SOL_SOCKET, SO_LINGER, (void*)&linger, sizeof( linger ) );
#endif
    soclose( data_socket );
    data_socket = INVALID_SOCKET;
    return( true );
}

#endif

#ifdef __RDOS__

#define recvData(buf,len)   recv(data_socket,buf,len,0)

#else

static int recvData( void *get, int len )
{
    int     rec, got;

    got = len;
    while( len > 0 ) {
        rec = recv( data_socket, get, len, 0 );
        if( !IS_RET_OK( rec ) )
            return( -1 );
        get = (char *)get + rec;
        len -= rec;
    }
    return( got );
}

#endif

trap_retval RemoteGet( void *data, trap_elen len )
{
    unsigned_16         rec_len;

    len = len;

    _DBG_NET(("RemoteGet\r\n"));

    if( IS_VALID_SOCKET( data_socket ) ) {
        int     size;

        size = recvData( &rec_len, sizeof( rec_len ) );
#ifdef __RDOS__
        while( size == 0 ) {
            if( !IS_VALID_SOCKET( data_socket ) )
                return( REQUEST_FAILED );
            size = recvData( &rec_len, sizeof( rec_len ) );
        }
#endif
        if( size == sizeof( rec_len ) ) {
            CONV_LE_16( rec_len );
#ifdef __RDOS__
            if( rec_len && recvData( data, rec_len ) == rec_len ) {
#else
            if( rec_len == 0 || recvData( data, rec_len ) == rec_len ) {
#endif
                _DBG_NET(("Got a packet - size=%d\r\n", rec_len));
                return( rec_len );
            }
        }
    }
    return( REQUEST_FAILED );
}

trap_retval RemotePut( void *data, trap_elen len )
{
    unsigned_16         send_len;
    int                 snd;

    _DBG_NET(("RemotePut\r\n"));

    if( IS_VALID_SOCKET( data_socket ) ) {
        send_len = len;
        CONV_LE_16( send_len );
        snd = send( data_socket, (void *)&send_len, sizeof( send_len ), 0 );
        if( IS_RET_OK( snd ) ) {
            if( len != 0 )
                snd = send( data_socket, data, len, 0 );
            if( len == 0 || IS_RET_OK( snd ) ) {
#ifdef __RDOS__
                RdosPushTcpConnection( data_socket );
#endif
                _DBG_NET(("RemotePut...OK\r\n"));
                return( len );
            }
        }
    }
    return( REQUEST_FAILED );
}

#ifndef __RDOS__

static int get_proto( const char *p )
{
    struct protoent     *proto;

    proto = getprotobyname( p );
    return( ( proto != NULL ) ? proto->p_proto : IPPROTO_TCP );
}

static void nodelay( void )
{
    int                 delayoff;

    delayoff = 1;
    setsockopt( data_socket, get_proto( "tcp" ), TCP_NODELAY, (void *)&delayoff, sizeof( delayoff ) );
}

#endif

bool RemoteConnect( void )
{
#ifdef SERVER
  #ifdef __RDOS__
    void *obj;

    obj = (void *)RdosWaitTimeout( wait_handle, 250 );
    if( obj != NULL ) {
        data_socket = RdosGetTcpListen( listen_handle );
        if( IS_VALID_SOCKET( data_socket ) ) {
            _DBG_NET(("Found a connection\r\n"));
            return( true );
        }
    }
  #else
    struct timeval  timeout;
    fd_set          ready;
    struct sockaddr dummy;
    trp_socklen     dummy_len = sizeof( dummy );
    int             rc;

    FD_ZERO( &ready );
    FD_SET( control_socket, &ready );
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    rc = select( control_socket + 1, &ready, 0, 0, &timeout );
    if( IS_RET_OK( rc ) && rc > 0 ) {
        data_socket = accept( control_socket, &dummy, &dummy_len );
        if( IS_VALID_SOCKET( data_socket ) ) {
            nodelay();
            _DBG_NET(("Found a connection\r\n"));
            return( true );
        }
    }
  #endif
#else
  #ifdef __RDOS__
    // todo: Add code for connect!
  #else
    int         rc;

    data_socket = socket( AF_INET, SOCK_STREAM, 0 );
    if( IS_VALID_SOCKET( data_socket ) ) {
        rc = connect( data_socket, (LPSOCKADDR)&socket_address, sizeof( socket_address ) );
        if( IS_RET_OK( rc ) ) {
            nodelay();
            return( true );
        }
    }
  #endif
#endif
    return( false );
}

void RemoteDisco( void )
{
    _DBG_NET(("RemoteDisco\r\n"));

    if( IS_VALID_SOCKET( data_socket ) ) {
        soclose( data_socket );
        data_socket = INVALID_SOCKET;
    }
}

static unsigned short get_port( const char *p )
{
    unsigned short port;
#ifndef __RDOS__
    struct servent      *sp;

    sp = getservbyname( ( *p == '\0' ) ? "tcplink" : p, "tcp" );
    if( sp != NULL )
        return( ntohs( sp->s_port ) );
#endif
    port = 0;
    while( isdigit( *p ) ) {
        port = port * 10 + ( *p - '0' );
        ++p;
    }
    if( port == 0 )
        port = DEFAULT_PORT;
    return( port );
}

#if !defined( SERVER ) && !defined( __RDOS__ )
static unsigned_32 get_addr( const char *p )
{
    unsigned_32 addr;

    addr = inet_addr( p );
    if( addr == INADDR_NONE ) {
        struct hostent  *hp;

        hp = gethostbyname( p );
        if( hp != NULL ) {
            addr = 0;
            memcpy( &addr, hp->h_addr, hp->h_length );
        } else {
            addr = INADDR_NONE;
        }
    }
    return( addr );
}
#endif

const char *RemoteLink( const char *parms, bool server )
{
    unsigned short      port;

#ifdef SERVER
  #if !defined( __RDOS__ )
    trp_socklen         length;
    char                buff[128];
  #endif

    _DBG_NET(("SERVER: Calling into RemoteLink\r\n"));

  #if defined(__NT__) || defined(__WINDOWS__)
    {
        WSADATA data;

        if( WSAStartup( 0x101, &data ) != 0 ) {
            return( TRP_ERR_unable_to_initialize_TCPIP );
        }
    }
  #endif

    port = get_port( parms );
  #ifdef __RDOS__
    wait_handle = RdosCreateWait( );
    listen_handle = RdosCreateTcpListen( port, 1, SOCKET_BUFFER );
    RdosAddWaitForTcpListen( wait_handle, listen_handle, (int)(&listen_handle) );
  #else
    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if( !IS_VALID_SOCKET( control_socket ) ) {
        return( TRP_ERR_unable_to_open_stream_socket );
    }
   #ifdef GUISERVER
    if( *ServParms == '\0' ) {
        sprintf( ServParms, "%u", port );
    }
   #endif

    /* Name socket using wildcards */
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons( port );
    if( bind( control_socket, (LPSOCKADDR)&socket_address, sizeof( socket_address ) ) ) {
        return( TRP_ERR_unable_to_bind_stream_socket );
    }
    /* Find out assigned port number and print it out */
    length = sizeof( socket_address );
    if( getsockname( control_socket, (LPSOCKADDR)&socket_address, &length ) ) {
        return( TRP_ERR_unable_to_get_socket_name );
    }
    sprintf( buff, "%s%u", TRP_TCP_socket_number, (unsigned)port );
    ServMessage( buff );
    _DBG_NET(("TCP: "));
    _DBG_NET((buff));
    _DBG_NET(("\r\n"));

   #ifdef LIST_INTERFACES
    // TODO: need rework to POSIX if_nameindex in <net/if.h>
    /* Find and print TCP/IP interface addresses, ignore aliases */
    {
        struct ifi_info     *ifi, *ifihead;
        struct sockaddr     *sa;

        ifihead = get_ifi_info( AF_INET, false );
        for( ifi = ifihead; ifi != NULL; ifi = ifi->ifi_next ) {
            /* Ignore loopback interfaces */
            if( ifi->flags & IFI_LOOP )
                continue;
            if( (sa = ifi->ifi_addr) != NULL ) {
                sprintf( buff, "%s%s", TRP_TCP_ip_address,
                    inet_ntoa( ((struct sockaddr_in *)sa)->sin_addr ) );
                ServMessage( buff );
            }
        }
        free_ifi_info( ifihead );
    }
   #endif
  #endif

    _DBG_NET(("Start accepting connections\r\n"));
    /* Start accepting connections */
  #ifndef __RDOS__
    listen( control_socket, 5 );
  #endif
#else
  #ifdef __RDOS__
    // Todo: handle connect
  #else
    char        buff[128];
    char        *p;

    #if defined(__NT__) || defined(__WINDOWS__)
    {
        WSADATA data;

        if( WSAStartup( 0x101, &data ) != 0 ) {
            return( TRP_ERR_unable_to_initialize_TCPIP );
        }
    }
    #endif

    /* get port number out of name */
    p = buff;
    while( *parms != '\0' ) {
        if( *parms == ':' ) {
            parms++;
            break;
        }
        *p++ = *parms++;
    }
    *p = '\0';
    port = get_port( parms );
    if( port == 0 ) {
        return( TRP_ERR_unable_to_parse_port_number );
    }
    /* Setup for socket connect using parms specified by command line. */
    socket_address.sin_family = AF_INET;
    /* OS/2's TCP/IP gethostbyname doesn't handle numeric addresses */
    socket_address.sin_addr.s_addr = get_addr( buff );
    if( socket_address.sin_addr.s_addr == INADDR_NONE ) {
        return( TRP_ERR_unknown_host );
    }
    socket_address.sin_port = htons( port );
  #endif
#endif
    /* unused parameters */ (void)server;
    return( NULL );
}


void RemoteUnLink( void )
{
#ifdef SERVER

  #ifdef __RDOS__
    if( wait_handle ) {
        RdosCloseWait( wait_handle );
        wait_handle = 0;
    }
    if( listen_handle ) {
        RdosCloseTcpListen( listen_handle );
        listen_handle = 0;
    }
  #else
    soclose( control_socket );
  #endif
#else
    terminate_connection();
#endif

#if defined(__NT__) || defined(__WINDOWS__)
    WSACleanup();
#elif defined(__DOS__)
    sock_exit();
#endif
}

#ifdef LIST_INTERFACES
// TODO: need rework to POSIX if_nameindex in <net/if.h>

#ifndef __RDOS__

  #ifdef SERVER

/* Functions to manage IP interface information lists. On multi-homed hosts,
 * determining the IP address the TCP debug link responds on is not entirely
 * straightforward.
 */

    #if defined( __OS2__ ) || defined( __DOS__ )

/* Actual implementation - feel free to port to further OSes */

/* Sort out implementation differences. */
      #if defined( __DOS__ )
        #define w_ioctl         ioctlsocket
        #define HAVE_SA_LEN     false
      #elif defined( __OS2__ )
        #define w_ioctl         ioctl
        #define HAVE_SA_LEN     true
      #endif

static struct ifi_info  *get_ifi_info( int family, int doaliases )
{
    struct ifi_info     *ifi, *ifihead, **ifipnext;
    int                 len, lastlen, flags, myflags;
    char                *ptr, *buf, lastname[IFNAMSIZ], *cptr;
    struct ifconf       ifc;
    struct ifreq        *ifr, ifrcopy;
    struct sockaddr_in  *sinptr;
    trp_socket          sockfd;

    sockfd = socket( AF_INET, SOCK_DGRAM, 0 );

    lastlen = 0;
    len = 20 * sizeof( struct ifreq );   /* initial buffer size guess */
    for( ;; ) {
        buf = malloc( len );
        ifc.ifc_len = len;
        ifc.ifc_buf = buf;
        if( w_ioctl( sockfd, SIOCGIFCONF, (char *)&ifc ) >= 0 ) {
            if( ifc.ifc_len == lastlen )
                break;      /* success, len has not changed */
            lastlen = ifc.ifc_len;
        }
        len += 10 * sizeof( struct ifreq );   /* increment */
        free( buf );
    }
    ifihead = NULL;
    ifipnext = &ifihead;
    lastname[0] = 0;

    for( ptr = buf; ptr < buf + ifc.ifc_len; ) {
        ifr = (struct ifreq *)ptr;

      #if HAVE_SA_LEN
        len = sizeof( struct sockaddr );
        if( len < ifr->ifr_addr.sa_len ) {
            len = ifr->ifr_addr.sa_len ;
        }
      #else
        len = sizeof( struct sockaddr );
      #endif
        ptr += sizeof( ifr->ifr_name ) + len; /* for next one in buffer */

        if( ifr->ifr_addr.sa_family != family )
            continue;   /* ignore if not desired address family */

        myflags = 0;
        if(( cptr = strchr( ifr->ifr_name, ':' )) != NULL )
            *cptr = 0;      /* replace colon will null */
        if( strncmp( lastname, ifr->ifr_name, IFNAMSIZ ) == 0 ) {
            if( doaliases == 0 )
                continue;   /* already processed this interface */
            myflags = IFI_ALIAS;
        }
        memcpy( lastname, ifr->ifr_name, IFNAMSIZ );

        ifrcopy = *ifr;
        w_ioctl( sockfd, SIOCGIFFLAGS, (char *)&ifrcopy );
        flags = ifrcopy.ifr_flags;
        if( !( flags & IFF_UP ) )
            continue;   /* ignore if interface not up */

        ifi = calloc( 1, sizeof( struct ifi_info ) );
        *ifipnext = ifi;            /* prev points to this new one */
        ifipnext  = &ifi->ifi_next; /* pointer to next one goes here */

        if( flags & IFF_LOOPBACK )
            myflags |= IFI_LOOP;

        ifi->ifi_flags = flags;     /* IFF_xxx values */
        ifi->flags     = myflags;   /* IFI_xxx values */
        memcpy( ifi->ifi_name, ifr->ifr_name, IFI_NAME );
        ifi->ifi_name[IFI_NAME - 1] = '\0';

        switch( ifr->ifr_addr.sa_family ) {
        case AF_INET:
            sinptr = (struct sockaddr_in *)&ifr->ifr_addr;
            if( ifi->ifi_addr == NULL ) {
                ifi->ifi_addr = calloc( 1, sizeof( struct sockaddr_in ) );
                memcpy( ifi->ifi_addr, sinptr, sizeof( struct sockaddr_in ) );
            }
            break;

        default:
            break;
        }
    }
    free( buf );
    return( ifihead );    /* pointer to first structure in linked list */
}

static void free_ifi_info( struct ifi_info *ifihead )
{
    struct ifi_info *ifi, *ifinext;

    for( ifi = ifihead; ifi != NULL; ifi = ifinext ) {
        if( ifi->ifi_addr != NULL )
            free( ifi->ifi_addr );
        ifinext = ifi->ifi_next;    /* can't fetch ifi_next after free() */
        free( ifi );                /* the ifi_info{} itself */
    }
}

    # else

/* Stubbed out */
static struct ifi_info *get_ifi_info( int family, int doaliases )
{
    /* unused parameters */ (void)family; (void)doaliases;

    return( NULL );
}

static void free_ifi_info( struct ifi_info *ifihead )
{
    /* unused parameters */ (void)ifihead;
}

    #endif

  #endif

#endif

// TODO: need rework to POSIX if_nameindex in <net/if.h>
#endif
