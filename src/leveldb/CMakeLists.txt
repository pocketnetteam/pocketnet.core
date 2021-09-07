# TODO there is an updated version of leveldb that handles cmake out of the box.
# Bitcoin also had merged this functionality, so consider delete this after bitcoin-part update 

cmake_minimum_required(VERSION 3.21)

project(leveldb)
include(CheckLibraryExists)

include_directories(include/)

add_library(leveldb port/atomic_pointer.h
                    port/port_example.h
                    port/port_posix.h
                    port/win/stdint.h
                    port/port.h
                    port/port_win.h
                    port/thread_annotations.h
                    include/leveldb/db.h
                    include/leveldb/options.h
                    include/leveldb/comparator.h
                    include/leveldb/filter_policy.h
                    include/leveldb/slice.h
                    include/leveldb/table_builder.h
                    include/leveldb/env.h
                    include/leveldb/c.h
                    include/leveldb/iterator.h
                    include/leveldb/cache.h
                    include/leveldb/dumpfile.h
                    include/leveldb/table.h
                    include/leveldb/write_batch.h
                    include/leveldb/status.h
                    db/log_format.h
                    db/memtable.h
                    db/version_set.h
                    db/write_batch_internal.h
                    db/filename.h
                    db/version_edit.h
                    db/dbformat.h
                    db/builder.h
                    db/log_writer.h
                    db/db_iter.h
                    db/skiplist.h
                    db/db_impl.h
                    db/table_cache.h
                    db/snapshot.h
                    db/log_reader.h
                    table/filter_block.h
                    table/block_builder.h
                    table/block.h
                    table/two_level_iterator.h
                    table/merger.h
                    table/format.h
                    table/iterator_wrapper.h
                    util/crc32c.h
                    util/env_posix_test_helper.h
                    util/arena.h
                    util/random.h
                    util/posix_logger.h
                    util/hash.h
                    util/histogram.h
                    util/coding.h
                    util/testutil.h
                    util/mutexlock.h
                    util/logging.h
                    util/testharness.h
                    db/builder.cc
                    db/c.cc
                    db/dbformat.cc
                    db/db_impl.cc
                    db/db_iter.cc
                    db/dumpfile.cc
                    db/filename.cc
                    db/log_reader.cc
                    db/log_writer.cc
                    db/memtable.cc
                    db/repair.cc
                    db/table_cache.cc
                    db/version_edit.cc
                    db/version_set.cc
                    db/write_batch.cc
                    table/block_builder.cc
                    table/block.cc
                    table/filter_block.cc
                    table/format.cc
                    table/iterator.cc
                    table/merger.cc
                    table/table_builder.cc
                    table/table.cc
                    table/two_level_iterator.cc
                    util/arena.cc
                    util/bloom.cc
                    util/cache.cc
                    util/coding.cc
                    util/comparator.cc
                    util/crc32c.cc
                    util/env.cc
                    util/env_posix.cc
                    util/filter_policy.cc
                    util/hash.cc
                    util/histogram.cc
                    util/logging.cc
                    util/options.cc
                    util/status.cc)
if(WIN32)
    target_sources(leveldb PRIVATE  util/env_win.cc
                                    port/port_win.cc)
    target_compile_definitions(leveldb PUBLIC LEVELDB_PLATFORM_WINDOWS)
else(WIN32)
    target_sources(leveldb PRIVATE port/port_posix.cc)
    target_compile_definitions(leveldb PUBLIC LEVELDB_PLATFORM_POSIX)
endif(WIN32)

target_include_directories(leveldb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# TODO perform any checks probably.
target_compile_definitions(leveldb PUBLIC LEVELDB_ATOMIC_PRESENT)


add_library(memenv  helpers/memenv/memenv.h
                    helpers/memenv/memenv.cc)
target_link_libraries(memenv PRIVATE leveldb)

add_library(leveldb_sse42 port/port_posix_sse.cc)
check_library_exists(crc32c crc32c_value "" HAVE_CRC32C)
if(HAVE_CRC32C)
  target_link_libraries(leveldb_sse42 crc32c)
endif(HAVE_CRC32C)
target_link_libraries(leveldb_sse42 PRIVATE leveldb)