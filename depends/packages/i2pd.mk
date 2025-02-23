package=i2pd
$(package)_version=2.55.0
$(package)_download_path=https://github.com/pocketnetteam/i2pd/releases/download/$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=f5792a1c0499143c716663e90bfb105aaa7ec47d1c4550b5f90ebfc25da00c6c
$(package)_dependencies=boost openssl zlib miniupnpc

define $(package)_set_vars
$(package)_build_opts=USE_UPNP=yes DEBUG=no USE_STATIC=yes
$(package)_build_opts_linux=USE_STATIC=yes
$(package)_cxxflags=-I$($($(1)_type)_prefix)/include
$(package)_cppflags=-I$($($(1)_type)_prefix)/include
$(package)_ldlibs+=$($($(1)_type)_prefix)/lib/libboost_program_options-mt-x64.a
$(package)_ldlibs+=$($($(1)_type)_prefix)/lib/libssl.a
$(package)_ldlibs+=$($($(1)_type)_prefix)/lib/libcrypto.a
$(package)_ldlibs+=$($($(1)_type)_prefix)/lib/libz.a
$(package)_ldlibs+=$($($(1)_type)_prefix)/lib/libminiupnpc.a
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts) CXXFLAGS="$($(package)_cxxflags)" CPPFLAGS="$($(package)_cppflags)" LDLIBS="$($(package)_ldlibs)"
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib && \
  cp libi2pd.a $($(package)_staging_prefix_dir)/lib && \
  cp libi2pdclient.a $($(package)_staging_prefix_dir)/lib && \
  cp libi2pdlang.a $($(package)_staging_prefix_dir)/lib && \
  mkdir -p $($(package)_staging_prefix_dir)/include/libi2pd && \
  cp -r libi2pd/* $($(package)_staging_prefix_dir)/include/libi2pd/ && \
  cp -r libi2pd_client/* $($(package)_staging_prefix_dir)/include/libi2pd/ && \
  cp -r i18n/* $($(package)_staging_prefix_dir)/include/libi2pd/
endef