/*
 * Program emulating the behavior of the tree command (https://linux.die.net/man/1/tree)
 *
 * */
#include <stdio.h>
#include <getopt.h>
#include "mytree_util.h"

int main(int argc, char **argv) {
    // Call options to set the options selected and retrieve the path
    char *path = options(argc, argv);
    // Print the name of the path colored
    printf("+ \033[1;36m%s\033[0m\n", path);
    // Print the tree
    print_tree(path, 1);
    // Print the number of files and directories
    print_entities_no();
    return 0;
}
