#include "tasks.h"
#include "reflex_ext/async/http.h"



using namespace Reflex;

REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr CString::View kGithubReleasesUrl = "https://api.github.com/repos/reflexplusplus/reflex_public/releases?per_page=100";
constexpr CString::View kGithubLatestReleaseUrl = "https://api.github.com/repos/reflexplusplus/reflex_public/releases/latest";
constexpr CString::View kGithubReleaseByTagUrlPrefix = "https://api.github.com/repos/reflexplusplus/reflex_public/releases/tags/";
constexpr CString::View kGithubTagArchiveUrlPrefix = "https://github.com/reflexplusplus/reflex_public/archive/refs/tags/";
constexpr CString::View kCoreFilesLabel = "core files";

void WaitForProcess(CString::View label, const WString & process_path, ArrayView <WString> args)
{
	if (auto process = Make<System::Process>(process_path, args, System::Process::Options{ .allow_window = false }))
	{
		process->Wait();

		auto exit_code = process->GetExitCode();

		if (And(exit_code, exit_code.value == 0)) return;
	}

	CLI::ThrowError(Join(label, " failed"));
}

void UnzipFile(CString::View label, WString::View zip_path, WString::View dest_path, System::FileHandle & std_out)
{
	File::MakePath(dest_path);

	CLI::Await(std_out, Join(kColourDim, "Extracting ", kColourDefault, label), false, {}, [zip_path = WString(zip_path), dest_path = WString(dest_path), label = Join(label)](CLI::TaskContext & ctx)
	{
		Array <WString> args;
	
		WString process_path;

		switch (System::kPlatform)
		{
		case System::kPlatformWindows:
			process_path = L"powershell";
			args.Append({ L"-NoProfile", L"-ExecutionPolicy", L"Bypass", L"-Command" });
			args.Push(Join(L"$ProgressPreference='SilentlyContinue'; Expand-Archive -LiteralPath '", zip_path, L"' -DestinationPath '", dest_path, L"' -Force"));
			break;

		case System::kPlatformMacOS:
			process_path = L"/usr/bin/unzip";
			args.Append({ L"-q", L"-o", zip_path, L"-d", dest_path });
			break;

		default:
			CLI::ThrowError("install unzip is not implemented on this platform");
		}

		WaitForProcess(label, process_path, args);
	});
}

struct ReleaseVersion
{
	CString tag;
	CString created_at;
};

struct PackageAsset
{
	CString label;
	CString asset_name;
	WString output_path;
	WString extract_path;
};

struct DeferredMove
{
	WString src_path;
	WString dst_path;
};

struct HttpCallbacks : public Async::Detail::StandardHttpRequestCallbacks
{
	REFLEX_OBJECT(ReflexCLI::HttpCallbacks, Async::Detail::StandardHttpRequestCallbacks);

	bool OnResponse(System::HttpConnection::Response status_code) override
	{
		return status_code == System::HttpConnection::kResponseFound || Async::Detail::StandardHttpRequestCallbacks::OnResponse(status_code);
	}

	void OnHeader(const CString::View & key, const CString::View & value) override
	{
		Async::Detail::StandardHttpRequestCallbacks::OnHeader(key, value);

		if (CaseInsensitive::eq(key, "location"))
		{
			location = Join(value);
		}
	}

	TRef <Object> OnComplete() override
	{
		if (location)
		{
			return New<Data::CStringProperty>(location);
		}

		return Async::Detail::StandardHttpRequestCallbacks::OnComplete();
	}

	CString location;
};

CString::View GetPlatform()
{
	switch (System::kPlatform)
	{
	case System::kPlatformWindows:
		return "win";

	case System::kPlatformMacOS:
		return "macos";
	}

	CLI::ThrowError("unknown platform");

	return {};
}

struct GithubReleaseCallbacks : public Async::Detail::StandardHttpRequestCallbacks
{
	REFLEX_OBJECT(ReflexCLI::GithubReleaseCallbacks, Async::Detail::StandardHttpRequestCallbacks);

