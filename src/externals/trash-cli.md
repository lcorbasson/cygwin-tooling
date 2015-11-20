# Commands to develop for a `trash-cli` equivalent
trash-empty
trash-empty -a --all (default)
trash-empty -f --force
trash-empty -q --quiet
trash-empty DAYS
trash-empty --drive=DRIVE (Cygwin-specific)
trash-list
trash-list DIR
trash-list -l DIR
trash-list -r --recursive (default) DIR
trash-list -a --all (default)
trash-put FILE
trash-restore (shows a menu)
trash-restore --last FILE
trash-restore --show-trash-id
trash-restore --trash-id ID
trash-restore -dDATE --deletion-date=DATE FILE
trash-restore FILE (shows a menu if needed)
trash-restore FILE TOFILE (shows a menu if needed)
trash-rm PATTERN
