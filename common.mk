# common.mk  --  common rules

.c.o:
	$(E) "  CC      " $@
	$(Q) $(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
.cpp.o:
	$(E) "  CXX     " $@
	$(Q) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
