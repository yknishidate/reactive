@echo off

for %%f in (*.rgen, *.rmiss, *.rchit) do (
  echo %%f
  %VULKAN_SDK%/Bin/glslc.exe %%f -o %%f.spv --target-env=vulkan1.2
)
pause
