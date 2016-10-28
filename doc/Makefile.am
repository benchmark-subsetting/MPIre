RONN_FLAGS=\
	--manual="MPIre manual" \
	--organization="Université de Versailles St-Quentin-en-Yvelines"

man_MANS = man1/mpire-tutorial.1
doc_DATA = html/mpire-tutorial.1.html

man1/%.1: %.1.md
	mkdir -p man1
	$(RONN) $(RONN_FLAGS) --pipe -r $< > $@

html/%.1.html: %.1.md
	mkdir -p html
	$(RONN) $(RONN_FLAGS) --pipe -5 $< > $@

clean-local:
	rm -rf html man1

EXTRA_DIST=\
	mpire-tutorial.1.md
