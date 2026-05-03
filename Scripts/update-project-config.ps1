# ========================================
# 项目配置更新脚本
# ========================================
# 此脚本会更新 .vscode/c_cpp_properties.json
# 添加 Drawing 和 imgui 文件夹到包含路径

param(
    [switch]$DryRun  # 仅显示将要做的更改，不实际修改
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  项目配置更新脚本" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$configFile = ".vscode\c_cpp_properties.json"

# 检查文件是否存在
if (-not (Test-Path $configFile)) {
    Write-Host "❌ 找不到配置文件: $configFile" -ForegroundColor Red
    Write-Host ""
    Write-Host "请确保您在项目根目录运行此脚本。" -ForegroundColor Yellow
    exit 1
}

# 读取配置文件
Write-Host "读取配置文件: $configFile" -ForegroundColor Yellow
$configContent = Get-Content $configFile -Raw | ConvertFrom-Json

# 检查是否已经包含 Drawing 路径
$config = $configContent.configurations[0]
$drawingPath = "`${workspaceFolder}/Drawing"
$imguiPath = "`${workspaceFolder}/imgui"
$imguiBackendsPath = "`${workspaceFolder}/imgui/backends"

$needsUpdate = $false
$changes = @()

# 检查 Drawing 路径
if ($config.includePath -notcontains $drawingPath) {
    $needsUpdate = $true
    $changes += "添加 Drawing 文件夹到包含路径"
}

# 检查 imgui 路径
if ($config.includePath -notcontains $imguiPath) {
    $needsUpdate = $true
    $changes += "添加 imgui 文件夹到包含路径"
}

if ($config.includePath -notcontains $imguiBackendsPath) {
    $needsUpdate = $true
    $changes += "添加 imgui/backends 文件夹到包含路径"
}

# 检查预处理器定义
$requiredDefines = @(
    "IMGUI_DEFINE_MATH_OPERATORS",
    "_CRT_SECURE_NO_WARNINGS"
)

foreach ($define in $requiredDefines) {
    if ($config.defines -notcontains $define) {
        $needsUpdate = $true
        $changes += "添加预处理器定义: $define"
    }
}

if (-not $needsUpdate) {
    Write-Host "✅ 配置文件已经是最新的，无需更新。" -ForegroundColor Green
    exit 0
}

# 显示将要做的更改
Write-Host ""
Write-Host "将要进行以下更改：" -ForegroundColor Yellow
Write-Host ""
foreach ($change in $changes) {
    Write-Host "  • $change" -ForegroundColor White
}
Write-Host ""

if ($DryRun) {
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  Dry Run 模式 - 不会实际修改文件" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "如果要实际修改，请运行：" -ForegroundColor Yellow
    Write-Host "  .\update-project-config.ps1" -ForegroundColor White
    exit 0
}

# 备份原文件
$backupFile = "$configFile.backup"
Write-Host "创建备份: $backupFile" -ForegroundColor Yellow
Copy-Item $configFile $backupFile -Force

# 更新配置
Write-Host "更新配置..." -ForegroundColor Yellow

# 添加包含路径
$pathsToAdd = @(
    "`${workspaceFolder}/Drawing",
    "`${workspaceFolder}/imgui",
    "`${workspaceFolder}/imgui/backends",
)

foreach ($path in $pathsToAdd) {
    if ($config.includePath -notcontains $path) {
        $config.includePath += $path
    }
}

# 添加预处理器定义
foreach ($define in $requiredDefines) {
    if ($config.defines -notcontains $define) {
        $config.defines += $define
    }
}

# 保存配置
$configContent.configurations[0] = $config
$configJson = $configContent | ConvertTo-Json -Depth 10
$configJson | Set-Content $configFile -Encoding UTF8

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  配置更新成功！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "已创建备份: $backupFile" -ForegroundColor Cyan
Write-Host ""
Write-Host "下一步操作：" -ForegroundColor Yellow
Write-Host "  1. 重启 VSCode 以应用新配置" -ForegroundColor White
Write-Host "  2. 测试 IntelliSense 是否正常工作" -ForegroundColor White
Write-Host "  3. 尝试编译项目" -ForegroundColor White
Write-Host ""
Write-Host "如果遇到问题，可以恢复备份：" -ForegroundColor Yellow
Write-Host "  Copy-Item $backupFile $configFile -Force" -ForegroundColor White
Write-Host ""
