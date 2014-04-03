PARSE = ../bin/msgparse.scm

STROKES = top-bottom bottom-top left-right right-left

FASTFILES = $(STROKES:%=fast-%)
SLOWFILES = $(STROKES:%=slow-%)
ALLFILES = $(FASTFILES:%=pen-up/%) $(FASTFILES:%=pen-down/%) $(SLOWFILES:%=pen-up/%) $(SLOWFILES:%=pen-down/%)
OUTFILES = $(ALLFILES:%=parsed/%)

all:	dirs $(OUTFILES)
	echo done

parsed/% : raw/%
#	$(PARSE) < $< > $@
	cp $< $@
	perl -pi -e 's/^\[4T0:([0-9a-z]+).*/\1\n/misg' $@

dirs:
	mkdir -p parsed/pen-up parsed/pen-down

clean:
	rm -rf parsed