	void OnHeader(const CString::View & key, const CString::View & value) override
	{
		Async::Detail::StandardHttpRequestCallbacks::OnHeader(key, value);

		if (CaseInsensitive::eq(key, "link"))
		{
			for (auto & raw_part : Split(value, ','))
			{
				auto part = Trim(raw_part);

				if (!Search(part, "rel=\"next\"")) continue;

				auto begin = Search(part, '<');
				auto end = Search(part, '>');

				if (begin && end && end.value > begin.value + 1)
				{
					next_url = Join(Mid<true>(part, begin.value + 1, end.value - begin.value - 1));

					break;
				}
			}
		}
	}

	CString next_url;
};

void PrintConfirmation(System::FileHandle & std_out, CString::View label, WString::View path)
{
	const auto size_mb = Float64(File::Open(path).GetSize()) / Float64(1024 * 1024);

	File::WriteLine(std_out, Join(kColourDim, "Installed ", kColourDefault, label, ' ', kColourDim, ToCString(size_mb, 2), "mb", kColourDefault));
}

Array <PackageAsset> GetInstallAssets(const Array <CString::View> & platforms, WString::View install_root, WString::View temp_path)
{
	Array <PackageAsset> assets;

	Map <Key32> trimmed;

	if (platforms)
	{
		for (auto & raw : platforms)
		{
			trimmed.Set(MakeKey32(Lowercase(raw)));
		}
	}
	else
	{
		trimmed.Set(GetPlatform());
	}

	const auto lib_path = Join(install_root, L"bin/lib", File::kStroke);
	const auto tools_path = Join(install_root, L"bin/tools", File::kStroke);

	for (auto & i : trimmed)
	{
		switch (i.key.value)
		{
		case K32("win"):
			assets.Push({ "win-binaries", "reflex-libs-windows.zip", Join(temp_path, L"download/reflex-libs-windows.zip"), lib_path });
			assets.Push({ "win-tools", "reflex-tools-windows.zip", Join(temp_path, L"download/reflex-tools-windows.zip"), tools_path });
			assets.Push({ "win-docs", "reflex-docs-windows.zip", Join(temp_path, L"download/reflex-docs-windows.zip"), tools_path });
			break;

		case K32("macos"):
			assets.Push({ "macos-binaries", "reflex-libs-macos.zip", Join(temp_path, L"download/reflex-libs-macos.zip"), lib_path });
			assets.Push({ "macos-tools", "reflex-tools-macos.zip", Join(temp_path, L"download/reflex-tools-macos.zip"), tools_path });
			assets.Push({ "macos-docs", "reflex-docs-macos.zip", Join(temp_path, L"download/reflex-docs-macos.zip"), tools_path });
			break;

		case K32("android"):
			assets.Push({ "android-binaries", "reflex-libs-android.zip", Join(temp_path, L"download/reflex-libs-android.zip"), lib_path });
			break;

		case K32("ios"):
			assets.Push({ "ios-binaries", "reflex-libs-ios.zip", Join(temp_path, L"download/reflex-libs-ios.zip"), lib_path });
			break;
		};
	}

	if (!assets)
	{
		CLI::ThrowError("no platforms selected");
	}

	return assets;
}

WString CreateInstallTempPath(WString::View install_root)
{
	auto path = Join(install_root, L"tmp/", ToWString(System::GetProcessID()), L"-", ToWString(System::GetTime()), File::kStroke);

	File::MakePath(path);
	File::MakePath(Join(path, L"download", File::kStroke));
	File::MakePath(Join(path, L"extract", File::kStroke));

	return path;
}

bool DeletePathIfExists(WString::View path)
{
	if (!System::Exists(path))
	{
		return true;
	}

	if (File::IsDirectory(path))
	{
		File::DeletePath(path);

		return !System::Exists(path);
	}
	else
	{
		return System::Delete(path);
	}
}

bool HasDeferredSourceWithin(WString::View folder_path, ArrayView <DeferredMove> deferred_moves)
{
	auto folder = File::CorrectTrailingStroke(folder_path);

	for (auto & move : deferred_moves)
	{
		if (CaseInsensitive::eq(Left<true>(move.src_path, folder.GetSize()), folder))
		{
			return true;
		}
	}

	return false;
}

