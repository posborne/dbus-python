check-local::
	@fail=0; \
	cd $(srcdir) || exit $$?; \
	if test -n "$(check_misc_sources)"; then \
		echo check-coding-style.mk: checking misc sources...; \
		top_srcdir=$(top_srcdir) \
		sh $(top_srcdir)/tools/check-whitespace.sh \
			$(check_misc_sources) || fail=1; \
	fi; \
	if test -n "$(check_py_sources)"; then \
		echo check-coding-style.mk: checking Python sources...; \
		top_srcdir=$(top_srcdir) \
		sh $(top_srcdir)/tools/check-py-style.sh \
			$(check_py_sources) || fail=1; \
	fi;\
	if test -n "$(check_c_sources)"; then \
		echo check-coding-style.mk: checking C sources...; \
		top_srcdir=$(top_srcdir) \
		sh $(top_srcdir)/tools/check-c-style.sh \
			$(check_c_sources) || fail=1; \
	fi;\
	if test yes = "@ENABLE_CODING_STYLE_CHECKS@"; then \
		exit "$$fail";\
	else \
		exit 0;\
	fi
