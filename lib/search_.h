/* For use with hsearch(3).  */
#ifndef __COMPAR_FN_T
# define __COMPAR_FN_T
typedef int (*__compar_fn_t) (const void *, const void *);

# ifdef	__USE_GNU
typedef __compar_fn_t comparison_fn_t;
# endif
#endif

/* The tsearch routines are very interesting. They make many
   assumptions about the compiler.  It assumes that the first field
   in node must be the "key" field, which points to the datum.
   Everything depends on that.  */
/* For tsearch */
typedef enum
{
  preorder,
  postorder,
  endorder,
  leaf
}
VISIT;

/* GCC 2.95 and later have "__restrict"; C99 compilers have
   "restrict", and "configure" may have defined "restrict".  */
#ifndef __restrict
# if ! (2 < __GNUC__ || (2 == __GNUC__ && 95 <= __GNUC_MINOR__))
#  if defined restrict || 199901L <= __STDC_VERSION__
#   define __restrict restrict
#  else
#   define __restrict
#  endif
# endif
#endif

/* Search for an entry matching the given KEY in the tree pointed to
   by *ROOTP and insert a new element if not found.  */
extern void *tsearch (const void *__key, void **__rootp,
		      __compar_fn_t __compar);

/* Search for an entry matching the given KEY in the tree pointed to
   by *ROOTP.  If no matching entry is available return NULL.  */
extern void *tfind (const void *__key, void *const *__rootp,
		    __compar_fn_t __compar);

/* Remove the element matching KEY from the tree pointed to by *ROOTP.  */
extern void *tdelete (const void *__restrict __key,
		      void **__restrict __rootp,
		      __compar_fn_t __compar);

#ifndef __ACTION_FN_T
# define __ACTION_FN_T
typedef void (*__action_fn_t) (const void *__nodep, VISIT __value,
			       int __level);
#endif

/* Walk through the whole tree and call the ACTION callback for every node
   or leaf.  */
extern void twalk (const void *__root, __action_fn_t __action);

#ifdef __USE_GNU
/* Callback type for function to free a tree node.  If the keys are atomic
   data this function should do nothing.  */
typedef void (*__free_fn_t) (void *__nodep);

/* Destroy the whole tree, call FREEFCT for each node or leaf.  */
extern void tdestroy (void *__root, __free_fn_t __freefct);
#endif
