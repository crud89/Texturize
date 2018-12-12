# Script parameters.
Param(
    [String]$installTo = "install/",        # Relative path (CMAKE_INSTALL_PREFIX)
    [String]$buildTo = "",                  # Relative path to source directory
    [String]$OpenCV,
    [String]$OpenEXR,
    [String]$TBB,
    [String]$HDF5,
    [Boolean]$x64 = $true,
    [String]$config = "Release"
)

# Build up a command to invoke.
$invoke = "cmake CMakeLists.txt "

if (![string]::IsNullOrEmpty($buildTo)) {
    $invoke += "-B`"$($buildTo)`" "
}

$invoke += "-DCMAKE_BUILD_TYPE=`"$($config)`" "

if ($x64) {
    $invoke += "-G `"Visual Studio 15 2017 Win64`" "
} else {
    $invoke += "-G `"Visual Studio 15 2017`" "
}

if (![String]::IsNullOrEmpty($installTo)) {
    $invoke += "-DCMAKE_INSTALL_PREFIX:STRING=`"$($installTo)`" "
}

if (![String]::IsNullOrEmpty($OpenCV)) {
    $invoke += "-DOpenCV_DIR:STRING=`"$($OpenCV)`" -DOpenCV_FOUND:BOOL=ON "
}

if (![String]::IsNullOrEmpty($OpenEXR)) {
    $invoke += "-DOPENEXR_LOCATION:STRING=`"$($OpenEXR)`" -DOpenEXR_FOUND:BOOL=ON "
}

if (![String]::IsNullOrEmpty($TBB)) {
    $invoke += "-DTBB_DIR:STRING=`"$($TBB)`" -DTBB_FOUND:BOOL=ON "
}

if (![String]::IsNullOrEmpty($HDF5)) {
    $invoke += "-DHDF5_ROOT:STRING=`"$($HDF5)`" -DHDF5_FOUND:BOOL=ON "
}

Write-Host "Invoking $($invoke)..."
Invoke-Expression $invoke