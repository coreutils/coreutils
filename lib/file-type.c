char const *
file_type (struct stat const *st)
{
  /* See POSIX 1003.1-2001 for these formats.

     To keep diagnostics grammatical in English, the returned string
     must start with a consonant.  */

  if (S_ISREG (st->st_mode))
    return st->st_size == 0 ? _("regular empty file") : _("regular file");

  if (S_ISDIR (st->st_mode))
    return _("directory");

  if (S_ISBLK (st->st_mode))
    return _("block special file");

  if (S_ISCHR (st->st_mode))
    return _("character special file");

  if (S_ISFIFO (st->st_mode))
    return _("fifo");

  if (S_ISLNK (st->st_mode))
    return _("symbolic link");

  if (S_ISSOCK (st->st_mode))
    return _("socket");

  if (S_TYPEISMQ (st))
    return _("message queue");

  if (S_TYPEISSEM (st))
    return _("semaphore");

  if (S_TYPEISSHM (st))
    return _("shared memory object");

  if (S_TYPEISTMO (st))
    return _("typed memory object");

  return _("weird file");
}
