PYTHON ?= python3
VASM ?= $(CURDIR)/local/vasm/vasmm68k_mot

.PHONY: bootstrap baseline dev diagnostic modcheck modcheck-amiga core-tests core-mutations project-mutations converter test fixture

bootstrap:
	sh tools/bootstrap_vasm.sh

baseline:
	$(PYTHON) tools/build_baseline.py --vasm "$(VASM)"

dev:
	$(PYTHON) tools/build_dev.py --vasm "$(VASM)"

core-tests:
	$(PYTHON) tools/build_core_tests.py

core-mutations:
	sh tools/test_core_mutations.sh

project-mutations:
	sh tools/test_project_mutations.sh

converter:
	mkdir -p build/host
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror -Isrc/core tools/pt24g_convert.c src/platform/file_save.c src/core/document.c src/core/pp20.c src/core/safe_save.c src/core/mod_project.c src/core/mod_inspect.c src/core/project.c src/core/channels.c src/core/pcm.c -o build/host/PT24GConvert

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
