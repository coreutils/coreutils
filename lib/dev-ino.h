#ifndef DEV_INO_H
# define DEV_INO_H 1

struct dev_ino
{
  ino_t st_ino;
  dev_t st_dev;
};

#endif
