void hash_init (void);
void forget_created (ino_t ino, dev_t dev);
char *remember_copied (char const *node, ino_t ino, dev_t dev)
  _GL_ATTRIBUTE_NONNULL ();
char *src_to_dest_lookup (ino_t ino, dev_t dev);
