// Background.cpp - Custom background (image / color) support for the editor
#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>
#include <gdiplus.h>

#include "Dark.h"
#include "Edit.h"
#include "Background.h"

using namespace Gdiplus;

// Defined in Notepatch.cpp
extern HINSTANCE	g_hInstance;
extern BOOL			g_bIsDark;
extern Edit*		g_pEdit;

//##########################################################
// Public state (persisted by Notepatch.cpp)
//##########################################################
BOOL		g_bBgImageEnabled = FALSE;
wchar_t		g_wszBgImagePath[MAX_PATH] = L"";
int			g_iBgOpacity = 100;
BOOL		g_bCustomBgColor = FALSE;
COLORREF	g_crEditorBg = RGB(255, 255, 255);
BOOL		g_bCustomTextColor = FALSE;
COLORREF	g_crEditorText = RGB(0, 0, 0);

//##########################################################
// Internals
//##########################################################
static ULONG_PTR	s_gpToken = 0;
static Bitmap*		s_pImage = NULL;
static HBITMAP		s_hBmp = NULL;
static HBRUSH		s_hBrush = NULL;

#define IDC_OPACITY_TRACK	9002
#define IDC_OPACITY_LABEL	9003

//##########################################################
// GDI+ startup / shutdown
//##########################################################
void BackgroundInit(void)
{
	GdiplusStartupInput input;
	GdiplusStartup(&s_gpToken, &input, NULL);
}

void BackgroundShutdown(void)
{
	BackgroundClear();
	if (s_gpToken)
		GdiplusShutdown(s_gpToken);
}

//##########################################################
// Detects editor scrolling (the standard EDIT control scrolls
// its pixels with ScrollWindow, which would misalign the fixed
// background image) and forces a full repaint when needed
//##########################################################
void BackgroundCheckScroll(void)
{
	if (!g_bBgImageEnabled || !g_pEdit || !g_pEdit->m_hWnd)
		return;

	static int s_iFirstLine = -1;
	static int s_iHorzPos = -1;

	int iFirstLine = (int)SendMessage(g_pEdit->m_hWnd, EM_GETFIRSTVISIBLELINE, 0, 0);
	SCROLLINFO si = { sizeof(si), SIF_POS };
	int iHorz = GetScrollInfo(g_pEdit->m_hWnd, SB_HORZ, &si) ? si.nPos : 0;

	if (iFirstLine != s_iFirstLine || iHorz != s_iHorzPos)
	{
		s_iFirstLine = iFirstLine;
		s_iHorzPos = iHorz;
		InvalidateRect(g_pEdit->m_hWnd, NULL, FALSE);
	}
}

//##########################################################
//
//##########################################################
BOOL BackgroundIsActive(void)
{
	return (g_bBgImageEnabled && s_hBrush != NULL) || g_bCustomBgColor;
}

//##########################################################
// Effective colors: custom values win, otherwise theme defaults
//##########################################################
COLORREF BackgroundGetEffectiveBgColor(void)
{
	if (g_bCustomBgColor)
		return g_crEditorBg;
	return g_bIsDark ? DARK_EDITOR_BG_COLOR : RGB(255, 255, 255);
}

COLORREF BackgroundGetEffectiveTextColor(void)
{
	if (g_bCustomTextColor)
		return g_crEditorText;
	return g_bIsDark ? DARK_EDITOR_TEXT_COLOR : RGB(0, 0, 0);
}

//##########################################################
// Picks black or white text for the given background color
//##########################################################
static COLORREF ContrastTextColor(COLORREF cr)
{
	int lum = (299 * GetRValue(cr) + 587 * GetGValue(cr) + 114 * GetBValue(cr)) / 1000;
	return lum > 140 ? RGB(0, 0, 0) : RGB(255, 255, 255);
}

//##########################################################
//
//##########################################################
void BackgroundRefresh(void)
{
	if (g_pEdit && g_pEdit->m_hWnd)
	{
		InvalidateRect(g_pEdit->m_hWnd, NULL, TRUE);
		UpdateWindow(g_pEdit->m_hWnd);
	}
}

//##########################################################
//
//##########################################################
BOOL BackgroundLoadImage(const wchar_t *wszPath)
{
	Bitmap *pNew = Bitmap::FromFile(wszPath);
	if (!pNew || pNew->GetLastStatus() != Ok)
	{
		delete pNew;
		return FALSE;
	}
	delete s_pImage;
	s_pImage = pNew;
	wcscpy_s(g_wszBgImagePath, MAX_PATH, wszPath);
	g_bBgImageEnabled = TRUE;
	return TRUE;
}

//##########################################################
//
//##########################################################
void BackgroundClear(void)
{
	delete s_pImage;
	s_pImage = NULL;
	if (s_hBrush) { DeleteObject(s_hBrush); s_hBrush = NULL; }
	if (s_hBmp) { DeleteObject(s_hBmp); s_hBmp = NULL; }
	g_bBgImageEnabled = FALSE;
	g_wszBgImagePath[0] = 0;
	g_bCustomBgColor = FALSE;
	g_bCustomTextColor = FALSE;
	BackgroundRefresh();
}

