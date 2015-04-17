#include <windows.h>
#include <wingdi.h>

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <wglarb.h>
#include "dwm_load.h"

LRESULT CALLBACK ViewProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

void display(HWND hWnd);

HGLRC   hRC  = NULL;

BOOL use_dwm = FALSE;
float bg_color[3];

int win_width;
int win_height;

HWND OpenGLWindowCreate(
	LPCTSTR   lpszWindowName,
	LPCTSTR   lpszClassName,
	WNDPROC   lpfnWndProc,
	HINSTANCE hInstance,
	DWORD     dwStyle,
	DWORD     dwExStyle,
	HWND      hWndParent)
{
	if(!hInstance) {
		hInstance = GetModuleHandle(NULL);
	}

	WNDCLASS wc;
	memset(&wc,0,sizeof(wc));
	wc.hInstance = hInstance;
	wc.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;

	/* Register DefWindowProc with each the intermediary and the
	 * proper window class. We're setting the window proc of the
	 * proper window to the user supplied window proc later so that
	 * we can use the default proper window class name if no class
	 * name was supplied by the user. It also allows to use the same
	 * user supplied window class name with multiple, different
	 * window functions. */
	wc.lpfnWndProc = DefWindowProc;
	/* register proper window class */
	if( !lpszClassName ) {
		return FALSE;
	}
	wc.lpszClassName = lpszClassName;
	RegisterClass(&wc);

	RECT rect;
	if( hWndParent
	 && (dwStyle & WS_CHILD) ) {
		GetClientRect(hWndParent, &rect);
	}
	else {
		rect.left   =
		rect.top    =
		rect.right  =
		rect.bottom = CW_USEDEFAULT;
	}

	HWND hWnd =
		CreateWindowEx(
			dwExStyle,
			wc.lpszClassName,
			lpszWindowName,
			dwStyle,
			rect.left,
			rect.top,
			rect.right,
			rect.bottom,
			hWndParent,
			NULL,
			hInstance,
			NULL);
	if(!hWnd) {
		return NULL;
	}

	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)ViewProc);

	HDC hDC = GetDC(hWnd);
	if(!hDC) {
		fprintf(stderr, "error retrieving proper DC\n");
		DestroyWindow(hWnd);
		return NULL;
	}

	int attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, TRUE,
		WGL_DOUBLE_BUFFER_ARB, TRUE,
		WGL_SUPPORT_OPENGL_ARB, TRUE, 
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_TRANSPARENT_ARB, TRUE,
		WGL_COLOR_BITS_ARB, 32,
		WGL_RED_BITS_ARB, 8,
		WGL_GREEN_BITS_ARB, 8,
		WGL_BLUE_BITS_ARB, 8,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0, 0
	};

	INT iPF;
	UINT num_formats_choosen;
	if( !wglarb_ChoosePixelFormatARB(
			hDC, 
			attribs, 
			NULL,
			1,
			&iPF,
			&num_formats_choosen) ) {
		fprintf(stderr, "error choosing proper pixel format\n");
		return NULL;
	}
	if( !num_formats_choosen ) {
		return NULL;
	}

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	/* now this is a kludge; we need to pass something in the PIXELFORMATDESCRIPTOR 
	 * to SetPixelFormat; it will be ignored, mostly. OTOH we want to send something
	 * sane, we're nice people after all - it doesn't hurt if this fails. */
	DescribePixelFormat(hDC, iPF, sizeof(pfd), &pfd);

	if( !SetPixelFormat(hDC, iPF, &pfd) ) {
		fprintf(stderr, "error setting proper pixel format\n");
		ReleaseDC(hWnd, hDC);
		DestroyWindow(hWnd);

		return NULL;
	}

	int context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		0, 0
	};
	hRC = wglarb_CreateContextAttribsARB(hDC, NULL, context_attribs);
	if(!hRC) {
		ReleaseDC(hWnd, hDC);
		DestroyWindow(hWnd);

		return NULL;
	}
	ReleaseDC(hWnd, hDC);

	if( use_dwm ) {
		DWM_BLURBEHIND bb = {0};
		bb.dwFlags = DWM_BB_ENABLE;
		bb.fEnable = TRUE;
		bb.hRgnBlur = NULL;
		DwmEnableBlurBehindWindow(hWnd, &bb);

		MARGINS margins = {-1};
		DwmExtendFrameIntoClientArea(hWnd, &margins);
	}

	return hWnd;

