# wglarb – a small helper library for making creating OpenGL contexts with extended attributes easy

wglarb is a helper library for Windows OpenGL application development. The way
OpenGL and WGL extensions are implemented in Windows require the creation of
an intermediary OpenGL context (and usually also intermediary window) to obtain
access to the extension functions.

wglarb hides the nasty details behind a initialization-less API. Just call
`wglarb_ChoosePixelFormatARB` and `wglarb_CreateContextAttribsARB`. Initialization
of the intermediary window and OpenGL context happen on demand as needed.

**Important Notice:** 
*At the moment only ChoosePixelFormatARB and CreateContextAttribsARB are covered.*
Further wgl…ARB functions are yet subject for inclusion.
