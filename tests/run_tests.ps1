$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = Split-Path -Parent $ScriptDir
Set-Location $ProjectDir

function Invoke-Check {
    param(
        [string]$Name,
        [string]$InputText,
        [string[]]$ExpectedTexts
    )

    $OutputLines = $InputText | .\project_scheduler.exe
    $Output = $OutputLines -join "`n"
    $IsPassed = $true

    foreach ($ExpectedText in $ExpectedTexts) {
        if ($Output -notmatch [regex]::Escape($ExpectedText)) {
            $IsPassed = $false
            Write-Host "[FAIL] $Name"
            Write-Host "Missing expected text: $ExpectedText"
            Write-Host "----- Program output -----"
            Write-Host $Output
            Write-Host "--------------------------"
            break
        }
    }

    if ($IsPassed) {
        Write-Host "[PASS] $Name"
    }

    return $IsPassed
}

$PassedCount = 0
$FailedCount = 0

Write-Host "Building project..."
mingw32-make clean | Out-Host
mingw32-make | Out-Host

if (Test-Path "tests\exported_by_test.PPM") {
    Remove-Item "tests\exported_by_test.PPM"
}

$Cases = @(
    @{
        Name = "Sample project validation, CPM and statistics"
        InputText = @"
2
samples\simple.PPM
15
16
17
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration: 22",
            "Critical path: 0 1 2 3 4",
            "Total cost: 9030.00"
        )
    },
    @{
        Name = "Task relation listing"
        InputText = @"
2
samples\simple.PPM
8
2
0
"@
        ExpectedTexts = @(
            "Task 2 (Coding) - 7 days",
            "Predecessors:",
            "Index: 1, Name: Design",
            "Successors:",
            "Index: 3, Name: Testing",
            "Index: 5, Name: Acceptance"
        )
    },
    @{
        Name = "Export PPM file"
        InputText = @"
2
samples\simple.PPM
3
tests\exported_by_test.PPM
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project exported."
        )
    },
    @{
        Name = "Isolated task validation"
        InputText = @"
2
tests\isolated.PPM
15
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is invalid:",
            "Task 2 is isolated."
        )
    },
    @{
        Name = "Cycle validation"
        InputText = @"
2
tests\cycle.PPM
15
16
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is invalid:",
            "Dependency graph has a cycle.",
            "Cannot schedule an invalid project."
        )
    },
    @{
        Name = "Self dependency import error"
        InputText = @"
2
tests\self_dependency.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "Self-dependency is not allowed."
        )
    },
    @{
        Name = "Duplicate task ID import error"
        InputText = @"
2
tests\duplicate_task_id.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "duplicate"
        )
    },
    @{
        Name = "Duplicate resource ID import error"
        InputText = @"
2
tests\duplicate_resource_id.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "duplicate"
        )
    },
    @{
        Name = "Negative duration import error"
        InputText = @"
2
tests\negative_duration.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "positive"
        )
    },
    @{
        Name = "Milestone resource allocation error"
        InputText = @"
2
tests\milestone_resource_allocation.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "cannot"
        )
    },
    @{
        Name = "Nonexistent dependency reference error"
        InputText = @"
2
tests\nonexistent_dependency_reference.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "missing"
        )
    },
    @{
        Name = "SS dependency scheduling"
        InputText = @"
2
tests\ss_dependency.PPM
15
16
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration:"
        )
    },
    @{
        Name = "FF dependency scheduling"
        InputText = @"
2
tests\ff_dependency.PPM
15
16
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration:"
        )
    },
    @{
        Name = "SF dependency scheduling"
        InputText = @"
2
tests\sf_dependency.PPM
15
16
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration:"
        )
    },
    @{
        Name = "Missing P line import error"
        InputText = @"
2
tests\missing_p_line.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "P"
        )
    },
    @{
        Name = "Zero resource quantity error"
        InputText = @"
2
tests\zero_resource_quantity.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "positive"
        )
    },
    @{
        Name = "Milestone nonzero duration error"
        InputText = @"
2
tests\milestone_nonzero_duration.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "Milestone"
        )
    },
    @{
        Name = "Negative resource cost error"
        InputText = @"
2
tests\negative_resource_cost.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error at line",
            "negative"
        )
    },
    @{
        Name = "Dependency removal by task pair import"
        InputText = @"
2
tests\dep_removal_by_pair.PPM
0
"@
        ExpectedTexts = @(
            "Project imported."
        )
    },
    @{
        Name = "Export failure with PPM file"
        InputText = @"
2
tests\export_failure.PPM
0
"@
        ExpectedTexts = @(
            "Project imported."
        )
    },
    @{
        Name = "Task type conversion PPM"
        InputText = @"
2
tests\task_type_conversion.PPM
0
"@
        ExpectedTexts = @(
            "Project imported."
        )
    },
    @{
        Name = "PPM blocks in wrong order error"
        InputText = @"
2
tests\ppm_wrong_order.PPM
0
"@
        ExpectedTexts = @(
            "PPM parse error"
        )
    },
    @{
        Name = "Multiple start and end nodes import"
        InputText = @"
2
tests\multi_start_end.PPM
0
"@
        ExpectedTexts = @(
            "Project imported."
        )
    },
    @{
        Name = "Negative lag constraint import"
        InputText = @"
2
tests\negative_lag_constraint.PPM
0
"@
        ExpectedTexts = @(
            "Project imported."
        )
    }
)

foreach ($Case in $Cases) {
    $Passed = Invoke-Check `
        -Name $Case.Name `
        -InputText $Case.InputText `
        -ExpectedTexts $Case.ExpectedTexts

    if ($Passed) {
        $PassedCount++
    }
    else {
        $FailedCount++
    }
}

if (Test-Path "tests\exported_by_test.PPM") {
    $ExportedContent = Get-Content "tests\exported_by_test.PPM" -Raw
    if (($ExportedContent -match "P ProjectDemo") `
            -and ($ExportedContent -match "T 0 Requirement 5") `
            -and ($ExportedContent -match "D 0 1 FS 0")) {
        Write-Host "[PASS] Exported file content"
        $PassedCount++
    }
    else {
        Write-Host "[FAIL] Exported file content"
        $FailedCount++
    }
}
else {
    Write-Host "[FAIL] Exported file content"
    Write-Host "Missing tests\exported_by_test.PPM"
    $FailedCount++
}

if (Test-Path "tests\exported_by_test.PPM") {
    Remove-Item "tests\exported_by_test.PPM"
}

mingw32-make clean | Out-Host

Write-Host "Tests passed: $PassedCount"
Write-Host "Tests failed: $FailedCount"

if ($FailedCount -ne 0) {
    exit 1
}
