PYTHON ?= python3
VASM ?= $(CURDIR)/local/vasm/vasmm68k_mot

.PHONY: bootstrap baseline diagnostic test fixture

bootstrap:
	sh tools/bootstrap_vasm.sh

baseline:
	$(PYTHON) tools/build_baseline.py --vasm "$(VASM)"

diagnostic:
	$(PYTHON) tools/build_diagnostic.py

test:
	$(PYTHON) -m unittest discover -s tests -v

fixture:
	$(PYTHON) tools/make_fixture.py build/fixtures/mod.baseline
