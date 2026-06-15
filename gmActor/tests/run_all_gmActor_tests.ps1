# run_all_gmActor_tests.ps1
#
# Compila ed esegue tutti i test di gmActor.
# Eseguire dalla root di game_lib:
#   .\gmActor\tests\run_all_gmActor_tests.ps1
#
# Prerequisiti: clang++ in PATH, gmSave/json.hpp presente per il test di serializzazione.
# ---------------------------------------------------------------------------

$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot\..\..   # game_lib root

$sources_core = @(
    "gmActor/stats/Health.cpp",
    "gmActor/stats/StatBlock.cpp",
    "gmActor/modifiers/Modifier.cpp",
    "gmActor/statuses/StatusContainer.cpp",
    "gmActor/items/InventoryState.cpp",
    "gmActor/items/EquipmentState.cpp",
    "gmActor/actors/ActorStore.cpp",
    "gmActor/actors/ActorQueries.cpp"
)

$tests = @(
    @{
        name   = "actor_common"
        extra  = @()
        file   = "gmActor/tests/test_actor_common.cpp"
    },
    @{
        name   = "health"
        extra  = @()
        file   = "gmActor/tests/test_health.cpp"
    },
    @{
        name   = "status_container"
        extra  = @()
        file   = "gmActor/tests/test_status_container.cpp"
    },
    @{
        name   = "modifier_container"
        extra  = @("gmActor/modifiers/Modifier.cpp")
        file   = "gmActor/tests/test_modifier_container.cpp"
        # modifier test only needs Modifier.cpp — override sources_core
        override_sources = @("gmActor/modifiers/Modifier.cpp")
    },
    @{
        name   = "inventory_equipment"
        extra  = @()
        file   = "gmActor/tests/test_inventory_equipment.cpp"
        override_sources = @(
            "gmActor/items/InventoryState.cpp",
            "gmActor/items/EquipmentState.cpp"
        )
    },
    @{
        name   = "actor_store"
        extra  = @()
        file   = "gmActor/tests/test_actor_store.cpp"
    },
    @{
        name   = "serialization"
        extra  = @("gmActor/serialization/ActorJson.cpp")
        file   = "gmActor/tests/test_serialization.cpp"
    }
)

$pass_count = 0
$fail_count = 0

foreach ($t in $tests)
{
    $exe = "bin/exe/test_gmActor_$($t.name).exe"

    if ($t.ContainsKey("override_sources"))
    {
        $src_list = $t.override_sources + @($t.file)
    }
    else
    {
        $src_list = $sources_core + $t.extra + @($t.file)
    }

    Write-Host ""
    Write-Host "--- Building $($t.name) ---"
    $compile_out = clang++ -std=c++17 -I. @src_list -o $exe 2>&1
    if ($LASTEXITCODE -ne 0)
    {
        Write-Host "[BUILD FAIL] $($t.name)"
        Write-Host $compile_out
        $fail_count++
        continue
    }

    $run_out = & "./$exe" 2>&1
    Write-Host $run_out
    if ($LASTEXITCODE -ne 0)
    {
        $fail_count++
    }
    else
    {
        $pass_count++
    }
}

Write-Host ""
Write-Host "============================================="
Write-Host "  gmActor test suites: $pass_count passed, $fail_count failed"
Write-Host "============================================="

exit ($fail_count -gt 0 ? 1 : 0)