void ReplacePath(WString::View src_path, WString::View dst_path, Array <DeferredMove> & deferred_moves)
{
	constexpr auto defer = [](WString::View src_path, WString::View dst_path, Array <DeferredMove> &deferred_moves)
	{
		if (System::kPlatform == System::kPlatformWindows && !System::IsDirectory(src_path))
		{
			deferred_moves.Push({ Join(src_path), Join(dst_path) });

			return true;
		}

		return false;
	};

	if (System::Exists(dst_path) && !DeletePathIfExists(dst_path))
	{
		if (defer(src_path, dst_path, deferred_moves))
		{
			return;
		}

		CLI::ThrowError(Join("failed to replace ", ToCString(dst_path)));
	}

	if (!System::Rename(File::RemoveTrailingStroke(src_path), File::RemoveTrailingStroke(dst_path)))
	{
		if (defer(src_path, dst_path, deferred_moves))
		{
			return;
		}

		CLI::ThrowError(Join("failed to move ", ToCString(src_path), " to ", ToCString(dst_path)));
	}
}

void MoveDirectoryContent(WString::View src_path, WString::View dst_path, Array <DeferredMove> & deferred_moves)
{
	File::MakePath(dst_path);

	auto [folders, files] = File::List(src_path, true);

	for (auto & folder : folders)
	{
		auto src_folder = Join(src_path, folder.key);
		auto dst_folder = Join(dst_path, folder.key);

		if (!System::Exists(dst_folder))
		{
			if (System::Rename(File::RemoveTrailingStroke(src_folder), File::RemoveTrailingStroke(dst_folder)))
			{
				continue;
			}
		}

		if (System::IsDirectory(dst_folder))
		{
			MoveDirectoryContent(src_folder, dst_folder, deferred_moves);

			if (!HasDeferredSourceWithin(src_folder, deferred_moves))
			{
				DeletePathIfExists(src_folder);
			}
		}
		else
		{
			ReplacePath(src_folder, dst_folder, deferred_moves);
		}
	}

	for (auto & file : files)
	{
		ReplacePath(Join(src_path, file.key), Join(dst_path, file.key), deferred_moves);
	}
}

void InstallReleaseArchive(WString::View zip_path, WString::View install_root, WString::View temp_path, System::FileHandle & std_out, Array <DeferredMove> & deferred_moves, bool test)
{
	auto extract_path = Join(temp_path, L"extract/repo", File::kStroke);

	UnzipFile(kCoreFilesLabel, zip_path, extract_path, std_out);


	//GetArchiveRootPath

	auto [folders, files] = File::List(extract_path, true);

	if (folders.GetSize() != 1 || files)
	{
		CLI::ThrowError("unexpected release zip layout");
	}

	auto archive_root = Join(extract_path, folders.GetFirst().key);

	if (System::kPlatform == System::kPlatformWindows)
	{
		// Temporary workaround: keep the existing root wrapper alive for the duration
		// of `reflex install` until the package no longer ships reflex.bat.
		DeletePathIfExists(Join(archive_root, L"reflex.bat"));
	}

	if (!test)
	{
		MoveDirectoryContent(archive_root, install_root, deferred_moves);
	}
}

void InstallPackageArchive(CString::View label, WString::View zip_path, WString::View dest_path, WString::View temp_path, System::FileHandle & std_out, Array <DeferredMove> & deferred_moves, bool test)
{
	auto extract_path = Join(temp_path, L"extract/", ToWString(label), File::kStroke);

	UnzipFile(label, zip_path, extract_path, std_out);

	if (!test)
	{
		MoveDirectoryContent(extract_path, dest_path, deferred_moves);
	}
}

