#include <apt-pkg/srcrecords.h>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <cstdint>

#include <assert.h>

#include <apt-pkg/configuration.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/indexfile.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/version.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/fileutl.h>

#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/policy.h>
#include <string>
// For Development Typing
#include "cxx-typing.h"
// Headers for the cxx bridge
#include "rust/cxx.h"
#include "rust-apt/src/raw.rs"
#include "apt-pkg.h"

// Couldn't get this to work without wrappers.
struct PCache {
	// Owned by us.
	pkgCacheFile *cache_file;

	// Borrowed from cache_file.
	pkgCache *cache;

	pkgSourceList *source;
};

struct PkgIndexFile {
	// Owned by us.
	pkgIndexFile *index;
};

struct PkgRecords {
	pkgRecords *records;

	pkgRecords::Parser *parser;
};

struct VerFileIterator {
	pkgCache::VerFileIterator iterator;
};

struct DescIterator {
	pkgCache::DescIterator iterator;
};

struct PkgFileIterator {
	pkgCache::PkgFileIterator iterator;
};

struct DepIterator {
	pkgCache::DepIterator iterator;
};

/// CXX Test Function
///
// int greet(rust::Str greetee) {
//   std::cout << "Hello, " << greetee << std::endl;
//   return get_num();
// }

static PackagePtr wrap_package(pkgCache::PkgIterator pkg) {
	if (pkg.end()) {
		return PackagePtr { NULL };
	}

	return PackagePtr { std::make_unique<pkgCache::PkgIterator>(pkg) };
}

static VersionPtr wrap_version(pkgCache::VerIterator ver) {
	if (ver.end()) {
		return VersionPtr { NULL };
	}

	return VersionPtr { std::make_unique<pkgCache::VerIterator>(ver) };
}

/// Main Initializers for APT
///
void init_config_system() {
	pkgInitConfig(*_config);
	pkgInitSystem(*_config, _system);
}

PCache *pkg_cache_create() {
	pkgCacheFile *cache_file = new pkgCacheFile();
	PCache *ret = new PCache();
	cache_file->BuildSourceList();

	ret->cache_file = cache_file;
	ret->cache = cache_file->GetPkgCache();
	ret->source = cache_file->GetSourceList();
	return ret;
}

PkgRecords *pkg_records_create(PCache *pcache) {
	PkgRecords *records = new PkgRecords();
	records->records = new pkgRecords(*pcache->cache);
	// Can't populate the parser until we need it.
	records->parser = NULL;
	return records;
}

pkgDepCache *depcache_create(PCache *pcache) {
	pkgDepCache *depcache = pcache->cache_file->GetDepCache();
//	pkgApplyStatus(*depcache);
	return depcache;
}

void pkg_cache_release(PCache *cache) {
	// pkgCache and pkgDepCache are cleaned up with cache_file.
	delete cache->cache_file;
	delete cache;
}

void pkg_index_file_release(PkgIndexFile *wrapper) {
	delete wrapper;
}

void pkg_records_release(PkgRecords *records) {
	delete records -> records;
	delete records;
}

rust::Vec<SourceFile> source_uris(PCache *pcache) {
	pkgAcquire fetcher;
	rust::Vec<SourceFile> list;

	pcache->source->GetIndexes(&fetcher, true);
	pkgAcquire::UriIterator I = fetcher.UriBegin();

	for (; I != fetcher.UriEnd(); ++I) {
		list.push_back(
			SourceFile {
				I->URI,
				flNotDir(I->Owner->DestFile)
			}
		);
	}
	return list;
}

int32_t pkg_cache_compare_versions(PCache *cache, const char *left, const char *right) {
	// an int is returned here; presumably it will always be -1, 0 or 1.
	return cache->cache->VS->DoCmpVersion(left, left+strlen(left), right, right+strlen(right));
}

/// Basic Iterator Management
///
/// Iterator Creators
rust::Vec<PackagePtr> pkg_list(PCache *cache) {
	rust::vec<PackagePtr> list;
	pkgCache::PkgIterator pkg;

	for (pkg = cache->cache->PkgBegin(); pkg.end() == false; pkg++) {
		list.push_back(wrap_package(pkg));
	}
	return list;
}

