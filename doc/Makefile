BUILD := $(shell gcc -dumpmachine)

ifeq ($(HCP),)
HCP = ${BUILD}-hcp
endif

.PHONY: all

%.hyp: %.stg
	 @$(AM_V_GEN)HCP="$(HCP)"; \
	 if test "$$HCP" != "" && $$HCP --version >/dev/null 2>&1; then \
	   $$HCP -o$@ $< || exit 1; \
	 else \
	   echo "HCP not found, output left in $<" >&2; \
	 fi

%.ref: %.hyp
	@test -f $@

all: en/teradesk.hyp de/teradesk.hyp ru/teradesk.hyp 


en/teradesk.hyp: en/teradesk.stg

de/teradesk.hyp: de/teradesk.stg

ru/teradesk.hyp: ru/teradesk.stg


clean::
	$(RM) */*.hyp */*.ref */*.uls */*.ulh */*.uhs */*.ula */*.uha