void CreateDeferredBatch(WString::View temp_path, ArrayView <DeferredMove> deferred_moves)
{
	auto batch_path = Join(temp_path, L"update.bat");
	auto self = System::GetExecutablePath();
	auto image_name = File::SplitFilename(self).b;

	auto batch_file = Make<System::FileHandle>(batch_path, System::FileHandle::kModeOverwrite);

	if (IsNull(batch_file))
	{
		CLI::ThrowError("failed to create deferred update script");
	}

	File::WriteLine(batch_file, "@echo off");
	File::WriteLine(batch_file, "setlocal");
	File::WriteLine(batch_file);
	File::WriteLine(batch_file, Join("set PID=", ToCString(System::GetProcessID())));
	File::WriteLine(batch_file, Join(L"set NAME=", image_name));
	File::WriteLine(batch_file);
	File::WriteLine(batch_file, ":wait");
	File::WriteLine(batch_file, "tasklist /FI \"PID eq %PID%\" /FI \"IMAGENAME eq %NAME%\" | find \"%PID%\" >nul");
	File::WriteLine(batch_file, "if not errorlevel 1 (");
	File::WriteLine(batch_file, "    timeout /T 1 /NOBREAK >nul");
	File::WriteLine(batch_file, "    goto wait");
	File::WriteLine(batch_file, ")");
	File::WriteLine(batch_file);

	for (auto & move : deferred_moves)
	{
		File::WriteLine(batch_file, Join(L"move /Y \"", Replace(move.src_path, File::kStroke, L'\\'), L"\" \"", Replace(move.dst_path, File::kStroke, L'\\'), L"\""));
	}

	File::WriteLine(batch_file);
	File::WriteLine(batch_file, "rem cleanup disabled for deferred update debugging");

	batch_file.Clear();

	Array <WString> args = { L"/c", batch_path };

	if (auto process = Make<System::Process>(L"cmd.exe", args, System::Process::Options{ .allow_window = false }))
	{
		process->Detach();
	}
	else
	{
		CLI::ThrowError("failed to launch deferred update");
	}
}

void ResetInstallTargets(WString::View install_root)
{
	const auto bin_path = Join(install_root, L"bin");
	const auto lib_path = Join(install_root, L"bin/lib");
	const auto tools_path = Join(install_root, L"bin/tools");

	if (System::Exists(lib_path))
	{
		File::DeletePath(lib_path);
	}

	if (System::Exists(tools_path))
	{
		File::DeletePath(tools_path);
	}

	File::MakePath(bin_path);
	File::MakePath(lib_path);
	File::MakePath(tools_path);
}

void CreateAliasLauncher(WString::View launcher_path, WString::View target_path)
{
	auto launcher_dir = File::SplitFilename(launcher_path).a;

	if (launcher_dir)
	{
		File::MakePath(launcher_dir);
	}

	switch (System::kPlatform)
	{
		case System::kPlatformWindows:
		{
			auto target_dir = File::SplitFilename(target_path).a;
			auto target_name = File::SplitFilename(target_path).b;
			const auto quote_powershell = [](WString::View value)
			{
				return Replace(Join(value), L"'", L"''");
			};

			Array <WString> args =
			{
				L"-NoProfile",
				L"-ExecutionPolicy",
				L"Bypass",
				L"-Command",
				Join(
					L"$ws = New-Object -ComObject WScript.Shell; ",
					L"$s = $ws.CreateShortcut('", quote_powershell(launcher_path), L"'); ",
					L"$s.TargetPath = '", quote_powershell(target_path), L"'; ",
					L"$s.WorkingDirectory = '", quote_powershell(target_dir), L"'; ",
					L"$s.IconLocation = '", quote_powershell(target_path), L",0'; ",
					L"$s.Description = '", quote_powershell(target_name), L"'; ",
					L"$s.Save()"
				)
			};

			WaitForProcess("create shortcut", L"powershell", args);
		}
		break;

	case System::kPlatformMacOS:
		{
			DeletePathIfExists(launcher_path);

			Array <WString> args = { L"-s", Join(target_path), Join(launcher_path) };

			WaitForProcess("create symlink", L"/bin/ln", args);
		}
		break;

	default:
		CLI::ThrowError("CreateAliasLauncher is not implemented on this platform");
	}

}

void UpdateAliasLaunchers(WString::View install_root)
{
	const WString::View legacy[] =
	{
		L"install.bat",
		L"install.ps1",
		L"install.command"
	};

	for (auto i : legacy)
	{
		DeletePathIfExists(Join(install_root, i));
	}

	switch (System::kPlatform)
	{
	case System::kPlatformWindows:
		CreateAliasLauncher(Join(install_root, L"documentation/ReflexDocumentation.lnk"), Join(install_root, L"bin/tools/win/ReflexDocumentation.exe"));
		CreateAliasLauncher(Join(install_root, L"ReflexProjectCreator.lnk"), Join(install_root, L"bin/tools/win/ReflexProjectCreator.exe"));
		break;

	case System::kPlatformMacOS:
		CreateAliasLauncher(Join(install_root, L"documentation/ReflexDocumentation"), Join(install_root, L"bin/tools/macos/ReflexDocumentation.app"));
		CreateAliasLauncher(Join(install_root, L"ReflexProjectCreator"), Join(install_root, L"bin/tools/macos/ReflexProjectCreator.app"));
		break;
	}
}

