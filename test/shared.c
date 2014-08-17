
#include <windows.h>
#include <wingdi.h>

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <wglarb.h>

int const pf_attribs[] = {
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

int context_attribs[] = {
	WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
	WGL_CONTEXT_MINOR_VERSION_ARB, 1,
	0, 0
};

typedef struct {
	HANDLE mutex;
	HGLRC hRC;
	int refcount;
} *RefCountGLRC;

typedef struct {
	void (*display)(HWND, void*);
} OpenGLWndFuncs;

typedef struct {
	RefCountGLRC glrc;
	bool cursor_needs_setting;

	OpenGLWndFuncs wndfuncs;
	void *userdata;
} OpenGLWndData;

LRESULT CALLBACK ViewProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

void display(HWND hWnd, void *userdata);

HWND CreateWindowForOpenGL(
	HWND      hWndParent,
	LPCTSTR   lpszWindowName,
	LPCTSTR   lpszClassName,
	WNDPROC   lpfnWndProc,
	int const *pPFAttribs,
	HINSTANCE hInstance,
	DWORD     dwStyle,
	DWORD     dwExStyle,
	OpenGLWndFuncs wndfuncs,
	void      *userdata)
{
	if(!hInstance) {
		hInstance = GetModuleHandle(NULL);
	}

	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	wc.hInstance = hInstance;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

	/* Register DefWindowProc as intermediary window proc in the
	 * window class. We're setting the proper window proc of the
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
		goto fail_create_wnd;

	OpenGLWndData *wnddata = calloc(1, sizeof(*wnddata));
	if( !wnddata ) {
		goto fail_alloc_wnddata;
	}
	wnddata->wndfuncs = wndfuncs;
	wnddata->userdata = userdata;
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)wnddata);

	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)ViewProc);

	HDC hDC = GetDC(hWnd);
	if(!hDC) {
		DestroyWindow(hWnd);
		goto fail_get_dc;
	}

	INT iPF;
	UINT num_formats_choosen;
	if( !wglarb_ChoosePixelFormatARB(
			hDC, 
			pPFAttribs, 
			NULL,
			1,
			&iPF,
			&num_formats_choosen)
	 || !num_formats_choosen ) {
		goto fail_choose_pf;
	}

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	/* now this is a kludge; we need to pass something in the
	 * PIXELFORMATDESCRIPTOR to SetPixelFormat; it will be ignored,
	 * mostly. OTOH we want to send something sane, we're nice
	 * people after all - it doesn't hurt if this fails. */
	DescribePixelFormat(hDC, iPF, sizeof(pfd), &pfd);

	if( !SetPixelFormat(hDC, iPF, &pfd) ) {
		goto fail_set_pf;
	}
	ReleaseDC(hWnd, hDC);

	return hWnd;

fail_set_pf:
fail_choose_pf:
	ReleaseDC(hWnd, hDC);
fail_get_dc:
fail_alloc_wnddata:
	DestroyWindow(hWnd);
fail_create_wnd:

	return NULL;
}

bool AssociateOpenGLContextWithWindow_L(
	HWND hWnd,
	RefCountGLRC glrc)
{
	if( 0 > glrc->refcount ) {
		/* OpenGL context has been destroyed */
		return false;
	}

	if( INT_MAX == glrc->refcount ) {
		/* refcount is at integer maximum.
		 * This is very unlikely to happen though. */
		return false;
	}

	OpenGLWndData *data = (void*)GetWindowLongPtr(hWnd, GWLP_USERDATA);	
	if( !data ) {
		return false;
	}

	++ glrc->refcount;
	data->glrc = glrc;

	return true;
}

RefCountGLRC CreateOpenGLContextToWindow(
	HWND hWnd,
	int const *pGLAttribs )
{
	if( !hWnd
	 || !pGLAttribs ) {
	 	return NULL;
	}

	RefCountGLRC glrc = cmalloc(1, sizeof(*glrc));
	if( !glrc ) {
		goto fail_alloc_glrc;
	}

	glrc->mutex = CreateMutex(NULL, TRUE, NULL);
	if( !glrc->mutex ) {
		goto fail_create_mutex;
	}

	HDC hDC = GetDC(hWnd);
	if( !hDC ) {
		goto fail_get_dc;
	}

	glrc->hRC = wglarb_CreateContextAttribsARB(hDC, NULL, pGLAttribs);
	if( !glrc->hRC ) {
		goto fail_create_glrc;
	}

	if( !AssociateOpenGLContextWithWindow_L(hWnd, glrc) ) {
		goto fail_associate_rc_to_wnd;
	}

	ReleaseDC(hWnd, hDC);
	ReleaseMutex(glrc->mutex);

	return glrc;

fail_associate_rc_to_wnd:
	wglDeleteContext(glrc->hRC);

fail_create_glrc:
	ReleaseDC(hWnd, hDC);

fail_get_dc:
	ReleaseMutex(glrc->mutex);
	CloseHandle(glrc->mutex);

fail_create_mutex:
	free(glrc);

fail_alloc_glrc:
	return NULL;
}

bool AssociateOpenGLContextWithWindow(
	HWND hWnd,
	RefCountGLRC glrc)
{
	if( !hWnd
	 || !glrc ) {
		return false;
	}
	
	if( WAIT_OBJECT_0 != WaitForSingleObject(glrc->mutex, INFINITE) ) {
		return false;
	}
	bool const rv = AssociateOpenGLContextWithWindow_L(hWnd, glrc);
	ReleaeMutex(glrc->mutex);

	return rv;
}

void OnOpenGLWindowDestroy(HWND hWnd)
{
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(hRC);
	PostQuitMessage(0);
}

BOOL cursor_needs_setting = TRUE;

LRESULT CALLBACK OpenGLWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam )
{
	OpenGLWndData *data = (void*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	assert(data);

	switch(uMsg)
	{
	case WM_MOUSELEAVE:
		data->cursor_needs_setting = TRUE;
		break;

	case WM_MOUSEMOVE:
		if( data->cursor_needs_setting ) {
			SetClassLongPtr(hWnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
			data->cursor_needs_setting = FALSE;
		}

		break;

	case WM_DESTROY:
		OnOpenGLWindowDestroy(hWnd);
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);
		if( data->glrc ) {
			wglMakeCurrent(hDC, data->glrc->hRC);

			if(data->wndfuncs->display) {
				data->wndfuncs->display(hWnd, data->userdata);
			}
			else {
				glClear(GL_COLOR_BUFFER_BIT);
			}

			/* SwapBuffers causes an implicit OpenGL command queue flush,
			 * but only on double buffered windows. On single buffered
			 * windows SwapBuffers has no effect at all. Also calling
			 * glFlush right before SwapBuffers has no ill effect other
			 * than that the OpenGL queue gets flushed a few CPU cycles
			 * earlier. */
			glFlush();
			SwapBuffers(hDC); 

			wglMakeCurrent(hDC, NULL);
		}
		EndPaint(hWnd, &ps);
	} break;

	default:
		break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


void display(HWND hWnd)
{
	RECT rect;
	GetClientRect(hWnd, &rect);

	float const ratio = (float)rect.right/(float)rect.bottom;
	glViewport(
		0,
		0,
		rect.right,
		rect.bottom);

	glClearColor(0., 0., 0., 0.);
	glClearDepth(1.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-ratio, ratio, -1., 1., -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

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

}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	MSG msg;
	BOOL bRet;

	OSVERSIONINFO os_vinfo;
	memset(&os_vinfo, 0, sizeof(os_vinfo));
	os_vinfo.dwOSVersionInfoSize = sizeof(os_vinfo);
	GetVersionEx(&os_vinfo);

	HWND hWndGL = OpenGLWindowCreate(
			"Test", "TestWnd",
			ViewProc,
			hInstance,
			WS_OVERLAPPEDWINDOW
			, WS_EX_APPWINDOW,
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
