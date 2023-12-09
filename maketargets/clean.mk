# The 'clean' target

clean:
	$(E) "  CLEAN"
#	$(RM) $(OBJS)
	$(RM) $(AP_OBJS)
	$(RM) $(MR_OBJS)
	$(RM) $(TGTS)
