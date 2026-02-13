ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src tests
EXTRA_DIST = ckpool.conf ckproxy.conf README README-SOLOMINING

.PHONY: check-perf

check-perf:
	$(MAKE) -C tests check-perf