//##########################################################
// Renders the background bitmap sized to the edit control:
// solid effective bg color, with the image blended on top
// according to g_iBgOpacity. Then creates a pattern brush.
//##########################################################
void BackgroundRebuild(void)
{
	if (!s_pImage || !g_pEdit || !g_pEdit->m_hWnd)
		return;

	RECT rc;
	GetClientRect(g_pEdit->m_hWnd, &rc);
	int cx = rc.right, cy = rc.bottom;
	if (cx < 1 || cy < 1)
		return;

	HDC hdcScreen = GetDC(NULL);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBmpNew = CreateCompatibleBitmap(hdcScreen, cx, cy);
	HBITMAP hBmpOld = (HBITMAP)SelectObject(hdcMem, hBmpNew);

	HBRUSH hbrFill = CreateSolidBrush(BackgroundGetEffectiveBgColor());
	FillRect(hdcMem, &rc, hbrFill);
	DeleteObject(hbrFill);

	{
		Graphics gr(hdcMem);
		gr.SetInterpolationMode(InterpolationModeBilinear);
		gr.SetPixelOffsetMode(PixelOffsetModeHalf);

		ImageAttributes ia;
		if (g_iBgOpacity < 100)
		{
			ColorMatrix cm = {};
			cm.m[0][0] = cm.m[1][1] = cm.m[2][2] = 1.0f;
			cm.m[4][4] = 1.0f;
			cm.m[3][3] = g_iBgOpacity / 100.0f;
			ia.SetColorMatrix(&cm, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
		}

		// Uniform fill ("cover"): keep aspect ratio, scale so the image
		// always covers the whole area, center-crop the overflow
		UINT iw = s_pImage->GetWidth(), ih = s_pImage->GetHeight();
		if (iw > 0 && ih > 0)
		{
			double fScale = max((double)cx / iw, (double)cy / ih);
			int dw = (int)(iw * fScale + 0.5);
			int dh = (int)(ih * fScale + 0.5);
			int dx = (cx - dw) / 2;
			int dy = (cy - dh) / 2;
			gr.DrawImage(s_pImage, Rect(dx, dy, dw, dh),
				0, 0, (INT)iw, (INT)ih,
				UnitPixel, g_iBgOpacity < 100 ? &ia : NULL);
		}
	}

	SelectObject(hdcMem, hBmpOld);
	DeleteDC(hdcMem);
	ReleaseDC(NULL, hdcScreen);

	if (s_hBrush) { DeleteObject(s_hBrush); s_hBrush = NULL; }
	if (s_hBmp) { DeleteObject(s_hBmp); s_hBmp = NULL; }
	s_hBmp = hBmpNew;
	s_hBrush = CreatePatternBrush(s_hBmp);
}

//##########################################################
//
//##########################################################
HBRUSH BackgroundGetBrush(void)
{
	return (g_bBgImageEnabled && s_pImage) ? s_hBrush : NULL;
}

//##########################################################
// File-open dialog for picking the background image
//##########################################################
BOOL BackgroundChooseFile(HWND hWndOwner)
{
	wchar_t wszFile[MAX_PATH] = L"";
	wchar_t wszFilter[] =
		L"图片文件 (*.bmp;*.jpg;*.jpeg;*.png;*.gif;*.tif;*.tiff;*.ico)\0*.bmp;*.jpg;*.jpeg;*.png;*.gif;*.tif;*.tiff;*.ico\0"
		L"所有文件 (*.*)\0*.*\0";

	OPENFILENAME ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndOwner;
	ofn.lpstrFilter = wszFilter;
	ofn.lpstrFile = wszFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"选择背景图片";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (!GetOpenFileName(&ofn))
		return FALSE;

	if (!BackgroundLoadImage(wszFile))
	{
		MessageBox(hWndOwner, L"无法加载该图片文件，请选择其他图片。", L"背景图片", MB_ICONERROR | MB_OK);
		return FALSE;
	}

	BackgroundRebuild();
	BackgroundRefresh();
	return TRUE;
}

//##########################################################
// Averages the image into a 1x1 pixel and applies the result
// as editor background color (+ a readable text color)
//##########################################################
BOOL BackgroundExtractColors(void)
{
	if (!s_pImage)
		return FALSE;

	Bitmap bmp1x1(1, 1, PixelFormat32bppARGB);
	{
		Graphics gr(&bmp1x1);
		gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
		gr.DrawImage(s_pImage, Rect(0, 0, 1, 1),
			0, 0, (INT)s_pImage->GetWidth(), (INT)s_pImage->GetHeight(), UnitPixel);
	}

	Gdiplus::Color c;
	if (bmp1x1.GetPixel(0, 0, &c) != Ok)
		return FALSE;

	g_crEditorBg = RGB(c.GetR(), c.GetG(), c.GetB());
	g_bCustomBgColor = TRUE;
	g_crEditorText = ContrastTextColor(g_crEditorBg);
	g_bCustomTextColor = TRUE;

	BackgroundRebuild();
	BackgroundRefresh();
	return TRUE;
}

//##########################################################
//
//##########################################################
void BackgroundSetOpacity(int iOpacity)
{
	if (iOpacity < 0) iOpacity = 0;
	if (iOpacity > 100) iOpacity = 100;
	if (g_iBgOpacity == iOpacity)
		return;
	g_iBgOpacity = iOpacity;
	BackgroundRebuild();
	BackgroundRefresh();
}

//##########################################################
// Opacity slider dialog
//##########################################################
struct OpacityDlgState
{
	int iOriginal;
};

static INT_PTR CALLBACK OpacityDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);

			HWND hTrack = GetDlgItem(hDlg, IDC_OPACITY_TRACK);
			SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
			SendMessage(hTrack, TBM_SETTICFREQ, 10, 0);
			SendMessage(hTrack, TBM_SETPOS, TRUE, g_iBgOpacity);

			wchar_t wsz[16];
			StringCbPrintfW(wsz, sizeof(wsz), L"%d%%", g_iBgOpacity);
			SetDlgItemText(hDlg, IDC_OPACITY_LABEL, wsz);
			return TRUE;
		}

		case WM_HSCROLL:
		{
			HWND hTrack = GetDlgItem(hDlg, IDC_OPACITY_TRACK);
			int iPos = (int)SendMessage(hTrack, TBM_GETPOS, 0, 0);

			wchar_t wsz[16];
			StringCbPrintfW(wsz, sizeof(wsz), L"%d%%", iPos);
			SetDlgItemText(hDlg, IDC_OPACITY_LABEL, wsz);

			BackgroundSetOpacity(iPos);
			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hDlg, IDOK);
					return TRUE;
				case IDCANCEL:
				{
					OpacityDlgState *pState = (OpacityDlgState*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
					if (pState)
						BackgroundSetOpacity(pState->iOriginal);
					EndDialog(hDlg, IDCANCEL);
					return TRUE;
				}
			}
			break;
	}
	return FALSE;
}

