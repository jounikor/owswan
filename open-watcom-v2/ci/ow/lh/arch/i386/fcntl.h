/*
 *  File access modes.
 */

#define O_RDONLY    000000  /*  Read-only mode  */
#define O_WRONLY    000001  /*  Write-only mode */
#define O_RDWR      000002  /*  Read-Write mode */

/*
 *  Mask for file access modes.
 */

#define O_ACCMODE   000003

/*
 *  File status flags used for open() and fcntl().
 */

#define O_NONBLOCK  004000  /*  Non-blocking I/O                */
#define O_APPEND    002000  /*  Append (writes guaranteed at the end) */
#define O_NDELAY    O_NONBLOCK
#define O_SYNC      010000
#define O_FSYNC     O_SYNC
#define O_ASYNC     020000

/*
 *  oflag values for open()
 */

#define O_CREAT     000100  /*  Opens with file create      */
#define O_EXCL      000200  /*  Exclusive open          */
#define O_NOCTTY    000400  /*  Don't assign a controlling terminal */
#define O_TRUNC     001000  /*  Open with truncation        */

#define O_LARGEFILE 0100000 /*  support files > 2G, allow _llseek() */
#define O_DIRECTORY 0200000 /* must be a directory */
#define O_NOFOLLOW  0400000 /* don't follow links */
#define O_NOATIME   01000000

/*
 *  fcntl() requests
 */

#define F_DUPFD     0       /* Duplicate file descriptor   */
#define F_GETFD     1       /* Get file descriptor flags   */
#define F_SETFD     2       /* Set file descriptor flags   */
#define F_GETFL     3       /* Get file status flags   */
#define F_SETFL     4       /* Set file status flags   */
#define F_GETLK     5       /* Get record locking info.  */
#define F_SETLK     6       /* Set record locking info (non-blocking). */
#define F_SETLKW    7       /* Set record locking info (blocking) */
#define F_GETLK64   12      /* Get record locking info.  */
#define F_SETLK64   13      /* Set record locking info (non-blocking).  */
#define F_SETLKW64  14      /* Set record locking info (blocking).  */

/*
 *  File descriptor flags used for fcntl()
 */

#define FD_CLOEXEC  0x01    /*  Close on exec       */

/*
 *  l_type values for record locking with fcntl()
 */

#define F_RDLCK     0       /* Read lock.  */
#define F_WRLCK     1       /* Write lock.  */
#define F_UNLCK     2       /* Remove lock.  */

/*
 *  flock structure.
 */

typedef struct flock {
    short int   l_type;     /* Type of lock: F_RDLCK, F_WRLCK, or F_UNLCK.  */
    short int   l_whence;   /* Where `l_start' is relative to (like `lseek').  */
    off_t       l_start;    /* Offset where the lock begins.  */
    off_t       l_len;      /* Size of the locked area; zero means until EOF.  */
    pid_t       l_pid;      /* Process holding the lock.  */
} flock_t;

