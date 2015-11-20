# vim: noexpandtab filetype=make

bin_SCRIPTS = cleantmp sudo

CLEANFILES = $(bin_SCRIPTS)

cleantmp: cleantmp.bash
	cp $< $@
	chmod +x $@

sudo: sudo.bash
	cp $< $@
	chmod +x $@

