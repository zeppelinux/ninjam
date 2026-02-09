#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#else
#include "../../../WDL/swell/swell.h"
#endif
#include <math.h>
#include <stdio.h>
#include "../../aeffectx.h"
#include "../../../WDL/mutex.h"
#include "../../../WDL/fastqueue.h"
#include "../../../WDL/queue.h"
#include "../../../WDL/wdlcstring.h"

#define WANT_LOCALIZE_IMPORT_INCLUDE
#include "winclient.h"
#define WDL_WIN32_HIDPI_IMPL
#include "../../../WDL/win32_hidpi.h" // for mmon SetWindowPos() tweaks
#define REAPER_COMMONFUNC_DECL
#include "../cockos_vst.h"

#include "resource.h"

#define VENDOR "Cockos"
#define EFFECT_NAME "ReaNINJAM"

#define SAMPLETYPE float


void audiostream_onsamples(float **inbuf, int innch, float **outbuf, int outnch, int len, int srate, bool isPlaying, bool isSeek, double curpos) ;
void InitializeInstance();
void QuitInstance();


void (*format_timestr_pos)(double tpos, char *buf, int buflen, int modeoverride); // actually implemented in tracklist.cpp for now


audioMasterCallback g_hostcb;

#ifndef _WIN32
static HWND customControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h);
#endif

#include "../../../WDL/db2val.h"

HANDLE * (*GetIconThemePointer)(const char *name);
HWND (*GetMainHwnd)();
void *(*CreateVorbisEncoder)(int srate, int nch, int serno, float qv, int cbr, int minbr, int maxbr);
void *(*CreateVorbisDecoder)();
void (*PluginWantsAlwaysRunFx)(int amt);
int (*GetWindowDPIScaling)(HWND hwnd);
#ifdef _WIN32
LRESULT (*handleCheckboxCustomDraw)(HWND, LPARAM, const unsigned short *list, int listsz, bool isdlg);
#endif
INT_PTR (*autoRepositionWindowOnMessage)(HWND hwnd, int msg, const char *desc_str, int flags); // flags unused currently
int (*GetPlayStateEx)(void *proj);
void (*OnPlayButtonForTime)(void *proj, double forTime);
void (*SetEditCurPos2)(void *proj, double time, bool moveview, bool seekplay);
void (*SetCurrentBPM)(void *proj, double bpm, bool wantUndo);
void (*GetSet_LoopTimeRange2)(void* proj, bool isSet, bool isLoop, double* startOut, double* endOut, bool allowautoseek);
int (*GetSetRepeatEx)(void* proj, int val);
double (*GetCursorPositionEx)(void *proj);
void (*Main_OnCommandEx)(int command, int flag, void *proj);

void (*GetProjectPath)(char *buf, int bufsz);
const char *(*get_ini_file)();
BOOL  (WINAPI *InitializeCoolSB)(HWND hwnd);
HRESULT (WINAPI *UninitializeCoolSB)(HWND hwnd);
int   (WINAPI *CoolSB_SetScrollInfo)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi, BOOL fRedraw);
BOOL (WINAPI *CoolSB_GetScrollInfo)(HWND hwnd, int fnBar, LPSCROLLINFO lpsi);
int (WINAPI *CoolSB_SetScrollPos)(HWND hwnd, int nBar, int nPos, BOOL fRedraw);
int (WINAPI *CoolSB_SetScrollRange)(HWND hwnd, int nBar, int nMinPos, int nMaxPos, BOOL fRedraw);
BOOL (WINAPI *CoolSB_SetMinThumbSize)(HWND hwnd, UINT wBar, UINT size);
int (*plugin_register)(const char *name, void *infostruct);

