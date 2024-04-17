package=libdatachanel
$(package)_version=0.20.1
$(package)_download_path=https://github.com/pocketnetteam/libdatachannel/releases/download/$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tgz
$(package)_sha256_hash=6ee65909734a934d16a48a094998ee68b7fa011251b7cbaf96249db27f1a367d
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
