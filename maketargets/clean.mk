# The 'clean' target

clean:
	$(E) "  CLEAN"
	$(RM) $(INCLUDE_DIR)ficspaths.h
	$(RM) $(OBJS)
	$(RM) $(AP_OBJS)
	$(RM) $(MR_OBJS)
	$(RM) $(TGTS)
