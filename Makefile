TARGETS = iap fw

.PHONY: all $(TARGETS)

all: $(TARGETS)

$(TARGETS):
	@$(MAKE) -C $@

clean:
	for target in $(TARGETS); do \
		$(MAKE) -C $$target clean; \
	done

program:
	for target in $(TARGETS); do \
		$(MAKE) -C $$target program; \
	done