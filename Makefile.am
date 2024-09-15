# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Pattern rule to print variables, e.g. make print-top_srcdir
print-%:
	@echo $* = $($*)

ACLOCAL_AMFLAGS = -I build-aux/m4
SUBDIRS = src
if ENABLE_MAN
SUBDIRS += doc/man
endif
.PHONY: deploy FORCE

export PYTHONPATH

if BUILD_POCKETCOIN_LIBS
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpocketcoinconsensus.pc
endif

POCKETCOIND_BIN=$(top_builddir)/src/$(POCKETCOIN_DAEMON_NAME)$(EXEEXT)
POCKETCOIN_QT_BIN=$(top_builddir)/src/qt/$(POCKETCOIN_GUI_NAME)$(EXEEXT)
POCKETCOIN_CLI_BIN=$(top_builddir)/src/$(POCKETCOIN_CLI_NAME)$(EXEEXT)
POCKETCOIN_TX_BIN=$(top_builddir)/src/$(POCKETCOIN_TX_NAME)$(EXEEXT)
# POCKETCOIN_WALLET_BIN=$(top_builddir)/src/$(POCKETCOIN_WALLET_TOOL_NAME)$(EXEEXT)
POCKETCOIN_WIN_INSTALLER=$(PACKAGE)_$(PACKAGE_VERSION)_win_x64_setup.exe
POCKETCOIN_LINUX_INSTALLER = $(PACKAGE)_$(PACKAGE_VERSION)_linux_x64
POCKETCOIN_LINUX_INSTALLER_NAME = $(PACKAGE)_$(PACKAGE_VERSION)_linux_x64

empty :=
space := $(empty) $(empty)

OSX_APP=Pocketcoin-Qt.app
OSX_VOLNAME = $(subst $(space),-,$(PACKAGE_NAME))
OSX_DMG = $(OSX_VOLNAME).dmg
OSX_BACKGROUND_SVG=background.svg
OSX_BACKGROUND_IMAGE=background.tiff
OSX_BACKGROUND_IMAGE_DPIS=36 72
OSX_DEPLOY_SCRIPT=$(top_srcdir)/contrib/macdeploy/macdeployqtplus
OSX_INSTALLER_ICONS=$(top_srcdir)/src/qt/res/icons/pocketcoin.icns
OSX_PLIST=$(top_builddir)/share/qt/Info.plist #not installed

DIST_CONTRIB = \
	       $(top_srcdir)/contrib/linearize/linearize-data.py \
	       $(top_srcdir)/contrib/linearize/linearize-hashes.py

DIST_SHARE = \
  $(top_srcdir)/share/genbuild.sh \
  $(top_srcdir)/share/rpcauth

BIN_CHECKS=$(top_srcdir)/contrib/devtools/symbol-check.py \
           $(top_srcdir)/contrib/devtools/security-check.py

WINDOWS_PACKAGING = $(top_srcdir)/share/pixmaps/pocketcoin.ico \
  $(top_srcdir)/share/pixmaps/nsis-header.bmp \
  $(top_srcdir)/share/pixmaps/nsis-wizard.bmp \
  $(top_srcdir)/doc/README_windows.txt

OSX_PACKAGING = $(OSX_DEPLOY_SCRIPT) $(OSX_INSTALLER_ICONS) \
  $(top_srcdir)/contrib/macdeploy/$(OSX_BACKGROUND_SVG) \
  $(OSX_DSSTORE_GEN) \
  $(top_srcdir)/contrib/macdeploy/detached-sig-apply.sh \
  $(top_srcdir)/contrib/macdeploy/detached-sig-create.sh

COVERAGE_INFO = $(COV_TOOL_WRAPPER) baseline.info \
  test_pocketcoin_filtered.info total_coverage.info \
  baseline_filtered.info functional_test.info functional_test_filtered.info \
  test_pocketcoin_coverage.info test_pocketcoin.info fuzz.info fuzz_filtered.info fuzz_coverage.info

dist-hook:
	-$(GIT) archive --format=tar HEAD -- src/clientversion.cpp | $(AMTAR) -C $(top_distdir) -xf -

