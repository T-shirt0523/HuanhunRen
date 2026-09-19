# 将 icon.png 转换为 icon.ico（供 build.bat 嵌入 exe 资源）
# 用法: powershell -ExecutionPolicy Bypass -File make_ico.ps1
$ErrorActionPreference = 'Stop'
if (-not (Test-Path "icon.png")) { Write-Host "[ERROR] icon.png not found"; exit 1 }
Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Image]::FromFile((Resolve-Path "icon.png"))
$sizes = @(16, 32, 48)
$imgs = New-Object System.Collections.Generic.List[System.Drawing.Bitmap]
foreach ($s in $sizes) { $imgs.Add((New-Object System.Drawing.Bitmap($img, $s, $s))) }
# 组合多尺寸 ICO（PNG 压缩格式存储，Vista+ 支持）
$fs = [IO.File]::Create("icon.ico")
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([UInt16]0); $bw.Write([UInt16]1); $bw.Write([UInt16]$imgs.Count)
$offset = 6 + 16 * $imgs.Count
$dataList = New-Object System.Collections.Generic.List[byte[]]
foreach ($bmp in $imgs) {
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $data = $ms.ToArray(); $ms.Close()
    $dataList.Add($data)
    $bw.Write([Byte]($bmp.Width)); $bw.Write([Byte]($bmp.Height))
    $bw.Write([Byte]0); $bw.Write([Byte]0)
    $bw.Write([UInt16]1); $bw.Write([UInt16]32)
    $bw.Write([UInt32]$data.Length); $bw.Write([UInt32]$offset)
    $offset += $data.Length
}
foreach ($d in $dataList) { $bw.Write($d) }
$bw.Close(); $fs.Close()
$img.Dispose()
foreach ($bmp in $imgs) { $bmp.Dispose() }
Write-Host "OK: icon.ico created (16/32/48)"
