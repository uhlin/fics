# The 'tidy' target

TIDY = clang-tidy
TIDYFLAGS = -checks=-clang-analyzer-security.insecureAPI.strcpy,-clang-analyzer-optin.performance.Padding,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling -quiet
FICS_CLANG_TIDYFLAGS ?=

tidy: $(INCLUDE_DIR)ficspaths.h
	$(TIDY) $(SRCS) $(TIDYFLAGS) $(FICS_CLANG_TIDYFLAGS) -- \
	-I $(INCLUDE_DIR) $(CPPFLAGS)