rust::Vec<PackagePtr> pkg_provides_list(PCache *cache, const PackagePtr &pkg, bool cand_only) {
	pkgCache::PrvIterator provide = pkg.ptr->ProvidesList();
	std::set<std::string> set;
	rust::vec<PackagePtr> list;

	for (; provide.end() == false; provide++) {
		pkgCache::PkgIterator pkg = provide.OwnerPkg();
		bool is_cand = (
			provide.OwnerVer() == cache->cache_file->GetPolicy()->GetCandidateVer(pkg)
		);
		if (!cand_only || is_cand) {
			if (!set.insert(pkg.FullName()).second) { continue; }

			list.push_back(wrap_package(pkg));
		}
	}
	return list;
}

VerFileIterator *ver_file(const VersionPtr &ver) {
	VerFileIterator *new_wrapper = new VerFileIterator();
	new_wrapper->iterator = ver.ptr->FileList();
	return new_wrapper;
}

VerFileIterator *ver_file_clone(VerFileIterator *iterator) {
	VerFileIterator *wrapper = new VerFileIterator();
	wrapper->iterator = iterator->iterator;
	return wrapper;
}

VersionPtr pkg_current_version(const PackagePtr &pkg) {
	return wrap_version(pkg.ptr->CurrentVer());
}

VersionPtr pkg_candidate_version(PCache *cache, const PackagePtr &pkg) {
	return wrap_version(
		cache->cache_file->GetPolicy()->GetCandidateVer(*pkg.ptr)
	);
}

rust::Vec<VersionPtr> pkg_version_list(const PackagePtr &pkg) {
	rust::Vec<VersionPtr> list;

	for (pkgCache::VerIterator I = pkg.ptr->VersionList(); I.end() == false; I++) {
		list.push_back(wrap_version(I));
	}
	return list;
}

PkgFileIterator *ver_pkg_file(VerFileIterator *wrapper) {
	PkgFileIterator *new_wrapper = new PkgFileIterator();
	new_wrapper->iterator = wrapper->iterator.File();
	return new_wrapper;
}

DescIterator *ver_desc_file(const VersionPtr &ver) {
	DescIterator *new_wrapper = new DescIterator();
	new_wrapper->iterator = ver.ptr->TranslatedDescription();
	return new_wrapper;
}

PkgIndexFile *pkg_index_file(PCache *pcache, PkgFileIterator *pkg_file) {
	PkgIndexFile *wrapper = new PkgIndexFile();
	pkgSourceList *SrcList = pcache->cache_file->GetSourceList();
	pkgIndexFile *Index;
	if (SrcList->FindIndex(pkg_file->iterator, Index) == false) { _system->FindIndex(pkg_file->iterator, Index);}
	wrapper->index = Index;
	return wrapper;
}

// These two are how we get a specific package by name.
PackagePtr pkg_cache_find_name(PCache *pcache, rust::string name) {
	return wrap_package(pcache->cache->FindPkg(name.c_str()));
}

PackagePtr pkg_cache_find_name_arch(PCache *pcache, rust::string name, rust::string arch) {
	return wrap_package(pcache->cache->FindPkg(name.c_str(), arch.c_str()));
}

/// Iterator Manipulation
///
void ver_file_next(VerFileIterator *wrapper) {
	++wrapper->iterator;
}

bool ver_file_end(VerFileIterator *wrapper) {
	return wrapper->iterator.end();
}

void ver_file_release(VerFileIterator *wrapper) {
	delete wrapper;
}

void pkg_file_release(PkgFileIterator *wrapper) {
	delete wrapper;
}

void ver_desc_release(DescIterator *wrapper) {
	delete wrapper;
}

void dep_release(DepIterator *wrapper) {
	delete wrapper;
}

/// Information Accessors
///
bool pkg_is_upgradable(pkgDepCache *depcache, const PackagePtr &pkg) {
	if (pkg.ptr->CurrentVer() == 0) { return false; }
	return (*depcache)[*pkg.ptr].Upgradable();

}

bool pkg_is_auto_installed(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].Flags & pkgCache::Flag::Auto;
}

bool pkg_is_garbage(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].Garbage;
}

bool pkg_marked_install(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].NewInstall();
}

bool pkg_marked_upgrade(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].Upgrade();
}

bool pkg_marked_delete(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].Delete();
}

bool pkg_marked_keep(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].Keep();
}

bool pkg_marked_downgrade(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].Downgrade();
}

bool pkg_marked_reinstall(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].ReInstall();
}

bool pkg_is_now_broken(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].NowBroken();
}

bool pkg_is_inst_broken(pkgDepCache *depcache, const PackagePtr &pkg) {
	return (*depcache)[*pkg.ptr].InstBroken();
}

