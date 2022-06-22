package=libdatachanel
$(package)_version=0.17.7
$(package)_download_path=https://github.com/lostystyg/libdatachannel/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version)-Source.tar.gz
$(package)_sha256_hash=3c8de85fde74c375fc9acc5e5614567542497d7607d5574ec9ee3cab97587d3f
$(package)_dependencies=openssl
$(package)_patches=build_static.patch
$(package)_patches+=global_definitions.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/build_static.patch && \
  patch -p1 < $($(package)_patch_dir)/global_definitions.patch
endef

define $(package)_config_cmds
  $($(package)_cmake) -DNO_MEDIA=ON -DNO_EXAMPLES=ON -DNO_TESTS=ON .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
