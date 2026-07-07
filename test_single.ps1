$ErrorActionPreference = "Stop"

$InputText = @"
2
tests\duplicate_task_id.PPM
0
"@

$OutputLines = .\project_scheduler.exe <<< $InputText
Write-Host $OutputLines