bool pkg_is_installed(const PackagePtr &pkg) {
	return !(pkg.ptr->CurrentVer() == 0);
}

bool pkg_has_versions(const PackagePtr &pkg) {
	return pkg.ptr->VersionList().end() == false;
}

bool pkg_has_provides(const PackagePtr &pkg) {
	return pkg.ptr->ProvidesList().end() == false;
}

rust::string get_fullname(const PackagePtr &pkg, bool pretty) {
	return pkg.ptr->FullName(pretty);
}

rust::string pkg_name(const PackagePtr &pkg) {
	return pkg.ptr->Name();
}

rust::string pkg_arch(const PackagePtr &pkg) {
	return pkg.ptr->Arch();
}

int32_t pkg_id(const PackagePtr &pkg) {
	return (*pkg.ptr)->ID;
}

int32_t pkg_current_state(const PackagePtr &pkg) {
	return (*pkg.ptr)->CurrentState;
}

int32_t pkg_inst_state(const PackagePtr &pkg) {
	return(*pkg.ptr)->InstState;
}

int32_t pkg_selected_state(const PackagePtr &pkg) {
	return (*pkg.ptr)->SelectedState;
}

bool pkg_essential(const PackagePtr &pkg) {
	return ((*pkg.ptr)->Flags & pkgCache::Flag::Essential) != 0;
}

const char *UntranslatedDepTypes[] = {
	"", "Depends","PreDepends","Suggests",
	"Recommends","Conflicts","Replaces",
	"Obsoletes", "Breaks", "Enhances"
};

rust::Vec<DepContainer> dep_list(const VersionPtr &ver) {
	rust::Vec<DepContainer> depend_list;

	for (pkgCache::DepIterator dep = ver.ptr->DependsList(); dep.end() == false;) {
		DepContainer depend = DepContainer();
		pkgCache::DepIterator Start;
		pkgCache::DepIterator End;
		dep.GlobOr(Start, End);

		depend.dep_type = UntranslatedDepTypes[Start->Type];
		rust::Vec<BaseDep> list;

		while (true) {
			DepIterator *dep_wrapper = new DepIterator();
			dep_wrapper->iterator = Start;

			rust::string version;
			if (Start->Version == 0) {
				version = "";
			} else {
				version = Start.TargetVer();
			}

			list.push_back(
				BaseDep {
					Start.TargetPkg().Name(),
					version,
					Start.CompType(),
					UntranslatedDepTypes[Start->Type],
					dep_wrapper
				}
			);

			if (Start == End) {
				depend.dep_list = list;
				depend_list.push_back(depend);
				break;
			}

			Start++;
		}
	}
	return depend_list;
}

rust::string ver_arch(const VersionPtr &ver) {
	return ver.ptr->Arch();
}

rust::string ver_str(const VersionPtr &ver) {
	return ver.ptr->VerStr();
}

rust::string ver_section(const VersionPtr &ver) {
   return ver.ptr->Section();
}

rust::string ver_priority_str(const VersionPtr &ver) {
	return ver.ptr->PriorityType();
}

rust::string ver_source_package(const VersionPtr &ver) {
	return ver.ptr->SourcePkgName();
}

rust::string ver_source_version(const VersionPtr &ver) {
	return ver.ptr->SourceVerStr();
}

rust::string ver_name(const VersionPtr &ver) {
	return ver.ptr->ParentPkg().Name();
}

int32_t ver_size(const VersionPtr &ver) {
	return (*ver.ptr)->Size;
}

int32_t ver_installed_size(const VersionPtr &ver) {
	return (*ver.ptr)->InstalledSize;
}

bool ver_downloadable(const VersionPtr &ver) {
	return ver.ptr->Downloadable();
}

int32_t ver_id(const VersionPtr &ver) {
	return (*ver.ptr)->ID;
}

bool ver_installed(const VersionPtr &ver) {
	return (*ver.ptr).ParentPkg().CurrentVer() == (*ver.ptr);
}

int32_t ver_priority(PCache *pcache, const VersionPtr &ver) {
	return pcache->cache_file->GetPolicy()->GetPriority(*ver.ptr);
}

/// Package Record Management
///
// Moves the Records into the correct place
void ver_file_lookup(PkgRecords *records, VerFileIterator *wrapper) {
	records->parser = &records->records->Lookup(wrapper->iterator);
}

