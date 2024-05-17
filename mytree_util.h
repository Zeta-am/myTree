#ifndef MYTREE_UTIL_H
#define MYTREE_UTIL_H
#include <sys/types.h>
/*-----Data structures-----*/

/* Struct to keep track of the short options selected */
typedef struct {
    int a_flag;     /* All files are listed */
    int d_flag;     /* List directories only */
    int f_flag;     /* Print the full path of the file */
    int p_flag;     /* Print the permission for each file */
    int u_flag;     /* Display the file owner or UID , of the file */
    int g_flag;     /* Display the group owner or GID, of the file */
    int s_flag;     /* Print the size in bytes of each file */
    int D_flag;     /* Print the date of the last modification */
    int t_flag;     /* Sort the files by the last modification */
    int r_flag;     /* Reverse the order of the sort */
    int n_flag;     /* List directories before files */
    int i_flag;     /* Print the inode number */
    int L_level;    /* Descend only level directories deep */
}flags_t;

/* Struct to keep track of the numbers of file and directories in a path */
struct entity_no{
    int files_no;
    int dirs_no;
};

/*-----Functions prototype-----*/
// Print the usage
void print_usage();

// Print the list of options available
void print_help();

// Check which option has been selected
char *options(int, char**);

//  Return the numbers of file and dirs
struct entity_no get_entities_no();

// Given a path check if is a directory
int is_dir(const char *);

// Sort alphabetically
int alphabetically(const void *, const void *); 

// Sort reverse alphabetically
int rev_alphabetically(const void *, const void *);

// Sort by date of the last modification
int last_modification(const void *, const void *);

// Sort by directory
int dirsfirst(const void *, const void *);

// Print entities numbers
void print_entities_no();

// Print the directories and files like a tree
void print_tree(char *, int);

// Change the color of the output to cyan
void cyan();

// Change the color of the output to purple
void purple();

// Reset to the default color
void reset();

// To manage the printing of the name of an entry
void print_entry(const char *, const char *, const int);

// Return a string like ls -l of the permission a file
void get_permission(mode_t, char *);

// To convert an integer into a string
#endif
