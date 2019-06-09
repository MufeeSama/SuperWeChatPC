#include "stdafx.h"
#include "weixin.h"

const SuppWxCfg g_Supported_WxSendTextMsg_Version[] = {
    { TEXT("2.6.6.44"), 0x2DA4A0 ,{0}}, //voiicemsg
    { TEXT("2.6.7.40"), 0x2E3E20 ,{0}}, //voiicemsg
	{ TEXT("2.6.7.57"), 0x2E4C40 ,{0}}, //voiicemsg
	{ TEXT("2.6.8.51"), 0x2EB4E0 ,{0}}, //voiicemsg
};



typedef void(__cdecl* PFN_WXSendTextMsg)(wxstring* msg, char* unk, int flag);


PFN_WXSendTextMsg pfn_WxSendTextMsg = NULL;


void WxSendTextMsg(wxstring* wxid, wxstring* msg)
{
    char v65[0x1000] = { 0 };
    char unk[0x14] = { 0 };

    __asm {
        lea ecx, v65;
        mov edx, wxid;
    }
    //pfn_WxSendTextMsg(msg, unk, 1);
    __asm {
        push 1;
        lea eax, unk;
        push eax;
        mov eax, msg;
        push eax;
        call pfn_WxSendTextMsg;
        add     esp, 0xC;
    }
    //f_free_1004B550(&v65);              // ÊÍ·ÅÄÚ´æ
    /*__asm {
        lea ecx, v65;
        call pfn_WxMsgPackFree;
    }*/
}


int InitFunction()
{
    if (pfn_WxSendTextMsg == NULL)
    {
        HMODULE hMod = GetModuleHandleA("WechatWin.dll");
        if (hMod == NULL) {
            return ERROR_NOT_ALL_ASSIGNED;
        }

        DWORD SendTextMsgOffset = 0;
        DWORD MsgFreeOffset = 0;
        DWORD SendMsgMgrInstanceOffset = 0;
        DWORD SendImageMsgOffset = 0;

        if (!IsSupportedWxVersion(
                g_Supported_WxSendTextMsg_Version,
                ARRAYSIZE(g_Supported_WxSendTextMsg_Version),
                &SendTextMsgOffset))
        {
            return ERROR_NOT_SUPPORTED;
        }
        pfn_WxSendTextMsg = (PFN_WXSendTextMsg)((DWORD)hMod + SendTextMsgOffset);        
    }

    return ERROR_SUCCESS;
}

int CoreSendTxtMsg(WCHAR* wxid, WCHAR* msg)
{
    int ret = InitFunction();
    if (ret != ERROR_SUCCESS) {
        return ret;
    }

    wxstring pmsg;
    wxstring pwxid;

    pwxid.buf = wxid;
    pwxid.len = wcslen(wxid);
    pwxid.maxlen = wcslen(wxid) + 2;

    pmsg.buf = msg;
    pmsg.len = wcslen(msg);
    pmsg.maxlen = wcslen(msg) + 2;

    WxSendTextMsg(&pwxid, &pmsg);

    return ret;
}

//Ë½ÓÐ
int CoreSendImageMsg(WCHAR* wxid, WCHAR* path)
{
    int ret = InitFunction();
    if (ret != ERROR_SUCCESS) {
        return ret;
    }

    /*wxstring pimage;
    wxstring pwxid;
    char msgpack[0x1000] = { 0 };

    pwxid.buf = wxid;
    pwxid.len = wcslen(wxid);
    pwxid.maxlen = wcslen(wxid) + 2;

    pimage.buf = path;
    pimage.len = wcslen(path);
    pimage.maxlen = wcslen(path) + 2;

    void* p = pfn_WxSendMsgMgrInstance();
    if (p)
    {
        __asm mov ecx, p;
        pfn_WxSendMsgMgr_SendImageMsg(msgpack, &pwxid, &pimage);
    }*/

    return ret;
}