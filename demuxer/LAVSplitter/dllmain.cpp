/*
 *      Copyright (C) 2010-2013 Hendrik Leppkes
 *      http://www.1f0.de
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// Based on the SampleParser Template by GDCL
// --------------------------------------------------------------------------------
// Copyright (c) GDCL 2004. All Rights Reserved. 
// You are free to re-use this as the basis for your own filter development,
// provided you retain this copyright notice in the source.
// http://www.gdcl.co.uk
// --------------------------------------------------------------------------------

#include "stdafx.h"

// Initialize the GUIDs
#include <InitGuid.h>

#include <qnetwork.h>
#include "LAVSplitter.h"
#include "moreuuids.h"

#include "registry.h"
#include "IGraphRebuildDelegate.h"

// The GUID we use to register the splitter media types
DEFINE_GUID(MEDIATYPE_LAVSplitter,
  0x9c53931c, 0x7d5a, 0x4a75, 0xb2, 0x6f, 0x4e, 0x51, 0x65, 0x4d, 0xb2, 0xc0);

// --- COM factory table and registration code --------------

const AMOVIESETUP_MEDIATYPE 
  sudMediaTypes[] = {
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_NULL },
};

const AMOVIESETUP_PIN sudOutputPins[] = 
{
  {
    L"Output",            // pin name
      FALSE,              // is rendered?    
      TRUE,               // is output?
      FALSE,              // zero instances allowed?
      TRUE,               // many instances allowed?
      &CLSID_NULL,        // connects to filter (for bridge pins)
      NULL,               // connects to pin (for bridge pins)
      0,                  // count of registered media types
      NULL                // list of registered media types
  },
  {
    L"Input",             // pin name
      FALSE,              // is rendered?    
      FALSE,              // is output?
      FALSE,              // zero instances allowed?
      FALSE,              // many instances allowed?
      &CLSID_NULL,        // connects to filter (for bridge pins)
      NULL,               // connects to pin (for bridge pins)
      1,                  // count of registered media types
      &sudMediaTypes[0]   // list of registered media types
  }
};

const AMOVIESETUP_FILTER sudFilterReg =
{
  &__uuidof(CLAVSplitter),        // filter clsid
  L"LAV Splitter",                // filter name
  MERIT_PREFERRED + 4,            // merit
  2,                              // count of registered pins
  sudOutputPins,                  // list of pins to register
  CLSID_LegacyAmFilterCategory
};

const AMOVIESETUP_FILTER sudFilterRegSource =
{
  &__uuidof(CLAVSplitterSource),  // filter clsid
  L"LAV Splitter Source",         // filter name
  MERIT_PREFERRED + 4,            // merit
  1,                              // count of registered pins
  sudOutputPins,                  // list of pins to register
  CLSID_LegacyAmFilterCategory
};

// --- COM factory table and registration code --------------

// DirectShow base class COM factory requires this table, 
// declaring all the COM objects in this DLL
CFactoryTemplate g_Templates[] = {
  // one entry for each CoCreate-able object
  {
    sudFilterReg.strName,
      sudFilterReg.clsID,
      CreateInstance<CLAVSplitter>,
      CLAVSplitter::StaticInit,
      &sudFilterReg
  },
  {
    sudFilterRegSource.strName,
      sudFilterRegSource.clsID,
      CreateInstance<CLAVSplitterSource>,
      NULL,
      &sudFilterRegSource
  },
  // This entry is for the property page.
  { 
      L"LAV Splitter Properties",
      &CLSID_LAVSplitterSettingsProp,
      CreateInstance<CLAVSplitterSettingsProp>,
      NULL, NULL
  },
  {
      L"LAV Splitter Input Formats",
      &CLSID_LAVSplitterFormatsProp,
      CreateInstance<CLAVSplitterFormatsProp>,
      NULL, NULL
  }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// self-registration entrypoint
STDAPI DllRegisterServer()
{
  std::list<LPCWSTR> chkbytes;

  // BluRay
  chkbytes.clear();
  chkbytes.push_back(L"0,4,,494E4458"); // INDX (index.bdmv)
  chkbytes.push_back(L"0,4,,4D4F424A"); // MOBJ (MovieObject.bdmv)
  chkbytes.push_back(L"0,4,,4D504C53"); // MPLS
  RegisterSourceFilter(__uuidof(CLAVSplitterSource),
    MEDIASUBTYPE_LAVBluRay, chkbytes, NULL);

  // base classes will handle registration using the factory template table
  return AMovieDllRegisterServer2(true);
}

STDAPI DllUnregisterServer()
{
  UnRegisterSourceFilter(MEDIASUBTYPE_LAVBluRay);

  // base classes will handle de-registration using the factory template table
  return AMovieDllRegisterServer2(false);
}

// if we declare the correct C runtime entrypoint and then forward it to the DShow base
// classes we will be sure that both the C/C++ runtimes and the base classes are initialized
// correctly
HMODULE g_currentModule = NULL;
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
  g_currentModule = (HMODULE)hDllHandle;
  return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpReserved);
}

STDAPI OpenConfiguration()
{
  HRESULT hr = S_OK;
  CUnknown *pInstance = CreateInstance<CLAVSplitter>(NULL, &hr);
  IBaseFilter *pFilter = NULL;
  pInstance->NonDelegatingQueryInterface(IID_IBaseFilter, (void **)&pFilter);
  if (pFilter) {
    pFilter->AddRef();
    CBaseDSPropPage::ShowPropPageDialog(pFilter);
  }
  delete pInstance;

  return 0;
}

#include <delayimp.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <string>
#include <atlbase.h>
#include <atlapp.h>
#include <atlmisc.h>
#include <algorithm>

BOOL IsVersionString(LPCTSTR lpszString)
{
    if (!lpszString)
        return FALSE;

    LPCTSTR	lpszTemp = lpszString;
    while (*lpszTemp != '\0')
    {
        if (*lpszTemp>='0'&&*lpszTemp<='9' || *lpszTemp=='.')
        {
            lpszTemp++;
            continue;
        }

        return FALSE;
    }

    return TRUE;
}

// CompareVersion.
//
// [Return Value]
// TRUE pszVersion1 < pszVersion2
// Otherwise pszVersion1 >= pszVersion2
BOOL CompareVersion(LPCTSTR pszVersion1, LPCTSTR pszVersion2)
{
    LPTSTR lpStop1 = NULL,lpStop2=NULL;
    ULONG ulVer1,ulVer2;
    ulVer1 = _tcstoul(pszVersion1,&lpStop1,10);
    ulVer2 = _tcstoul(pszVersion2,&lpStop2,10);
    if (ulVer1 < ulVer2) // 主版本号
    {
        return TRUE;
    }
    else if (ulVer1 > ulVer2)
    {
        return FALSE;
    }
    pszVersion1 = ++lpStop1;
    pszVersion2 = ++lpStop2;
    ulVer1 = _tcstoul(pszVersion1,&lpStop1,10);
    ulVer2 = _tcstoul(pszVersion2,&lpStop2,10);
    if (ulVer1 < ulVer2) // 次版本号
    {
        return TRUE;
    }
    else if (ulVer1 > ulVer2)
    {
        return FALSE;
    }

    pszVersion1 = ++lpStop1;
    pszVersion2 = ++lpStop2;
    ulVer1 = _tcstoul(pszVersion1,&lpStop1,10);
    ulVer2 = _tcstoul(pszVersion2,&lpStop2,10);
    if (ulVer1 < ulVer2) // 小版本号
    {
        return TRUE;
    }
    else if (ulVer1 > ulVer2)
    {
        return FALSE;
    }

    pszVersion1 = ++lpStop1;
    pszVersion2 = ++lpStop2;
    ulVer1 = _tcstoul(pszVersion1,&lpStop1,10);
    ulVer2 = _tcstoul(pszVersion2,&lpStop2,10);
    if (ulVer1 < ulVer2) // 修订号
    {
        return TRUE;
    }
    else if (ulVer1 > ulVer2)
    {
        return FALSE;
    }
    return FALSE;
}

BOOL SortVersion(const CString& version1, const CString& version2)
{
    return !CompareVersion(version1,version2);
}

/**
 * 在aBasePath下查找最高版本的子目录
 * 比如: aBasePath 下有 \1.0.0.1 \1.0.0.2, 就返回 aBasePath\1.0.0.2
 * return value: TRUE  查找成功
 *               FALSE 失败
 */
