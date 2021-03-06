// WeChatResource.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "util.h"
#include "weixin.h"
#include "stdio.h"

//#define WECHATRESOURCE TEXT("WeChatResource.dll.1")
const SuppWxCfg g_Supported_wxInfo_Version[] = {
	{ TEXT("2.6.7.40"), 0x125C074 ,{0}},
	{ TEXT("2.6.7.57"), 0x125D10C ,{0}},
};

const SuppWxCfg g_Supported_wx_Version[] = {
	{ TEXT("2.6.5.38"), 0x247EF1 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
	{ TEXT("2.6.6.25"), 0x24BA81 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
	{ TEXT("2.6.6.28"), 0x24B451 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
    { TEXT("2.6.6.44"), 0x24B821 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
    { TEXT("2.6.7.32"), 0x252DB1 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
    { TEXT("2.6.7.40"), 0x252E31 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
	{ TEXT("2.6.7.57"), 0x252F61 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
	{ TEXT("2.6.8.51"), 0x257B11 ,{3, {0x8A, 0x45, 0xF3}, 3, {0x33, 0xc0, 0x90}}},
};

const SuppWxCfg g_Supported_wxRevoke_Version[] = {
    { TEXT("2.6.7.40"), 0x2533DB ,{0}},
	{ TEXT("2.6.7.57"), 0x25350B ,{0}},
	{ TEXT("2.6.8.51"), 0x2580BB ,{0}},
};

/* //2.6.5.38
text:10247EF1 8A 45 F3                                      mov     al, [ebp+var_D]
*/

int CoreFakeRevokeMsg()
{
	DWORD offset = 0x247EF1;
	//33 C0                xor eax,eax 
	BYTE code[] = { 0x33, 0xc0, 0x90 };
	DWORD code_count = 3;

	if (!IsSupportedWxVersion(
        g_Supported_wx_Version,
        ARRAYSIZE(g_Supported_wx_Version),
        &offset,
        NULL,
        NULL,
        code,
        &code_count)) {
		return ERROR_NOT_SUPPORTED;
	}
	
	HMODULE hMod = GetModuleHandleA("WechatWin.dll");
	if (!hMod) {
		return GetLastError();
	}

	PVOID addr = (BYTE*)hMod + offset;
	Patch(addr, code_count, code);

	return ERROR_SUCCESS;
}

void CoreRestoreRevokeMsg()
{
	DWORD offset = 0x247EF1;
	BYTE code[] = { 0x8A, 0x45, 0xF3 };
	DWORD code_count = 3;

	if (!IsSupportedWxVersion(
        g_Supported_wx_Version,
        ARRAYSIZE(g_Supported_wx_Version), 
        &offset,
        code,
        &code_count)) {
		return;
	}
	
	HMODULE hMod = GetModuleHandle(WECHATWINDLL);
	if (!hMod) {
		return;
	}

	PVOID addr = (BYTE*)hMod + offset;
	Patch(addr, code_count, code);
}

//带提示的防消息撤销


typedef int(__stdcall* PFNRevokeMsg)(wchar_t* msg, int len);
PVOID pfnRevokeMsgAddr = NULL;
PFNRevokeMsg pfnOrgRevokeMsg = NULL;

int __stdcall fakeRevokeMsg(wchar_t* msg, int len)
{
	__asm {
        pushad;
        pushfd;
    }

    wchar_t* recv_msg = NULL;
    pwxstring this_ = 0;
    __asm mov this_, ecx;


    if (this_ && this_->len && this_->buf) {
        recv_msg = new wchar_t[len + this_->len + 200];
        if (recv_msg) {
            memset(recv_msg, 0, (len + this_->len + 200) * sizeof(wchar_t));
            wchar_t* p = wcsstr(msg, L"撤回了一条消息</revokemsg>");
            if (p) {
                p = wcsstr(msg, L"</revokemsg>");
                p[0] = L'\0';
                StrCpyW(recv_msg, msg);
                StrCatW(recv_msg, L"\n撤销内容：");
                StrCatW(recv_msg, this_->buf);
                StrCatW(recv_msg, L"</revokemsg>");
                msg = recv_msg;
                len = wcslen(recv_msg);
            }
        }
    }

    __asm {
        popfd;
        popad;
    }

    int result = 0;

    if (pfnOrgRevokeMsg) {
        __asm mov ecx, this_;
        //ret = pfnOrgRevokeMsg(msg, len);
        __asm mov eax, len;
        __asm push eax;
        __asm mov eax, msg;
        __asm push eax;
        __asm call pfnOrgRevokeMsg;
        __asm mov  result, eax;
    }

    if (recv_msg) {
        delete[] recv_msg;
        recv_msg = NULL;
    }

    return result;
}

int CoreFakeRevokeMsg_()
{
    HMODULE hMod = GetModuleHandleA("WechatWin.dll");
    if (hMod == NULL) {
        return ERROR_NOT_ALL_ASSIGNED;
    }

    if (pfnOrgRevokeMsg) {
        return ERROR_SUCCESS;
    }	

    DWORD offset = 0;
    if (!IsSupportedWxVersion(
        g_Supported_wxRevoke_Version,
        ARRAYSIZE(g_Supported_wxRevoke_Version),
        &offset,
        NULL,
        NULL)) {
        return ERROR_NOT_SUPPORTED;
    }

    pfnRevokeMsgAddr = (PVOID)((DWORD)hMod + offset);// 

    InlineHookE8(pfnRevokeMsgAddr, fakeRevokeMsg, (PVOID*)&pfnOrgRevokeMsg);

    return ERROR_SUCCESS;
}

void CoreRestoreRevokeMsg_()
{
    if (pfnRevokeMsgAddr && pfnOrgRevokeMsg) {
        InlineHookE8(pfnRevokeMsgAddr, pfnOrgRevokeMsg, NULL);
        pfnRevokeMsgAddr = NULL;
        pfnOrgRevokeMsg = NULL;
    }
}

int CoreGetCurrentWxid(wchar_t *wxid)
{
	HMODULE hMod = GetModuleHandleA("WechatWin.dll");
	if (hMod == NULL) {
		return ERROR_NOT_ALL_ASSIGNED;
	}

	DWORD offset = 0;
	if (!IsSupportedWxVersion(
		g_Supported_wxInfo_Version,
		ARRAYSIZE(g_Supported_wxInfo_Version),
		&offset,
		NULL,
		NULL)) {
		return ERROR_NOT_SUPPORTED;
	}
	wchar_t temp[0x200] = { 0 };

	char wxidd[0x20] = { 0 };
	sprintf_s(wxidd, "%s", hMod + 0x125C120);
	//if (wxidd == "") {
	//	sprintf_s(wxidd, "%s", *((DWORD *)(hMod + 0x125C120)));
	//}
	swprintf_s(temp, L"%S", wxidd);
	MessageBox(NULL, temp, L"读取成功", MB_OK);
	
	//if (wxid == L"") {
	//	wxid = (wchar_t *)((DWORD *)(hMod + offset));
	//}

	return 99;//ERROR_SUCCESS;
	
}