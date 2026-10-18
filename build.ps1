param([string]$Toolchain = 'V:\dev\msys64\mingw64')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = $PSScriptRoot
$version = 'modular-m1'
$runId = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0, 6)
$run = Join-Path $root "build\$runId"
$dist = Join-Path $root "dist\$runId"
$package = Join-Path $dist 'Starfall-Squadron'
$compiler = Join-Path $Toolchain 'bin\g++.exe'
$objdump = Join-Path $Toolchain 'bin\objdump.exe'
$required = @('bin\g++.exe', 'bin\objdump.exe', 'include\raylib.h', 'lib\libraylib.dll.a',
    'bin\libraylib.dll', 'bin\glfw3.dll', 'bin\libwinpthread-1.dll')
foreach ($relative in $required) {
    if (!(Test-Path -LiteralPath (Join-Path $Toolchain $relative))) { throw "Missing toolchain file: $relative" }
}
New-Item -ItemType Directory -Path $run,$package -Force | Out-Null
$originalPath = $env:PATH
$systemPath = "$env:SystemRoot\System32;$env:SystemRoot"
function Invoke-Check(
    [string]$Executable, [string]$Argument, [string]$WorkingDirectory, [string]$Label,
    [ValidateRange(1, 2147483647)][int]$TimeoutMs = 60000
) {
    $stdout = Join-Path $run "$Label.stdout.log"
    $stderr = Join-Path $run "$Label.stderr.log"
    $status = Join-Path $run "$Label.process.log"
    # HF2: own the process handle from creation. Start-Process with redirected
    # streams can return a Process whose ExitCode is null on Windows PowerShell.
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo.FileName = $Executable
    $process.StartInfo.Arguments = $Argument
    $process.StartInfo.WorkingDirectory = $WorkingDirectory
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $outFile = $null
    $errFile = $null
    $started = $false
    $exitCode = $null
    $failure = $null
    try {
        $outFile = [IO.File]::Open($stdout, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::Read)
        $errFile = [IO.File]::Open($stderr, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::Read)
        if (!$process.Start()) { throw 'Process.Start returned false' }
        $started = $true
        # Drain BOTH pipes while the child runs; preserve raw bytes in the logs.
        # Reading one pipe synchronously can deadlock a noisy child process.
        $outTask = $process.StandardOutput.BaseStream.CopyToAsync($outFile)
        $errTask = $process.StandardError.BaseStream.CopyToAsync($errFile)
        $timedOut = !$process.WaitForExit($TimeoutMs)
        if ($timedOut) {
            try { if (!$process.HasExited) { $process.Kill() } }
            catch { if (!$process.HasExited) { throw } }
            if (!$process.WaitForExit(5000)) { throw "Timeout after $TimeoutMs ms; child did not stop" }
        }
        # Call the getter explicitly so an invalid handle raises an exception
        # instead of PowerShell silently treating a property-read failure as null.
        $exitCode = $process.get_ExitCode()
        if ($null -eq $exitCode) { throw 'Exit code unavailable; refusing to report PASS' }
        $copies = [System.Threading.Tasks.Task[]]@($outTask, $errTask)
        if (![System.Threading.Tasks.Task]::WaitAll($copies, 5000)) {
            throw 'Output capture did not finish after process exit'
        }
        $outFile.Flush()
        $errFile.Flush()
        if ($timedOut) { throw "Timeout after $TimeoutMs ms" }
        if ($exitCode -ne 0) {
            throw ("Process returned exit {0} (0x{1})" -f $exitCode, ([int]$exitCode).ToString('X8'))
        }
    } catch {
        $failure = $_.Exception.Message
    } finally {
        if ($started) {
            try {
                if (!$process.HasExited) { $process.Kill(); [void]$process.WaitForExit(5000) }
            } catch {
                $failure = "$failure; child cleanup failed: $($_.Exception.Message)"
            }
        }
        # Dispose closes the redirected pipes as well as the owned process handle.
        $process.Dispose()
        if ($null -ne $outFile) { $outFile.Dispose() }
        if ($null -ne $errFile) { $errFile.Dispose() }
    }
    $exitText = 'UNAVAILABLE'
    if ($null -ne $exitCode) {
        $exitText = "{0} (0x{1})" -f $exitCode, ([int]$exitCode).ToString('X8')
    }
    @("executable=$Executable", "arguments=$Argument", "workingDirectory=$WorkingDirectory",
        "exitCode=$exitText", "failure=$failure") | Set-Content -LiteralPath $status -Encoding UTF8
    if ($null -ne $failure) {
        Write-Host "FAIL: $Label; $failure"
        foreach ($log in @($stdout, $stderr)) {
            Write-Host "--- $log (last 60 lines) ---"
            if (Test-Path -LiteralPath $log) { Get-Content -LiteralPath $log -Encoding UTF8 -Tail 60 | ForEach-Object { Write-Host $_ } }
        }
        throw "$Label failed: $failure; logs in $run"
    }
    Write-Host "PASS: $Label (exit $exitCode)"
    Get-Content -LiteralPath $stdout -Encoding UTF8 | Select-String 'self-test:|render-test:|checks:|VICTORY|validation:' | ForEach-Object { Write-Host $_.Line }
}
try {
    # Do not let Qt/other SDK DLLs on the ambient PATH contaminate this compiler.
    $env:PATH = "$(Join-Path $Toolchain 'bin');$systemPath"
    $exe = Join-Path $package 'starfall.exe'
    $common = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', '-D_GLIBCXX_ASSERTIONS',
        '-static-libgcc', '-static-libstdc++')
    $core = @('src/content/json.cpp', 'src/content/catalog.cpp', 'src/gameplay/campaign.cpp',
        'src/gameplay/game.cpp', 'src/gameplay/simulate.cpp', 'src/runtime/options.cpp', 'src/runtime/paths.cpp') |
        ForEach-Object { Join-Path $root $_ }
    $regression = Join-Path $root 'tests/regression.cpp'
    & $compiler @common '-mwindows' "-I$(Join-Path $Toolchain 'include')" `
        (Join-Path $root 'main.cpp') (Join-Path $root 'src/runtime/renderer.cpp') @core $regression `
        '-o' $exe (Join-Path $Toolchain 'lib/libraylib.dll.a') '-lopengl32' '-lgdi32' '-lwinmm' '-lshell32' '-luser32'
    if ($LASTEXITCODE -ne 0) { throw 'Game compilation failed' }
    $contentTool = Join-Path $package 'starfall-content.exe'
    & $compiler @common (Join-Path $root 'src/content/tool_main.cpp') @core $regression `
        '-o' $contentTool '-lshell32' '-luser32'
    if ($LASTEXITCODE -ne 0) { throw 'Content tool compilation failed' }
    $testExe = Join-Path $package 'starfall-tests.exe'
    & $compiler @common (Join-Path $root 'tests/test_main.cpp') @core $regression `
        '-o' $testExe '-lshell32' '-luser32'
    if ($LASTEXITCODE -ne 0) { throw 'Tests compilation failed' }
    foreach ($dll in @('libraylib.dll','glfw3.dll','libwinpthread-1.dll')) {
        Copy-Item -LiteralPath (Join-Path $Toolchain "bin\$dll") -Destination $package
    }
    # Package only known runtime dependencies; reject an unexpected SDK dependency.
    $systemDlls = @('kernel32.dll','msvcrt.dll','user32.dll','winmm.dll','gdi32.dll','shell32.dll',
        'advapi32.dll','ole32.dll','opengl32.dll','ws2_32.dll','ntdll.dll')
    $imports = @{}
    foreach ($binary in Get-ChildItem -LiteralPath $package -File) {
        $headers = & $objdump '-p' $binary.FullName
        if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $($binary.Name)" }
        $dependencies = @($headers | Select-String 'DLL Name:\s*(.+)' | ForEach-Object { $_.Matches[0].Groups[1].Value.Trim() })
        foreach ($dependency in $dependencies) {
            if ($dependency.ToLowerInvariant() -notin $systemDlls -and !(Test-Path -LiteralPath (Join-Path $package $dependency))) {
                throw "Unpackaged DLL: $dependency (required by $($binary.Name))"
            }
        }
        $imports[$binary.Name] = $dependencies
    }
    foreach ($folder in @('content','docs')) { Copy-Item -LiteralPath (Join-Path $root $folder) -Destination $package -Recurse }
    foreach ($launcher in @('PLAY-CLASSIC.cmd','PLAY-DEMO.cmd','TEST-SECTOR.cmd','VALIDATE-CONTENT.cmd','NEW-SECTOR.cmd','new-sector.ps1')) {
        Copy-Item -LiteralPath (Join-Path $root $launcher) -Destination $package
    }
    foreach ($document in @('README.md','ACCEPTANCE.md')) {
        Copy-Item -LiteralPath (Join-Path $root $document) -Destination $package
    }
    $sources = New-Item -ItemType Directory -Path (Join-Path $package 'sources')
    foreach ($source in @('main.cpp','pixel_art.h','pixel_font.h','Makefile','build.ps1','README.md','ACCEPTANCE.md','CMakeLists.txt','BUILD-MODULAR.cmd','PLAY-CLASSIC.cmd','PLAY-DEMO.cmd','TEST-SECTOR.cmd','VALIDATE-CONTENT.cmd','NEW-SECTOR.cmd','new-sector.ps1')) {
        Copy-Item -LiteralPath (Join-Path $root $source) -Destination $sources.FullName
    }
    foreach ($folder in @('src','content','tests','docs')) {
        Copy-Item -LiteralPath (Join-Path $root $folder) -Destination $sources.FullName -Recurse
    }
    $licenses = New-Item -ItemType Directory -Path (Join-Path $package 'licenses')
    foreach ($name in @('raylib','glfw','libwinpthread','gcc-libs')) {
        Copy-Item -LiteralPath (Join-Path $Toolchain "share\licenses\$name") -Destination $licenses.FullName -Recurse
    }
    $compilerVersion = (& $compiler '--version' | Select-Object -First 1)
    # Launch a real copy outside the repo, with spaces and non-ASCII in its path,
    # without MinGW, Qt or any developer runtime on PATH. No user data is read.
    $isolated = Join-Path ([IO.Path]::GetTempPath()) ("Starfall test żółć " + [Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $isolated | Out-Null
    Copy-Item -LiteralPath $package -Destination $isolated -Recurse
    $copiedExe = Join-Path $isolated 'Starfall-Squadron\starfall.exe'
    $env:PATH = $systemPath
    Invoke-Check $copiedExe '--validate-content' $isolated 'portable-content-validation'
    Invoke-Check $copiedExe '--simulate-campaign --campaign example:two_sectors' $isolated 'portable-two-sectors'
    Invoke-Check (Join-Path $isolated 'Starfall-Squadron\starfall-tests.exe') ('"' + (Join-Path $isolated 'Starfall-Squadron\content') + '"') $isolated 'portable-modular-tests'
    Invoke-Check $copiedExe '--self-test' $isolated 'portable-self-test'
    Invoke-Check $copiedExe '--render-test' $isolated 'portable-render-test'
    # Prove JSON-only expansion on the isolated COPY, never the distributed content.
    $beforeExpansion = (Get-FileHash -LiteralPath $copiedExe).Hash
    $creator = Join-Path $isolated 'Starfall-Squadron\new-sector.ps1'
    $powershell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    Invoke-Check $powershell ('-NoProfile -ExecutionPolicy Bypass -File "' + $creator + '" -Name proof_sector') $isolated 'portable-sector-creator'
    Invoke-Check $copiedExe '--simulate-campaign --campaign example:two_sectors' $isolated 'portable-json-only-expansion'
    if (!(Select-String -LiteralPath (Join-Path $run 'portable-json-only-expansion.stdout.log') -Pattern 'VICTORY sectors=3' -Quiet)) {
        throw 'JSON-only expansion did not reach the third sector'
    }
    if ($beforeExpansion -ne (Get-FileHash -LiteralPath $copiedExe).Hash) { throw 'EXE changed during content-only expansion' }
    $verification = New-Item -ItemType Directory -Path (Join-Path $run 'screenshots')
    foreach ($name in @('starfall-preview.png','paused.png','game-over.png','victory.png','boss.png','sprite-sheet.png')) {
        Copy-Item -LiteralPath (Join-Path $isolated $name) -Destination $verification.FullName
    }
    $info = [ordered]@{
        version = $version; status = 'Candidate for user acceptance, not an accepted final release';
        builtAt = (Get-Date).ToString('o'); compiler = $compilerVersion;
        sourceSha256 = (Get-FileHash -LiteralPath (Join-Path $root 'main.cpp')).Hash;
        coreSourceHashes = @(Get-ChildItem -LiteralPath (Join-Path $root 'src') -Recurse -File | ForEach-Object {
            [ordered]@{ path = $_.FullName.Substring($root.Length + 1); sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash }
        });
        pixelArtSha256 = (Get-FileHash -LiteralPath (Join-Path $root 'pixel_art.h')).Hash;
        pixelFontSha256 = (Get-FileHash -LiteralPath (Join-Path $root 'pixel_font.h')).Hash;
        executableSha256 = (Get-FileHash -LiteralPath $exe).Hash;
        checks = @('Base simulation and sprite/font regression', 'Content and campaign checks', 'Two-sector real-code simulation', 'Sector creator and JSON-only expansion with unchanged EXE', '12 real OpenGL render checks', 'portable isolated PATH launch', 'PE import audit');
        contentFiles = @(Get-ChildItem -LiteralPath (Join-Path $package 'content') -Recurse -File | ForEach-Object {
            [ordered]@{ path = $_.FullName.Substring($package.Length + 1); sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash }
        });
        imports = $imports
    }
    $info | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $package 'BUILD-INFO.json') -Encoding UTF8
    $zip = Join-Path $dist "Starfall-Squadron-$version-windows-x64.zip"
    Compress-Archive -LiteralPath $package -DestinationPath $zip
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [IO.Compression.ZipFile]::OpenRead($zip)
    try {
        foreach ($entry in $archive.Entries) {
            if (!$entry.Name) { continue }
            $expected = Join-Path $dist ($entry.FullName.Replace('/', '\'))
            if (!(Test-Path -LiteralPath $expected -PathType Leaf)) { throw "Unexpected ZIP member: $($entry.FullName)" }
            $stream = $entry.Open()
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $actualHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
            finally { $stream.Dispose(); $sha.Dispose() }
            if ($actualHash -ne (Get-FileHash -LiteralPath $expected).Hash) { throw "ZIP integrity failure: $($entry.FullName)" }
        }
    } finally { $archive.Dispose() }
    Write-Host 'PASS: ZIP entries match packaged file hashes'
    $hash = (Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($zip))" | Set-Content -LiteralPath "$zip.sha256" -Encoding ASCII
    $package | Set-Content -LiteralPath (Join-Path $root 'LATEST-PACKAGE.txt') -Encoding UTF8
    Write-Host "PACKAGE=$package"
    Write-Host "ZIP=$zip"
    Write-Host "VERIFICATION=$run"
    Write-Host "ISOLATED_TEST=$isolated"
} finally {
    $env:PATH = $originalPath
}
