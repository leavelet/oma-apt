#[cfg(test)]
mod cache {
	use rust_apt::cache;
	use rust_apt::cache::*;

	#[test]
	fn version_vec() {
		let cache = Cache::new();

		let mut versions = Vec::new();
		// Don't unwrap and assign so that the package can
		// Get out of scope from the version
		if let Some(apt) = cache.get("apt") {
			for version in apt.versions() {
				versions.push(version);
			}
		}
		// Apt is now out of scope
		assert!(!versions.is_empty());
	}

	#[test]
	fn all_packages() {
		let cache = Cache::new();
		let sort = PackageSort::default();

		// All real packages should not be empty.
		assert!(!cache.packages(&sort).collect::<Vec<_>>().is_empty());
		for pkg in cache.packages(&sort) {
			println!("{pkg}")
		}
	}

	#[test]
	fn descriptions() {
		let cache = Cache::new();

		// Apt should exist
		let pkg = cache.get("apt").unwrap();
		// Apt should have a candidate
		let cand = pkg.candidate().unwrap();
		// Apt should be installed
		let inst = pkg.installed().unwrap();

		// Assign installed descriptions
		let inst_sum = inst.summary();
		let inst_desc = inst.description();

		// Assign candidate descriptions
		let cand_sum = cand.summary();
		let cand_desc = cand.description();

		// If the lookup fails for whatever reason
		// The summary and description are the same
		assert_ne!(inst_sum, inst_desc);
		assert_ne!(cand_sum, cand_desc);
	}

	#[test]
	fn version_uris() {
		let cache = Cache::new();
		let pkg = cache.get("apt").unwrap();
		// Only test the candidate.
		// It's possible for the installed version to have no uris
		let cand = pkg.candidate().unwrap();
		assert!(!cand.uris().collect::<Vec<_>>().is_empty());
	}

	#[test]
	fn depcache_marked() {
		let cache = Cache::new();
		let pkg = cache.get("apt").unwrap();
		assert!(!pkg.marked_install());
		assert!(!pkg.marked_upgrade());
		assert!(!pkg.marked_delete());
		assert!(pkg.marked_keep());
		assert!(!pkg.marked_downgrade());
		assert!(!pkg.marked_reinstall());
		assert!(!pkg.is_now_broken());
		assert!(!pkg.is_inst_broken());
	}

	#[test]
	fn hashes() {
		let cache = Cache::new();
		let pkg = cache.get("apt").unwrap();
		let version_list = pkg.versions().collect::<Vec<_>>();
		// Should not be zero for apt
		assert!(!version_list.is_empty());
		for version in pkg.versions() {
			assert!(version.sha256().is_some());
			assert!(version.hash("sha256").is_some());
			assert!(version.sha512().is_none());
			assert!(version.hash("md5sum").is_some());
			assert!(version.hash("sha1").is_none())
		}
	}

	#[test]
	fn provides() {
		let cache = Cache::new();
		if let Some(pkg) = cache.get("www-browser") {
			let provides = cache.provides(&pkg, true).collect::<Vec<_>>();
			assert!(!provides.is_empty());
		};
	}

	#[test]
	fn depends() {
		let cache = Cache::new();

		let pkg = cache.get("apt").unwrap();
		let cand = pkg.candidate().unwrap();
		// Apt candidate should have dependencies
		for deps in cand.dependencies().unwrap() {
			for dep in &deps.base_deps {
				// Apt Dependencies should have targets
				assert!(!dep.all_targets().collect::<Vec<_>>().is_empty());
			}
		}
		assert!(cand.recommends().is_some());
		assert!(cand.suggests().is_some());
		// TODO: Add these as methods
		assert!(cand.get_depends("Replaces").is_some());
		assert!(cand.get_depends("Conflicts").is_some());
		assert!(cand.get_depends("Breaks").is_some());

		// This part is basically just formatting an apt depends String
		// Like you would see in `apt show`
		let mut dep_str = String::new();
		dep_str.push_str("Depends: ");
		for dep in cand.depends_map().get("Depends").unwrap() {
			if dep.is_or() {
				let mut or_str = String::new();
				let total = dep.base_deps.len() - 1;
				for (num, base_dep) in dep.base_deps.iter().enumerate() {
					or_str.push_str(base_dep.name());
					if !base_dep.comp().is_empty() {
						or_str.push_str(&format!("({} {})", base_dep.comp(), base_dep.version()))
					}
					if num != total {
						or_str.push_str(" | ");
					} else {
						or_str.push_str(", ");
					}
				}
				dep_str.push_str(&or_str)
			} else {
				let lone_dep = dep.first();
				dep_str.push_str(lone_dep.name().as_str());
				if !lone_dep.comp().is_empty() {
					dep_str.push_str(&format!(" ({} {})", lone_dep.comp(), lone_dep.version()))
				}
				dep_str.push_str(", ");
			}
		}
		println!("{dep_str}");
	}

