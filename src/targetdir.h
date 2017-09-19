struct stat;
enum targetdir { TARGETDIR_VULNERABLE = -1, TARGETDIR_NOT, TARGETDIR_OK };
enum targetdir targetdir_operand_type (char *restrict,
                                       struct stat const *restrict);