int reaninjamAccelProc(MSG *msg, accelerator_register_t *ctx)
{
  extern HWND g_hwnd;
  if (g_hwnd && (
        msg->message == WM_KEYDOWN || msg->message == WM_KEYUP ||
        msg->message == WM_SYSKEYDOWN || msg->message == WM_SYSKEYUP ||
        msg->message == WM_CHAR) &&
      msg->hwnd && (g_hwnd==msg->hwnd || IsChild(g_hwnd,msg->hwnd)))
  {

    if (msg->message != WM_CHAR)
    {
      const int flags = ((GetAsyncKeyState(VK_MENU)&0x8000) ? FALT : 0) |
                        ((GetAsyncKeyState(VK_SHIFT)&0x8000) ? FSHIFT : 0) |
                        ((GetAsyncKeyState(VK_CONTROL)&0x8000) ? FCONTROL : 0);
      const bool isDown = msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN;
      if (IS_MSG_VIRTKEY(msg) && msg->wParam >= VK_F1 && msg->wParam <= VK_F10)
      {
        int idx = msg->wParam - VK_F1;
        switch (flags)
        {
          case 0:
            if (isDown)
              SendMessage(g_hwnd,WM_COMMAND,ID_LOCAL_CHANNEL_1+idx,0);
          return 1;
          case FSHIFT:
            if (isDown)
              SendMessage(g_hwnd,WM_COMMAND,ID_REMOTE_USER_1+idx,0);
          return 1;
          case FCONTROL|FSHIFT:
            if (isDown)
              SendMessage(g_hwnd,WM_COMMAND,ID_REMOTE_USER_CHANNEL_1+idx,0);
          return 1;
        }
      }
      else switch (flags)
      {
      case FALT:
        switch (msg->wParam)
        {
#ifdef __APPLE__
          case 'Y':
            if (isDown) SendMessage(g_hwnd,WM_COMMAND,IDC_SYNC,0);
          return 1;
#endif
          case 'S':
          case 'M':
            if (isDown)
              SendMessage(g_hwnd,WM_COMMAND,msg->wParam == 'S' ? IDC_SOLO : IDC_MUTE,0);
          return 1;
          case 'T':
            if (isDown) SetFocus(GetDlgItem(g_hwnd,IDC_CHATENT));
          return 1;
        }
      break;
      case FCONTROL|FSHIFT:
        switch (msg->wParam)
        {
          case 'D':
            if (isDown)
              SendMessage(g_hwnd,WM_COMMAND,IDC_REMOVE,0);
          return 1;
          case 'N':
            if (isDown)
              SendMessage(g_hwnd,WM_COMMAND,IDC_ADDCH,0);
          return 1;
          case 'M':
            if (isDown) SendMessage(g_hwnd,WM_COMMAND,IDC_MASTERMUTE,0);
          return 1;
        }
      break;
      case FCONTROL:
        switch (msg->wParam)
        {
          case 'M':
            if (isDown) SendMessage(g_hwnd,WM_COMMAND,IDC_METROMUTE,0);
          return 1;
          case 'O':
            if (isDown) SendMessage(g_hwnd,WM_COMMAND,ID_FILE_CONNECT,0);
          return 1;
          case 'D':
            if (isDown) SendMessage(g_hwnd,WM_COMMAND,ID_FILE_DISCONNECT,0);
          return 1;
#ifdef __APPLE__
          case ',':
#else
          case 'P':
#endif
            if (isDown) SendMessage(g_hwnd,WM_COMMAND,ID_OPTIONS_PREFERENCES,0);
          return 1;
        }
      break;
      }
    }

    HWND list = GetDlgItem(g_hwnd,IDC_CHATDISP);
    HWND e = GetDlgItem(g_hwnd,IDC_CHATENT);
    if (e)
    {
      if (msg->hwnd == e || IsChild(e,msg->hwnd))
      {
#ifdef _WIN32
        if (msg->message == WM_CHAR && msg->wParam == VK_RETURN)
#else
        if (msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN)
#endif
        {
          SendMessage(g_hwnd,WM_COMMAND,IDC_CHATOK,0);
          return 1;
        }
      }
      else if (list && (msg->hwnd == list || IsChild(list,msg->hwnd)))
      {
#ifndef __APPLE__
        // this appears to be unsafe on macOS (could get it to throw exceptions bleh)
        if (msg->message != WM_CHAR) switch (msg->wParam)
        {
          case VK_CONTROL:
          case VK_SHIFT:
          case VK_UP:
          case VK_DOWN:
          case VK_LEFT:
          case VK_RIGHT:
          case VK_NEXT:
          case VK_PRIOR:
          case VK_TAB:
            // allow ctrl/shift/nav keys to go to control, everything else causes focus change to edit
          return -1;
        }
        if (!(GetAsyncKeyState(VK_CONTROL)&0x8000))
        {
          SetFocus(e);
#ifdef _WIN32
          msg->hwnd = e;
#else
          if (msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN)
          {
            SendMessage(g_hwnd,WM_COMMAND,IDC_CHATOK,0);
          }
          else
          {
            // linux at least needs this (otherwise it selects existing text and then overwrites it)
            SendMessage(e,EM_SETSEL,-1,0);
            SendMessage(e,msg->message,msg->wParam,msg->lParam);
          }
          return 1;
#endif
        }
#endif
      }
    }
    return -1;
  }
  return 0;
}


