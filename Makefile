# -*- coding: utf-8 -*-
# :Project:   python-rapidjson -- Development Makefile
# :Author:    Lele Gaifax <lele@metapensiero.it>
# :License:   MIT License
# :Copyright: © 2017 Lele Gaifax
#

export TOPDIR := $(CURDIR)
export VENVDIR := $(TOPDIR)/env
export PYTHON := $(VENVDIR)/bin/python
export SHELL := /bin/bash
export SYS_PYTHON := $(shell which python3.6 || which python3)

all: rapidjson/license.txt virtualenv help

rapidjson/license.txt:
	git submodule update --init

.PHONY: help
help::
	@printf "\nBuild targets\n"
	@printf   "=============\n\n"

help::
	@printf "build\n\tbuild the module\n"

build: virtualenv
	$(PYTHON) setup.py build_ext --inplace

help::
	@printf "clean\n\tremove rebuildable stuff\n"

.PHONY: clean
clean:
	$(MAKE) -C docs clean
	rm -f *.so

help::
	@printf "distclean\n\tremove anything superfluous\n"

.PHONY: distclean
distclean:: clean
	rm -rf build dist
	git submodule deinit --all

help::
	@printf "doc\n\tbuild Sphinx documentation\n"

SPHINXBUILD := $(VENVDIR)/bin/sphinx-build

.PHONY: doc
doc:
	$(MAKE) -C docs SPHINXBUILD=$(SPHINXBUILD) html

help::
	@printf "check\n\trun the test suite\n"

PYTEST = $(VENVDIR)/bin/pytest $(PYTEST_OPTIONS)

.PHONY: check
check: build
	$(PYTEST) tests/
	$(MAKE) -C docs SPHINXBUILD=$(SPHINXBUILD) doctest

help::
	@printf "benchmarks\n\trun the benchmarks\n"

.PHONY: benchmarks
benchmarks: build
	$(PYTEST) benchmarks/

help::
	@printf "benchmarks-other\n\trun the benchmarks against other engines\n"

.PHONY: benchmarks-others
benchmarks-others: PYTEST_OPTIONS = --compare-other-engines
benchmarks-others: benchmarks

help::
	@printf "benchmarks-table\n\tproduce a reST table out of benchmarks-other results\n"

.PHONY: benchmarks-tables
benchmarks-tables: PYTEST_OPTIONS = --compare-other-engines --benchmark-json=comparison.json
benchmarks-tables: benchmarks
	$(PYTHON) benchmarks/tablize.py

include Makefile.virtualenv
include Makefile.release
