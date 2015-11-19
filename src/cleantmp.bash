#!/bin/bash
if [ "$1" == "--help" ]; then
  (
    echo "Deletes files and directories in /tmp older than the latest boot."
    echo "Usage: $0"
  ) >&2
  exit 0
fi

cygdatetime() {
  dt="$1"
  DATEF="$2"
  TIMEF="$3"
  [ -z "$DATEF" ] && DATEF="$(reg query 'HKCU\Control Panel\International' /v sShortDate | sed -ne '/sShortDate/{s|.* \([^ ]*\)$|\1|;p}')"
  [ -z "$TIMEF" ] && TIMEF="$(reg query 'HKCU\Control Panel\International' /v sTimeFormat | sed -ne '/sTimeFormat/{s|.* \([^ ]*\)$|\1|;p}')"
  d="${dt% *}"
  t="${dt#* }"
  re_y="$(sed -e 's_[A-Za-xz]_._g' -e 's_yy*_\\(.*\\)_g' <<< "$DATEF")"
  re_m="$(sed -e 's_[A-LN-Za-z]_._g' -e 's_MM*_\\(.*\\)_g' <<< "$DATEF")"
  re_d="$(sed -e 's_[A-Za-ce-z]_._g' -e 's_dd*_\\(.*\\)_g' <<< "$DATEF")"
  re_H="$(sed -e 's_[A-GI-Za-z]_._g' -e 's_HH*_\\(.*\\)_g' <<< "$TIMEF")"
  re_M="$(sed -e 's_[A-Za-ln-z]_._g' -e 's_mm*_\\(.*\\)_g' <<< "$TIMEF")"
  re_S="$(sed -e 's_[A-Za-rt-z]_._g' -e 's_ss*_\\(.*\\)_g' <<< "$TIMEF")"
  nd="$(sed -e "s_${re_y}_\\1_" <<< "$d")-$(sed -e "s_${re_m}_\\1_" <<< "$d")-$(sed -e "s_${re_d}_\\1_" <<< "$d")"
  nt="$(sed -e "s_${re_H}_\\1_" <<< "$t"):$(sed -e "s_${re_M}_\\1_" <<< "$t"):$(sed -e "s_${re_S}_\\1_" <<< "$t")"
  echo "${nd}T${nt}$(date +%z)"
}

BOOTTIME="$(net statistics workstation | sed -ne '4{s|.* \([^ ][^ ]* [^ ][^ ]* *\)$|\1|;p}')"
if [ ! -z "$BOOTTIME" ]; then
  BOOTTIME="$(cygdatetime "$BOOTTIME")"
else
  BOOTTIME="$(wmic os get lastbootuptime | sed -ne '2{s|\(....\)\(..\)\(..\)\(..\)\(..\)\(..\)\.\(.*\)+\(.*\)|\1-\2-\3 \4:\5:\6,\7|;p}')"
  if [ ! -z "$BOOTTIME" ]; then
    BOOTTIME="$(date -Iseconds --date="$BOOTTIME")"
  else
    echo "Error: Unable to determine boot time. Aborting." >&2
    exit 1
  fi
fi
BOOTTIME="$(date +%s --date="$BOOTTIME")"

ls -AgGtu --time-style=+%s /tmp | sed -e 's,^[^ ][^ ]*  *[^ ][^ ]*  *[^ ][^ ]*  *,,' | while read atime file; do
  file="/tmp/$file"
  ctime="$(ls -AgGtc --time-style=+%s "$file" | sed -e 's,^[^ ][^ ]*  *[^ ][^ ]*  *[^ ][^ ]*  *,,' -e 's,^\([^ ][^ ]*\) .*,\1,g')"
  mtime="$(ls -AgGt --time-style=+%s "$file" | sed -e 's,^[^ ][^ ]*  *[^ ][^ ]*  *[^ ][^ ]*  *,,' -e 's,^\([^ ][^ ]*\) .*,\1,g')"
  if [ -f "$file" ]; then
    if [ "$atime" -lt "$BOOTTIME" ] && [ "$ctime" -lt "$BOOTTIME" ] && [ "$mtime" -lt "$BOOTTIME" ]; then
      if [ -O "$file" ]; then
        rm "$file"
      else
        echo "Warning: $file was not cleaned up because it is not owned by the current user." >&2
      fi
    fi
  elif [ -d "$file" ]; then
    if [ -n "$(
        find "$file" -type b
        find "$file" -type c
        find "$file" -type p
        find "$file" -type s
      )" ]; then
        echo "Warning: $file contains special (block/character/named pipe/socket) files and was not cleaned up." >&2
    else
      maxtime="$((
          find "$file" -exec ls -dAgGtu --time-style=+%s {} \;
          find "$file" -exec ls -dAgGtc --time-style=+%s {} \;
          find "$file" -exec ls -dAgGt --time-style=+%s {} \;
        ) | sed -e 's,^[^ ][^ ]*  *[^ ][^ ]*  *[^ ][^ ]*  *,,' | sort -n | sed -ne '${s,^\([^ ][^ ]*\) .*,\1,;p}')"
      if [ "$maxtime" -lt "$BOOTTIME" ]; then
        owned=1
        find "$file" | while read f; do
          [ ! -O "$f" ] && owned=0
        done
        if [ "$owned" -eq 1 ]; then
          rm -R "$file"
        else
          echo "Warning: $file was not cleaned up because some of its files are not owned by the current user." >&2
        fi
      fi
    fi
  elif [ -b "$file" ]; then
    echo "Warning: $file is a block special file and was not cleaned up." >&2
  elif [ -c "$file" ]; then
    echo "Warning: $file is a character special file and was not cleaned up." >&2
  elif [ -h "$file" ]; then
    echo "Warning: $file is a symbolic link file and was not cleaned up." >&2
  elif [ -p "$file" ]; then
    echo "Warning: $file is a named pipe file and was not cleaned up." >&2
  elif [ -S "$file" ]; then
    echo "Warning: $file is a socket file and was not cleaned up." >&2
  else
    echo "Warning: $file is not a file or a directory and was not cleaned up." >&2
  fi
done


