#ifndef _FS_H_INCLUDED
#define _FS_H_INCLUDED

#define MAX_BIN_CAPACITY 65536
#define PATHSEP	'/'


typedef struct {
	char *name;
	int size;
	int bin_id;
} file_entry_t;

typedef struct {
	int capacity;
} bin_t;



/* function prototypes */
int fs(int argc, char **argv);


#endif  /* _FS_H_INCLUDED */
