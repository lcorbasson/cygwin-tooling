# cygwin-tooling
Various code snippets for your `~/bin`

## `cleantmp`
Cleans the leftover files under `/tmp`, i.e. files you own that are older than the last boot time of Windows.

## `sudo`
A `sudo` equivalent, raising an UAC prompt; for now, environment variables are not kept AFAIK. If one day Cygwin ships with a real `sudo` command, it will be silently invoked. Inspired by http://stackoverflow.com/a/21024592/1424087