void DownloadFile(CString::View label, CString::View url, WString::View output_path, System::FileHandle & std_out)
{
	CString redirect_url;

	CLI::Await(std_out, Join("Resolving URL ", label), false, {}, [url = Join(url), &redirect_url, label = Join(label)](CLI::TaskContext & ctx)
	{
		auto [success, result] = Async::Detail::Fetch(ctx, System::HttpConnection::kGET, url, {}, {}, New<HttpCallbacks>());

		if (auto redirect = DynamicCast<Data::CStringProperty>(result))
		{
			redirect_url = redirect->value;
		}
		else if (success)
		{
			CLI::ThrowError(Join(label, " did not provide redirect location"));
		}
		else
		{
			CLI::ThrowError(Join("failed to resolve ", label));
		}
	});

	CLI::Await(std_out, Join(kColourDim, "Downloading ", kColourDefault, label), true, {}, [output_path = WString(output_path), &redirect_url, label = Join(label)](CLI::TaskContext & ctx)
	{
		//TODO override callbacks to stream to disk

		auto [success, result] = Async::Detail::Fetch(ctx, System::HttpConnection::kGET, redirect_url, {}, {}, New<Async::Detail::StandardHttpRequestCallbacks>());

		if (auto archive = DynamicCast<Data::ArchiveObject>(result))
		{
			if (!File::Save(output_path, archive->value))
			{
				CLI::ThrowError(Join("failed to write ", ToCString(output_path)));
			}
		}
		else if (success)
		{
			CLI::ThrowError("unexpected response");
		}
		else
		{
			CLI::ThrowError(Join("failed to download ", label));
		}
	});
}

REFLEX_END_INTERNAL

void ReflexCLI::ListVersions(System::FileHandle & std_out)
{
	const Async::HttpHeaders headers =
	{
		{ "Accept", "application/vnd.github+json" },
		{ "User-Agent", "ReflexCLI" },
		{ "X-GitHub-Api-Version", "2022-11-28" },
	};

	Array <ReleaseVersion> releases;

	CString next_url = Join(kGithubReleasesUrl);

	bool has_more_pages = true;

	while (has_more_pages)
	{
		CLI::Await(std_out, "fetching github release versions...", false, {}, [headers, &has_more_pages, &next_url, &releases](CLI::TaskContext & ctx)
		{
			auto callbacks = Make<GithubReleaseCallbacks>();

			auto [success, result] = Async::Detail::Fetch(ctx, System::HttpConnection::kGET, next_url, headers, {}, *callbacks);

			if (auto response = DynamicCast<Data::PropertySet>(result))
			{
				for (auto & release : Data::GetPropertySetArray(*response, kNullKey))
				{
					if (auto tag = Data::GetCString(release, "tag_name"))
					{
						releases.Push
						({
							.tag = Join(tag),
							.created_at = Join(Data::GetCString(release, "created_at"))
						});
					}
				}

				next_url = callbacks->next_url;

				has_more_pages = True(next_url);
			}
			else
			{
				has_more_pages = false;

				if (success)
				{
					CLI::ThrowError("unexpected github response");
				}
				else
				{
					CLI::ThrowError("failed to fetch github releases");
				}
			}
		});
	}

	Sort(releases, [](const ReleaseVersion & a, const ReleaseVersion & b)
	{
		if (a.created_at == b.created_at)
		{
			return a.tag < b.tag;
		}

		return a.created_at < b.created_at;
	});

	for (auto & release : ReverseIterate(releases))
	{
		File::WriteLine(std_out, release.tag);
	}
}