	#[test]
	fn sources() {
		let cache = Cache::new();
		// If the source lists don't exists there is problems.
		assert!(!cache.sources().collect::<Vec<_>>().is_empty());
	}

	#[test]
	fn cache_count() {
		let cache = Cache::new();
		match cache.disk_size() {
			DiskSpace::Require(num) => {
				assert_eq!(num, 0);
			},
			DiskSpace::Free(num) => {
				panic!("Whoops it should be 0, not {num}.");
			},
		}
	}

	#[test]
	fn unit_str() {
		let testcase = [
			(1649267441664_u64, "1.50 TiB", "1.65 TB"),
			(1610612736_u64, "1.50 GiB", "1.61 GB"),
			(1572864_u64, "1.50 MiB", "1.57 MB"),
			(1536_u64, "1.50 KiB", "1.54 KB"),
			(1024_u64, "1024 B", "1.02 KB"),
			(1_u64, "1 B", "1 B"),
		];

		for (num, binary, decimal) in testcase {
			assert_eq!(binary, cache::unit_str(num, NumSys::Binary));
			assert_eq!(decimal, cache::unit_str(num, NumSys::Decimal));
		}
	}
}

#[cfg(test)]
mod sort {
	use rust_apt::cache::*;

	#[test]
	fn defaults() {
		let cache = Cache::new();
		let mut installed = false;
		let mut auto_installed = false;

		// Test defaults and ensure there are no virtual packages.
		// And that we have any packages at all.
		let mut real_pkgs = Vec::new();
		let mut virtual_pkgs = Vec::new();

		let sort = PackageSort::default();

		for pkg in cache.packages(&sort) {
			if pkg.is_auto_installed() {
				auto_installed = true;
			}
			if pkg.is_installed() {
				installed = true;
			}

			if pkg.has_versions() {
				real_pkgs.push(pkg);
				continue;
			}
			virtual_pkgs.push(pkg);
		}
		assert!(!real_pkgs.is_empty());
		assert!(virtual_pkgs.is_empty());
		assert!(auto_installed);
		assert!(installed)
	}

	#[test]
	fn include_virtual() {
		let cache = Cache::new();

		// Check that we have virtual and real packages after sorting.
		let mut real_pkgs = Vec::new();
		let mut virtual_pkgs = Vec::new();

		let sort = PackageSort::default().include_virtual();

		for pkg in cache.packages(&sort) {
			if pkg.has_versions() {
				real_pkgs.push(pkg);
				continue;
			}
			virtual_pkgs.push(pkg);
		}
		assert!(!real_pkgs.is_empty());
		assert!(!virtual_pkgs.is_empty());
	}

	#[test]
	fn only_virtual() {
		let cache = Cache::new();

		// Check that we have only virtual packages.
		let mut real_pkgs = Vec::new();
		let mut virtual_pkgs = Vec::new();

		let sort = PackageSort::default().only_virtual();

		for pkg in cache.packages(&sort) {
			if pkg.has_versions() {
				real_pkgs.push(pkg);
				continue;
			}
			virtual_pkgs.push(pkg);
		}
		assert!(real_pkgs.is_empty());
		assert!(!virtual_pkgs.is_empty());
	}

