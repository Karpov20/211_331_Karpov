#Requires -Version 5.1
param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

function Convert-HexToBytes {
    param([string]$Hex)
    $bytes = New-Object byte[] ($Hex.Length / 2)
    for ($i = 0; $i -lt $Hex.Length; $i += 2) {
        $bytes[$i / 2] = [Convert]::ToByte($Hex.Substring($i, 2), 16)
    }
    return $bytes
}

$keyHex = "ab9f5f69737f3f02f1e2a6d17305eae239f2bba9d6a8ed5e322ad87d3654c9d8"
$ivHex = "1af38c2dc2b96ffdd86694092341bc04"
$key = Convert-HexToBytes $keyHex
$iv = Convert-HexToBytes $ivHex

if (-not (Test-Path $InputPath)) {
    throw "Файл '$InputPath' не найден."
}

$plain = [System.IO.File]::ReadAllText((Resolve-Path $InputPath))
$plainBytes = [System.Text.Encoding]::UTF8.GetBytes($plain)

$aes = [System.Security.Cryptography.Aes]::Create()
$aes.Mode = [System.Security.Cryptography.CipherMode]::CBC
$aes.Padding = [System.Security.Cryptography.PaddingMode]::PKCS7
$aes.KeySize = 256
$aes.BlockSize = 128
$aes.Key = $key
$aes.IV = $iv

$encryptor = $aes.CreateEncryptor()
$cipherBytes = $encryptor.TransformFinalBlock($plainBytes, 0, $plainBytes.Length)
$encryptor.Dispose()
$aes.Dispose()

$base64 = [Convert]::ToBase64String($cipherBytes)
$resolvedOutput = $OutputPath
if (-not [System.IO.Path]::IsPathRooted($resolvedOutput)) {
    $resolvedOutput = Join-Path (Get-Location) $resolvedOutput
}
$directory = [System.IO.Path]::GetDirectoryName($resolvedOutput)
if ($directory -and -not (Test-Path $directory)) {
    New-Item -ItemType Directory -Path $directory | Out-Null
}
[System.IO.File]::WriteAllText($resolvedOutput, $base64)
