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
if (Test-Path "tests\missing_dir") {
    Remove-Item "tests\missing_dir" -Recurse -Force
}

$Cases = @(
    @{
        Name = "Sample project validation, CPM and statistics"
        InputText = @"
2
samples\simple.PPM
16
17
18
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
16
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
16
17
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is invalid:",
            "Dependency graph has a cycle.",
            "Task 0 is in or blocked by a cycle.",
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
        Name = "Basic task zero duration import error"
        InputText = @"
2
tests\basic_task_zero_duration.PPM
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
        Name = "Add dependency out-of-range task index"
        InputText = @"
2
samples\simple.PPM
9
99
0
FS
0
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Index out of range.",
            "Detail: Task index is out of range."
        )
    },
    @{
        Name = "Assign resource out-of-range resource index"
        InputText = @"
2
samples\simple.PPM
15
0
99
1
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Index out of range.",
            "Detail: Resource index is out of range."
        )
    },
    @{
        Name = "SS dependency scheduling"
        InputText = @"
2
tests\ss_dependency.PPM
16
17
5
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration: 5",
            "Critical path: 0 1",
            "[0] Task1 Duration=5 Cost=250.00 ES=0 EF=5 LS=0 LF=5 Slack=0",
            "[1] Task2 Duration=3 Cost=150.00 ES=2 EF=5 LS=2 LF=5 Slack=0"
        )
    },
    @{
        Name = "FF dependency scheduling"
        InputText = @"
2
tests\ff_dependency.PPM
16
17
5
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration: 4",
            "Critical path: 0",
            "[0] Task1 Duration=4 Cost=0.00 ES=0 EF=4 LS=0 LF=4 Slack=0",
            "[1] Task2 Duration=3 Cost=0.00 ES=0 EF=3 LS=1 LF=4 Slack=1"
        )
    },
    @{
        Name = "SF dependency scheduling"
        InputText = @"
2
tests\sf_dependency.PPM
16
17
5
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration: 3",
            "Critical path: 0",
            "[0] Task1 Duration=3 Cost=0.00 ES=0 EF=3 LS=0 LF=3 Slack=0",
            "[1] Task2 Duration=5 Cost=0.00 ES=-4 EF=1 LS=-2 LF=3 Slack=2"
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
        Name = "Milestone non-integer duration token error"
        InputText = @"
2
tests\milestone_bad_duration_token.PPM
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
        Name = "Export failure source project import"
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
        Name = "Export failure reports detail"
        InputText = @"
2
tests\export_failure.PPM
3
tests\missing_dir\cannot_write.PPM
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "File cannot be opened or its type is not supported.",
            "Detail:",
            "open fail"
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
16
17
5
0
"@
        ExpectedTexts = @(
            "Project imported.",
            "Project is valid.",
            "Project duration: 9",
            "Critical path: 0 1 2",
            "[0] Task1 Duration=5 Cost=0.00 ES=0 EF=5 LS=0 LF=5 Slack=0",
            "[1] Task2 Duration=3 Cost=0.00 ES=3 EF=6 LS=3 LF=6 Slack=0",
            "[2] Task3 Duration=4 Cost=0.00 ES=5 EF=9 LS=5 LF=9 Slack=0"
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
