#!/usr/bin/env python3
import os
import sys
import stat
import argparse

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

WHO_MAP = {
    'u': stat.S_IRWXU,
    'g': stat.S_IRWXG,
    'o': stat.S_IRWXO,
    'a': stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO
}

PERM_MAP = {
    'r': (stat.S_IRUSR, stat.S_IRGRP, stat.S_IROTH),
    'w': (stat.S_IWUSR, stat.S_IWGRP, stat.S_IWOTH),
    'x': (stat.S_IXUSR, stat.S_IXGRP, stat.S_IXOTH)
}

def error(msg, force=False):
    if not force:
        print(f"chmod: {msg}", file=sys.stderr)

def parse_octal(mode):
    try:
        return int(mode, 8)
    except ValueError:
        raise ValueError(f"invalid mode: '{mode}'")

def apply_octal(path, mode, verbose=False, changes_only=False):
    st = os.lstat(path)
    old = stat.S_IMODE(st.st_mode)
    if old != mode:
        os.chmod(path, mode)
        if verbose or changes_only:
            print(f"mode of '{path}' changed from {oct(old)[2:]} to {oct(mode)[2:]}")
    elif verbose:
        print(f"mode of '{path}' retained as {oct(old)[2:]}")

def parse_symbolic(expr, st_mode, is_dir):
    mode = st_mode
    umask = os.umask(0)
    os.umask(umask)

    clauses = expr.split(',')
    for clause in clauses:
        i = 0
        who = set()
        while i < len(clause) and clause[i] in 'ugoa':
            who.add(clause[i])
            i += 1
        if not who:
            who = {'a'}

        if i >= len(clause) or clause[i] not in '+-=':
            raise ValueError(f"invalid mode: '{expr}'")
        op = clause[i]
        i += 1

        perms = set()
        while i < len(clause):
            perms.add(clause[i])
            i += 1

        for w in who:
            for p in perms:
                if p == 'X':
                    if not is_dir and not (mode & (stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)):
                        continue
                    bits = PERM_MAP['x']
                else:
                    bits = PERM_MAP.get(p)
                    if not bits:
                        continue

                idx = 'ugo'.index(w) if w != 'a' else None
                if idx is not None:
                    bit = bits[idx]
                else:
                    bit = sum(bits)

                if op == '+':
                    mode |= bit
                elif op == '-':
                    mode &= ~bit
                elif op == '=':
                    clear = WHO_MAP[w]
                    mode &= ~clear
                    mode |= bit

    return mode

def chmod_path(path, mode_expr, args):
    try:
        st = os.lstat(path)
        is_dir = stat.S_ISDIR(st.st_mode)
        old_mode = stat.S_IMODE(st.st_mode)

        if mode_expr.isdigit():
            new_mode = parse_octal(mode_expr)
        else:
            new_mode = parse_symbolic(mode_expr, old_mode, is_dir)

        if new_mode != old_mode:
            os.chmod(path, new_mode)
            if args.verbose or args.changes_only:
                print(f"mode of '{path}' changed from {oct(old_mode)[2:]} to {oct(new_mode)[2:]}")
        elif args.verbose:
            print(f"mode of '{path}' retained as {oct(old_mode)[2:]}")

    except Exception as e:
        error(str(e), args.force)

def walk_paths(paths, args, mode):
    for path in paths:
        if args.recursive and os.path.isdir(path):
            for root, dirs, files in os.walk(path, followlinks=args.follow):
                chmod_path(root, mode, args)
                for name in dirs + files:
                    chmod_path(os.path.join(root, name), mode, args)
        else:
            chmod_path(path, mode, args)

def main():
    parser = argparse.ArgumentParser(prog="chmod")
    parser.add_argument("-R", "--recursive", action="store_true")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-c", "--changes-only", action="store_true")
    parser.add_argument("-f", "--force", action="store_true")
    parser.add_argument("-L", dest="follow", action="store_true")
    parser.add_argument("-P", dest="nofollow", action="store_true")
    parser.add_argument("mode")
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()

    try:
        walk_paths(args.paths, args, args.mode)
    except Exception as e:
        error(str(e))
        sys.exit(EXIT_FAILURE)

    sys.exit(EXIT_SUCCESS)

if __name__ == "__main__":
    main()
