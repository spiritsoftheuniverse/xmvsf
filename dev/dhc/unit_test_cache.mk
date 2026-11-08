# XMVSF local ROM-free regression cache. Invoked only by BUILD_TEST.sh.
# The .d file is compiler-generated (-MMD/-MP), so changing any
# transitive project header rebuilds ONLY the affected test.
.DEFAULT_GOAL := unit
.PHONY: unit
ifndef UNIT_SOURCE
$(error UNIT_SOURCE is required)
endif
ifndef UNIT_OUTPUT
$(error UNIT_OUTPUT is required)
endif
ifndef UNIT_SIGNATURE
$(error UNIT_SIGNATURE is required)
endif
unit: $(UNIT_OUTPUT)
$(UNIT_OUTPUT): $(UNIT_SOURCE) $(UNIT_SIGNATURE)
	@mkdir -p "$(dir $@)"
	@echo "UNIT COMPILE: $(notdir $@)"
	@g++ -std=c++11 -Wall -Wextra -Werror \
	  -I"$(UNIT_INCLUDE_CAPCOM)" -I"$(UNIT_INCLUDE_SDL)" \
	  -MMD -MP -MF "$@.d" -MT "$@" \
	  "$(UNIT_SOURCE)" -o "$@.pending"
	@mv -f "$@.pending" "$@"
-include $(UNIT_OUTPUT).d
