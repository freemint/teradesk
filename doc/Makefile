HCP ?= hcp

.PHONY: all

%.hyp: %.stg
	@if command -v $(HCP) >/dev/null 2>&1 && test -f $<; then \
		$(HCP) -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

%.ref: %.hyp
	@test -f $@

all: en/teradesk.hyp de/teradesk.hyp ru/teradesk.hyp 


en/teradesk.hyp: en/teradesk.stg

de/teradesk.hyp: de/teradesk.stg

ru/teradesk.hyp: ru/teradesk.stg


clean::
	$(RM) */*.hyp */*.ref */*.uls */*.ulh */*.uhs */*.ula */*.uha
