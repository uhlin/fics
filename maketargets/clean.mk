# The 'clean' target

clean:
	$(E) "  CLEAN"
	$(RM) $(INCLUDE_DIR)ficspaths.h
	$(RM) $(OBJS)
	$(RM) $(AP_OBJS)
	$(RM) $(MR_OBJS)
	$(RM) $(TGTS)
	$(RM) $(ROOT)PVS-Studio.log
	$(RM) $(ROOT)strace_out
	$(RM) -R $(ROOT)tmp
