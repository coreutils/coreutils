/* For created inodes, a pointer in the search structure to this
   character identifies the inode as being new.  */
extern char new_file;

void hash_init PARAMS ((unsigned int modulus, unsigned int entry_tab_size));
void forget_all PARAMS ((void));
char *remember_copied PARAMS ((const char *node, ino_t ino, dev_t dev));
int remember_created PARAMS ((const char *path));
