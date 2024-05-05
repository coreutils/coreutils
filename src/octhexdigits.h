#define isoct(c) ('0' <= (c) && (c) <= '7')
#define fromoct(c) ((c) - '0')
#define fromhex(c) ('a' <= (c) && (c) <= 'f' ? (c) - 'a' + 10 : \
                     'A' <= (c) && (c) <= 'F' ? (c) - 'A' + 10 : (c) - '0')
