/****************************************************************************
 * Copyright (c) 1998,1999,2000 Free Software Foundation, Inc.              *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/

/*
 *      read_entry.c -- Routine for reading in a compiled terminfo file
 *
 */

#include <curses_p.h>

#include <tic.h>
#include <term_ent.h>

MODULE_ID("$Id: read_entry.c,v 1.72 2000/12/10 02:55:08 tom Exp $")

#if !HAVE_TELL
#define tell(fd) 0              /* lseek() is POSIX, but not tell() - odd... */
#endif

/*
 *      int
 *      _nc_read_file_entry(filename, ptr)
 *
 *      Read the compiled terminfo entry in the given file into the
 *      structure pointed to by ptr, allocating space for the string
 *      table.
 */

#undef  BYTE
#define BYTE(p,n)       (unsigned char)((p)[n])

#define IS_NEG1(p)      ((BYTE(p,0) == 0377) && (BYTE(p,1) == 0377))
#define IS_NEG2(p)      ((BYTE(p,0) == 0376) && (BYTE(p,1) == 0377))
#define LOW_MSB(p)      (BYTE(p,0) + 256*BYTE(p,1))

#define IS_TIC_MAGIC(p) (LOW_MSB(p) == MAGIC || LOW_MSB(p) == MAGIC2)

#define MAX_ENTRY_SIZE  MAX_ENTRY_SIZE1

#define MAX_OF_TYPE(t)  (t)(((unsigned t)(~0))>>1)

static bool have_tic_directory = false;
static bool keep_tic_directory = false;

/*
 * Record the "official" location of the terminfo directory, according to
 * the place where we're writing to, or the normal default, if not.
 */
NCURSES_EXPORT(const char *)
_nc_tic_dir(const char *path)
{
    static const char *result = TERMINFO;

    if (!keep_tic_directory) {
        if (path != 0) {
            result = path;
            have_tic_directory = true;
        } else if (!have_tic_directory && use_terminfo_vars()) {
            char *envp;
            if ((envp = getenv("TERMINFO")) != 0)
                return _nc_tic_dir(envp);
        }
    }
    return result;
}

/*
 * Special fix to prevent the terminfo directory from being moved after tic
 * has chdir'd to it.  If we let it be changed, then if $TERMINFO has a
 * relative path, we'll lose track of the actual directory.
 */
NCURSES_EXPORT(void)
_nc_keep_tic_dir(const char *path)
{
    _nc_tic_dir(path);
    keep_tic_directory = true;
}

static void
convert_16bits(char *buf, short *Numbers, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        Numbers[i] = (short) LOW_MSB(buf + 2 * i);
        TR(TRACE_DATABASE, ("Numbers[%d]=%d", i, Numbers[i]));
    }
}

static void
convert_32bits(char *buf, short *Numbers, int count)
{
    int i;
    int j;
    unsigned char ch;

    for (i = 0; i < count; i++) {
#ifdef _M_I86
        long value = 0;
#else
        int value = 0;
#endif
        for (j = 0; j < 4; ++j) {
            ch = UChar(*buf++);
            value |= (ch << (8 * j));
        }
        if( value == ABSENT_NUMERIC ) {
            Numbers[i] = ABSENT_NUMERIC;
        } else if( value == CANCELLED_NUMERIC ) {
            Numbers[i] = CANCELLED_NUMERIC;
        } else if( value > MAX_OF_TYPE( short ) ) {
            Numbers[i] = MAX_OF_TYPE( short );
        } else {
            Numbers[i] = value;
        }
        TR(TRACE_DATABASE, ("Numbers[%d]=%d", i, Numbers[i]));
    }
}

