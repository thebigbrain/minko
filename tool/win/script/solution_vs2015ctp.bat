@echo off

pushd ..\..\..
tool\win\bin\premake5.exe --no-test --no-tutorial vs2015ctp
popd
pause