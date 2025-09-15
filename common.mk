# common.mk  --  common rules

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) -I $(INCLUDE_DIR) $(CPPFLAGS) -c -o $@ $<
.cpp.o:
	$(E) "  CXX     " $@
	$(Q) $(CXX) $(CXXFLAGS) -I $(INCLUDE_DIR) $(CPPFLAGS) -c -o $@ $<
