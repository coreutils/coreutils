enum chown_modes
{
  /* This is for the 'chown' program.  */
  CHOWN_CHOWN,

  /* This is for the 'chgrp' program.  */
  CHOWN_CHGRP,
};

extern enum chown_modes chown_mode;
