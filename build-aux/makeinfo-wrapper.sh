#!/bin/sh
# makeinfo wrapper that post-processes HTML output to replace _002d with -,
# only on lines containing "option", corresponding to our @optAnchor macro.
# Note texi uses "-" in anchors for spaces, hence why it escapes - with _002d.

makeinfo "$@" || exit

process_html()
{
  sed_anchor_cleanup=\
'/id=.*_002doption/{ s/id="\([^"]*\)_002doption/id="\1/g; s/_002d/-/g; }'

  sed -e "$sed_anchor_cleanup" "$1" > "$1.t" &&
  mv "$1.t" "$1"
}

case " $* " in
  *" --html"*)
    # Find the output file/directory
    output=""
    next_is_output=false
    for arg in "$@"; do
      if [ "$next_is_output" = true ]; then
        output="$arg"
        break
      fi
      case "$arg" in
        -o) next_is_output=true ;;
        --output=*) output="${arg#--output=}" ;;
      esac
    done

    # Process the output file/directory
    if test -n "$output"; then
      test -f "$output" && NAMES='*' || NAMES='*.html'
      find "$output" -name "$NAMES" -type f |
        # dash doesn't support read -d '' yet.
        while IFS= read -r htmlfile; do process_html "$htmlfile"; done
    fi
    ;;
esac
