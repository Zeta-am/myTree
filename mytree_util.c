#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>          
#include <dirent.h>          
#include <sys/types.h>       
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "mytree_util.h"


/*-----Globals variables-----*/

// To keep track of how many files and directories there are
struct entity_no entities = {0, 0};

// To keep track of the short options
flags_t flags = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// To keep track of the long options
struct option longopts[] = {
    {"inodes", no_argument, 0, 'i'},
    {"dirsfirst", no_argument, 0, 'n'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

// To save the parent_dir of the current entry
char *parent_dir = NULL;

/*-----Functions implementations-----*/

/*
 * Save into a string the permissions
 *      - permission: it's the permission of a file
 *      - out: string into save the permission
 * */
void get_permission(mode_t permission, char *out) {
    sprintf(out, "%s%s%s%s%s%s%s%s%s%s"
            , S_ISDIR(permission) ? "d" : "-"
            , (permission & S_IRUSR) ? "r" : "-"
            , (permission & S_IWUSR) ? "w" : "-"
            , (permission & S_IXUSR) ? "x" : "-"
            , (permission & S_IRGRP) ? "r" : "-"
            , (permission & S_IWGRP) ? "w" : "-"
            , (permission & S_IXGRP) ? "x" : "-"
            , (permission & S_IROTH) ? "r" : "-"
            , (permission & S_IWOTH) ? "w" : "-"
            , (permission & S_IXOTH) ? "x" : "-"
            );
}

/*
 * To manage the printing of the name of an entry
 *      - name: the name of the entry
 *      - parent_dir: the parent directory of name
 *      - level: the level of indentation
 * */
void print_entry(const char *name, const char *parent_dir, const int level) {
    // The complete path of the entry
    char *path = malloc(strlen(parent_dir) + strlen(name) + 2);
    sprintf(path, "%s/%s", parent_dir, name);
    // To keep track of the stats of the entry
    struct stat entry_stat;
    // To have the user name of the entry
    struct passwd *pwd;
    // To have the group name of the entry
    struct group *grp;
    //To save the string of the attributes
    char entry_attr[256];
    // To save the formatted permission (10 -> type:- user:--- group:--- other:---)
    char *permission = malloc(10*sizeof(char)), *date = NULL;
    // To save the inode number and the dimension into a string
    char inode[128], byte_dim[128]; 
    // Save the stat of the entry and check if there is an error
    if (stat(path, &entry_stat) < 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    // Populate pwd
    pwd = getpwuid(entry_stat.st_uid);
    // Populate grp
    grp = getgrgid(entry_stat.st_gid);
    // Save the permission
    get_permission(entry_stat.st_mode, permission);
    // Save the inode
    sprintf(inode, "%d",(int)entry_stat.st_ino);
    // Save byte dimension
    sprintf(byte_dim, "%d", (int)entry_stat.st_size);
    // Save the date
    date = ctime(&entry_stat.st_mtime);
    // Replace the last character with \0
    date[strlen(date)-1] = 0;
    // True if there is a stat flag setted, false otherwise
    int is_stat_flag = flags.p_flag || flags.g_flag || flags.s_flag || flags.u_flag ||
                        flags.D_flag || flags.i_flag;

    // Check which flag is setted 
    sprintf(entry_attr, "%s%s%s%s%s%s%s%s%s%s%s%s%s"
            ,!is_stat_flag ? "" : " ["
            ,!flags.i_flag ? "" : inode                  // Print the inode number 
            ,!is_stat_flag ? "" : "  "
            ,!flags.p_flag ? "" : permission             // Print the file type and permission
            ,!is_stat_flag ? "" : "  "
            ,!flags.u_flag ? "" : pwd->pw_name           // Print the username
            ,!is_stat_flag ? "" : "  "
            ,!flags.g_flag ? "" : grp->gr_name           // Print the groupname
            ,!is_stat_flag ? "" : "  "
            ,!flags.s_flag ? "" : byte_dim               // Print the dimension
            ,!is_stat_flag ? "" : "  "
            ,!flags.D_flag ? "" : date                   // Print the date of the last modification
            ,!is_stat_flag ? "" : "]"                
            );
    // Print the entry name
    printf("%*s%s%s %s%s%s\033[0m\n"
            ,level*4, "", is_dir(path) ? "+" : "-"
            ,entry_attr
            ,is_dir(path) ? "\033[1;36m" : !access(name, X_OK) ? "\033[1;35m" : ""  
            ,!flags.f_flag ? name : path
            ,(access(path, R_OK) == -1 || access(path, X_OK) == -1) && is_dir(path) ? " [error opening dir]" : ""
            );
    // Free the memory
    free(permission);
}
/*
 * Save all the entries into an array of pointer to struct dirent,
 * sort the array and print the entries
 *      - path: the path of an entry
 *      - level: the level of indentation
 * */
void print_tree(char *path, int level) {
    DIR *dp = NULL;                 // To open a directory
    struct dirent *entry;           // To read a directory
    struct dirent **entries;        // To save the entries of a directory
    int entries_no;                 // To count the number of entries
    int i;                          // A counter

    // Try to open the directory
    if ((dp = opendir(path)) == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Save in parent_dir the path
    parent_dir = path;
    // Count the numbers of entries in path
    entries_no = 0;
    while ((entry = readdir(dp)) != NULL) {
        // Skip the current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // If the entry is a hidden file and the a_flag is setted, skip the entry
        if (entry->d_name[0] == '.' && !flags.a_flag) {
            continue;
        }
        entries_no++;
    }

    // Allocate entrie_no location in entries
    entries = (struct dirent **) malloc(entries_no * sizeof(struct dirent *));

    // Check if is possible allocate memory
    if (entries == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
   
    // Resets the position of the directory stream dp to the beginning of the directory  
    rewinddir(dp);

    // Save the entry into entries
    i = 0;
    while ((entry = readdir(dp)) != NULL) {
        // Skip the current and parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // If the entry is a hidden file and the a_flag is setted, skip the entry
        if (entry->d_name[0] == '.' && !flags.a_flag) {
            continue;
        }
        // Save entry into the i-th position in entries and increment the counter
        entries[i++] = entry;
    }

    //Sort the array of entries, based on the entered flag
    // Sort in the reverse order
    if (flags.r_flag) {
        qsort(entries, entries_no, sizeof(struct dirent *), rev_alphabetically);
    } 
    // Sort by last modification
    else if (flags.t_flag) {
        qsort(entries, entries_no, sizeof(struct dirent *), last_modification);
    } 
    // Sort the directories first
    else if (flags.n_flag) {
        qsort(entries, entries_no, sizeof(struct dirent *), dirsfirst);
    } 
    // Sort alphabetically
    else {
        qsort(entries, entries_no, sizeof(struct dirent *), alphabetically);
    }

    // Print the tree
    for (i = 0; i < entries_no; i++) {
        // The current entry
        entry = entries[i];
        // The fullpath of the entry
        char *sub_path = (char *) malloc(strlen(path) + strlen(entry->d_name) + 2);
        sprintf(sub_path, "%s/%s", path, entry->d_name);
        // Check if the entry is a directory
        if (entry->d_type == 4) {   //DT_DIR = 4
            // Print the name of the directory
            print_entry(entry->d_name, path, level);
            // if the directory has no read or execution permissions, skip the directory
            if (access(sub_path, R_OK) == -1 || access(sub_path, X_OK) == -1) {
                continue;
            }
        } else {
            // If d_flag is setted, don't print the files
            if (!flags.d_flag) {
                print_entry(entry->d_name, path, level);
            }
        }
        // If the current entry is a directory, call recursevely print_tree
        if (entry->d_type == 4) {   //DT_DIR = 4
            // If the level has reached, don't call recursevely the function
            if (flags.L_level == level) {
                continue;
            }
            // Increment the count of the number of directories
            entities.dirs_no++;
            // Call recursevely and increment the level of indentation
            print_tree(sub_path, level + 1);
        } else {
            // Increment the count of the number of file
            entities.files_no++;
        }
        // Free the memory of sub_path
        free(sub_path);
    }
    // Close the directory stream
    closedir(dp);
    // Free the allocate memory of entries
    free(entries);
}

/* Sort by date of the last modification */
int last_modification(const void *a, const void *b) {
    const struct dirent **entry_a = (const struct dirent **)a;
    const struct dirent **entry_b = (const struct dirent **)b;
    struct stat stat_a, stat_b;         // Struct to keep track of the stat of the two entries
    stat((*entry_a)->d_name, &stat_a);  // Save the stats of entry_a
    stat((*entry_b)->d_name, &stat_b);  // Save the stat of entry_b
    if (stat_a.st_mtime < stat_b.st_mtime) {
        return -1;
    } else if (stat_a.st_mtime > stat_b.st_mtime) {
        return 1;
    }
    return 0;    
}

/*
 * Print the total number of files and directories
 * */
void print_entities_no() {
    if (!flags.d_flag) {
        printf("\n%d directories, %d files\n", entities.dirs_no, entities.files_no);
    } else {
        printf("\n%d directories\n", entities.dirs_no);
    }

}

/*
 * Sort directories first
 * */
int dirsfirst(const void *a, const void *b) {
    const struct dirent **entry_a = (const struct dirent **)a;
    const struct dirent **entry_b = (const struct dirent **)b;
    // Save the name of the entries
    char *path_a = (char *) malloc(strlen((*entry_a)->d_name) + strlen(parent_dir) + 2), 
         *path_b = (char *) malloc(strlen((*entry_b)->d_name) + strlen(parent_dir) + 2);
    sprintf(path_a, "%s/%s", parent_dir, (*entry_a)->d_name);
    sprintf(path_b, "%s/%s", parent_dir, (*entry_b)->d_name);
    struct stat stat_a, stat_b;
    // Save the stats of the entries
    if (stat(path_a, &stat_a) < 0 || stat(path_b, &stat_b) < 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    int type_a = S_ISDIR(stat_a.st_mode),
        type_b = S_ISDIR(stat_b.st_mode);

    // Check which entry is a directory
    if (type_a && !type_b) {
        return -1;
    } else if (!type_a && type_b) {
        return 1;
    }
    return strcmp(path_a, path_b);
}


/* 
 * Sort alphabetically reversed 
 * */
int rev_alphabetically(const void *a, const void *b) {
    const struct dirent **entry_a = (const struct dirent **)a;
    const struct dirent **entry_b = (const struct dirent **)b;
    return strcmp((*entry_b)->d_name, (*entry_a)->d_name);    
}


/* 
 * Sort alphabetically 
 * */
int alphabetically(const void *a, const void *b) {
    const struct dirent **entry_a = (const struct dirent **)a;
    const struct dirent **entry_b = (const struct dirent **)b;
    return strcmp((*entry_a)->d_name, (*entry_b)->d_name);
}


/* 
 * Check if the given path is a directory 
 * */
int is_dir(const char *path) {
    struct stat path_stat; 
    // Save the stats of path into path_stat
    if (stat(path, &path_stat) < 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }        
    return S_ISDIR(path_stat.st_mode);      // Check if the path is a directory
}


/* 
 * Return the number of files and directories 
 * */
struct entity_no get_entities_no() { return entities; }


/*
 * Set command line option flags to 1
 * */
char *options(int argc, char **argv) {
    // To keep track of the option
    int option;    
    // To save the path
    char *path = NULL;
    // Say to compiler to find opterr in another file
    extern int opterr;
    // Set opterr to 0 to change the behavior of getopt in printing error messages
    opterr = 0;
    // Loop over argv to check which option was selected
    while((option = getopt_long(argc, argv, ":adfpugsDtrL:inh", longopts, NULL)) != -1) {
        switch(option) {
            // Print all the file (also the hidden file)
            case 'a':
                flags.a_flag = 1;
                break;

            // List only the directory
            case 'd':
                flags.d_flag = 1;
                break;

            // Print the full path
            case 'f':
                flags.f_flag = 1;
                break;

            // Print the file type and permission
            case 'p':
                flags.p_flag = 1;
                break;

            // Print the username or the UID, if no username is available
            case 'u':
                flags.u_flag = 1;
                break;

            // Print the groupname or the GID, if no groupname is available
            case 'g':
                flags.g_flag = 1;
                break;

            // Print the size of each file in bytes
            case 's':
                flags.s_flag = 1;
                break;

            // Print the date of the last modification
            case 'D':
                flags.D_flag = 1;
                break;

            // Sort the output by the last modification
            case 't':
                flags.t_flag = 1;
                break;

            // Sort the output in reverse order
            case 'r':
                flags.r_flag = 1;
                break;

            // Display level of depth of tree
            case 'L':
                if (strlen(optarg) == 1 && isdigit(optarg[0])) {
                    flags.L_level = atoi(optarg);
                } else {
                    fprintf(stderr, "%s: Invalid level, must be a digit greater than 0\n", argv[0]);
                    exit(EXIT_FAILURE);
                }
                break;

            // Print the help of the function
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;

            // List directories before files
            case 'n':
                flags.n_flag = 1;
                break;

            // Print the inode number
            case 'i':
                flags.i_flag = 1;
                break;

            // Invalid option
            case '?':   
                fprintf(stderr, "%s: option -'%c' is invalid\n", argv[0], optopt);
                print_usage();
                break;

            // Missing option argument
            case ':': 
                fprintf(stderr, "%s: option -'%c' requires an argument\n", argv[0], optopt);
                break;

            default:
                break;
        }
    }    
    // Save into path the path inserted from the user
    if (optind < argc) {
        path = argv[optind];
    } 
    // If not specified is the current directory
    else {
        path = ".";
    }

    return path;
}


/*
 * Print the help of the command
 * */
void print_help() {
    puts("usage: mytree [-adfpugstrD] [-L level] [--inodes] [--dirsfirst] [--help] [directory ...]\n" 
 " ------- Listing options -------\n"
 " -a            All files are listed.\n" 
 " -d            List directories only.\n" 
 " -f            Print the full path prefix for each file.\n" 
 " -L level      Descend only level directories deep.\n" 
 " ------- File options -------\n"
 " -p            Print the permission for each file.\n"
 " -u            Displays file owner or UID number.\n"
 " -g            Displays file group owner or GID number.\n"
 " -s            Print the size in bytes of each file.\n"
 " -D            Print the date of last modification.\n"
 " --inodes      Print inode number of each file.\n"
 " ------- Sorting options -------\n"
 " -t            Sort files by last modification time.\n"
 " -r            Reverse the order of the sort.\n"
 " --dirsfirst   List directories before files.\n"
 " ------- Miscellaneous options -------\n"
 " --help        Print usage and this help message and exit."
 ); 
}

/*
 * Print the usage
 * */
void print_usage() {
    puts("usage: mytree [-adfpugstrD] [-L level] [--inodes] [--dirsfirst] [--help] [directory ...]"); 
    exit(EXIT_FAILURE);
}

