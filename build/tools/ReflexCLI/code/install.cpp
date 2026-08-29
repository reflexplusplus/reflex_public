#include "tasks.h"
#include "reflex_ext/async/http.h"

#if defined(REFLEX_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef GetEnvironmentVariable
#endif


using namespace Reflex;

REFLEX_BEGIN_INTERNAL(ReflexCLI)

namespace CLI = Bootstrap::CLI;

constexpr CString::View kReleasesApiUrl = "https://reflexplusplus.dev/api/releases/reflex_public";
constexpr CString::View kCoreFilesLabel = "core files";
constexpr CString::View kUnexpectedApiResponse = "unexpected api response";

constexpr WString::View kReflexRepositoryNames[] = { L"reflex", L"reflex_public", L"reflex_private" };

bool IsReflexRepositoryPath(WString::View path)
{
	auto t1 = File::CorrectStrokes(path);

	auto t2 = Split(t1, File::kPathDelimiter);

	for (auto i : kReflexRepositoryNames)
	{
		if (Search<CaseInsensitive>(t2, i)) return true;
	}

	return false;
}

#if defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_LINUX)
WString SetUnixPathImpl(WString::View reflex_path, WString::View platform, ArrayView <Pair <WString::View>> profiles)
{
	static constexpr WString::View kExportPath = L"export PATH=";

	auto IsPathExport = [](WString::View line)
	{
		const WString::View paths[2] = { kExportPath, L"PATH=" };

		auto trimmed = Trim(line);

		for (auto & i : paths)
		{
			if (Left<true>(trimmed, i.size) == i) return true;
		}

		return false;
	};

	auto home = File::CorrectTrailingStroke(System::GetEnvironmentVariable(L"HOME"));
	auto shell = System::GetEnvironmentVariable(L"SHELL");

	if (!home) CLI::ThrowError("HOME environment variable is not set");

	WString::View profile = L".profile";
	auto shell_name = MakeKey32(File::SplitFilename(shell).b);
	for (auto & item : profiles)
	{
		if (shell_name == MakeKey32(item.a))
		{
			profile = item.b;
			break;
		}
	}

	auto profile_path = Join(home, profile);

	Data::Archive output;

	if (System::Exists(profile_path))
	{
		auto source = File::Open(profile_path);
		Data::Archive::View input = source;
		WString line;

		while (Data::ReadLine(input, line))
		{
			if (!(IsPathExport(line) && IsReflexRepositoryPath(line)))
			{
				Data::WriteLine(output, line);
			}
		}
	}

	auto cli_path = Join(reflex_path, L"bin/tools/", platform);

	Data::WriteLine(output, Join(kExportPath, WChar(kDoubleQuote), cli_path, L":$PATH", WChar(kDoubleQuote)));

	if (!File::Save(profile_path, output)) ThrowError("failed to update shell profile", profile_path);

	return cli_path;
}
#endif

#if defined(REFLEX_OS_MACOS)
WString SetPathImpl(WString::View reflex_path)
{
	static constexpr Pair <WString::View> kProfiles[] =
	{
		{ L"zsh", L".zshrc" },
		{ L"bash", L".bash_profile" },
	};

	return SetUnixPathImpl(reflex_path, L"macos", kProfiles);
}
#elif defined(REFLEX_OS_LINUX)
WString SetPathImpl(WString::View reflex_path)
{
	static constexpr Pair <WString::View> kProfiles[] =
	{
		{ L"zsh", L".zshrc" },
		{ L"bash", L".bashrc" },
	};

	return SetUnixPathImpl(reflex_path, L"linux", kProfiles);
}
#elif defined(REFLEX_OS_WINDOWS)
WString ReadWindowsUserPath(HKEY key)
{
	DWORD type = 0;
	DWORD size = 0;
	DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND;

	auto result = RegGetValueW(key, nullptr, L"Path", flags, &type, nullptr, &size);

	if (result == ERROR_FILE_NOT_FOUND) return {};

	if (result == ERROR_SUCCESS /*&& (type == REG_SZ || type == REG_EXPAND_SZ)*/)
	{
		WString value(size / sizeof(WChar));

		result = RegGetValueW(key, nullptr, L"Path", flags, &type, Reinterpret<BYTE>(value.GetData()), &size);

		if (result == ERROR_SUCCESS)
		{
			if (UInt length = size / sizeof(WChar))
			{
				value.SetSize(length - 1);
			}
			else
			{
				value.Clear();
			}

			return value;
		}
	}

	CLI::ThrowError("failed to read user PATH");

	return {};
}