HINSTANCE g_hInst;
DWORD g_main_thread;

class VSTEffectClass;
VSTEffectClass *g_vst_object;

class VSTEffectClass
{
public:
  VSTEffectClass(audioMasterCallback cb)
  {
    g_vst_object = this;
    memset(&m_effect,0,sizeof(m_effect));
    m_samplerate=44100.0;
    m_hwndcfg=0;
    m_effect.magic = kEffectMagic;
    m_effect.dispatcher = staticDispatcher;
    m_effect.process = staticProcess;
    m_effect.getParameter = staticGetParameter;
    m_effect.setParameter = staticSetParameter;
    m_effect.numPrograms = 1;
    m_effect.numParams = 0;
    m_effect.numInputs = 2; // these will get overridden on config load
    m_effect.numOutputs = 2;

    m_effect.flags=effFlagsCanReplacing|effFlagsHasEditor;
    m_effect.processReplacing=staticProcessReplacing;//do nothing
    m_effect.uniqueID=CCONST('r','e','n','j');
    m_effect.version=1100;

    m_effect.object=this;
    m_effect.ioRatio=1.0;
    m_lasttransportpos=-100000000.0;
    m_lastplaytrackpos=-100000000.0;
  }


  ~VSTEffectClass()
  {
    g_vst_object = NULL;
    if (m_hwndcfg) DestroyWindow(m_hwndcfg);
  }


