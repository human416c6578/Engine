@echo off
set "sourceDir=%~dp0Engine\shaders"

if not exist "%sourceDir%" (
    echo Source directory "%sourceDir%" does not exist. Aborting.
    exit /b 1
)

cd /d "%sourceDir%"

for %%f in (*.vert *.frag *.geom *.comp *.tesc *.tese *.rgen *.rint *.rahit *.rchit *.rmiss *.rcall) do (
    echo Compiling shader: %%f
    glslc "%%f" -o "%%~nf.spv"
    if %ERRORLEVEL% neq 0 (
        echo Error compiling shader: %%f
    ) else (
        echo Successfully compiled: %%f to %%~nf.spv
    )
)

echo Compilation of all shaders complete.
exit /b 0
