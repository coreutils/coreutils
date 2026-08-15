static inline bool _GL_ATTRIBUTE_CONST
isoct (char c)
{
  return '0' <= c && c <= '7';
}
static inline int _GL_ATTRIBUTE_CONST
fromoct (char c)
{
  return c - '0';
}
static inline int _GL_ATTRIBUTE_CONST
fromhex (char c)
{
  return ('a' <= c && c <= 'f' ? c - 'a' + 10
          : 'A' <= c && c <= 'F' ? c - 'A' + 10 : c - '0');
}
