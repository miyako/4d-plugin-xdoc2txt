/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : xdoc2txt
 #	author : miyako
 #	2018/09/03
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

#include <mutex>

std::mutex mutexDllPath;
std::mutex mutexDllCall;

std::wstring dllPath;

void getDllPath(std::wstring &dllPath)
{
    wchar_t fDrive[_MAX_DRIVE],
    fDir[_MAX_DIR],
    fName[_MAX_FNAME],
    fExt[_MAX_EXT];
    
    wchar_t thisPath[_MAX_PATH] = { 0 };
    
    HMODULE hplugin = GetModuleHandleW(L"xdoc2txt.4DX");
    GetModuleFileNameW(hplugin, thisPath, _MAX_PATH);
    _wsplitpath_s(thisPath, fDrive, fDir, fName, fExt);
    std::wstring windowsPath = fDrive;
    windowsPath += fDir;
    
    if (windowsPath.at(windowsPath.size() - 1) != L'\\')
        windowsPath += L"\\";
    
    dllPath = windowsPath + L"xd2txlib.dll";
}

void OnStartup()
{
    std::lock_guard<std::mutex> lock(mutexDllPath);
    
    getDllPath(dllPath);
}

void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;

		CommandDispatcher(pProcNum, pResult, pParams); 
	}
	catch(...)
	{

	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
        case kInitPlugin :
        case kServerInitPlugin :
            OnStartup();
            break;
            
// --- xdoc2txt

		case 1 :
			xdoc2txt(pResult, pParams);
			break;

	}
}

// ----------------------------------- xdoc2txt -----------------------------------

bool wcs2mbs(C_TEXT &Param, std::string &mbs)
{
	bool success = false;

#ifdef _WIN32
	LPCWCH wc = (LPCWCH)Param.getUTF16StringPtr();
	int wcl = Param.getUTF16Length();

	int len = WideCharToMultiByte(CP_ACP, 0, wc, wcl, NULL, 0, NULL, NULL);
	if (len)
	{
		std::vector<char> buf(len + 1);
		LPSTR mb = (LPSTR)&buf[0];

		if (WideCharToMultiByte(CP_ACP, 0, wc, wcl, mb, len, NULL, NULL))
		{
			mbs = std::string(mb);

			success = true;
		}
	}

#endif

	return success;
}

typedef int(*pExtractTextEx)(BSTR lpFilePath, bool bProp, BSTR lpOptions, BSTR*lpFileText);

void extractTextEx(pExtractTextEx ExtractTextEx, C_TEXT &Param1_path, C_TEXT &Param2_options, bool bProp, C_TEXT &Value)
{
	BSTR fileText = ::SysAllocString(L"");

	int len;

	len = ExtractTextEx(
		(BSTR)Param1_path.getUTF16StringPtr(),
		bProp,
		(BSTR)Param2_options.getUTF16StringPtr(),
		&fileText);

	Value.setUTF16String((PA_Unichar *)fileText, len);

	::SysFreeString(fileText);
}

void xdoc2txt(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1_path;
	C_TEXT Param2_options;
	C_TEXT Param3_props;
	C_TEXT returnValue;

	Param1_path.fromParamAtIndex(pParams, 1);
	Param2_options.fromParamAtIndex(pParams, 2);

	std::lock_guard<std::mutex> lock(mutexDllCall);

	std::wstring dllPath;
	getDllPath(dllPath);

	HMODULE h = LoadLibraryEx(
		dllPath.c_str(),
		NULL,
		LOAD_WITH_ALTERED_SEARCH_PATH
	);

	if (h)
	{
		pExtractTextEx ExtractTextEx = (pExtractTextEx)GetProcAddress(h, "ExtractTextEx");

		extractTextEx(ExtractTextEx, Param1_path, Param2_options, false, returnValue);
		extractTextEx(ExtractTextEx, Param1_path, Param2_options, true , Param3_props);

		FreeLibrary(h);
	}

	Param3_props.toParamAtIndex(pParams, 3);
	returnValue.setReturn(pResult);
} 
