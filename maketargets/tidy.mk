# The 'tidy' target

TIDY = clang-tidy
TIDYFLAGS = -checks=-clang-analyzer-security.insecureAPI.strcpy,-clang-analyzer-optin.performance.Padding,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling -quiet

tidy: $(INCLUDE_DIR)ficspaths.h
	$(E) "  TIDY"
	$(TIDY) $(SRCS) $(TIDYFLAGS) -- -I $(INCLUDE_DIR) $(CPPFLAGS)
