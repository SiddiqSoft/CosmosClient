if( $Env:GITVERSION_NUGETVERSION -eq "" ){ $ver = &{gitversion /output json /showvariable FullSemVer} }
if( $Env:GITVERSION_NUGETVERSION -ne "" ){ $ver = $Env:GITVERSION_NUGETVERSION }
&{ type ..\Doxyfile ; echo "PROJECT_NUMBER=$ver" } | doxygen -