WString SetPathImpl(WString::View reflex_path)
{
	HKEY key = nullptr;

	if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, nullptr, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
	{
		CLI::ThrowError("failed to open user environment settings");
	}

	auto path = ReadWindowsUserPath(key);

	Array <WString> entries;

	for (auto & raw : Split(path, L';'))
	{
		auto entry = Trim(raw);

		if (entry && !IsReflexRepositoryPath(entry)) entries.Push(entry);
	}

	entries.Push(PlatformPath(Join(reflex_path, L"bin/tools/win"), System::kPlatformWindows));

	auto updated_path = Merge(entries, L';');

	auto size = DWORD((updated_path.GetSize() + 1) * sizeof(WChar));

	if (RegSetValueExW(key, L"Path", 0, REG_SZ, Reinterpret<const BYTE>(updated_path.GetData()), size) != ERROR_SUCCESS)
	{
		RegCloseKey(key);
		CLI::ThrowError("failed to update user PATH");
	}

	RegCloseKey(key);

	DWORD_PTR unused;
	SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"), SMTO_ABORTIFHUNG, 5000, &unused);

	return entries.GetLast();
}
#endif

void WaitForProcess(CString::View label, const WString & process_path, ArrayView <WString> args)
{
	Require(RunCommand(process_path, args), label, "failed");
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
	UInt64 date = 0;
};

struct PackageAsset
{
	CString id;
	CString label;
	CString url;
	WString output_path;
	WString install_path;
};

