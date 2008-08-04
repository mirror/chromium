# vim:set ts=8 sw=8 sts=8 noet:

BUILD :
	@nmake /nologo /f $(ProjectDir)\..\source\data\makedata.mak ICUMAKE=$(ProjectDir)\..\source\data CFG=$(ConfigurationName) all
	@xcopy /R /C /Y /d $(ProjectDir)\..\bin\icudt38.DLL $(ProjectDir)\..\..\..\$(ConfigurationName)\

REBUILD :
	@nmake /nologo /f $(ProjectDir)\..\source\data\makedata.mak ICUMAKE=$(ProjectDir)\..\source\data CFG=$(ConfigurationName) clean all
	@xcopy /R /C /Y /d $(ProjectDir)\..\bin\icudt38.DLL $(ProjectDir)\..\..\..\$(ConfigurationName)\

CLEAN :
	@nmake /nologo /f $(ProjectDir)\..\source\data\makedata.mak ICUMAKE=$(ProjectDir)\..\source\data CFG=$(ConfigurationName) clean
