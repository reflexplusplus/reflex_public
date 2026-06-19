#requires -Version 5
# Installs the prebuilt Reflex SDK (libs + tools + docs) for Windows from the
# repo's GitHub Release. The git tag is the version; version.txt (baked into the
# distributed tree by CI) selects it, falling back to the latest release.
#
# Environment overrides:
#   REFLEX_GITHUB_REPO   owner/name (default: derived from the git origin remote)
#   REFLEX_GITHUB_TOKEN  required only for the private/licensed repo

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Platform = 'win'
$InstalledVersionFile = Join-Path $Root 'bin\version.txt'

# --- resolve version --------------------------------------------------------
$VersionFile = Join-Path $Root 'version.txt'
if (Test-Path $VersionFile) { $Version = (Get-Content $VersionFile -Raw).Trim() } else { $Version = 'latest' }

# --- resolve repo -----------------------------------------------------------
$Repo = $env:REFLEX_GITHUB_REPO
if (-not $Repo) {
	$origin = (& git -C $Root remote get-url origin 2>$null)
	if ($origin -and ($origin -match '[/:]([^/]+/[^/]+?)(\.git)?$')) { $Repo = $Matches[1] }
}
if (-not $Repo) { Write-Error 'Cannot determine GitHub repo. Set REFLEX_GITHUB_REPO=owner/name.'; exit 1 }

$Token = $env:REFLEX_GITHUB_TOKEN

# --- skip if already current ------------------------------------------------
if ((Test-Path $InstalledVersionFile) -and ($Version -ne 'latest')) {
	$installed = (Get-Content $InstalledVersionFile -Raw).Trim()
	if (($installed -eq $Version) -and (Test-Path (Join-Path $Root "bin\tools\$Platform\reflex.exe"))) {
		Write-Host "Reflex SDK $Version binaries already installed."
		exit 0
	}
}

$Tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("reflex-sdk-" + [System.Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $Tmp | Out-Null
try {
	function Get-Asset($name, $dest) {
		$out = Join-Path $Tmp $name
		if ($Token) {
			# Private repo: resolve the asset id, then fetch via the asset API endpoint.
			$api = if ($Version -eq 'latest') { "https://api.github.com/repos/$Repo/releases/latest" } `
			       else { "https://api.github.com/repos/$Repo/releases/tags/$Version" }
			$relHeaders = @{ Authorization = "Bearer $Token"; Accept = 'application/vnd.github+json'; 'User-Agent' = 'reflex-installer' }
			$rel = Invoke-RestMethod -Uri $api -Headers $relHeaders
			$asset = $rel.assets | Where-Object { $_.name -eq $name } | Select-Object -First 1
			if (-not $asset) { Write-Error "Release asset not found: $name"; exit 1 }
			$dlHeaders = @{ Authorization = "Bearer $Token"; Accept = 'application/octet-stream'; 'User-Agent' = 'reflex-installer' }
			Invoke-WebRequest -Uri $asset.url -Headers $dlHeaders -OutFile $out
		} else {
			# Public repo: anonymous download via the stable release URL.
			$url = if ($Version -eq 'latest') { "https://github.com/$Repo/releases/latest/download/$name" } `
			       else { "https://github.com/$Repo/releases/download/$Version/$name" }
			Invoke-WebRequest -Uri $url -OutFile $out
		}
		New-Item -ItemType Directory -Force -Path $dest | Out-Null
		Expand-Archive -Path $out -DestinationPath $dest -Force
	}

	Write-Host "Installing Reflex SDK $Version ($Platform) from $Repo ..."
	Get-Asset "reflex-libs-$Platform.zip"  (Join-Path $Root 'bin\lib')
	Get-Asset "reflex-tools-$Platform.zip" (Join-Path $Root 'bin\tools')
	Get-Asset "reflex-docs-$Platform.zip"  (Join-Path $Root 'bin\tools')

	# Record the installed version (resolve "latest" from the stamp baked into libs).
	$resolved = $Version
	$libVer = Join-Path $Root "bin\lib\$Platform\version.txt"
	if (($resolved -eq 'latest') -and (Test-Path $libVer)) { $resolved = (Get-Content $libVer -Raw).Trim() }
	New-Item -ItemType Directory -Force -Path (Join-Path $Root 'bin') | Out-Null
	Set-Content -Path $InstalledVersionFile -Value $resolved
	Write-Host "Reflex SDK $resolved binaries installed."
}
finally {
	Remove-Item -Recurse -Force $Tmp -ErrorAction SilentlyContinue
}