static void
convert_strings(char *buf, char **Strings, int count, int size, char *table)
{
    int i;
    char *p;

    for (i = 0; i < count; i++) {
        if (IS_NEG1(buf + 2 * i)) {
            Strings[i] = ABSENT_STRING;
        } else if (IS_NEG2(buf + 2 * i)) {
            Strings[i] = CANCELLED_STRING;
        } else if (LOW_MSB(buf + 2 * i) > size) {
            Strings[i] = ABSENT_STRING;
        } else {
            Strings[i] = (LOW_MSB(buf + 2 * i) + table);
            TR(TRACE_DATABASE, ("Strings[%d] = %s", i, _nc_visbuf(Strings[i])));
        }

        /* make sure all strings are NUL terminated */
        if (VALID_STRING(Strings[i])) {
            for (p = Strings[i]; p <= table + size; p++)
                if (*p == '\0')
                    break;
            /* if there is no NUL, ignore the string */
            if (p > table + size)
                Strings[i] = ABSENT_STRING;
        }
    }
}

#define read_shorts(fd, buf, count) (read(fd, buf, (count)*2) == (count)*2)

#define read_numbers(fd, buf, count) (read(fd, buf, (count)*(unsigned)size_of_numbers) == (count)*size_of_numbers)

#define even_boundary(value) \
    if ((value) % 2 != 0) read(fd, buf, 1)

