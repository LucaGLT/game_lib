param(
	[string]$Compiler = "clang++"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $root

Write-Host "=== gmAlea test runner ==="
Write-Host "Root: $root"
Write-Host "Compiler: $Compiler"
Write-Host ""

$tests = @(
	@{
		Name = "test_gmDeck_v2"
		Sources = @("gmAlea/GmDeck.cpp", "gmAlea/tests/test_gmDeck_v2.cpp")
		Exe = "test_gmDeck_v2.exe"
	},
	@{
		Name = "test_gmCompDeck"
		Sources = @("gmAlea/GmDeck.cpp", "gmAlea/GmCompDeck.cpp", "gmAlea/tests/test_gmCompDeck.cpp")
		Exe = "test_gmCompDeck.exe"
	},
	@{
		Name = "test_gmDice"
		Sources = @("gmAlea/GmDeck.cpp", "gmAlea/SimpleDeck.cpp", "gmAlea/GmDice.cpp", "gmAlea/tests/test_gmDice.cpp")
		Exe = "test_gmDice.exe"
	},
	@{
		Name = "test_stdDice"
		Sources = @("gmAlea/GmDeck.cpp", "gmAlea/SimpleDeck.cpp", "gmAlea/GmDice.cpp", "gmAlea/StdDice.cpp", "gmAlea/tests/test_stdDice.cpp")
		Exe = "test_stdDice.exe"
	}
)

$failed = @()

foreach ($test in $tests)
{
	Write-Host "--- Building $($test.Name) ---"
	& $Compiler "-std=c++17" "-I." @($test.Sources) "-o" $test.Exe
	if ($LASTEXITCODE -ne 0)
	{
		$failed += "$($test.Name) [build]"
		continue
	}

	Write-Host "--- Running  $($test.Name) ---"
	& ".\$($test.Exe)"
	if ($LASTEXITCODE -ne 0)
	{
		$failed += "$($test.Name) [run]"
	}

	Write-Host ""
}

if ($failed.Count -eq 0)
{
	Write-Host "=== All gmAlea tests passed ==="
	exit 0
}

Write-Host "=== Some gmAlea tests failed ==="
$failed | ForEach-Object { Write-Host " - $_" }
exit 1
