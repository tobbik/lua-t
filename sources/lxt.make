# vim: ft=make


$(COMPDIR)/Lua/src/lua: $(COMPDIR)/xt/xt.so
	cd $(COMPDIR)/Lua ; $(MAKE) CC=$(CC) LD=$(LD) \
		MYCFLAGS=" -g -fPIC -DL_XT_ROOT=\"\\\"$(OUTDIR)/\\\"\" -I$(COMPDIR)/xt" \
		MYOBJS="$(COMPDIR)/xt/l_xt*.o" \
		linux

clean:
	rm -rf $(COMPDIR)/Lua $(COMPDIR)/xt
