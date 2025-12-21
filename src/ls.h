enum ls_modes
{
  /* This is for the 'ls' program.  */
  LS_LS,

  /* This is for the 'dir' program.  */
  LS_MULTI_COL,

  /* This is for the 'vdir' program.  */
  LS_LONG_FORMAT
};

extern enum ls_modes ls_mode;
