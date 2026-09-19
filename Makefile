PYTHON ?= python3
VASM ?= $(CURDIR)/local/vasm/vasmm68k_mot

.PHONY: bootstrap baseline diagnostic modcheck modcheck-amiga test fixture

bootstrap:
	sh tools/bootstrap_vasm.sh

baseline:
	$(PYTHON) tools/build_baseline.py --vasm "$(VASM)"

diagnostic:
	$(PYTHON) tools/build_diagnostic.py

modcheck:
	mkdir -p build/host
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -Isrc/core tools/modcheck.c src/core/mod_inspect.c -o build/host/PTModCheck

modcheck-amiga:
	$(PYTHON) tools/build_diagnostic.py --tool PTModCheck

test:
	$(PYTHON) -m unittest discover -s tests -v

fixture:
	$(PYTHON) tools/make_fixture.py build/fixtures/mod.baseline
