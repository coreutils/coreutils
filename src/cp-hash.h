/* For created inodes, a pointer in the search structure to this
   character identifies the inode as being new.  */
extern char new_file;

void hash_init __P ((unsigned int modulus, unsigned int entry_tab_size));
void forget_all __P ((void));
char *hash_insert __P ((ino_t ino, dev_t dev, const char *node));
char *remember_copied __P ((const char *node, ino_t ino, dev_t dev));
int remember_created __P ((const char *path));
