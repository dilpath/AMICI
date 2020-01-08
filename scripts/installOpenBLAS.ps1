Write-Host 'script installOpenBLAS.ps1 started'
New-Item -Path 'C:\BLAS' -ItemType Directory -Force
# New-Item -Path 'C:\BLAS\OpenBLAS-0.3.6-x64' -ItemType Directory -Force
# Set-Location 'C:\BLAS'
# Copy-Item -Path "$BASE_DIR\ThirdParty\BLAS\OpenBLAS-0.3.6-x64.zip" -Destination "C:\BLAS" -Force
$uri = 'https://sourceforge.net/projects/openblas/files/v0.3.6/OpenBLAS-0.3.6-x64.zip/download'
$output = 'C:\BLAS\OpenBLAS-0.3.6-x64.zip'
# Invoke-WebRequest $uri -OutFile $output
$webclient = New-Object System.Net.WebClient
$webclient.DownloadFile($uri,"$output")
Expand-Archive -Path 'C:\BLAS\OpenBLAS-0.3.6-x64.zip' -DestinationPath 'C:\BLAS\OpenBLAS-0.3.6-x64' -Force
New-Item -Path Env:BLAS_LIBS -Value "/LIBPATH:C:\BLAS\OpenBLAS-0.3.6-x64\lib" -Force
New-Item -Path Env:BLAS_CFLAGS -Value "/IC:\BLAS\OpenBLAS-0.3.6-x64\include" -Force
Write-Host 'script installOpenBLAS.ps1 completed'