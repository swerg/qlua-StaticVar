rd /S /Q .tmp\.zip
md .tmp\.zip\x32
md .tmp\.zip\x64

copy .Build\Win32-Release\*.dll .tmp\.zip\x32
copy .Build\x64-Release\*.dll .tmp\.zip\x64

copy nul .tmp\.zip\ReadMe.txt
echo ���������� StaticVar ��� Lua. >> .tmp\.zip\ReadMe.txt
echo ��������� ������������ ������� ����� ����������� ��������� Lua. >> .tmp\.zip\ReadMe.txt
echo ��������� ��������: http://quik2dde.ru/viewtopic.php?id=61 >> .tmp\.zip\ReadMe.txt
echo. >> .tmp\.zip\ReadMe.txt
echo \x32       -- ��� QUIK 6.x, 7.x >> .tmp\.zip\ReadMe.txt
echo \x64       -- ��� QUIK 8.5 � ����� >> .tmp\.zip\ReadMe.txt

cd .tmp\.zip
"C:\Program Files\7-Zip\7z.exe" a -r -tZip ..\..\StaticVar.zip *.dll ReadMe.txt
cd ..\..

rd /S /Q .tmp\.zip