struct DeferredMove
{
	WString src_path;
	WString dst_path;
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

void PrintConfirmation(System::FileHandle & std_out, CString::View label, WString::View path)
{
	const auto size_mb = Float64(File::Open(path).GetSize()) / Float64(1024 * 1024);

	File::WriteLine(std_out, Join(kColourDim, "Installed ", kColourDefault, label, ' ', kColourDim, ToCString(size_mb, 2), "mb", kColourDefault));
}

Map <Key32> GetRequestedPlatforms(const Array <CString::View> & platforms)
{
	Map <Key32> requested;

	if (platforms)
	{
		for (auto & raw : platforms)
		{
			requested.Set(MakeKey32(Lowercase(raw)));
		}
	}
	else
	{
		requested.Set(GetPlatform());
	}

	return requested;
}

WString GetInstallPathForAsset(CString::View label, WString::View install_root)
{
	if (CaseInsensitive::eq(label, "binaries"))
	{
		return Join(install_root, L"bin/lib", File::kStroke);
	}

	if (CaseInsensitive::eq(label, "tools") || CaseInsensitive::eq(label, "docs"))
	{
		return Join(install_root, L"bin/tools", File::kStroke);
	}

	CLI::ThrowError(Join("unknown asset label ", label));

	return {};
}

Array <PackageAsset> GetInstallAssets(const Data::PropertySet & release, const Map <Key32> & platforms, WString::View install_root, WString::View temp_path)
{
	Array <PackageAsset> assets;

	for (auto & asset : Data::GetPropertySetArray(release, "assets"))
	{
		auto platform = Data::GetCString(asset, "platform");
		auto name = Data::GetCString(asset, "name");
		auto label = Data::GetCString(asset, "label");
		auto url = Data::GetCString(asset, "url");

		if (!platform || !name || !label || !url)
		{
			CLI::ThrowError("release asset is missing required fields");
		}

		if (!platforms.Search(MakeKey32(platform))) continue;

		auto id = Join(platform, "-", label);

		assets.Push
		({
			.id = id,
			.label = Join(platform, "-", label),
			.url = Join(url),
			.output_path = Join(temp_path, L"download/", ToWString(name)),
			.install_path = GetInstallPathForAsset(label, install_root)
		});
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
		File::WriteLine(batch_file, Join(L"move /Y \"", PlatformPath(move.src_path, System::kPlatformWindows), L"\" \"", PlatformPath(move.dst_path, System::kPlatformWindows), L"\""));
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

Reference <Data::PropertySet> FetchJson(System::FileHandle & std_out, CString::View busy_label, CString::View url)
{
	Reference <Data::PropertySet> response;

	CLI::Await(std_out, Join(kColourDim, busy_label, kColourDefault), false, {}, [url = Join(url), &response](CLI::TaskContext & ctx)
	{
		auto [success, result] = Async::Detail::Fetch(ctx, System::HttpConnection::kGET, url, {}, {}, Make<Async::Detail::StandardHttpRequestCallbacks>(Data::kJsonFormatOptionInt64));

		if (auto decoded = DynamicCast<Data::PropertySet>(result))
		{
			response = decoded;
		}
		else if (success)
		{
			CLI::ThrowError(kUnexpectedApiResponse);
		}
		else
		{
			CLI::ThrowError("failed to fetch api response");
		}
	});

	if (!response || !Data::GetBool(*response, "status"))
	{
		CLI::ThrowError("api request failed");
	}

	return response;
}

Array <ReleaseVersion> GetReleases(System::FileHandle & std_out, UInt limit)
{
	Array <ReleaseVersion> releases;

	auto response = FetchJson(std_out, "Contacting server...", Join(kReleasesApiUrl, "?limit=50"));

	for (auto & release : Data::GetPropertySetArray(*response, "releases"))
	{
		releases.Push({ .tag = Data::GetCString(release, "tag"), .date = UInt64(Data::GetInt64(release, "date")) });
	}

	return releases;
}

void DownloadFile(CString::View label, CString::View url, WString::View output_path, System::FileHandle & std_out)
{
	CLI::Await(std_out, Join(kColourDim, "Downloading ", kColourDefault, label), true, {}, [output_path = WString(output_path), url = Join(url), label = Join(label)](CLI::TaskContext & ctx)
	{
		auto [success, result] = Async::Detail::Fetch(ctx, System::HttpConnection::kGET, url, {}, {}, Make<Async::Detail::StandardHttpRequestCallbacks>());

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

CString ReadInstalledVersion()
{
	auto version_path = Join(GetReflexPath(), L"bin/lib/", ToWString(GetPlatform()), L"/version.txt");

	auto utf8 = File::Open(version_path);
	
	return Trim(Data::Unpack<CString::View>(utf8));
}

REFLEX_END_INTERNAL

void ReflexCLI::ListVersions(System::FileHandle & std_out)
{
	auto installed_version = ReadInstalledVersion();

	Array <ReleaseVersion> releases = GetReleases(std_out, 50);
	
	for (auto & i : releases)
	{
		//auto [year,month,day] = Reflex::UnixTimestampToDate(i.date);

		if (installed_version && i.tag == installed_version)
		{
			File::WriteLine(std_out, Join(i.tag, kColourDim, " [installed]", kColourDefault));
		}
		else
		{
			File::WriteLine(std_out, i.tag);
		}
	}
}

void ReflexCLI::GetVersion(System::FileHandle & std_out)
{
	if (auto version = ReadInstalledVersion())
	{
		File::WriteLine(std_out, version);
	}
	else
	{
		CLI::ThrowError("version info not found");
	}
}

void ReflexCLI::SetPath(const WString & path, System::FileHandle & std_out)
{
	auto reflex_path = File::CorrectTrailingStroke(path);

	if (System::Exists(GetReflexExecutablePath(reflex_path)))
	{
		auto cli_path = SetPathImpl(reflex_path);

		File::WriteLine(std_out, Join(L"PATH updated to ", cli_path));
		File::WriteLine(std_out, "Restart your terminal session for the change to take effect.");
	}
	else
	{
		ThrowError("reflex executable not found", reflex_path);
	}
}

void ReflexCLI::Install(CString::View version, const Array <CString::View> & platforms, const WString & path, bool test, System::FileHandle & std_out)
{
	auto install_root = path ? WString(path) : GetReflexPath();

	auto temp_path = CreateInstallTempPath(install_root);

	CString release_url = Join(kReleasesApiUrl, '/', version);

	if (!version)
	{
		auto releases = GetReleases(std_out, 1);

		if (!releases) CLI::ThrowError(kUnexpectedApiResponse);

		release_url.Append(releases.GetFirst().tag);
	}

	auto release = FetchJson(std_out, "Contacting server...", release_url);
	
	auto release_tag = Data::GetCString(*release, "tag");

	if (!release_tag) CLI::ThrowError(kUnexpectedApiResponse);

	auto requested_platforms = GetRequestedPlatforms(platforms);
	auto install_assets = GetInstallAssets(*release, requested_platforms, install_root, temp_path);

	File::WriteLine(std_out, Join(kColourDim, "Installing ", kColourDefault, release_tag, kColourDim, kColourDefault));

	auto content = Data::GetPropertySet(*release, "content");
	auto content_name = Data::GetCString(content, "name");
	auto content_url = Data::GetCString(content, "url");
	auto content_label = Data::GetCString(content, "label", kCoreFilesLabel);

	if (!content_name || !content_url)
	{
		CLI::ThrowError("release content is missing required fields");
	}

	auto zip_path = Join(temp_path, L"download/", ToWString(content_name));

	DownloadFile(content_label, content_url, zip_path, std_out);
		
	Array <DeferredMove> deferred_moves;

	InstallReleaseArchive(zip_path, install_root, temp_path, std_out, deferred_moves, test);

	PrintConfirmation(std_out, content_label, zip_path);

	if (!test)
	{
		ResetInstallTargets(install_root);
	}

	for (auto & asset : install_assets)
	{
		DownloadFile(asset.label, asset.url, asset.output_path, std_out);
			
		InstallPackageArchive(asset.id, asset.output_path, asset.install_path, temp_path, std_out, deferred_moves, test);

		PrintConfirmation(std_out, asset.label, asset.output_path);
	}

	if (!test)
	{
		UpdateAliasLaunchers(install_root);
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