BOOL GetHighestVersionPath(TCHAR* aBasePath, TCHAR* aVersionPath)
{
    if (!aBasePath || !aVersionPath) {
        return FALSE;
    }


    TCHAR szFinderPattern[_MAX_PATH] = {0};
    ::PathAppend(szFinderPattern, aBasePath);
    ::PathAppend(szFinderPattern, _T("*.*"));
    std::vector<CString> versionVec;
    WTL::CFindFile finder;
    BOOL bWorking = finder.FindFile(szFinderPattern);
    while (bWorking) {
        if (finder.IsDirectory() && !finder.IsDots())
        {
            CString strName = finder.GetFileName();
            if (IsVersionString(strName))
            {
                CString strFileName = finder.GetFileName();
                versionVec.push_back(strFileName);
            }
        }
        bWorking = finder.FindNextFile();
    }

    if (versionVec.size() <= 0) {
        return FALSE;
    }
    std::sort(versionVec.begin(), versionVec.end(), SortVersion);
    aVersionPath[0] = _T('\x0');
    ::PathAppend(aVersionPath, aBasePath);
    ::PathAppend(aVersionPath, versionVec[0]);
    return TRUE;
}

/**
 * 获取解码器的安装目录
 * param: aPath  _MAX_PATH 个字符的缓冲区，用于返回目录
 * return value: TRUE  成功
 *               FALSE 失败，没有安装解码器
 */