static int
read_termtype(int fd, TERMTYPE * ptr)
/* return 1 if read, 0 if not found or garbled */
{
    int name_size, bool_count, num_count, str_count, str_size;
    int i;
    char buf[MAX_ENTRY_SIZE + 2];
    unsigned read_size;
    bool need_ints;
    void (*convert_numbers) (char *, short *, int);
    int size_of_numbers;
    int max_entry_size;

    TR(TRACE_DATABASE, ("READ termtype header @%d", tell(fd)));

    memset(ptr, 0, sizeof(*ptr));

    /* grab the header */
    if (!read_shorts(fd, buf, 6)
        || !IS_TIC_MAGIC(buf)) {
        return (0);
    }

    if (need_ints = (LOW_MSB(buf) == MAGIC2)) {
        max_entry_size = MAX_ENTRY_SIZE;
        size_of_numbers = 4;
        convert_numbers = convert_32bits;
    } else {
        max_entry_size = MAX_ENTRY_SIZE1;
        size_of_numbers = 2;
        convert_numbers = convert_16bits;
    }

    _nc_free_termtype(ptr);
    name_size = LOW_MSB(buf + 2);
    bool_count = LOW_MSB(buf + 4);
    num_count = LOW_MSB(buf + 6);
    str_count = LOW_MSB(buf + 8);
    str_size = LOW_MSB(buf + 10);

    TR(TRACE_DATABASE,
       ("TERMTYPE name_size=%d, bool=%d/%d, num=%d/%d str=%d/%d(%d)",
        name_size, bool_count, BOOLCOUNT, num_count, NUMCOUNT,
        str_count, STRCOUNT, str_size));
    if (name_size < 0
        || bool_count < 0
        || num_count < 0
        || str_count < 0
        || str_size < 0) {
        return (0);
    }

    if (str_size) {
        /* try to allocate space for the string table */
        if (str_count * 2 >= max_entry_size
            || (ptr->str_table = typeMalloc(char, (unsigned) str_size)) == 0) {
            return (0);
        }
    } else {
        str_count = 0;
    }

    /* grab the name (a null-terminate string) */
    read_size = (unsigned)name_size;
    if( read_size > MAX_NAME_SIZE )
        read_size = MAX_NAME_SIZE;
    read(fd, buf, read_size);
    buf[MAX_NAME_SIZE] = '\0';
    TR(TRACE_DATABASE, ("TERMTYPE Name = %s", buf));
    ptr->term_names = typeCalloc(char, strlen(buf) + 1);
    if (ptr->term_names == NULL) {
        return (0);
    }
    (void) strcpy(ptr->term_names, buf);
    if (name_size > MAX_NAME_SIZE)
        lseek(fd, (off_t) (name_size - MAX_NAME_SIZE), 1);

    /* grab the booleans */
    i = bool_count;
    if( i < BOOLCOUNT )
        i = BOOLCOUNT;
    if( (ptr->Booleans = typeCalloc( NCURSES_SBOOL, i )) == 0
        || read(fd, ptr->Booleans, (unsigned) bool_count) < bool_count) {
        return (0);
    }

    /*
     * If booleans end on an odd byte, skip it.  The machine they
     * originally wrote terminfo on must have been a 16-bit
     * word-oriented machine that would trap out if you tried a
     * word access off a 2-byte boundary.
     */
    even_boundary(name_size + bool_count);

    /* grab the numbers */
    i = num_count;
    if( i < NUMCOUNT )
        i = NUMCOUNT;
    if( (ptr->Numbers = typeCalloc( short, i )) == 0
        || !read_numbers(fd, buf, num_count)) {
        return (0);
    }
    convert_numbers(buf, ptr->Numbers, num_count);
    i = str_count;
    if( i < STRCOUNT )
        i = STRCOUNT;
    if( (ptr->Strings = typeCalloc( char *, i )) == 0 )
          return (0);

    if (str_count) {
        /* grab the string offsets */
        if (!read_shorts(fd, buf, str_count)) {
            return (0);
        }
        /* finally, grab the string table itself */
        if (read(fd, ptr->str_table, (unsigned) str_size) != str_size)
            return (0);
        convert_strings(buf, ptr->Strings, str_count, str_size, ptr->str_table);
    }
#if NCURSES_XNAMES

    ptr->num_Booleans = BOOLCOUNT;
    ptr->num_Numbers = NUMCOUNT;
    ptr->num_Strings = STRCOUNT;

    /*
     * Read extended entries, if any, after the normal end of terminfo data.
     */
    even_boundary(str_size);
    TR(TRACE_DATABASE, ("READ extended_header @%d", tell(fd)));
    if (_nc_user_definable && read_shorts(fd, buf, 5)) {
        int ext_bool_count = LOW_MSB(buf + 0);
        int ext_num_count = LOW_MSB(buf + 2);
        int ext_str_count = LOW_MSB(buf + 4);
        int ext_str_size = LOW_MSB(buf + 6);
        int ext_str_limit = LOW_MSB(buf + 8);
        int need = (ext_bool_count + ext_num_count + ext_str_count);
        int base = 0;

        if (need >= (max_entry_size / 2)
            || ext_str_size >= max_entry_size
            || ext_str_limit >= max_entry_size
            || ext_bool_count < 0
            || ext_num_count < 0
            || ext_str_count < 0
            || ext_str_size < 0
            || ext_str_limit < 0)
            return (0);

        ptr->num_Booleans = BOOLCOUNT + ext_bool_count;
        ptr->num_Numbers = NUMCOUNT + ext_num_count;
        ptr->num_Strings = STRCOUNT + ext_str_count;

        ptr->Booleans = typeRealloc(NCURSES_SBOOL, ptr->num_Booleans, ptr->Booleans);
        ptr->Numbers = typeRealloc(short, ptr->num_Numbers, ptr->Numbers);
        ptr->Strings = typeRealloc(char *, ptr->num_Strings, ptr->Strings);

        TR(TRACE_DATABASE, ("extended header is %d/%d/%d(%d:%d)",
                            ext_bool_count, ext_num_count, ext_str_count,
                            ext_str_size, ext_str_limit));

        TR(TRACE_DATABASE, ("READ %d extended-booleans @%d",
                            ext_bool_count, tell(fd)));
        if ((ptr->ext_Booleans = ext_bool_count) != 0) {
            if (read(fd, ptr->Booleans + BOOLCOUNT, (unsigned)
                     ext_bool_count) != ext_bool_count)
                return (0);
        }
        even_boundary(ext_bool_count);

        TR(TRACE_DATABASE, ("READ %d extended-numbers @%d",
                            ext_num_count, tell(fd)));
        if ((ptr->ext_Numbers = ext_num_count) != 0) {
            if (!read_numbers(fd, buf, ext_num_count))
                return (0);
            TR(TRACE_DATABASE, ("Before converting extended-numbers"));
            convert_numbers(buf, ptr->Numbers + NUMCOUNT, ext_num_count);
        }

        TR(TRACE_DATABASE, ("READ extended-offsets @%d", tell(fd)));
        if ((ext_str_count + need) >= (max_entry_size / 2)) {
            return (0);
        }
        if ((ext_str_count || need)
            && !read_shorts(fd, buf, ext_str_count + need))
            return (0);

        TR(TRACE_DATABASE, ("READ %d bytes of extended-strings @%d",
                            ext_str_limit, tell(fd)));

        if (ext_str_limit) {
            if ((ptr->ext_str_table = typeMalloc(char, ext_str_limit)) == 0)
                  return (0);
            if (read(fd, ptr->ext_str_table, ext_str_limit) != ext_str_limit)
                return (0);
            TR(TRACE_DATABASE, ("first extended-string is %s", _nc_visbuf(ptr->ext_str_table)));
        }

        if ((ptr->ext_Strings = ext_str_count) != 0) {
            TR(TRACE_DATABASE,
               ("Before computing extended-string capabilities str_count=%d, ext_str_count=%d",
                str_count, ext_str_count));
            convert_strings(buf, ptr->Strings + str_count, ext_str_count,
                            ext_str_limit, ptr->ext_str_table);
            for (i = ext_str_count - 1; i >= 0; i--) {
                TR(TRACE_DATABASE, ("MOVE from [%d:%d] %s",
                                    i, i + str_count,
                                    _nc_visbuf(ptr->Strings[i + str_count])));
                ptr->Strings[i + STRCOUNT] = ptr->Strings[i + str_count];
                if (VALID_STRING(ptr->Strings[i + STRCOUNT]))
                    base += (strlen(ptr->Strings[i + STRCOUNT]) + 1);
                TR(TRACE_DATABASE, ("... to    [%d] %s",
                                    i + STRCOUNT,
                                    _nc_visbuf(ptr->Strings[i + STRCOUNT])));
            }
        }

        if (need) {
            if (ext_str_count >= (max_entry_size / 2)) {
                  return (0);
            }
            if ((ptr->ext_Names = typeCalloc(char *, need)) == 0)
                  return (0);
            TR(TRACE_DATABASE,
               ("ext_NAMES starting @%d in extended_strings, first = %s",
                base, _nc_visbuf(ptr->ext_str_table + base)));
            convert_strings(buf + (2 * ext_str_count), ptr->ext_Names, need,
                            ext_str_limit, ptr->ext_str_table + base);
        }

        T(("...done reading terminfo bool %d(%d) num %d(%d) str %d(%d)",
           ptr->num_Booleans, ptr->ext_Booleans,
           ptr->num_Numbers, ptr->ext_Numbers,
           ptr->num_Strings, ptr->ext_Strings));

        TR(TRACE_DATABASE, ("extend: num_Booleans:%d", ptr->num_Booleans));
    } else
#endif /* NCURSES_XNAMES */
    {
        T(("...done reading terminfo bool %d num %d str %d",
           bool_count, num_count, str_count));
#if NCURSES_XNAMES
        TR(TRACE_DATABASE, ("normal: num_Booleans:%d", ptr->num_Booleans));
#endif
    }

    for (i = bool_count; i < BOOLCOUNT; i++)
        ptr->Booleans[i] = false;
    for (i = num_count; i < NUMCOUNT; i++)
        ptr->Numbers[i] = ABSENT_NUMERIC;
    for (i = str_count; i < STRCOUNT; i++)
        ptr->Strings[i] = ABSENT_STRING;

    return (1);
}

