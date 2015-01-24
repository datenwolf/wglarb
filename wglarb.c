/*
Copyright (c) 2014 Wolfgang 'datenwolf' Draxinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <GL/gl.h>
#include "wglarb.h"

#define wglarb_BuildAssert(cond) ((void)sizeof(char[1 - 2*!(cond)]))

static HANDLE wglarb_intermediary_mutex = NULL;

static DWORD wglarb_intermediary_lock(void)
{
	wglarb_BuildAssert( sizeof(PVOID) == sizeof(HANDLE) );

	if( !wglarb_intermediary_mutex ) {
		/* Between testing for the validity of the mutex handle,
		 * creating a new mutex handle and using the interlocked
		 * exchange there is a race... */

		/* //// START \\\\ */

		HANDLE const new_mutex =
			CreateMutex(NULL, TRUE, NULL);

		HANDLE const dst_mutex =
			InterlockedCompareExchangePointer(
				&wglarb_intermediary_mutex,
				new_mutex,
				NULL );

		/* //// FINISH \\\\ */

		if( !dst_mutex ) {
			/* mutex created in one time initialization and held
			 * by calling thread. Return signaled status. */
			return WAIT_OBJECT_0;
		}
		/* In this case we lost the race and another thread
		 * beat this thread in creating a mutex object.
		 * Clean up and wait for the proper mutex. */
		ReleaseMutex(new_mutex);
		CloseHandle(new_mutex);
	}
	return WaitForSingleObject(wglarb_intermediary_mutex, INFINITE);
}

static BOOL wglarb_intermediary_unlock(void)
{
	return ReleaseMutex(wglarb_intermediary_mutex);
}

#define WGLARB_INTERMEDIARY_CLASS	"wglarb intermediary"
#define WGLARB_INTERMEDIARY_STYLE	(WS_CLIPSIBLINGS|WS_CLIPCHILDREN)
#define WGLARB_INTERMEDIARY_EXSTYLE	0

static HWND wglarb_intermediary_hWnd = 0;

static BOOL wglarb_intermediary_create_Wnd(void)
{
	HINSTANCE const hInstance = GetModuleHandle(NULL);

	WNDCLASS wc;
	memset(&wc,0,sizeof(wc));
	wc.hInstance = hInstance;
	wc.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = WGLARB_INTERMEDIARY_CLASS;
	RegisterClass(&wc);

	wglarb_intermediary_hWnd =
		CreateWindowEx(
			WGLARB_INTERMEDIARY_EXSTYLE,
			WGLARB_INTERMEDIARY_CLASS,
			NULL,
			WGLARB_INTERMEDIARY_STYLE,
			0,0,0,0,
			NULL,NULL,
			hInstance,
			NULL );

	if( !wglarb_intermediary_hWnd ) {
		FALSE;
	}

	return TRUE;
}

static HDC wglarb_intermediary_hDC = 0;

static BOOL wglarb_intermediary_create_DC(void)
{
	if( !wglarb_intermediary_hWnd
	 && !wglarb_intermediary_create_Wnd() ) {
		return FALSE;
	}

	wglarb_intermediary_hDC = GetDC(wglarb_intermediary_hWnd);
	if( !wglarb_intermediary_hDC ) {
		return FALSE;
	}
	
	return TRUE;
}

static HGLRC wglarb_intermediary_hRC = 0;

static BOOL wglarb_intermediary_create_RC(void)
{
	if( !wglarb_intermediary_hDC
	 && !wglarb_intermediary_create_DC() ) {
		return FALSE;
	}

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd,0,sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL|PFD_GENERIC_ACCELERATED|PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;

	int iPF;
	if( !(iPF = ChoosePixelFormat(wglarb_intermediary_hDC, &pfd)) ) {
		return FALSE;
	}

	if( !SetPixelFormat(wglarb_intermediary_hDC, iPF, &pfd) ) {
		return FALSE;
	}

	if( !(wglarb_intermediary_hRC = wglCreateContext(wglarb_intermediary_hDC)) ) {
		return FALSE;
	}

	return TRUE;
}

static BOOL wglarb_intermediary_makecurrent(HDC *hOrigDC, HGLRC *hOrigRC)
{
	*hOrigDC = wglGetCurrentDC();
	*hOrigRC = wglGetCurrentContext();

	if( !wglarb_intermediary_hRC
	 && !wglarb_intermediary_create_RC() )	{
		return FALSE;
	}

	return wglMakeCurrent(wglarb_intermediary_hDC, wglarb_intermediary_hRC);
}

HGLRC WINAPI wglarb_CreateContextAttribsARB(
	HDC hDC,
	HGLRC hShareContext,
	const int *attribList)
{
	if( WAIT_OBJECT_0 != wglarb_intermediary_lock() ) {
		return NULL;
	}

	HDC   hOrigDC;
	HGLRC hOrigRC;
	if( !wglarb_intermediary_makecurrent(&hOrigDC, &hOrigRC) ) {
		wglarb_intermediary_unlock();
		return NULL;
	}

	PFNWGLCREATECONTEXTATTRIBSARBPROC impl =
		(PFNWGLCREATECONTEXTATTRIBSARBPROC)
			wglGetProcAddress("wglCreateContextAttribsARB");
	
	HGLRC ret = NULL;
	if( impl ) {
		ret = impl(hDC, hShareContext, attribList);
	}	
	
	wglMakeCurrent(hOrigDC, hOrigRC);
	wglarb_intermediary_unlock();
	return ret;
}

BOOL WINAPI wglarb_ChoosePixelFormatARB(
	HDC hdc,
	const int *piAttribIList,
	const FLOAT *pfAttribFList,
	UINT nMaxFormats,
	int *piFormats,
	UINT *nNumFormats)
{
	if( WAIT_OBJECT_0 != wglarb_intermediary_lock() ) {
		return FALSE;
	}

	HDC   hOrigDC;
	HGLRC hOrigRC;
	if( !wglarb_intermediary_makecurrent(&hOrigDC, &hOrigRC) ) {
		wglarb_intermediary_unlock();
		return FALSE;
	}

	PFNWGLCHOOSEPIXELFORMATARBPROC impl = NULL;

	impl = (PFNWGLCHOOSEPIXELFORMATARBPROC)
			wglGetProcAddress("wglChoosePixelFormatARB");
	if( !impl ) {
		/* WGL_EXT_pixel_format uses the same function prototypes
		 * as the WGL_ARB_pixel_format extension */
		impl = (PFNWGLCHOOSEPIXELFORMATARBPROC)
				wglGetProcAddress("wglChoosePixelFormatEXT");
	}
	
	BOOL ret = FALSE;
	if( impl ) {
		ret = impl(
			hdc,
			piAttribIList,
			pfAttribFList,
			nMaxFormats,
			piFormats,
			nNumFormats );
	}	
	
	wglMakeCurrent(hOrigDC, hOrigRC);
	wglarb_intermediary_unlock();
	return ret;
}
