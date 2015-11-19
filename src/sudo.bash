#!/bin/bash
if [ -z "$(which sudo --all | sed -e "/^${0//\//\\/}/d")" ]; then
  cygstart --action=runas "$@"
else
  "$(which sudo --all | sed -e "/^${0//\//\\/}/d" | head -1)" "$@"
fi
exit $?