NCURSES_EXPORT(int)
_nc_read_file_entry
(const char *const filename, TERMTYPE * ptr)
/* return 1 if read, 0 if not found or garbled */
{
    int code, fd = -1;

    if (_nc_access(filename, R_OK) < 0
        || (fd = open(filename, O_RDONLY | O_BINARY)) < 0) {
        T(("cannot open terminfo %s (errno=%d)", filename, errno));
        return (0);
    }

    T(("read terminfo %s", filename));
    if ((code = read_termtype(fd, ptr)) == 0)
        _nc_free_termtype(ptr);
    close(fd);

    return (code);
}

/*
 * Build a terminfo pathname and try to read the data.  Returns 1 on success,
 * 0 on failure.
 */
static int
_nc_read_tic_entry(char *const filename,
                   const char *const dir, const char *ttn, TERMTYPE * const tp)
{
/* maximum safe length of terminfo root directory name */
#define MAX_TPATH       (PATH_MAX - MAX_ALIAS - 6)

    if (strlen(dir) > MAX_TPATH)
        return 0;
    (void) sprintf(filename, "%s/%s", dir, ttn);
    return _nc_read_file_entry(filename, tp);
}

/*
 * Process the list of :-separated directories, looking for the terminal type.
 * We don't use strtok because it does not show us empty tokens.
 */