void desc_file_lookup(PkgRecords *records, DescIterator *wrapper) {
	records->parser = &records->records->Lookup(wrapper->iterator.FileList());
}

rust::string ver_uri(PkgRecords *records, PkgIndexFile *index_file) {
	return index_file->index->ArchiveURI(records->parser->FileName());
}

rust::string long_desc(PkgRecords *records) {
	return records->parser->LongDesc();
}

rust::string short_desc(PkgRecords *records) {
	return records->parser->ShortDesc();
}

rust::string hash_find(PkgRecords *records, rust::string hash_type) {
	auto hashes = records->parser->Hashes();
	auto hash = hashes.find(hash_type.c_str());
	if (hash == NULL) { return "KeyError"; }
	return hash->HashValue();
}

rust::Vec<VersionPtr> dep_all_targets(DepIterator *wrapper) {
	rust::Vec<VersionPtr> list;

	std::unique_ptr<pkgCache::Version *[]> versions(wrapper->iterator.AllTargets());
	for (pkgCache::Version **I = versions.get(); *I != 0; I++) {
		list.push_back(
			wrap_version(pkgCache::VerIterator(*wrapper->iterator.Cache(), *I))
		);
	}
	return list;
}

// #define VALIDATE_ITERATOR(I) {
// 	if ((I).Cache() != &depcache->GetCache()) return(false);
// 	return(true); }

// template<typename Iterator>
// static bool _validate(Iterator iter, pkgDepCache *depcache) {
// 	if (iter.Cache() != &depcache->GetCache())
// 	{return false;} else {return true;}
// }

// bool validate(VerIterator *wrapper, PCache *pcache) {
// 	// if (pkg.ptr->Cache() != &pcache->depcache->GetCache())
// 	// {return false;} else {return true;}
// 	return _validate(wrapper->iterator, pcache->depcache);
// }

// bool validate(VerIterator *wrapper, PCache *pcache) {
// 	if (pkg.ptr->Cache() != &pcache->depcache->GetCache())
// 	{return false;} else {return true;}
// }

// PDepIterator *ver_iter_dep_iter(VerIterator *wrapper) {
// 	PDepIterator *new_wrapper = new PDepIterator();
// 	new_wrapper->iterator = pkg.ptr->DependsList();
// //	new_wrapper->cache = wrapper->cache;
// 	return new_wrapper;
// }

// void dep_iter_release(PDepIterator *wrapper) {
// 	delete wrapper;
// }

// void dep_iter_next(PDepIterator *wrapper) {
// 	++wrapper->iterator;
// }

// bool dep_iter_end(PDepIterator *wrapper) {
// 	return pkg.ptr->end();
// }

// PackagePtr *dep_iter_target_pkg(PDepIterator *wrapper) {
// 	PackagePtr *new_wrapper = new PackagePtr();
// 	new_wrapper->iterator = pkg.ptr->TargetPkg();
// //	new_wrapper->cache = wrapper->cache;
// 	return new_wrapper;
// }

// const char *ver_file_parser_maintainer(VerFileParser *parser) {
// 	std::string maint = parser->parser->Maintainer();
// 	return to_c_string(maint);
// }

// const char *ver_file_parser_homepage(VerFileParser *parser) {
// 	std::string hp = parser->parser->Homepage();
// 	return to_c_string(hp);
// }

/// Unused Functions
/// They may be used in the future
///
// void pkg_file_iter_next(PkgFileIterator *wrapper) {
// 	++wrapper->iterator;
// }

// bool pkg_file_iter_end(PkgFileIterator *wrapper) {
// 	return pkg.ptr->end();
// }

// const char *pkg_file_iter_file_name(PkgFileIterator *wrapper) {
// 	return pkg.ptr->FileName();
// }

// const char *pkg_file_iter_archive(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Archive();
// }

// const char *pkg_file_iter_version(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Version();
// }

// const char *pkg_file_iter_origin(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Origin();
// }

// const char *pkg_file_iter_codename(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Codename();
// }

// const char *pkg_file_iter_label(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Label();
// }

// const char *pkg_file_iter_site(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Site();
// }

// const char *pkg_file_iter_component(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Component();
// }

// const char *pkg_file_iter_architecture(PkgFileIterator *wrapper) {
// 	return pkg.ptr->Architecture();
// }

// const char *pkg_file_iter_index_type(PkgFileIterator *wrapper) {
// 	return pkg.ptr->IndexType();
// }
