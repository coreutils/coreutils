size_t
__fpending (FILE *fp)
{
#if STREAM_FPENDING_GLIBC2
  return fp->_IO_write_ptr - fp->_IO_write_base;
#endif

#if STREAM_FPENDING__PTR
  /* Traditional Unix */
  return fp->_ptr - fp->_base;
#endif

#if STREAM_FPENDING__P
  /* BSD */
  return fp->_p - fp->_bf._base;
#endif

#if STREAM_FPENDING__P
  /* SCO, Unixware */
  return fp->__ptr - fp->__base;
#endif

#if STREAM_FPENDING__BUFP
  /* old glibc? */
  return fp->__bufp - fp->__buffer;
#endif

#if STREAM_FPENDING__PPTR
  /* old glibc iostream? */
  return fp->_pptr - fp->_pbase;
#endif

#if STREAM_FPENDING__PTR_DEREF
  /* VMS */
  return (*fp)->_ptr - (*fp)->_base;
#endif

#if STREAM_FPENDING_NOT_AVAILABLE
  /* e.g., DGUX R4.11 */
  return 1; /* i.e. the info is not available */
#endif

}
