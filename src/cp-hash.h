void hash_init PARAMS ((void));
void forget_all PARAMS ((void));
char *remember_copied PARAMS ((const char *node, ino_t ino, dev_t dev));
int remember_created PARAMS ((const char *path));
