"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild" /t:Rebuild /p:Configuration=Release
if exist x64\SkyCheckers rmdir /s/q x64\SkyCheckers
mkdir x64\SkyCheckers
robocopy ..\Data x64\SkyCheckers\Data\ /e /NFL /NDL /NJH /NJS
robocopy x64\Release x64\SkyCheckers\ SkyCheckers.exe *.dll /NFL /NDL /NJH /NJS