fail_create_rc:
fail_set_pf:
fail_choose_pf:
	ReleaseDC(hWnd, hDC);
fail_get_dc:
	DestroyWindow(hWnd);
fail_create_wnd:

	return NULL;
}

void OnOpenGLWindowDestroy()
{
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(hRC);
	PostQuitMessage(0);
}

BOOL cursor_needs_setting = TRUE;

LRESULT CALLBACK ViewProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	DWORD bgcol;
	BOOL opaque;
	switch(uMsg)
	{
	case WM_CREATE:
	case WM_DWMNCRENDERINGCHANGED:
		if( use_dwm ) {
			DwmGetColorizationColor(&bgcol, &opaque);
			if( opaque ) {
				bg_color[0] = (float)GetRValue(bgcol)/255.f;
				bg_color[1] = (float)GetGValue(bgcol)/255.f;
				bg_color[2] = (float)GetBValue(bgcol)/255.f;
			}
			else {
				bg_color[0] = 0.f;
				bg_color[1] = 0.f;
				bg_color[2] = 0.f;
			}
		}
		break;

	case WM_MOUSELEAVE:
		cursor_needs_setting = TRUE;
		break;

	case WM_MOUSEMOVE:
		if( cursor_needs_setting ) {
			SetClassLongPtr(hWnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
			cursor_needs_setting = FALSE;
		}

		break;

	case WM_DESTROY:
		OnOpenGLWindowDestroy();
		break;

	case WM_PAINT:
		display(hWnd);
		break;

	default:
		break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

void display(HWND hWnd)
{
	HDC hDC = GetDC(hWnd);
	RECT rect;
	GetClientRect(hWnd, &rect);

	wglMakeCurrent(hDC, hRC);

	float const ratio = (float)rect.right/(float)rect.bottom;
	glViewport(
		0,
		0,
		rect.right,
		rect.bottom);

	glClearColor(
		bg_color[0],
		bg_color[1],
		bg_color[2],
		0.0);
	glClearDepth(1.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-ratio, ratio, -1., 1., -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GLfloat const gradient_blend[] = {
		-1., -1., 0., 0., 0., 1.,
		 1., -1., 0., 0., 0., 1.,

		-1., 0.95, 0., 0., 0., 1.,
		 1., 0.95, 0., 0., 0., 1.,

		-1., 1., bg_color[0], bg_color[1], bg_color[2], 0.,
		 1., 1., bg_color[0], bg_color[1], bg_color[2], 0.,
	};

	float const cos60 = cosf(M_PI*60./180.);
	float const sin60 = sinf(M_PI*60./180.);

	GLfloat const triangle[] = {
		-1., -sin60, 1., 0., 0.,
		 1., -sin60, 0., 1., 0.,
		 0.,  sin60, 0., 0., 1.
	};

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(2, GL_FLOAT, sizeof(GLfloat)*5, &triangle[0]);
	glColorPointer( 3, GL_FLOAT, sizeof(GLfloat)*5, &triangle[0]);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	SwapBuffers(hDC);
	glFinish();

	wglMakeCurrent(NULL, NULL);
	ReleaseDC(hWnd, hDC);
}

#if 1
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
#else
int main(int argc, char *argv[])
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
#endif
	MSG msg;
	BOOL bRet;

	OSVERSIONINFO os_vinfo;
	memset(&os_vinfo, 0, sizeof(os_vinfo));
	os_vinfo.dwOSVersionInfoSize = sizeof(os_vinfo);
	GetVersionEx(&os_vinfo);
	use_dwm = 6 <= os_vinfo.dwMajorVersion;

	HWND hWndGL = OpenGLWindowCreate(
			"Custom Frame Window", "CustFrmWnd",
			ViewProc,
			hInstance,
			WS_OVERLAPPEDWINDOW,
			WS_EX_APPWINDOW,
			NULL);
	if(!hWndGL) {
		return -1;
	}
	UpdateWindow(hWndGL);
	ShowWindow(hWndGL, SW_SHOW);

	while( (bRet = GetMessage(&msg, NULL, 0, 0)) ) {
		if(bRet == -1) {
			// Handle Error
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}
