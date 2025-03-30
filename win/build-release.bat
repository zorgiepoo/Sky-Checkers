"C:\Program Files\Microsoft Visual Studio\2022\Preview\MSBuild\Current\Bin\MSBuild" /t:Rebuild /p:Configuration=Release
if exist x64\SkyCheckers rmdir /s/q x64\SkyCheckers
mkdir x64\SkyCheckers
robocopy ..\Data x64\SkyCheckers\Data\ /e /NFL /NDL /NJH /NJS
robocopy x64\Release x64\SkyCheckers\ SkyCheckers.exe *.dll /NFL /NDL /NJH /NJS
