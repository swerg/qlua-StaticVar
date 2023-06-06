rd /S /Q .tmp\.zip
md .tmp\.zip\x32-Lua51
md .tmp\.zip\x64-Lua53
md .tmp\.zip\x64-Lua54

copy .Build\Win32-Release\*.dll .tmp\.zip\x32-Lua51
copy .Build\x64-Release\*.dll .tmp\.zip\x64-Lua53
copy .Build\x64-Release54\*.dll .tmp\.zip\x64-Lua54

copy nul .tmp\.zip\ReadMe.txt
echo Библиотека StaticVar для Lua. >> .tmp\.zip\ReadMe.txt
echo Позволяет обмениваться данными между несколькими скриптами Lua. >> .tmp\.zip\ReadMe.txt
echo Подробное описание: http://quik2dde.ru/viewtopic.php?id=61 >> .tmp\.zip\ReadMe.txt
echo. >> .tmp\.zip\ReadMe.txt
echo \x32-Lua51       -- для QUIK 6.x, 7.x >> .tmp\.zip\ReadMe.txt
echo \x64-Lua53       -- для QUIK 8.5 и далее при выборе Lua5.3 для выполнении скрипта >> .tmp\.zip\ReadMe.txt
echo \x64-Lua54       -- для QUIK 8.11 и далее при выборе Lua5.4 для выполнении скрипта >> .tmp\.zip\ReadMe.txt

cd .tmp\.zip
"C:\Program Files\7-Zip\7z.exe" a -r -tZip ..\..\StaticVar.zip *.dll ReadMe.txt
cd ..\..

rd /S /Q .tmp