BOOL GetCodecPath(TCHAR* aPath)
{
    CRegKey key;
    if (key.Open(HKEY_CURRENT_USER, _T("Software\\PPLive\\player\\codecs"), KEY_READ) == ERROR_SUCCESS)
    {
        ULONG nChars = _MAX_PATH;
        if (key.QueryStringValue(_T("path"), aPath, &nChars) == ERROR_SUCCESS
            && PathFileExists(aPath))
        {
            return TRUE;
        }
        key.Close();
    }
    if (key.Open(HKEY_LOCAL_MACHINE, _T("Software\\PPLive\\player\\codecs"), KEY_READ) == ERROR_SUCCESS)
    {
        ULONG nChars = _MAX_PATH;
        if (key.QueryStringValue(_T("path"), aPath, &nChars) == ERROR_SUCCESS
            && PathFileExists(aPath))
        {
            return TRUE;
        }
        key.Close();
    }
    TCHAR szPath[_MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, szPath))) {
        ::PathAppend(szPath, _T("PPLive\\Codec"));
        if (GetHighestVersionPath(szPath, aPath)) {
            return TRUE;
        }
    }
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, SHGFP_TYPE_DEFAULT, szPath))) {
        ::PathAppend(szPath, _T("PPLiveNetwork\\Codec"));
        if (GetHighestVersionPath(szPath, aPath)) {
            return TRUE;
        }
    }
    return FALSE;
}

FARPROC WINAPI fnDliHook(
    unsigned        dliNotify,
    PDelayLoadInfo  pdli
    )
{
    static const char* ffmpeg_dll [] = {
        "avformat-lav-55.dll", (const char*)L"avformat-lav-55.dll",
        "avutil-lav-52.dll", (const char*)L"avformat-lav-55.dll",
        "avcodec-lav-55.dll", (const char*)L"avformat-lav-55.dll",
        "libbluray.dll", (const char*)L"avformat-lav-55.dll",
    };
    if (dliNotify == dliFailLoadLib) {
        for (int i = 0; i < _countof(ffmpeg_dll); i += 2) {
            if (_stricmp(pdli->szDll, ffmpeg_dll[i]) == 0) {
                // dll 目录
                TCHAR szPath[_MAX_PATH];
                GetModuleFileNameW(g_currentModule, szPath, _MAX_PATH);
                PathRemoveFileSpecW(szPath);
                PathAppendW(szPath, (LPCWSTR)ffmpeg_dll[i + 1]);
                HMODULE hModule = LoadLibraryExW(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                if (hModule == NULL) {
                    if (GetCodecPath(szPath)) {
                        PathAppendW(szPath, (LPCWSTR)ffmpeg_dll[i + 1]);
                        hModule =  LoadLibraryExW(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                    }
                }
                return (FARPROC)hModule;
            }
        }
    }

    return NULL;
}

ExternC
PfnDliHook __pfnDliNotifyHook2 = fnDliHook;
