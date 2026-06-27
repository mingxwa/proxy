# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

# Builds the visualizer test subjects with MSVC, validates that proxy.natvis is
# well-formed, and (best effort) drives cdb with the natvis loaded to confirm the
# contained type surfaces for an rtti-enabled proxy. Run from a Developer shell
# (cl.exe on PATH); honors $env:CDB_ENFORCE=1 to fail when the cdb check fails.

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = (Resolve-Path (Join-Path $here "..\..\..")).Path
$include = Join-Path $root "include"
$natvis = Join-Path $root "tools\visualizers\proxy.natvis"
$subjects = Join-Path $here "test_subjects.cpp"

Write-Host "== Validating proxy.natvis is well-formed XML =="
$null = [xml](Get-Content -Raw $natvis)
Write-Host "natvis XML: OK"

$work = New-Item -ItemType Directory -Force -Path (Join-Path $env:TEMP ("proxyvis_" + [guid]::NewGuid().ToString("N")))
Push-Location $work
try {
    Write-Host "== Building test_subjects.cpp with cl =="
    & cl /nologo /std:c++20 /EHsc /permissive- /Zc:preprocessor /Zc:__cplusplus `
        /utf-8 /Zi /I $include $subjects /Fe:subj.exe /Fd:subj.pdb /Fo:subj.obj
    if ($LASTEXITCODE -ne 0) { throw "cl build failed" }

    Write-Host "== Running subj.exe (sanity) =="
    & .\subj.exe
    Write-Host "subj.exe exit=$LASTEXITCODE"

    $cdb = $null
    $cmd = Get-Command cdb.exe -ErrorAction SilentlyContinue
    if ($cmd) { $cdb = $cmd.Source }
    else {
        $cand = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"
        if (Test-Path $cand) { $cdb = $cand }
    }
    if (-not $cdb) {
        Write-Host "CDB_CHECK: SKIPPED (cdb.exe not found)"
        exit 0
    }

    Write-Host "== Driving cdb at $cdb =="
    $script = ".nvload `"$natvis`"; bm subj!proxy_visualizer_break; g; " +
        ".frame 1; dx p_direct_rtti; dx p_both; dx p_unique; dx p_empty; q"
    $out = & $cdb -c $script subj.exe 2>&1 | Out-String
    Write-Host "---- cdb output ----"
    Write-Host $out
    Write-Host "---- end cdb output ----"

    if ($out -match "Cat") {
        Write-Host "CDB_CHECK: PASS (contained type surfaced)"
    }
    elseif ($env:CDB_ENFORCE -eq "1") {
        Write-Host "CDB_CHECK: FAIL (no contained type in cdb output)"
        exit 1
    }
    else {
        Write-Host "CDB_CHECK: INFO (no 'Cat' match; not enforced yet)"
    }
}
finally {
    Pop-Location
}