$(POCKETCOIN_WIN_INSTALLER): all-recursive
	$(MKDIR_P) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(POCKETCOIND_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(POCKETCOIN_QT_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(POCKETCOIN_CLI_BIN) $(top_builddir)/release
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM) $(POCKETCOIN_TX_BIN) $(top_builddir)/release
	@test -f $(MAKENSIS) && echo 'OutFile "$@"' | cat $(top_builddir)/share/setup.nsi - | $(MAKENSIS) -V2 - || \
	  echo error: could not build $@
	@echo built $@

$(POCKETCOIN_LINUX_INSTALLER): all-recursive
	$(MKDIR_P) $(abs_top_builddir)/release
	$(MAKE) install DESTDIR=$(abs_top_builddir)/release
	$(AMTAR) -czf $(POCKETCOIN_LINUX_INSTALLER_NAME).tar.gz -C $(abs_top_builddir)/release .
	$(MKDIR_P) $(abs_top_builddir)/release/DEBIAN
	$(MKDIR_P) $(abs_top_builddir)/release/usr/local/share/applications
	$(MKDIR_P) $(abs_top_builddir)/release/usr/local/share/pixmaps
	@cp $(abs_top_builddir)/contrib/debian/control $(abs_top_builddir)/release/DEBIAN/
	@cp $(abs_top_builddir)/share/data/pocketnetcore.desktop $(abs_top_builddir)/release/usr/local/share/applications/
	@cp $(abs_top_builddir)/share/pixmaps/logo_color/sky_250.png $(abs_top_builddir)/release/usr/local/share/pixmaps/pocketcoin.png
	@sed -i "s/Version:.*/Version: $(PACKAGE_VERSION)/" $(abs_top_builddir)/release/DEBIAN/control
	@sed -i "s/Version=.*/Version=$(PACKAGE_VERSION)/" $(abs_top_builddir)/release/usr/local/share/applications/pocketnetcore.desktop
	@dpkg -b $(abs_top_builddir)/release
	@mv $(abs_top_builddir)/release.deb $(abs_top_builddir)/$(POCKETCOIN_LINUX_INSTALLER_NAME)_setup.deb

$(OSX_APP)/Contents/PkgInfo:
	$(MKDIR_P) $(@D)
	@echo "APPL????" > $@

$(OSX_APP)/Contents/Resources/empty.lproj:
	$(MKDIR_P) $(@D)
	@touch $@

$(OSX_APP)/Contents/Info.plist: $(OSX_PLIST)
	$(MKDIR_P) $(@D)
	$(INSTALL_DATA) $< $@

$(OSX_APP)/Contents/Resources/pocketcoin.icns: $(OSX_INSTALLER_ICONS)
	$(MKDIR_P) $(@D)
	$(INSTALL_DATA) $< $@

$(OSX_APP)/Contents/MacOS/Pocketcoin-Qt: all-recursive
	$(MKDIR_P) $(@D)
	STRIPPROG="$(STRIP)" $(INSTALL_STRIP_PROGRAM)  $(POCKETCOIN_QT_BIN) $@

$(OSX_APP)/Contents/Resources/Base.lproj/InfoPlist.strings:
	$(MKDIR_P) $(@D)
	echo '{	CFBundleDisplayName = "$(PACKAGE_NAME)"; CFBundleName = "$(PACKAGE_NAME)"; }' > $@

OSX_APP_BUILT=$(OSX_APP)/Contents/PkgInfo $(OSX_APP)/Contents/Resources/empty.lproj \
  $(OSX_APP)/Contents/Resources/pocketcoin.icns $(OSX_APP)/Contents/Info.plist \
  $(OSX_APP)/Contents/MacOS/Pocketcoin-Qt $(OSX_APP)/Contents/Resources/Base.lproj/InfoPlist.strings

osx_volname:
	echo $(OSX_VOLNAME) >$@

if BUILD_DARWIN
$(OSX_DMG): $(OSX_APP_BUILT) $(OSX_PACKAGING) $(OSX_BACKGROUND_IMAGE)
	$(PYTHON) $(OSX_DEPLOY_SCRIPT) $(OSX_APP) $(OSX_VOLNAME) -translations-dir=$(QT_TRANSLATION_DIR) -dmg

$(OSX_BACKGROUND_IMAGE).png: contrib/macdeploy/$(OSX_BACKGROUND_SVG)
	sed 's/PACKAGE_NAME/$(PACKAGE_NAME)/' < "$<" | $(RSVG_CONVERT) -f png -d 36 -p 36 -o $@
$(OSX_BACKGROUND_IMAGE)@2x.png: contrib/macdeploy/$(OSX_BACKGROUND_SVG)
	sed 's/PACKAGE_NAME/$(PACKAGE_NAME)/' < "$<" | $(RSVG_CONVERT) -f png -d 72 -p 72 -o $@
$(OSX_BACKGROUND_IMAGE): $(OSX_BACKGROUND_IMAGE).png $(OSX_BACKGROUND_IMAGE)@2x.png
	tiffutil -cathidpicheck $^ -out $@

deploydir: $(OSX_DMG)
else
APP_DIST_DIR=$(top_builddir)/dist
APP_DIST_EXTRAS=$(APP_DIST_DIR)/.background/$(OSX_BACKGROUND_IMAGE) $(APP_DIST_DIR)/.DS_Store $(APP_DIST_DIR)/Applications

$(APP_DIST_DIR)/Applications:
	@rm -f $@
	@cd $(@D); $(LN_S) /Applications $(@F)

$(APP_DIST_EXTRAS): $(APP_DIST_DIR)/$(OSX_APP)/Contents/MacOS/Pocketcoin-Qt

$(OSX_DMG): $(APP_DIST_EXTRAS)
	$(GENISOIMAGE) -no-cache-inodes -D -l -probe -V "$(OSX_VOLNAME)" -no-pad -r -dir-mode 0755 -apple -o $@ dist

dpi%.$(OSX_BACKGROUND_IMAGE): contrib/macdeploy/$(OSX_BACKGROUND_SVG)
	sed 's/PACKAGE_NAME/$(PACKAGE_NAME)/' < "$<" | $(RSVG_CONVERT) -f png -d $* -p $* | $(IMAGEMAGICK_CONVERT) - $@
OSX_BACKGROUND_IMAGE_DPIFILES := $(foreach dpi,$(OSX_BACKGROUND_IMAGE_DPIS),dpi$(dpi).$(OSX_BACKGROUND_IMAGE))
$(APP_DIST_DIR)/.background/$(OSX_BACKGROUND_IMAGE): $(OSX_BACKGROUND_IMAGE_DPIFILES)
	$(MKDIR_P) $(@D)
	$(TIFFCP) -c none $(OSX_BACKGROUND_IMAGE_DPIFILES) $@

$(APP_DIST_DIR)/$(OSX_APP)/Contents/MacOS/Pocketcoin-Qt: $(OSX_APP_BUILT) $(OSX_PACKAGING)
    INSTALLNAMETOOL=$(INSTALLNAMETOOL) OTOOL=$(OTOOL) STRIP=$(STRIP) $(PYTHON) $(OSX_DEPLOY_SCRIPT) $(OSX_APP) $(OSX_VOLNAME) -translations-dir=$(QT_TRANSLATION_DIR)

deploydir: $(APP_DIST_EXTRAS)
endif

if TARGET_DARWIN
appbundle: $(OSX_APP_BUILT)
deploy: $(OSX_DMG)
endif
if TARGET_WINDOWS
deploy: $(POCKETCOIN_WIN_INSTALLER)
endif
if TARGET_LINUX
deploy: $(POCKETCOIN_LINUX_INSTALLER)
endif

$(POCKETCOIN_QT_BIN): FORCE
	$(MAKE) -C src qt/$(@F)

$(POCKETCOIND_BIN): FORCE
	$(MAKE) -C src $(@F)

$(POCKETCOIN_CLI_BIN): FORCE
	$(MAKE) -C src $(@F)

$(POCKETCOIN_TX_BIN): FORCE
	$(MAKE) -C src $(@F)

# $(POCKETCOIN_WALLET_BIN): FORCE
# 	$(MAKE) -C src $(@F)

if USE_LCOV
LCOV_FILTER_PATTERN = \
	-p "/usr/local/" \
	-p "/usr/include/" \
	-p "/usr/lib/" \
	-p "/usr/lib64/" \
	-p "src/leveldb/" \
	-p "src/crc32c/" \
	-p "src/bench/" \
	-p "src/univalue" \
	-p "src/crypto/ctaes" \
	-p "src/secp256k1" \
	-p "depends"

DIR_FUZZ_SEED_CORPUS ?= qa-assets/fuzz_seed_corpus

$(COV_TOOL_WRAPPER):
	@echo 'exec $(COV_TOOL) "$$@"' > $(COV_TOOL_WRAPPER)
	@chmod +x $(COV_TOOL_WRAPPER)

baseline.info: $(COV_TOOL_WRAPPER)
	$(LCOV) -c -i -d $(abs_builddir)/src -o $@

baseline_filtered.info: baseline.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

fuzz.info: baseline_filtered.info
	@TIMEOUT=15 test/fuzz/test_runner.py $(DIR_FUZZ_SEED_CORPUS) -l DEBUG
	$(LCOV) -c $(LCOV_OPTS) -d $(abs_builddir)/src --t fuzz-tests -o $@
	$(LCOV) -z $(LCOV_OPTS) -d $(abs_builddir)/src

fuzz_filtered.info: fuzz.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

test_pocketcoin.info: baseline_filtered.info
	$(MAKE) -C src/ check
	$(LCOV) -c $(LCOV_OPTS) -d $(abs_builddir)/src -t test_pocketcoin -o $@
	$(LCOV) -z $(LCOV_OPTS) -d $(abs_builddir)/src

test_pocketcoin_filtered.info: test_pocketcoin.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

functional_test.info: test_pocketcoin_filtered.info
	@TIMEOUT=15 test/functional/test_runner.py $(EXTENDED_FUNCTIONAL_TESTS)
	$(LCOV) -c $(LCOV_OPTS) -d $(abs_builddir)/src --t functional-tests -o $@
	$(LCOV) -z $(LCOV_OPTS) -d $(abs_builddir)/src

functional_test_filtered.info: functional_test.info
	$(abs_builddir)/contrib/filter-lcov.py $(LCOV_FILTER_PATTERN) $< $@
	$(LCOV) -a $@ $(LCOV_OPTS) -o $@

fuzz_coverage.info: fuzz_filtered.info
	$(LCOV) -a $(LCOV_OPTS) baseline_filtered.info -a fuzz_filtered.info -o $@ | $(GREP) "\%" | $(AWK) '{ print substr($$3,2,50) "/" $$5 }' > coverage_percent.txt

test_pocketcoin_coverage.info: baseline_filtered.info test_pocketcoin_filtered.info
	$(LCOV) -a $(LCOV_OPTS) baseline_filtered.info -a test_pocketcoin_filtered.info -o $@

total_coverage.info: test_pocketcoin_filtered.info functional_test_filtered.info
	$(LCOV) -a $(LCOV_OPTS) baseline_filtered.info -a test_pocketcoin_filtered.info -a functional_test_filtered.info -o $@ | $(GREP) "\%" | $(AWK) '{ print substr($$3,2,50) "/" $$5 }' > coverage_percent.txt

fuzz.coverage/.dirstamp: fuzz_coverage.info
	$(GENHTML) -s $(LCOV_OPTS) $< -o $(@D)
	@touch $@

test_pocketcoin.coverage/.dirstamp:  test_pocketcoin_coverage.info
	$(GENHTML) -s $(LCOV_OPTS) $< -o $(@D)
	@touch $@

total.coverage/.dirstamp: total_coverage.info
	$(GENHTML) -s $(LCOV_OPTS) $< -o $(@D)
	@touch $@

cov_fuzz: fuzz.coverage/.dirstamp

cov: test_pocketcoin.coverage/.dirstamp total.coverage/.dirstamp

endif

dist_noinst_SCRIPTS = autogen.sh

EXTRA_DIST = $(DIST_SHARE) $(DIST_CONTRIB) $(WINDOWS_PACKAGING) $(OSX_PACKAGING) $(BIN_CHECKS)

EXTRA_DIST += \
    test/functional \
    test/fuzz

EXTRA_DIST += \
    test/util/pocketcoin-util-test.py \
    test/util/data/pocketcoin-util-test.json \
    test/util/data/blanktxv1.hex \
    test/util/data/blanktxv1.json \
    test/util/data/blanktxv2.hex \
    test/util/data/blanktxv2.json \
    test/util/data/tt-delin1-out.hex \
    test/util/data/tt-delin1-out.json \
    test/util/data/tt-delout1-out.hex \
    test/util/data/tt-delout1-out.json \
    test/util/data/tt-locktime317000-out.hex \
    test/util/data/tt-locktime317000-out.json \
    test/util/data/tx394b54bb.hex \
    test/util/data/txcreate1.hex \
    test/util/data/txcreate1.json \
    test/util/data/txcreate2.hex \
    test/util/data/txcreate2.json \
    test/util/data/txcreatedata1.hex \
    test/util/data/txcreatedata1.json \
    test/util/data/txcreatedata2.hex \
    test/util/data/txcreatedata2.json \
    test/util/data/txcreatedata_seq0.hex \
    test/util/data/txcreatedata_seq0.json \
    test/util/data/txcreatedata_seq1.hex \
    test/util/data/txcreatedata_seq1.json \
    test/util/data/txcreatemultisig1.hex \
    test/util/data/txcreatemultisig1.json \
    test/util/data/txcreatemultisig2.hex \
    test/util/data/txcreatemultisig2.json \
    test/util/data/txcreatemultisig3.hex \
    test/util/data/txcreatemultisig3.json \
    test/util/data/txcreatemultisig4.hex \
    test/util/data/txcreatemultisig4.json \
    test/util/data/txcreatemultisig5.json \
    test/util/data/txcreateoutpubkey1.hex \
    test/util/data/txcreateoutpubkey1.json \
    test/util/data/txcreateoutpubkey2.hex \
    test/util/data/txcreateoutpubkey2.json \
    test/util/data/txcreateoutpubkey3.hex \
    test/util/data/txcreateoutpubkey3.json \
    test/util/data/txcreatescript1.hex \
    test/util/data/txcreatescript1.json \
    test/util/data/txcreatescript2.hex \
    test/util/data/txcreatescript2.json \
    test/util/data/txcreatescript3.hex \
    test/util/data/txcreatescript3.json \
    test/util/data/txcreatescript4.hex \
    test/util/data/txcreatescript4.json \
    test/util/data/txcreatescript5.hex \
    test/util/data/txcreatescript6.hex \
    test/util/data/txcreatesignv1.hex \
    test/util/data/txcreatesignv1.json \
    test/util/data/txcreatesignv2.hex \
    test/util/rpcauth-test.py

CLEANFILES = $(OSX_DMG) $(POCKETCOIN_WIN_INSTALLER) $(POCKETCOIN_LINUX_ARCHIVE)

.INTERMEDIATE: $(COVERAGE_INFO)

DISTCHECK_CONFIGURE_FLAGS = --enable-man

doc/doxygen/.stamp: doc/Doxyfile FORCE
	$(MKDIR_P) $(@D)
	$(DOXYGEN) $^
	$(AM_V_at) touch $@

if HAVE_DOXYGEN
docs: doc/doxygen/.stamp
else
docs:
	@echo "error: doxygen not found"
endif

clean-docs:
	rm -rf doc/doxygen

clean-local: clean-docs
	rm -rf coverage_percent.txt test_pocketcoin.coverage/ total.coverage/ fuzz.coverage/ test/tmp/ cache/ $(OSX_APP)
	rm -rf test/functional/__pycache__ test/functional/test_framework/__pycache__ test/cache share/rpcauth/__pycache__
	rm -rf osx_volname dist/ dpi36.background.tiff dpi72.background.tiff

test-security-check:
if TARGET_DARWIN
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/test-security-check.py TestSecurityChecks.test_MACHO
endif
if TARGET_WINDOWS
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/test-security-check.py TestSecurityChecks.test_PE
endif
if TARGET_LINUX
	$(AM_V_at) $(PYTHON) $(top_srcdir)/contrib/devtools/test-security-check.py TestSecurityChecks.test_ELF
endif