  WDL_DLGRET CfgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg)
    {
      case WM_INITDIALOG:
        m_hwndcfg=hwndDlg;

        ShowWindow(hwndDlg,SW_SHOWNA);
        WDL_FALLTHROUGH;
      case WM_USER+6606:

      return 0;
      case WM_TIMER:
      return 0;
      case WM_COMMAND:
        if (LOWORD(wParam)==IDC_BUTTON1)
        {
          extern HWND g_hwnd;
          if (g_hwnd)
          {
            ShowWindow(g_hwnd,SW_SHOWNORMAL);
            SetForegroundWindow(g_hwnd);
          }
        }
      return 0;
      case WM_DESTROY:
        m_hwndcfg=0;
      return 0;
    }
    return 0;
  }

  static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (uMsg == WM_INITDIALOG) SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
    VSTEffectClass *_this = (VSTEffectClass *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
    return _this->CfgProc(hwndDlg,uMsg,wParam,lParam);
  }

  static VstIntPtr VSTCALLBACK staticDispatcher(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
  {
    VSTEffectClass *_this = (VSTEffectClass *)effect->object;
    switch (opCode)
    {
      case effCanDo:
        if (ptr && !strcmp((char *)ptr,"hasCockosViewAsConfig")) return 0xbeef0000;
        if (ptr && !strcmp((char *)ptr,"hasCockosExtensions")) return 0xbeef0000;
      return 0;
      case effStopProcess:
      case effMainsChanged:
        if (_this && !value)
        {
        }
      return 0;
      case effSetBlockSize:

        // initialize, yo
        if (GetCurrentThreadId()==g_main_thread)
        {
          static int first;
          if (!first)
          {
            first=1;
            char buf[4096];
            GetModuleFileName(g_hInst,buf,sizeof(buf));
            LoadLibrary(buf);// keep us resident //-V530
#ifndef _WIN32
            SWELL_RegisterCustomControlCreator(customControlCreator);
#endif

            if (plugin_register)
            {
              static accelerator_register_t accel = { reaninjamAccelProc, TRUE};
              plugin_register("accelerator",&accel);
            }
          }
          InitializeInstance();
        }

      return 0;
      case effSetSampleRate:
        if (_this)
        {
          _this->m_samplerate=opt;
        }
      return 0;
      case effGetInputProperties:
      case effGetOutputProperties:
        if (_this && ptr)
        {
          if (index<0) return 0;

          VstPinProperties *pp=(VstPinProperties*)ptr;
          pp->flags=0;
          pp->arrangementType=kSpeakerArrStereo;
          if (opCode == effGetInputProperties)
          {
            if (index >= _this->m_effect.numInputs) return 0;
            snprintf(pp->label,sizeof(pp->label),__LOCALIZE_VERFMT("Input %d","reaplugs_io"),index+1);
            if ((index&1)==0)
            {
              if (index < _this->m_effect.numInputs-1) pp->flags|=kVstPinIsStereo;
              else pp->arrangementType=kSpeakerArrMono;
            }
          }
          else
          {
            if (index >= _this->m_effect.numOutputs) return 0;
            snprintf(pp->label,sizeof(pp->label),__LOCALIZE_VERFMT("Output %d","reaplugs_io"),index+1);
            if ((index&1)==0)
            {
              if (index < _this->m_effect.numOutputs-1) pp->flags|=kVstPinIsStereo;
              else pp->arrangementType=kSpeakerArrMono;
            }
          }

          return 1;
        }
      return 0;
      case effGetPlugCategory: return kPlugCategEffect;
      case effGetEffectName:
        if (ptr) lstrcpyn((char*)ptr,EFFECT_NAME,32);
      return 0;

      case effGetProductString:
        if (ptr) lstrcpyn((char*)ptr,EFFECT_NAME,32);
      return 0;
      case effGetVendorString:
        if (ptr) lstrcpyn((char*)ptr,VENDOR,32);
      return 0;

      case effEditOpen:
        if (_this)
        {
          if (_this->m_hwndcfg) DestroyWindow(_this->m_hwndcfg);

          return !!CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_VSTCFG),(HWND)ptr,dlgProc,(LPARAM)_this);
        }
      return 0;
      case effEditClose:
        if (_this && _this->m_hwndcfg) DestroyWindow(_this->m_hwndcfg);
      return 0;
      case effClose:
        QuitInstance();
        if (PluginWantsAlwaysRunFx) PluginWantsAlwaysRunFx(-1);
        delete _this;
      return 0;
      case effOpen:
      return 0;
      case effEditGetRect:
        if (_this)// && _this->m_hwndcfg)
        {
          RECT r;
          if (_this->m_hwndcfg) GetClientRect(_this->m_hwndcfg,&r);
          else {r.left=r.top=0; r.right=400; r.bottom=300; }
          _this->cfgRect[0]=(short)r.top;
          _this->cfgRect[1]=(short)r.left;
          _this->cfgRect[2]=(short)r.bottom;
          _this->cfgRect[3]=(short)r.right;

          *(void **)ptr = _this->cfgRect;

          return 1;

        }
      return 0;
      case effIdentify:
        return CCONST ('N', 'v', 'E', 'f');    }
    return 0;
  }
  short cfgRect[4];

  static void VSTCALLBACK staticProcessReplacing(AEffect *effect, SAMPLETYPE **inputs, SAMPLETYPE **outputs, VstInt32 sampleframes)
  {
    VSTEffectClass *_this=(VSTEffectClass *)effect->object;
    if (_this)
    {
      VstTimeInfo *p=(VstTimeInfo *)g_hostcb(effect,audioMasterVendorSpecific,0xdeadbeef,audioMasterGetTime,NULL,0.0f);

      bool isPlaying=false;
      bool isSeek=false;

      // we call our extended audiomastergettime which just returns everything as seconds
      if (p) // &&  &&
           // )
      {
        isPlaying = !!(p->flags&(kVstTransportPlaying|kVstTransportRecording));


        if (isPlaying)
        {
          // if we've looped, and are before the start position (meaning we're a duped block)
          if ((p->flags&(kVstPpqPosValid|kVstCyclePosValid)) == (kVstPpqPosValid|kVstCyclePosValid) &&
              p->ppqPos < _this->m_lasttransportpos-0.001 && p->ppqPos < p->cycleStartPos-1.0/_this->m_samplerate+0.0000000001)
          {
            _this->m_lasttransportpos=p->ppqPos;
            // leave the output buffers as is (which should preserve them from the last time, we hope)
            return;
          }
          _this->m_lasttransportpos=p->ppqPos;

        }
        else _this->m_lasttransportpos=-100000000.0;

      }
      else _this->m_lasttransportpos=-100000000.0;


      if (_this->m_lasttransportpos <= _this->m_lastplaytrackpos || _this->m_lasttransportpos > _this->m_lastplaytrackpos + 0.5)
      {
        isSeek=true;
      }
      _this->m_lastplaytrackpos=_this->m_lasttransportpos;

      audiostream_onsamples(inputs,effect->numInputs,outputs,effect->numOutputs,sampleframes,(int)(_this->m_samplerate+0.5),isPlaying,isSeek,_this->m_lastplaytrackpos);
      effect->numInputs = g_config_num_inputs;
      effect->numOutputs = g_config_num_outputs;
    }
  }

  static void VSTCALLBACK staticProcess(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleframes)
  {

  }


  static void VSTCALLBACK staticSetParameter(AEffect *effect, VstInt32 index, float parameter)
  {
  }

  static float VSTCALLBACK staticGetParameter(AEffect *effect, VstInt32 index)
  {
    return 0.0f;
  }

  HWND m_hwndcfg;
  double m_samplerate;
  AEffect m_effect;

  double m_lasttransportpos,m_lastplaytrackpos;

};

