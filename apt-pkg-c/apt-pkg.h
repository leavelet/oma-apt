#pragma once
//#include "apt-pkg.cc"
// For Development Typing
#include "cxx-typing.h"
#include "rust/cxx.h"
#include <apt-pkg/depcache.h>
//#include <memory>

// C++ owned structs
struct PCache;
struct PkgFileIterator;
struct VerFileIterator;
struct DepIterator;
struct VerFileParser;
struct PkgRecords;
struct PkgIndexFile;
struct DescIterator;

// Rust Shared Structs
struct PackagePtr;
struct VersionPtr;
struct DepIterator;
struct DepContainer;
struct SourceFile;
struct Provider;

// Apt Aliases
using PkgDepCache = pkgDepCache;
using VerIterator = pkgCache::VerIterator;
using PkgIterator = pkgCache::PkgIterator;
// From Rust to C++
//
// CXX Test Function
// int greet(rust::Str greetee);


// From C++ to Rust
//
/// Main Initializers for APT
void init_config_system();

PCache *pkg_cache_create();
PkgRecords *pkg_records_create(PCache *pcache);
pkgDepCache *depcache_create(PCache *pcache);

void pkg_cache_release(PCache *cache);
void pkg_records_release(PkgRecords *records);

rust::Vec<SourceFile> source_uris(PCache *pcache);
int32_t pkg_cache_compare_versions(PCache *cache, const char *left, const char *right);

/// Iterator Creators
rust::Vec<PackagePtr> pkg_list(PCache *cache);
VerFileIterator *ver_file(const VersionPtr &ver);
VerFileIterator *ver_file_clone(VerFileIterator *iterator);

VersionPtr pkg_current_version(const PackagePtr &pkg);
VersionPtr pkg_candidate_version(PCache *cache, const PackagePtr &pkg);
rust::vec<VersionPtr> pkg_version_list(const PackagePtr &pkg);

PkgFileIterator *ver_pkg_file(VerFileIterator *iterator);
DescIterator *ver_desc_file(const VersionPtr &ver);
PkgIndexFile *pkg_index_file(PCache *pcache, PkgFileIterator *pkg_file);

PackagePtr pkg_cache_find_name(PCache *pcache, rust::string name);
PackagePtr pkg_cache_find_name_arch(PCache *pcache, rust::string name, rust::string arch);

/// Iterator Manipulation
void ver_file_next(VerFileIterator *iterator);
bool ver_file_end(VerFileIterator *iterator);
void ver_file_release(VerFileIterator *iterator);

void pkg_file_release(PkgFileIterator *iterator);
void pkg_index_file_release(PkgIndexFile *wrapper);
void ver_desc_release(DescIterator *wrapper);
void dep_release(DepIterator *wrapper);

/// Information Accessors
bool pkg_is_upgradable(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_is_auto_installed(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_is_garbage(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_marked_install(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_marked_upgrade(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_marked_delete(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_marked_keep(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_marked_downgrade(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_marked_reinstall(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_is_now_broken(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_is_inst_broken(pkgDepCache *depcache, const PackagePtr &pkg);
bool pkg_is_installed(const PackagePtr &pkg);
bool pkg_has_versions(const PackagePtr &pkg);
bool pkg_has_provides(const PackagePtr &pkg);
rust::Vec<PackagePtr> pkg_provides_list(PCache *cache, const PackagePtr &pkg, bool cand_only);
rust::string get_fullname(const PackagePtr &pkg, bool pretty);
rust::string pkg_name(const PackagePtr &pkg);
rust::string pkg_arch(const PackagePtr &pkg);
int32_t pkg_id(const PackagePtr &pkg);
int32_t pkg_current_state(const PackagePtr &pkg);
int32_t pkg_inst_state(const PackagePtr &pkg);
int32_t pkg_selected_state(const PackagePtr &pkg);
bool pkg_essential(const PackagePtr &pkg);

rust::Vec<DepContainer> dep_list(const VersionPtr &ver);
rust::string ver_arch(const VersionPtr &ver);
rust::string ver_str(const VersionPtr &ver);
rust::string ver_section(const VersionPtr &ver);
rust::string ver_priority_str(const VersionPtr &ver);
rust::string ver_source_package(const VersionPtr &ver);
rust::string ver_source_version(const VersionPtr &ver);
rust::string ver_name(const VersionPtr &ver);
int32_t ver_size(const VersionPtr &ver);
int32_t ver_installed_size(const VersionPtr &ver);
bool ver_downloadable(const VersionPtr &ver);
int32_t ver_id(const VersionPtr &ver);
bool ver_installed(const VersionPtr &ver);
int32_t ver_priority(PCache *pcache, const VersionPtr &ver);

/// Package Record Management
void ver_file_lookup(PkgRecords *records, VerFileIterator *iterator);
void desc_file_lookup(PkgRecords *records, DescIterator *wrapper);
rust::string ver_uri(PkgRecords *records, PkgIndexFile *index_file);
rust::string long_desc(PkgRecords *records);
rust::string short_desc(PkgRecords *records);
rust::string hash_find(PkgRecords *records, rust::string hash_type);

rust::Vec<VersionPtr> dep_all_targets(DepIterator *wrapper);

/// Unused Functions
/// They may be used in the future
///
// dep_iter creation and deletion
// DepIterator *ver_iter_dep_iter(VerIterator *iterator);
// void dep_iter_release(DepIterator *iterator);

// // dep_iter mutation
// void dep_iter_next(DepIterator *iterator);
// bool dep_iter_end(DepIterator *iterator);

// // dep_iter access
// PkgIterator *dep_iter_target_pkg(DepIterator *iterator);
// const char *dep_iter_target_ver(DepIterator *iterator);
// const char *dep_iter_comp_type(DepIterator *iterator);
// const char *dep_iter_dep_type(DepIterator *iterator);

// //template<typename Iterator>
// bool validate(VerIterator *iterator, PCache *pcache);

// // ver_file_parser access
// const char *ver_file_parser_short_desc(VerFileParser *parser);
// const char *ver_file_parser_long_desc(VerFileParser *parser);

// const char *ver_file_parser_maintainer(VerFileParser *parser);
// const char *ver_file_parser_homepage(VerFileParser *parser);
// // pkg_file_iter mutation
// void pkg_file_iter_next(PkgFileIterator *iterator);
// bool pkg_file_iter_end(PkgFileIterator *iterator);

// // pkg_file_iter access
// const char *pkg_file_iter_file_name(PkgFileIterator *iterator);
// const char *pkg_file_iter_archive(PkgFileIterator *iterator);
// const char *pkg_file_iter_version(PkgFileIterator *iterator);
// const char *pkg_file_iter_origin(PkgFileIterator *iterator);
// const char *pkg_file_iter_codename(PkgFileIterator *iterator);
// const char *pkg_file_iter_label(PkgFileIterator *iterator);
// const char *pkg_file_iter_site(PkgFileIterator *iterator);
// const char *pkg_file_iter_component(PkgFileIterator *iterator);
// const char *pkg_file_iter_architecture(PkgFileIterator *iterator);
// const char *pkg_file_iter_index_type(PkgFileIterator *iterator);