//##########################################################
// Growable DWORD-aligned byte buffer for building a dialog
// template in memory
//##########################################################
struct AlignedBuffer
{
	BYTE *p;
	void Align() { p = (BYTE*)(((ULONG_PTR)p + 3) & ~3); }
	void Write(const void *src, size_t cb) { memcpy(p, src, cb); p += cb; }
	template<typename T> void WriteVal(T v) { Write(&v, sizeof(v)); }
	void WriteString(const wchar_t *s) { Write(s, (wcslen(s) + 1) * sizeof(wchar_t)); }
};

//##########################################################
// Builds a minimal dialog template in memory:
// caption + static label + trackbar + OK/Cancel
//##########################################################
static void OpacityDlgShow(HWND hWndOwner)
{
	BYTE buf[2048];
	AlignedBuffer b = { buf };

	// DLGTEMPLATE
	DLGTEMPLATE dt = {};
	dt.style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_SETFONT;
	dt.dwExtendedStyle = 0;
	dt.cdit = 4;
	dt.x = 0; dt.y = 0;
	dt.cx = 240; dt.cy = 62;
	b.WriteVal(dt);
	b.WriteVal((WORD)0);			// menu: none
	b.WriteVal((WORD)0);			// class: default dialog
	b.WriteString(L"背景不透明度");	// title
	b.WriteVal((WORD)9);			// font size
	b.WriteString(L"Microsoft YaHei UI");

	auto WriteItem = [&b](DWORD style, short x, short y, short cx, short cy, WORD id,
		const wchar_t *wszClass, const wchar_t *wszText)
	{
		b.Align();
		DLGITEMTEMPLATE di = {};
		di.style = style;
		di.dwExtendedStyle = 0;
		di.x = x; di.y = y; di.cx = cx; di.cy = cy;
		di.id = id;
		b.WriteVal(di);
		b.WriteString(wszClass);
		b.WriteString(wszText);
		b.WriteVal((WORD)0);		// no creation data
	};

	WriteItem(WS_CHILD | WS_VISIBLE | SS_LEFT, 7, 7, 180, 10, IDC_OPACITY_LABEL, L"Static", L"100%");
	WriteItem(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS, 4, 17, 232, 20, IDC_OPACITY_TRACK, L"msctls_trackbar32", L"");
	WriteItem(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 122, 42, 55, 14, IDOK, L"Button", L"确定");
	WriteItem(WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 182, 42, 55, 14, IDCANCEL, L"Button", L"取消");

	OpacityDlgState state;
	state.iOriginal = g_iBgOpacity;

	DialogBoxIndirectParam(g_hInstance, (LPCDLGTEMPLATE)buf, hWndOwner, OpacityDlgProc, (LPARAM)&state);
}

void BackgroundShowOpacityDialog(HWND hWndOwner)
{
	OpacityDlgShow(hWndOwner);
}
