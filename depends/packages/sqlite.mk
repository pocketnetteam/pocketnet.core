package=sqlite
$(package)_version=3340100
$(package)_download_path=https://sqlite.org/2021/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=2a3bca581117b3b88e5361d0ef3803ba6d8da604b1c1a47d902ef785c1b53e89

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
