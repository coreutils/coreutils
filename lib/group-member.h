#ifndef GROUP_MEMBER_H_
# define GROUP_MEMBER_H_ 1

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

int
  group_member PARAMS ((gid_t));

#endif /* GROUP_MEMBER_H_ */