	#[test]
	fn upgradable() {
		let cache = Cache::new();

		let sort = PackageSort::default().upgradable();
		for pkg in cache.packages(&sort) {
			// Sorting by upgradable skips the pkgDepCache same as `.is_upgradable(true)`
			// Here we check is_upgradable with the pkgDepCache to make sure there is
			// consistency
			assert!(pkg.is_upgradable(false))
		}

		let sort = PackageSort::default().not_upgradable();
		for pkg in cache.packages(&sort) {
			assert!(!pkg.is_upgradable(false))
		}
	}

	#[test]
	fn installed() {
		let cache = Cache::new();

		let sort = PackageSort::default().installed();
		for pkg in cache.packages(&sort) {
			assert!(pkg.is_installed())
		}

		let sort = PackageSort::default().not_installed();
		for pkg in cache.packages(&sort) {
			assert!(!pkg.is_installed())
		}
	}

	#[test]
	fn auto_installed() {
		let cache = Cache::new();

		let sort = PackageSort::default().auto_installed();
		for pkg in cache.packages(&sort) {
			assert!(pkg.is_auto_installed())
		}

		let sort = PackageSort::default().manually_installed();
		for pkg in cache.packages(&sort) {
			assert!(!pkg.is_auto_installed());
		}
	}

	#[test]
	fn auto_removable() {
		let cache = Cache::new();

		let sort = PackageSort::default().auto_removable();
		for pkg in cache.packages(&sort) {
			assert!(pkg.is_auto_removable())
		}

		let sort = PackageSort::default().not_auto_removable();
		for pkg in cache.packages(&sort) {
			assert!(!pkg.is_auto_removable())
		}
	}
}

/// Tests that require root
mod root {
	use rust_apt::cache::*;
	use rust_apt::progress::{AptUpdateProgress, UpdateProgress};
	use rust_apt::raw::apt;

	#[test]
	fn update() {
		let cache = Cache::new();
		struct Progress {}

		impl UpdateProgress for Progress {
			fn pulse_interval(&self) -> usize { 0 }

			fn hit(&mut self, id: u32, description: String) {
				println!("\rHit:{} {}", id, description);
			}

			fn fetch(&mut self, id: u32, description: String, file_size: u64) {
				if file_size != 0 {
					println!(
						"\rGet:{id} {description} [{}]",
						unit_str(file_size, NumSys::Decimal)
					);
				} else {
					println!("\rGet:{id} {description}");
				}
			}

			fn done(&mut self) {}

			fn start(&mut self) {}

			fn stop(
				&mut self,
				fetched_bytes: u64,
				elapsed_time: u64,
				current_cps: u64,
				_pending_errors: bool,
			) {
				if fetched_bytes != 0 {
					println!(
						"Fetched {} in {} ({}/s)",
						unit_str(fetched_bytes, NumSys::Decimal),
						time_str(elapsed_time),
						unit_str(current_cps, NumSys::Decimal)
					);
				} else {
					println!("Nothing to fetch.");
				}
			}

			fn fail(&mut self, id: u32, description: String, status: u32, error_text: String) {
				let mut show_error = true;

				if status == 0 || status == 2 {
					println!("\rIgn: {id} {description}");
					if error_text.is_empty() {
						show_error = false;
					}
				} else {
					println!("\rErr: {id} {description}");
				}
				if show_error {
					println!("\r{error_text}");
				}
			}

			fn pulse(
				&mut self,
				_workers: Vec<apt::Worker>,
				_percent: f32,
				_total_bytes: u64,
				_current_bytes: u64,
				_current_cps: u64,
			) {
				return;
			}
		}

		// Test a new impl for UpdateProgress
		let mut progress: Box<dyn UpdateProgress> = Box::new(Progress {});
		cache.update(&mut progress).unwrap();

		// Test the default implementation for it
		let mut progress: Box<dyn UpdateProgress> = Box::new(AptUpdateProgress::new());
		cache.update(&mut progress).unwrap();
	}
}
