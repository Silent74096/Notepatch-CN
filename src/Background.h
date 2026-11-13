// Background.h - Custom background (image / color) support for the editor
#pragma once

#include <windows.h>

// Background state (persisted to registry by Notepatch.cpp)
extern BOOL		g_bBgImageEnabled;		// background image active
extern wchar_t	g_wszBgImagePath[MAX_PATH];	// path of the selected image
extern int		g_iBgOpacity;			// image blend percentage 0-100
extern BOOL		g_bCustomBgColor;		// custom editor background color active
extern COLORREF	g_crEditorBg;			// custom editor background color
extern BOOL		g_bCustomTextColor;		// custom editor text color active
extern COLORREF	g_crEditorText;			// custom editor text color

void	BackgroundInit(void);
void	BackgroundShutdown(void);

// Returns TRUE if a custom background (image or color) is active
BOOL	BackgroundIsActive(void);

// Loads an image file (via GDI+), returns FALSE on failure
BOOL	BackgroundLoadImage(const wchar_t *wszPath);

// Removes image and custom colors, restores theme defaults
void	BackgroundClear(void);

// (Re)creates the sized background bitmap + pattern brush.
// Call after image/opacity/color changes and on window resize.
void	BackgroundRebuild(void);

// Pattern brush for WM_CTLCOLOREDIT, NULL when no image is active
HBRUSH	BackgroundGetBrush(void);

COLORREF BackgroundGetEffectiveBgColor(void);
COLORREF BackgroundGetEffectiveTextColor(void);

// Shows the file-open dialog and applies the chosen image
BOOL	BackgroundChooseFile(HWND hWndOwner);

// Extracts the average color of the current image and applies it
// as editor background color (plus a readable text color)
BOOL	BackgroundExtractColors(void);

void	BackgroundSetOpacity(int iOpacity);

// Slider dialog for adjusting the image opacity
void	BackgroundShowOpacityDialog(HWND hWndOwner);

// Repaints the editor so background changes become visible
void	BackgroundRefresh(void);

// Periodically detects scrolling of the editor (keyboard, wheel,
// scrollbar) and repaints so the fixed background stays aligned
void	BackgroundCheckScroll(void);

// Timer id used for the scroll detection
#define BG_SCROLL_TIMER_ID	0xBEEF