void ReflexCLI::GetVersion(System::FileHandle & std_out)
{
	auto version_path = Join(GetReflexPath(), L"bin/lib/", ToWString(GetPlatform()), L"/version.txt");

	if (!System::Exists(version_path))
	{
		CLI::ThrowError(Join("version file not found ", ToCString(version_path)));
	}

	auto utf8 = Data::DecodeUTF8(File::Open(version_path));
	auto version = Trim<WChar>(utf8);

	if (!version)
	{
		CLI::ThrowError("version file is empty");
	}

	File::WriteLine(std_out, version);
}

void ReflexCLI::Install(CString::View version, const Array <CString::View> & platforms, const WString & path, bool test, System::FileHandle & std_out)
{
	const Async::HttpHeaders headers =
	{
		{ "Accept", "application/vnd.github+json" },
		{ "User-Agent", "ReflexCLI" },
		{ "X-GitHub-Api-Version", "2022-11-28" },
	};

	auto install_root = path ? WString(path) : GetReflexPath();

	auto temp_path = CreateInstallTempPath(install_root);

	CString release_url;
	
	switch (MakeKey32(Lowercase(version)))
	{
	case K32(""):
	case K32("latest"):
		release_url = kGithubLatestReleaseUrl;
		break;

	default:
		release_url = Join(kGithubReleaseByTagUrlPrefix, version);
		break;
	};

	CString release_tag;

	CLI::Await(std_out, Join(kColourDim, "Resolving URL", kColourDefault), false, {}, [headers, &release_tag, &release_url](CLI::TaskContext & ctx)
	{
		auto callbacks = New<Async::Detail::StandardHttpRequestCallbacks>();

		auto [success, result] = Async::Detail::Fetch(ctx, System::HttpConnection::kGET, release_url, headers, {}, *callbacks);

		if (auto response = DynamicCast<Data::PropertySet>(result))
		{
			release_tag = Data::GetCString(*response, "tag_name");

			if (!release_tag)
			{
				if (auto message = Data::GetCString(*response, "message"))
				{
					CLI::ThrowError(message);
				}

				CLI::ThrowError("release is missing tag_name");
			}
		}
		else if (success)
		{
			CLI::ThrowError("unexpected github response");
		}
		else
		{
			CLI::ThrowError("failed to resolve github release");
		}
	});

	Array <DeferredMove> deferred_moves;

	auto install_assets = GetInstallAssets(platforms, install_root, temp_path);

	File::WriteLine(std_out, Join(kColourDim, "Installing ", kColourDefault, release_tag, kColourDim, kColourDefault));

	auto zip_path = Join(temp_path, L"download/reflex_public-", ToWString(release_tag), L".zip");
	
	auto archive_url = Join(kGithubTagArchiveUrlPrefix, release_tag, ".zip");
		
	DownloadFile(kCoreFilesLabel, archive_url, zip_path, std_out);
		
	InstallReleaseArchive(zip_path, install_root, temp_path, std_out, deferred_moves, test);

	if (!test)
	{
		ResetInstallTargets(install_root);
	}

	PrintConfirmation(std_out, kCoreFilesLabel, zip_path);

	for (auto & asset : install_assets)
	{
		auto asset_url = Join("https://github.com/reflexplusplus/reflex_public/releases/download/", release_tag, '/', asset.asset_name);

		DownloadFile(asset.label, asset_url, asset.output_path, std_out);
			
		InstallPackageArchive(asset.label, asset.output_path, asset.extract_path, temp_path, std_out, deferred_moves, test);

		PrintConfirmation(std_out, asset.label, asset.output_path);
	}

	if (!test)
	{
		UpdateAliasLaunchers(install_root);
	}

	if (test)
	{
		File::WriteLine(std_out, Join(kColourDim, "Test mode enabled; extracted files were not moved into ", kColourDefault, ToCString(install_root), kColourDefault));
	}

	if (deferred_moves)
	{
		if (System::kPlatform == System::kPlatformWindows)
		{
			CreateDeferredBatch(temp_path, deferred_moves);

			return;
		}
		else
		{
			for (auto & move : deferred_moves)
			{
				CLI::Print(std_out, CLI::kColourBrightRed, ToCString(move.dst_path));
			}

			CLI::ThrowError(Join("failed to install ", ToCString(deferred_moves.GetSize()), "files"));
		}
	}

	DeletePathIfExists(temp_path);
}
