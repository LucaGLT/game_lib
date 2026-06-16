# run_all_gmRules_tests.ps1
#
# Compila ed esegue tutte le suite test di gmRules.
# Eseguire dalla root di game_lib:
#   .\gmRules\tests\run_all_gmRules_tests.ps1

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot\..\..

$common = @(
    "gmRules/core/RuleResult.cpp",
    "gmRules/target/TargetResult.cpp",
    "gmRules/target/TargetResolver.cpp",
    "gmRules/condition/ConditionEvaluator.cpp",
    "gmRules/effect/EffectResult.cpp",
    "gmRules/effect/EffectResolver.cpp",
    "gmRules/status/StatusEngine.cpp",
    "gmRules/facade/gmRulesEngine.cpp"
)

$tests = @(
    "gmRules/tests/test_target_resolver.cpp",
    "gmRules/tests/test_condition_evaluator.cpp",
    "gmRules/tests/test_effect_resolver.cpp",
    "gmRules/tests/test_status_engine.cpp",
    "gmRules/tests/test_rules_integration.cpp"
)

$pass = 0
$fail = 0

foreach ($t in $tests)
{
    $name = [System.IO.Path]::GetFileNameWithoutExtension($t)
    $exe  = "bin/exe/$name.exe"

    Write-Host ""
    Write-Host "--- Building $name ---"

    $build = clang++ -std=c++17 -I. @common $t -o $exe 2>&1
    if ($LASTEXITCODE -ne 0)
    {
        Write-Host "[BUILD FAIL] $name"
        Write-Host $build
        $fail++
        continue
    }

    $run = & "./$exe" 2>&1
    Write-Host $run
    if ($LASTEXITCODE -eq 0) { $pass++ } else { $fail++ }
}

Write-Host ""
Write-Host "============================================="
Write-Host " gmRules test suites: $pass passed, $fail failed"
Write-Host "============================================="

if ($fail -gt 0)
{
    exit 1
}
else
{
    exit 0
}
