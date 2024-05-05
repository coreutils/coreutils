#define isodigit(c) ('0' <= (c) && (c) <= '7')
#define octtobin(c) ((c) - '0')
/* FIXME-maybe: macros names may be misleading: "bin" may be interpreted as
   "having a value of (char)'0' or (char)'1'". Rename? `hextonative`?
   `hextoint`?  */
#define hextobin(c) ('a' <= (c) && (c) <= 'f' ? (c) - 'a' + 10 : \
                     'A' <= (c) && (c) <= 'F' ? (c) - 'A' + 10 : (c) - '0')