static int
_nc_read_terminfo_dirs(const char *dirs, char *const filename, const char *const
                       ttn, TERMTYPE * const tp)
{
    char *list, *a;
    const char *b;
    int code = 0;

    /* we'll modify the argument, so we must copy */
    if ((b = a = list = strdup(dirs)) == NULL)
        return (0);

    for (;;) {
        int c = *a;
        if (c == 0 || c == NCURSES_PATHSEP) {
            *a = 0;
            if ((b + 1) >= a)
                b = TERMINFO;
            if (_nc_read_tic_entry(filename, b, ttn, tp) == 1) {
                code = 1;
                break;
            }
            b = a + 1;
            if (c == 0)
                break;
        }
        a++;
    }

    free(list);
    return (code);
}

/*
 *      _nc_read_entry(char *tn, char *filename, TERMTYPE *tp)
 *
 *      Find and read the compiled entry for a given terminal type,
 *      if it exists.  We take pains here to make sure no combination
 *      of environment variables and terminal type name can be used to
 *      overrun the file buffer.
 */

NCURSES_EXPORT(int)
_nc_read_entry
(const char *const tn, char *const filename, TERMTYPE * const tp)
{
    char *envp;
    char ttn[MAX_ALIAS + 3];

    /* truncate the terminal name to prevent dangerous buffer airline */
#if defined( __OSX__ ) || defined( __APPLE__ )
    (void) sprintf(ttn, "%02x/%.*s", *tn, (int) sizeof(ttn) - 4, tn);
#else
    (void) sprintf(ttn, "%c/%.*s", *tn, MAX_ALIAS, tn);
#endif

    /* This is System V behavior, in conjunction with our requirements for
     * writing terminfo entries.
     */
    if (have_tic_directory) {
        if (_nc_read_tic_entry(filename, _nc_tic_dir(0), ttn, tp) == 1) {
            return 1;
        }
    }

    if (use_terminfo_vars()) {
        if ((envp = getenv("TERMINFO")) != 0) {
            if (_nc_read_tic_entry(filename, _nc_tic_dir(envp), ttn, tp) == 1) {
                return 1;
            }
        }

        /* this is an ncurses extension */
        if ((envp = _nc_home_terminfo()) != 0) {
            if (_nc_read_tic_entry(filename, envp, ttn, tp) == 1) {
                return (1);
            }
        }

        /* this is an ncurses extension */
        if ((envp = getenv("TERMINFO_DIRS")) != 0) {
            return _nc_read_terminfo_dirs(envp, filename, ttn, tp);
        }
    }

    /* Try the system directory.  Note that the TERMINFO_DIRS value, if
     * defined by the configure script, begins with a ":", which will be
     * interpreted as TERMINFO.
     */
#ifdef TERMINFO_DIRS
    return _nc_read_terminfo_dirs(TERMINFO_DIRS, filename, ttn, tp);
#else
    return _nc_read_tic_entry(filename, TERMINFO, ttn, tp);
#endif
}
