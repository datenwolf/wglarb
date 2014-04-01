#include <windows.h>
#include <wingdi.h>

#include <stdio.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glext.h>

#include <wglarb.h>

LRESULT CALLBACK ViewProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

void reshape(int w,int h);
void display();

HWND    hWnd;
HDC     hDC;
HGLRC   hRC;

int win_width;
int win_height;

BOOL OpenGLWindowCreate(
	LPCTSTR   lpszWindowName,
	LPCTSTR   lpszClassName,
	WNDPROC   lpfnWndProc,
	HINSTANCE hInstance,
	DWORD     dwStyle,
	DWORD     dwExStyle )
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

	fprintf(stderr, "creating proper window...\n");
	hWnd = CreateWindowEx(	dwExStyle,
				wc.lpszClassName,
				lpszWindowName,
				dwStyle,
				0,0,100,100,
				NULL,NULL,
				hInstance,
				NULL);

	if(!hWnd) {
		return FALSE;
	}

	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)ViewProc);

	fprintf(stderr, "retrieving proper DC...\n");
	hDC = GetDC(hWnd);
	if(!hDC) {
		fprintf(stderr, "error retrieving proper DC\n");
		DestroyWindow(hWnd);
		return FALSE;
	}

	int attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, TRUE,
		WGL_DOUBLE_BUFFER_ARB, TRUE,
		WGL_SUPPORT_OPENGL_ARB, TRUE, 
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, 24,
		WGL_RED_BITS_ARB, 8,
		WGL_GREEN_BITS_ARB, 8,
		WGL_BLUE_BITS_ARB, 8,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0, 0
	};

	INT iPF;
	PIXELFORMATDESCRIPTOR pfd;
	fprintf(stderr, "choosing proper pixelformat...\n");
	UINT num_formats_choosen;
	if( !wglarb_ChoosePixelFormatARB(
			hDC, 
			attribs, 
			NULL,
			1,
			&iPF,
			&num_formats_choosen) ) {
		fprintf(stderr, "error choosing proper pixel format\n");
		return FALSE;
	}

	/* now this is a kludge; we need to pass something in the PIXELFORMATDESCRIPTOR 
	 * to SetPixelFormat; it will be ignored, mostly. OTOH we want to send something
	 * sane, we're nice people after all - it doesn't hurt if this fails. */
	DescribePixelFormat(hDC, iPF, sizeof(pfd), &pfd);

	fprintf(stderr, "setting proper pixelformat...\n");
	if( !SetPixelFormat(hDC, iPF, &pfd) ) {
		fprintf(stderr, "error setting proper pixel format\n");
		ReleaseDC(hWnd, hDC);
		DestroyWindow(hWnd);

		return FALSE;
	}

	fprintf(stderr, "creating proper OpenGL context...\n");
	int context_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		0, 0
	};
	hRC = wglarb_CreateContextAttribsARB(hDC, NULL, context_attribs);
	if(!hRC) {
		ReleaseDC(hWnd, hDC);
		DestroyWindow(hWnd);

		return FALSE;
	}

	/* Finally we can bind the proper OpenGL context to our window */
	wglMakeCurrent(hDC, hRC);

	SetLayeredWindowAttributes(hWnd, RGB(0,0,0), 0xff, LWA_ALPHA);
	UpdateWindow(hWnd);
	ShowWindow(hWnd, SW_SHOW);

	return TRUE;

fail_create_rc:
fail_set_pf:
fail_choose_pf:
	ReleaseDC(hWnd, hDC);
fail_get_dc:
	DestroyWindow(hWnd);
fail_create_wnd:

	return FALSE;
}

void OnOpenGLWindowDestroy()
{
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(hRC);
	ReleaseDC(hWnd,hDC);
	PostQuitMessage(0);
}

BOOL cursor_needs_setting = TRUE;

LRESULT CALLBACK ViewProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
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
		display();
		break;

	case WM_SIZE:
		reshape(LOWORD(lParam),HIWORD(lParam));
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


void reshape(int w,int h)
{
	win_width = w;
	win_height = h;
}


void display()
{
	float const ratio = (float)win_width/(float)win_height;
	glViewport(
		0,
		0,
		win_width,
		win_height);

	glClearColor(0., 0., 0., 0.25);
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

	if( !OpenGLWindowCreate(
			"Test", "TestWnd",
			ViewProc,
			hInstance,
			WS_OVERLAPPEDWINDOW,
			WS_EX_APPWINDOW | WS_EX_LAYERED) ) {
		return -1;
	}

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
