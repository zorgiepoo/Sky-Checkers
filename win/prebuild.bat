if not exist "%~1\..\Data\Shaders" mkdir "%~1\..\Data\Shaders"

echo F | xcopy /y "%~1\..\gamecontrollerdb.txt" "%~1\..\Data\gamecontrollerdb.txt"