void *get_parent_project(void)
{
  if (g_vst_object && g_hostcb)
    return (void *)g_hostcb(&g_vst_object->m_effect,0xdeadbeef,0xdeadf00e,3,NULL,0.0f);

  return NULL;
}



extern "C" {


#ifdef _WIN32
__declspec(dllexport) AEffect *VSTPluginMain(audioMasterCallback hostcb)
#else
  __attribute__ ((visibility ("default"))) AEffect *VSTPluginMain(audioMasterCallback hostcb)
#endif
{
  if (!hostcb) return 0;
  if (g_vst_object) return 0;

  g_hostcb=hostcb;

  static bool api_ok;
  if (!api_ok)
  {
    IMPORT_REAPER_COMMON(hostcb);
    import_vst_reaper_api((void **)&get_ini_file, "get_ini_file", hostcb);
    import_vst_reaper_api((void **)&plugin_register, "plugin_register", hostcb);
    import_vst_reaper_api((void **)&GetProjectPath, "GetProjectPath", hostcb);

    import_vst_reaper_api((void **)&InitializeCoolSB, "InitializeCoolSB", hostcb);
    import_vst_reaper_api((void **)&UninitializeCoolSB, "UninitializeCoolSB", hostcb);
    import_vst_reaper_api((void **)&CoolSB_SetScrollInfo, "CoolSB_SetScrollInfo", hostcb);
    import_vst_reaper_api((void **)&CoolSB_GetScrollInfo, "CoolSB_GetScrollInfo", hostcb);
    import_vst_reaper_api((void **)&CoolSB_SetScrollPos, "CoolSB_SetScrollPos", hostcb);
    import_vst_reaper_api((void **)&CoolSB_SetScrollRange, "CoolSB_SetScrollRange", hostcb);
    import_vst_reaper_api((void **)&CoolSB_SetMinThumbSize, "CoolSB_SetMinThumbSize", hostcb);

    import_vst_reaper_api((void **)&SetWindowAccessibilityString, "SetWindowAccessibilityString", hostcb);

    import_vst_reaper_api((void **)&format_timestr_pos, "format_timestr_pos", hostcb);
    import_vst_reaper_api((void **)&GetMainHwnd, "GetMainHwnd", hostcb);
    import_vst_reaper_api((void **)&GetIconThemePointer, "GetIconThemePointer", hostcb);
    import_vst_reaper_api((void **)&PluginWantsAlwaysRunFx, "PluginWantsAlwaysRunFx", hostcb);
    import_vst_reaper_api((void **)&GetWindowDPIScaling, "GetWindowDPIScaling", hostcb);
    import_vst_reaper_api((void **)&autoRepositionWindowOnMessage, "autoRepositionWindowOnMessage", hostcb);
    import_vst_reaper_api((void **)&GetPlayStateEx, "GetPlayStateEx", hostcb);
    import_vst_reaper_api((void **)&OnPlayButtonForTime, "OnPlayButtonForTime", hostcb);
    import_vst_reaper_api((void **)&SetEditCurPos2, "SetEditCurPos2", hostcb);
    import_vst_reaper_api((void **)&SetCurrentBPM, "SetCurrentBPM", hostcb);
    import_vst_reaper_api((void **)&GetSet_LoopTimeRange2, "GetSet_LoopTimeRange2", hostcb);
    import_vst_reaper_api((void **)&GetSetRepeatEx, "GetSetRepeatEx", hostcb);
    import_vst_reaper_api((void **)&GetCursorPositionEx, "GetCursorPositionEx", hostcb);
    import_vst_reaper_api((void **)&Main_OnCommandEx, "Main_OnCommandEx", hostcb);
#ifdef _WIN32
    import_vst_reaper_api((void **)&handleCheckboxCustomDraw, "handleCheckboxCustomDraw", hostcb);
#endif

    IMPORT_LOCALIZE_VST(hostcb);
    if (!GetMainHwnd || !GetIconThemePointer) return 0;
  }

  if ((!CreateVorbisDecoder || !CreateVorbisEncoder) && GetMainHwnd())
  {
#define GETAPI(x) import_vst_reaper_api((void **)&x, #x, hostcb);
    GETAPI(CreateVorbisDecoder)
    GETAPI(CreateVorbisEncoder)

    if (!CreateVorbisEncoder||!CreateVorbisDecoder) return 0;
  }

  if (!api_ok)
  {
    g_main_thread=GetCurrentThreadId();
    api_ok = true;
  }

  if (PluginWantsAlwaysRunFx) PluginWantsAlwaysRunFx(1);
  VSTEffectClass *obj = new VSTEffectClass(hostcb);
  return &obj->m_effect;
}

BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
{
  if (fdwReason==DLL_PROCESS_ATTACH)
  {
    g_hInst=hDllInst;
  }
  return TRUE;
}

};



#ifndef _WIN32 // MAC resources

#define SET_IDD_EMPTY_SCROLL_STYLE SWELL_DLG_FLAGS_AUTOGEN|SWELL_DLG_WS_CHILD
#define SET_IDD_EMPTY_STYLE SWELL_DLG_FLAGS_AUTOGEN|SWELL_DLG_WS_CHILD


#include "../../../WDL/swell/swell-dlggen.h"
#include "res.rc_mac_dlg"
#undef BEGIN
#undef END
#include "../../../WDL/swell/swell-menugen.h"
#include "res.rc_mac_menu"


static HWND customControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h)
{
  if (!stricmp(classname,"RichEditChild"))
  {
    if ((style & 0x2800))
    {
      return SWELL_MakeEditField(idx,-x,-y,-w,-h,ES_READONLY|WS_VSCROLL|ES_MULTILINE);
    }
    else
      return SWELL_MakeEditField(idx,-x,-y,-w,-h,0);
  }
  return 0;
}

#endif
