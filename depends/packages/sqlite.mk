package=sqlite
$(package)_version=3420000
$(package)_download_path=https://sqlite.org/2023/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=7abcfd161c6e2742ca5c6c0895d1f853c940f203304a0b49da4e1eca5d088ca6

define $(package)_set_vars
$(package)_config_opts=--disable-shared --disable-readline --disable-dynamic-extensions --enable-option-checking
$(package)_config_opts_linux=--with-pic
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) libsqlite3.la
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS install-pkgconfigDATA
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
