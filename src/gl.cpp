/**
 * SPDX-License-Identifier: (WTFPL OR CC0-1.0) AND Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glad/gl.h"

#ifndef GLAD_IMPL_UTIL_C_
#define GLAD_IMPL_UTIL_C_

#ifdef _MSC_VER
#define GLAD_IMPL_UTIL_SSCANF sscanf_s
#else
#define GLAD_IMPL_UTIL_SSCANF sscanf
#endif

#endif /* GLAD_IMPL_UTIL_C_ */

#ifdef __cplusplus
extern "C" {
#endif



int GLAD_GL_VERSION_1_0 = 0;
int GLAD_GL_VERSION_1_1 = 0;
int GLAD_GL_VERSION_1_2 = 0;
int GLAD_GL_VERSION_1_3 = 0;
int GLAD_GL_VERSION_1_4 = 0;
int GLAD_GL_VERSION_1_5 = 0;
int GLAD_GL_VERSION_2_0 = 0;
int GLAD_GL_VERSION_2_1 = 0;
int GLAD_GL_VERSION_3_0 = 0;
int GLAD_GL_VERSION_3_1 = 0;
int GLAD_GL_VERSION_3_2 = 0;
int GLAD_GL_VERSION_3_3 = 0;
int GLAD_GL_VERSION_4_0 = 0;
int GLAD_GL_VERSION_4_1 = 0;
int GLAD_GL_VERSION_4_2 = 0;
int GLAD_GL_VERSION_4_3 = 0;
int GLAD_GL_EXT_texture_filter_anisotropic = 0;


struct GLADPTRS gladptrs{};


static void glad_gl_load_GL_VERSION_1_0( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_0) return;
    gladptrs.glad_glAccum = (PFNGLACCUMPROC) load(userptr, "glAccum");
    gladptrs.glad_glAlphaFunc = (PFNGLALPHAFUNCPROC) load(userptr, "glAlphaFunc");
    gladptrs.glad_glBegin = (PFNGLBEGINPROC) load(userptr, "glBegin");
    gladptrs.glad_glBitmap = (PFNGLBITMAPPROC) load(userptr, "glBitmap");
    gladptrs.glad_glBlendFunc = (PFNGLBLENDFUNCPROC) load(userptr, "glBlendFunc");
    gladptrs.glad_glCallList = (PFNGLCALLLISTPROC) load(userptr, "glCallList");
    gladptrs.glad_glCallLists = (PFNGLCALLLISTSPROC) load(userptr, "glCallLists");
    gladptrs.glad_glClear = (PFNGLCLEARPROC) load(userptr, "glClear");
    gladptrs.glad_glClearAccum = (PFNGLCLEARACCUMPROC) load(userptr, "glClearAccum");
    gladptrs.glad_glClearColor = (PFNGLCLEARCOLORPROC) load(userptr, "glClearColor");
    gladptrs.glad_glClearDepth = (PFNGLCLEARDEPTHPROC) load(userptr, "glClearDepth");
    gladptrs.glad_glClearIndex = (PFNGLCLEARINDEXPROC) load(userptr, "glClearIndex");
    gladptrs.glad_glClearStencil = (PFNGLCLEARSTENCILPROC) load(userptr, "glClearStencil");
    gladptrs.glad_glClipPlane = (PFNGLCLIPPLANEPROC) load(userptr, "glClipPlane");
    gladptrs.glad_glColor3b = (PFNGLCOLOR3BPROC) load(userptr, "glColor3b");
    gladptrs.glad_glColor3bv = (PFNGLCOLOR3BVPROC) load(userptr, "glColor3bv");
    gladptrs.glad_glColor3d = (PFNGLCOLOR3DPROC) load(userptr, "glColor3d");
    gladptrs.glad_glColor3dv = (PFNGLCOLOR3DVPROC) load(userptr, "glColor3dv");
    gladptrs.glad_glColor3f = (PFNGLCOLOR3FPROC) load(userptr, "glColor3f");
    gladptrs.glad_glColor3fv = (PFNGLCOLOR3FVPROC) load(userptr, "glColor3fv");
    gladptrs.glad_glColor3i = (PFNGLCOLOR3IPROC) load(userptr, "glColor3i");
    gladptrs.glad_glColor3iv = (PFNGLCOLOR3IVPROC) load(userptr, "glColor3iv");
    gladptrs.glad_glColor3s = (PFNGLCOLOR3SPROC) load(userptr, "glColor3s");
    gladptrs.glad_glColor3sv = (PFNGLCOLOR3SVPROC) load(userptr, "glColor3sv");
    gladptrs.glad_glColor3ub = (PFNGLCOLOR3UBPROC) load(userptr, "glColor3ub");
    gladptrs.glad_glColor3ubv = (PFNGLCOLOR3UBVPROC) load(userptr, "glColor3ubv");
    gladptrs.glad_glColor3ui = (PFNGLCOLOR3UIPROC) load(userptr, "glColor3ui");
    gladptrs.glad_glColor3uiv = (PFNGLCOLOR3UIVPROC) load(userptr, "glColor3uiv");
    gladptrs.glad_glColor3us = (PFNGLCOLOR3USPROC) load(userptr, "glColor3us");
    gladptrs.glad_glColor3usv = (PFNGLCOLOR3USVPROC) load(userptr, "glColor3usv");
    gladptrs.glad_glColor4b = (PFNGLCOLOR4BPROC) load(userptr, "glColor4b");
    gladptrs.glad_glColor4bv = (PFNGLCOLOR4BVPROC) load(userptr, "glColor4bv");
    gladptrs.glad_glColor4d = (PFNGLCOLOR4DPROC) load(userptr, "glColor4d");
    gladptrs.glad_glColor4dv = (PFNGLCOLOR4DVPROC) load(userptr, "glColor4dv");
    gladptrs.glad_glColor4f = (PFNGLCOLOR4FPROC) load(userptr, "glColor4f");
    gladptrs.glad_glColor4fv = (PFNGLCOLOR4FVPROC) load(userptr, "glColor4fv");
    gladptrs.glad_glColor4i = (PFNGLCOLOR4IPROC) load(userptr, "glColor4i");
    gladptrs.glad_glColor4iv = (PFNGLCOLOR4IVPROC) load(userptr, "glColor4iv");
    gladptrs.glad_glColor4s = (PFNGLCOLOR4SPROC) load(userptr, "glColor4s");
    gladptrs.glad_glColor4sv = (PFNGLCOLOR4SVPROC) load(userptr, "glColor4sv");
    gladptrs.glad_glColor4ub = (PFNGLCOLOR4UBPROC) load(userptr, "glColor4ub");
    gladptrs.glad_glColor4ubv = (PFNGLCOLOR4UBVPROC) load(userptr, "glColor4ubv");
    gladptrs.glad_glColor4ui = (PFNGLCOLOR4UIPROC) load(userptr, "glColor4ui");
    gladptrs.glad_glColor4uiv = (PFNGLCOLOR4UIVPROC) load(userptr, "glColor4uiv");
    gladptrs.glad_glColor4us = (PFNGLCOLOR4USPROC) load(userptr, "glColor4us");
    gladptrs.glad_glColor4usv = (PFNGLCOLOR4USVPROC) load(userptr, "glColor4usv");
    gladptrs.glad_glColorMask = (PFNGLCOLORMASKPROC) load(userptr, "glColorMask");
    gladptrs.glad_glColorMaterial = (PFNGLCOLORMATERIALPROC) load(userptr, "glColorMaterial");
    gladptrs.glad_glCopyPixels = (PFNGLCOPYPIXELSPROC) load(userptr, "glCopyPixels");
    gladptrs.glad_glCullFace = (PFNGLCULLFACEPROC) load(userptr, "glCullFace");
    gladptrs.glad_glDeleteLists = (PFNGLDELETELISTSPROC) load(userptr, "glDeleteLists");
    gladptrs.glad_glDepthFunc = (PFNGLDEPTHFUNCPROC) load(userptr, "glDepthFunc");
    gladptrs.glad_glDepthMask = (PFNGLDEPTHMASKPROC) load(userptr, "glDepthMask");
    gladptrs.glad_glDepthRange = (PFNGLDEPTHRANGEPROC) load(userptr, "glDepthRange");
    gladptrs.glad_glDisable = (PFNGLDISABLEPROC) load(userptr, "glDisable");
    gladptrs.glad_glDrawBuffer = (PFNGLDRAWBUFFERPROC) load(userptr, "glDrawBuffer");
    gladptrs.glad_glDrawPixels = (PFNGLDRAWPIXELSPROC) load(userptr, "glDrawPixels");
    gladptrs.glad_glEdgeFlag = (PFNGLEDGEFLAGPROC) load(userptr, "glEdgeFlag");
    gladptrs.glad_glEdgeFlagv = (PFNGLEDGEFLAGVPROC) load(userptr, "glEdgeFlagv");
    gladptrs.glad_glEnable = (PFNGLENABLEPROC) load(userptr, "glEnable");
    gladptrs.glad_glEnd = (PFNGLENDPROC) load(userptr, "glEnd");
    gladptrs.glad_glEndList = (PFNGLENDLISTPROC) load(userptr, "glEndList");
    gladptrs.glad_glEvalCoord1d = (PFNGLEVALCOORD1DPROC) load(userptr, "glEvalCoord1d");
    gladptrs.glad_glEvalCoord1dv = (PFNGLEVALCOORD1DVPROC) load(userptr, "glEvalCoord1dv");
    gladptrs.glad_glEvalCoord1f = (PFNGLEVALCOORD1FPROC) load(userptr, "glEvalCoord1f");
    gladptrs.glad_glEvalCoord1fv = (PFNGLEVALCOORD1FVPROC) load(userptr, "glEvalCoord1fv");
    gladptrs.glad_glEvalCoord2d = (PFNGLEVALCOORD2DPROC) load(userptr, "glEvalCoord2d");
    gladptrs.glad_glEvalCoord2dv = (PFNGLEVALCOORD2DVPROC) load(userptr, "glEvalCoord2dv");
    gladptrs.glad_glEvalCoord2f = (PFNGLEVALCOORD2FPROC) load(userptr, "glEvalCoord2f");
    gladptrs.glad_glEvalCoord2fv = (PFNGLEVALCOORD2FVPROC) load(userptr, "glEvalCoord2fv");
    gladptrs.glad_glEvalMesh1 = (PFNGLEVALMESH1PROC) load(userptr, "glEvalMesh1");
    gladptrs.glad_glEvalMesh2 = (PFNGLEVALMESH2PROC) load(userptr, "glEvalMesh2");
    gladptrs.glad_glEvalPoint1 = (PFNGLEVALPOINT1PROC) load(userptr, "glEvalPoint1");
    gladptrs.glad_glEvalPoint2 = (PFNGLEVALPOINT2PROC) load(userptr, "glEvalPoint2");
    gladptrs.glad_glFeedbackBuffer = (PFNGLFEEDBACKBUFFERPROC) load(userptr, "glFeedbackBuffer");
    gladptrs.glad_glFinish = (PFNGLFINISHPROC) load(userptr, "glFinish");
    gladptrs.glad_glFlush = (PFNGLFLUSHPROC) load(userptr, "glFlush");
    gladptrs.glad_glFogf = (PFNGLFOGFPROC) load(userptr, "glFogf");
    gladptrs.glad_glFogfv = (PFNGLFOGFVPROC) load(userptr, "glFogfv");
    gladptrs.glad_glFogi = (PFNGLFOGIPROC) load(userptr, "glFogi");
    gladptrs.glad_glFogiv = (PFNGLFOGIVPROC) load(userptr, "glFogiv");
    gladptrs.glad_glFrontFace = (PFNGLFRONTFACEPROC) load(userptr, "glFrontFace");
    gladptrs.glad_glFrustum = (PFNGLFRUSTUMPROC) load(userptr, "glFrustum");
    gladptrs.glad_glGenLists = (PFNGLGENLISTSPROC) load(userptr, "glGenLists");
    gladptrs.glad_glGetBooleanv = (PFNGLGETBOOLEANVPROC) load(userptr, "glGetBooleanv");
    gladptrs.glad_glGetClipPlane = (PFNGLGETCLIPPLANEPROC) load(userptr, "glGetClipPlane");
    gladptrs.glad_glGetDoublev = (PFNGLGETDOUBLEVPROC) load(userptr, "glGetDoublev");
    gladptrs.glad_glGetError = (PFNGLGETERRORPROC) load(userptr, "glGetError");
    gladptrs.glad_glGetFloatv = (PFNGLGETFLOATVPROC) load(userptr, "glGetFloatv");
    gladptrs.glad_glGetIntegerv = (PFNGLGETINTEGERVPROC) load(userptr, "glGetIntegerv");
    gladptrs.glad_glGetLightfv = (PFNGLGETLIGHTFVPROC) load(userptr, "glGetLightfv");
    gladptrs.glad_glGetLightiv = (PFNGLGETLIGHTIVPROC) load(userptr, "glGetLightiv");
    gladptrs.glad_glGetMapdv = (PFNGLGETMAPDVPROC) load(userptr, "glGetMapdv");
    gladptrs.glad_glGetMapfv = (PFNGLGETMAPFVPROC) load(userptr, "glGetMapfv");
    gladptrs.glad_glGetMapiv = (PFNGLGETMAPIVPROC) load(userptr, "glGetMapiv");
    gladptrs.glad_glGetMaterialfv = (PFNGLGETMATERIALFVPROC) load(userptr, "glGetMaterialfv");
    gladptrs.glad_glGetMaterialiv = (PFNGLGETMATERIALIVPROC) load(userptr, "glGetMaterialiv");
    gladptrs.glad_glGetPixelMapfv = (PFNGLGETPIXELMAPFVPROC) load(userptr, "glGetPixelMapfv");
    gladptrs.glad_glGetPixelMapuiv = (PFNGLGETPIXELMAPUIVPROC) load(userptr, "glGetPixelMapuiv");
    gladptrs.glad_glGetPixelMapusv = (PFNGLGETPIXELMAPUSVPROC) load(userptr, "glGetPixelMapusv");
    gladptrs.glad_glGetPolygonStipple = (PFNGLGETPOLYGONSTIPPLEPROC) load(userptr, "glGetPolygonStipple");
    gladptrs.glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    gladptrs.glad_glGetTexEnvfv = (PFNGLGETTEXENVFVPROC) load(userptr, "glGetTexEnvfv");
    gladptrs.glad_glGetTexEnviv = (PFNGLGETTEXENVIVPROC) load(userptr, "glGetTexEnviv");
    gladptrs.glad_glGetTexGendv = (PFNGLGETTEXGENDVPROC) load(userptr, "glGetTexGendv");
    gladptrs.glad_glGetTexGenfv = (PFNGLGETTEXGENFVPROC) load(userptr, "glGetTexGenfv");
    gladptrs.glad_glGetTexGeniv = (PFNGLGETTEXGENIVPROC) load(userptr, "glGetTexGeniv");
    gladptrs.glad_glGetTexImage = (PFNGLGETTEXIMAGEPROC) load(userptr, "glGetTexImage");
    gladptrs.glad_glGetTexLevelParameterfv = (PFNGLGETTEXLEVELPARAMETERFVPROC) load(userptr, "glGetTexLevelParameterfv");
    gladptrs.glad_glGetTexLevelParameteriv = (PFNGLGETTEXLEVELPARAMETERIVPROC) load(userptr, "glGetTexLevelParameteriv");
    gladptrs.glad_glGetTexParameterfv = (PFNGLGETTEXPARAMETERFVPROC) load(userptr, "glGetTexParameterfv");
    gladptrs.glad_glGetTexParameteriv = (PFNGLGETTEXPARAMETERIVPROC) load(userptr, "glGetTexParameteriv");
    gladptrs.glad_glHint = (PFNGLHINTPROC) load(userptr, "glHint");
    gladptrs.glad_glIndexMask = (PFNGLINDEXMASKPROC) load(userptr, "glIndexMask");
    gladptrs.glad_glIndexd = (PFNGLINDEXDPROC) load(userptr, "glIndexd");
    gladptrs.glad_glIndexdv = (PFNGLINDEXDVPROC) load(userptr, "glIndexdv");
    gladptrs.glad_glIndexf = (PFNGLINDEXFPROC) load(userptr, "glIndexf");
    gladptrs.glad_glIndexfv = (PFNGLINDEXFVPROC) load(userptr, "glIndexfv");
    gladptrs.glad_glIndexi = (PFNGLINDEXIPROC) load(userptr, "glIndexi");
    gladptrs.glad_glIndexiv = (PFNGLINDEXIVPROC) load(userptr, "glIndexiv");
    gladptrs.glad_glIndexs = (PFNGLINDEXSPROC) load(userptr, "glIndexs");
    gladptrs.glad_glIndexsv = (PFNGLINDEXSVPROC) load(userptr, "glIndexsv");
    gladptrs.glad_glInitNames = (PFNGLINITNAMESPROC) load(userptr, "glInitNames");
    gladptrs.glad_glIsEnabled = (PFNGLISENABLEDPROC) load(userptr, "glIsEnabled");
    gladptrs.glad_glIsList = (PFNGLISLISTPROC) load(userptr, "glIsList");
    gladptrs.glad_glLightModelf = (PFNGLLIGHTMODELFPROC) load(userptr, "glLightModelf");
    gladptrs.glad_glLightModelfv = (PFNGLLIGHTMODELFVPROC) load(userptr, "glLightModelfv");
    gladptrs.glad_glLightModeli = (PFNGLLIGHTMODELIPROC) load(userptr, "glLightModeli");
    gladptrs.glad_glLightModeliv = (PFNGLLIGHTMODELIVPROC) load(userptr, "glLightModeliv");
    gladptrs.glad_glLightf = (PFNGLLIGHTFPROC) load(userptr, "glLightf");
    gladptrs.glad_glLightfv = (PFNGLLIGHTFVPROC) load(userptr, "glLightfv");
    gladptrs.glad_glLighti = (PFNGLLIGHTIPROC) load(userptr, "glLighti");
    gladptrs.glad_glLightiv = (PFNGLLIGHTIVPROC) load(userptr, "glLightiv");
    gladptrs.glad_glLineStipple = (PFNGLLINESTIPPLEPROC) load(userptr, "glLineStipple");
    gladptrs.glad_glLineWidth = (PFNGLLINEWIDTHPROC) load(userptr, "glLineWidth");
    gladptrs.glad_glListBase = (PFNGLLISTBASEPROC) load(userptr, "glListBase");
    gladptrs.glad_glLoadIdentity = (PFNGLLOADIDENTITYPROC) load(userptr, "glLoadIdentity");
    gladptrs.glad_glLoadMatrixd = (PFNGLLOADMATRIXDPROC) load(userptr, "glLoadMatrixd");
    gladptrs.glad_glLoadMatrixf = (PFNGLLOADMATRIXFPROC) load(userptr, "glLoadMatrixf");
    gladptrs.glad_glLoadName = (PFNGLLOADNAMEPROC) load(userptr, "glLoadName");
    gladptrs.glad_glLogicOp = (PFNGLLOGICOPPROC) load(userptr, "glLogicOp");
    gladptrs.glad_glMap1d = (PFNGLMAP1DPROC) load(userptr, "glMap1d");
    gladptrs.glad_glMap1f = (PFNGLMAP1FPROC) load(userptr, "glMap1f");
    gladptrs.glad_glMap2d = (PFNGLMAP2DPROC) load(userptr, "glMap2d");
    gladptrs.glad_glMap2f = (PFNGLMAP2FPROC) load(userptr, "glMap2f");
    gladptrs.glad_glMapGrid1d = (PFNGLMAPGRID1DPROC) load(userptr, "glMapGrid1d");
    gladptrs.glad_glMapGrid1f = (PFNGLMAPGRID1FPROC) load(userptr, "glMapGrid1f");
    gladptrs.glad_glMapGrid2d = (PFNGLMAPGRID2DPROC) load(userptr, "glMapGrid2d");
    gladptrs.glad_glMapGrid2f = (PFNGLMAPGRID2FPROC) load(userptr, "glMapGrid2f");
    gladptrs.glad_glMaterialf = (PFNGLMATERIALFPROC) load(userptr, "glMaterialf");
    gladptrs.glad_glMaterialfv = (PFNGLMATERIALFVPROC) load(userptr, "glMaterialfv");
    gladptrs.glad_glMateriali = (PFNGLMATERIALIPROC) load(userptr, "glMateriali");
    gladptrs.glad_glMaterialiv = (PFNGLMATERIALIVPROC) load(userptr, "glMaterialiv");
    gladptrs.glad_glMatrixMode = (PFNGLMATRIXMODEPROC) load(userptr, "glMatrixMode");
    gladptrs.glad_glMultMatrixd = (PFNGLMULTMATRIXDPROC) load(userptr, "glMultMatrixd");
    gladptrs.glad_glMultMatrixf = (PFNGLMULTMATRIXFPROC) load(userptr, "glMultMatrixf");
    gladptrs.glad_glNewList = (PFNGLNEWLISTPROC) load(userptr, "glNewList");
    gladptrs.glad_glNormal3b = (PFNGLNORMAL3BPROC) load(userptr, "glNormal3b");
    gladptrs.glad_glNormal3bv = (PFNGLNORMAL3BVPROC) load(userptr, "glNormal3bv");
    gladptrs.glad_glNormal3d = (PFNGLNORMAL3DPROC) load(userptr, "glNormal3d");
    gladptrs.glad_glNormal3dv = (PFNGLNORMAL3DVPROC) load(userptr, "glNormal3dv");
    gladptrs.glad_glNormal3f = (PFNGLNORMAL3FPROC) load(userptr, "glNormal3f");
    gladptrs.glad_glNormal3fv = (PFNGLNORMAL3FVPROC) load(userptr, "glNormal3fv");
    gladptrs.glad_glNormal3i = (PFNGLNORMAL3IPROC) load(userptr, "glNormal3i");
    gladptrs.glad_glNormal3iv = (PFNGLNORMAL3IVPROC) load(userptr, "glNormal3iv");
    gladptrs.glad_glNormal3s = (PFNGLNORMAL3SPROC) load(userptr, "glNormal3s");
    gladptrs.glad_glNormal3sv = (PFNGLNORMAL3SVPROC) load(userptr, "glNormal3sv");
    gladptrs.glad_glOrtho = (PFNGLORTHOPROC) load(userptr, "glOrtho");
    gladptrs.glad_glPassThrough = (PFNGLPASSTHROUGHPROC) load(userptr, "glPassThrough");
    gladptrs.glad_glPixelMapfv = (PFNGLPIXELMAPFVPROC) load(userptr, "glPixelMapfv");
    gladptrs.glad_glPixelMapuiv = (PFNGLPIXELMAPUIVPROC) load(userptr, "glPixelMapuiv");
    gladptrs.glad_glPixelMapusv = (PFNGLPIXELMAPUSVPROC) load(userptr, "glPixelMapusv");
    gladptrs.glad_glPixelStoref = (PFNGLPIXELSTOREFPROC) load(userptr, "glPixelStoref");
    gladptrs.glad_glPixelStorei = (PFNGLPIXELSTOREIPROC) load(userptr, "glPixelStorei");
    gladptrs.glad_glPixelTransferf = (PFNGLPIXELTRANSFERFPROC) load(userptr, "glPixelTransferf");
    gladptrs.glad_glPixelTransferi = (PFNGLPIXELTRANSFERIPROC) load(userptr, "glPixelTransferi");
    gladptrs.glad_glPixelZoom = (PFNGLPIXELZOOMPROC) load(userptr, "glPixelZoom");
    gladptrs.glad_glPointSize = (PFNGLPOINTSIZEPROC) load(userptr, "glPointSize");
    gladptrs.glad_glPolygonMode = (PFNGLPOLYGONMODEPROC) load(userptr, "glPolygonMode");
    gladptrs.glad_glPolygonStipple = (PFNGLPOLYGONSTIPPLEPROC) load(userptr, "glPolygonStipple");
    gladptrs.glad_glPopAttrib = (PFNGLPOPATTRIBPROC) load(userptr, "glPopAttrib");
    gladptrs.glad_glPopMatrix = (PFNGLPOPMATRIXPROC) load(userptr, "glPopMatrix");
    gladptrs.glad_glPopName = (PFNGLPOPNAMEPROC) load(userptr, "glPopName");
    gladptrs.glad_glPushAttrib = (PFNGLPUSHATTRIBPROC) load(userptr, "glPushAttrib");
    gladptrs.glad_glPushMatrix = (PFNGLPUSHMATRIXPROC) load(userptr, "glPushMatrix");
    gladptrs.glad_glPushName = (PFNGLPUSHNAMEPROC) load(userptr, "glPushName");
    gladptrs.glad_glRasterPos2d = (PFNGLRASTERPOS2DPROC) load(userptr, "glRasterPos2d");
    gladptrs.glad_glRasterPos2dv = (PFNGLRASTERPOS2DVPROC) load(userptr, "glRasterPos2dv");
    gladptrs.glad_glRasterPos2f = (PFNGLRASTERPOS2FPROC) load(userptr, "glRasterPos2f");
    gladptrs.glad_glRasterPos2fv = (PFNGLRASTERPOS2FVPROC) load(userptr, "glRasterPos2fv");
    gladptrs.glad_glRasterPos2i = (PFNGLRASTERPOS2IPROC) load(userptr, "glRasterPos2i");
    gladptrs.glad_glRasterPos2iv = (PFNGLRASTERPOS2IVPROC) load(userptr, "glRasterPos2iv");
    gladptrs.glad_glRasterPos2s = (PFNGLRASTERPOS2SPROC) load(userptr, "glRasterPos2s");
    gladptrs.glad_glRasterPos2sv = (PFNGLRASTERPOS2SVPROC) load(userptr, "glRasterPos2sv");
    gladptrs.glad_glRasterPos3d = (PFNGLRASTERPOS3DPROC) load(userptr, "glRasterPos3d");
    gladptrs.glad_glRasterPos3dv = (PFNGLRASTERPOS3DVPROC) load(userptr, "glRasterPos3dv");
    gladptrs.glad_glRasterPos3f = (PFNGLRASTERPOS3FPROC) load(userptr, "glRasterPos3f");
    gladptrs.glad_glRasterPos3fv = (PFNGLRASTERPOS3FVPROC) load(userptr, "glRasterPos3fv");
    gladptrs.glad_glRasterPos3i = (PFNGLRASTERPOS3IPROC) load(userptr, "glRasterPos3i");
    gladptrs.glad_glRasterPos3iv = (PFNGLRASTERPOS3IVPROC) load(userptr, "glRasterPos3iv");
    gladptrs.glad_glRasterPos3s = (PFNGLRASTERPOS3SPROC) load(userptr, "glRasterPos3s");
    gladptrs.glad_glRasterPos3sv = (PFNGLRASTERPOS3SVPROC) load(userptr, "glRasterPos3sv");
    gladptrs.glad_glRasterPos4d = (PFNGLRASTERPOS4DPROC) load(userptr, "glRasterPos4d");
    gladptrs.glad_glRasterPos4dv = (PFNGLRASTERPOS4DVPROC) load(userptr, "glRasterPos4dv");
    gladptrs.glad_glRasterPos4f = (PFNGLRASTERPOS4FPROC) load(userptr, "glRasterPos4f");
    gladptrs.glad_glRasterPos4fv = (PFNGLRASTERPOS4FVPROC) load(userptr, "glRasterPos4fv");
    gladptrs.glad_glRasterPos4i = (PFNGLRASTERPOS4IPROC) load(userptr, "glRasterPos4i");
    gladptrs.glad_glRasterPos4iv = (PFNGLRASTERPOS4IVPROC) load(userptr, "glRasterPos4iv");
    gladptrs.glad_glRasterPos4s = (PFNGLRASTERPOS4SPROC) load(userptr, "glRasterPos4s");
    gladptrs.glad_glRasterPos4sv = (PFNGLRASTERPOS4SVPROC) load(userptr, "glRasterPos4sv");
    gladptrs.glad_glReadBuffer = (PFNGLREADBUFFERPROC) load(userptr, "glReadBuffer");
    gladptrs.glad_glReadPixels = (PFNGLREADPIXELSPROC) load(userptr, "glReadPixels");
    gladptrs.glad_glRectd = (PFNGLRECTDPROC) load(userptr, "glRectd");
    gladptrs.glad_glRectdv = (PFNGLRECTDVPROC) load(userptr, "glRectdv");
    gladptrs.glad_glRectf = (PFNGLRECTFPROC) load(userptr, "glRectf");
    gladptrs.glad_glRectfv = (PFNGLRECTFVPROC) load(userptr, "glRectfv");
    gladptrs.glad_glRecti = (PFNGLRECTIPROC) load(userptr, "glRecti");
    gladptrs.glad_glRectiv = (PFNGLRECTIVPROC) load(userptr, "glRectiv");
    gladptrs.glad_glRects = (PFNGLRECTSPROC) load(userptr, "glRects");
    gladptrs.glad_glRectsv = (PFNGLRECTSVPROC) load(userptr, "glRectsv");
    gladptrs.glad_glRenderMode = (PFNGLRENDERMODEPROC) load(userptr, "glRenderMode");
    gladptrs.glad_glRotated = (PFNGLROTATEDPROC) load(userptr, "glRotated");
    gladptrs.glad_glRotatef = (PFNGLROTATEFPROC) load(userptr, "glRotatef");
    gladptrs.glad_glScaled = (PFNGLSCALEDPROC) load(userptr, "glScaled");
    gladptrs.glad_glScalef = (PFNGLSCALEFPROC) load(userptr, "glScalef");
    gladptrs.glad_glScissor = (PFNGLSCISSORPROC) load(userptr, "glScissor");
    gladptrs.glad_glSelectBuffer = (PFNGLSELECTBUFFERPROC) load(userptr, "glSelectBuffer");
    gladptrs.glad_glShadeModel = (PFNGLSHADEMODELPROC) load(userptr, "glShadeModel");
    gladptrs.glad_glStencilFunc = (PFNGLSTENCILFUNCPROC) load(userptr, "glStencilFunc");
    gladptrs.glad_glStencilMask = (PFNGLSTENCILMASKPROC) load(userptr, "glStencilMask");
    gladptrs.glad_glStencilOp = (PFNGLSTENCILOPPROC) load(userptr, "glStencilOp");
    gladptrs.glad_glTexCoord1d = (PFNGLTEXCOORD1DPROC) load(userptr, "glTexCoord1d");
    gladptrs.glad_glTexCoord1dv = (PFNGLTEXCOORD1DVPROC) load(userptr, "glTexCoord1dv");
    gladptrs.glad_glTexCoord1f = (PFNGLTEXCOORD1FPROC) load(userptr, "glTexCoord1f");
    gladptrs.glad_glTexCoord1fv = (PFNGLTEXCOORD1FVPROC) load(userptr, "glTexCoord1fv");
    gladptrs.glad_glTexCoord1i = (PFNGLTEXCOORD1IPROC) load(userptr, "glTexCoord1i");
    gladptrs.glad_glTexCoord1iv = (PFNGLTEXCOORD1IVPROC) load(userptr, "glTexCoord1iv");
    gladptrs.glad_glTexCoord1s = (PFNGLTEXCOORD1SPROC) load(userptr, "glTexCoord1s");
    gladptrs.glad_glTexCoord1sv = (PFNGLTEXCOORD1SVPROC) load(userptr, "glTexCoord1sv");
    gladptrs.glad_glTexCoord2d = (PFNGLTEXCOORD2DPROC) load(userptr, "glTexCoord2d");
    gladptrs.glad_glTexCoord2dv = (PFNGLTEXCOORD2DVPROC) load(userptr, "glTexCoord2dv");
    gladptrs.glad_glTexCoord2f = (PFNGLTEXCOORD2FPROC) load(userptr, "glTexCoord2f");
    gladptrs.glad_glTexCoord2fv = (PFNGLTEXCOORD2FVPROC) load(userptr, "glTexCoord2fv");
    gladptrs.glad_glTexCoord2i = (PFNGLTEXCOORD2IPROC) load(userptr, "glTexCoord2i");
    gladptrs.glad_glTexCoord2iv = (PFNGLTEXCOORD2IVPROC) load(userptr, "glTexCoord2iv");
    gladptrs.glad_glTexCoord2s = (PFNGLTEXCOORD2SPROC) load(userptr, "glTexCoord2s");
    gladptrs.glad_glTexCoord2sv = (PFNGLTEXCOORD2SVPROC) load(userptr, "glTexCoord2sv");
    gladptrs.glad_glTexCoord3d = (PFNGLTEXCOORD3DPROC) load(userptr, "glTexCoord3d");
    gladptrs.glad_glTexCoord3dv = (PFNGLTEXCOORD3DVPROC) load(userptr, "glTexCoord3dv");
    gladptrs.glad_glTexCoord3f = (PFNGLTEXCOORD3FPROC) load(userptr, "glTexCoord3f");
    gladptrs.glad_glTexCoord3fv = (PFNGLTEXCOORD3FVPROC) load(userptr, "glTexCoord3fv");
    gladptrs.glad_glTexCoord3i = (PFNGLTEXCOORD3IPROC) load(userptr, "glTexCoord3i");
    gladptrs.glad_glTexCoord3iv = (PFNGLTEXCOORD3IVPROC) load(userptr, "glTexCoord3iv");
    gladptrs.glad_glTexCoord3s = (PFNGLTEXCOORD3SPROC) load(userptr, "glTexCoord3s");
    gladptrs.glad_glTexCoord3sv = (PFNGLTEXCOORD3SVPROC) load(userptr, "glTexCoord3sv");
    gladptrs.glad_glTexCoord4d = (PFNGLTEXCOORD4DPROC) load(userptr, "glTexCoord4d");
    gladptrs.glad_glTexCoord4dv = (PFNGLTEXCOORD4DVPROC) load(userptr, "glTexCoord4dv");
    gladptrs.glad_glTexCoord4f = (PFNGLTEXCOORD4FPROC) load(userptr, "glTexCoord4f");
    gladptrs.glad_glTexCoord4fv = (PFNGLTEXCOORD4FVPROC) load(userptr, "glTexCoord4fv");
    gladptrs.glad_glTexCoord4i = (PFNGLTEXCOORD4IPROC) load(userptr, "glTexCoord4i");
    gladptrs.glad_glTexCoord4iv = (PFNGLTEXCOORD4IVPROC) load(userptr, "glTexCoord4iv");
    gladptrs.glad_glTexCoord4s = (PFNGLTEXCOORD4SPROC) load(userptr, "glTexCoord4s");
    gladptrs.glad_glTexCoord4sv = (PFNGLTEXCOORD4SVPROC) load(userptr, "glTexCoord4sv");
    gladptrs.glad_glTexEnvf = (PFNGLTEXENVFPROC) load(userptr, "glTexEnvf");
    gladptrs.glad_glTexEnvfv = (PFNGLTEXENVFVPROC) load(userptr, "glTexEnvfv");
    gladptrs.glad_glTexEnvi = (PFNGLTEXENVIPROC) load(userptr, "glTexEnvi");
    gladptrs.glad_glTexEnviv = (PFNGLTEXENVIVPROC) load(userptr, "glTexEnviv");
    gladptrs.glad_glTexGend = (PFNGLTEXGENDPROC) load(userptr, "glTexGend");
    gladptrs.glad_glTexGendv = (PFNGLTEXGENDVPROC) load(userptr, "glTexGendv");
    gladptrs.glad_glTexGenf = (PFNGLTEXGENFPROC) load(userptr, "glTexGenf");
    gladptrs.glad_glTexGenfv = (PFNGLTEXGENFVPROC) load(userptr, "glTexGenfv");
    gladptrs.glad_glTexGeni = (PFNGLTEXGENIPROC) load(userptr, "glTexGeni");
    gladptrs.glad_glTexGeniv = (PFNGLTEXGENIVPROC) load(userptr, "glTexGeniv");
    gladptrs.glad_glTexImage1D = (PFNGLTEXIMAGE1DPROC) load(userptr, "glTexImage1D");
    gladptrs.glad_glTexImage2D = (PFNGLTEXIMAGE2DPROC) load(userptr, "glTexImage2D");
    gladptrs.glad_glTexParameterf = (PFNGLTEXPARAMETERFPROC) load(userptr, "glTexParameterf");
    gladptrs.glad_glTexParameterfv = (PFNGLTEXPARAMETERFVPROC) load(userptr, "glTexParameterfv");
    gladptrs.glad_glTexParameteri = (PFNGLTEXPARAMETERIPROC) load(userptr, "glTexParameteri");
    gladptrs.glad_glTexParameteriv = (PFNGLTEXPARAMETERIVPROC) load(userptr, "glTexParameteriv");
    gladptrs.glad_glTranslated = (PFNGLTRANSLATEDPROC) load(userptr, "glTranslated");
    gladptrs.glad_glTranslatef = (PFNGLTRANSLATEFPROC) load(userptr, "glTranslatef");
    gladptrs.glad_glVertex2d = (PFNGLVERTEX2DPROC) load(userptr, "glVertex2d");
    gladptrs.glad_glVertex2dv = (PFNGLVERTEX2DVPROC) load(userptr, "glVertex2dv");
    gladptrs.glad_glVertex2f = (PFNGLVERTEX2FPROC) load(userptr, "glVertex2f");
    gladptrs.glad_glVertex2fv = (PFNGLVERTEX2FVPROC) load(userptr, "glVertex2fv");
    gladptrs.glad_glVertex2i = (PFNGLVERTEX2IPROC) load(userptr, "glVertex2i");
    gladptrs.glad_glVertex2iv = (PFNGLVERTEX2IVPROC) load(userptr, "glVertex2iv");
    gladptrs.glad_glVertex2s = (PFNGLVERTEX2SPROC) load(userptr, "glVertex2s");
    gladptrs.glad_glVertex2sv = (PFNGLVERTEX2SVPROC) load(userptr, "glVertex2sv");
    gladptrs.glad_glVertex3d = (PFNGLVERTEX3DPROC) load(userptr, "glVertex3d");
    gladptrs.glad_glVertex3dv = (PFNGLVERTEX3DVPROC) load(userptr, "glVertex3dv");
    gladptrs.glad_glVertex3f = (PFNGLVERTEX3FPROC) load(userptr, "glVertex3f");
    gladptrs.glad_glVertex3fv = (PFNGLVERTEX3FVPROC) load(userptr, "glVertex3fv");
    gladptrs.glad_glVertex3i = (PFNGLVERTEX3IPROC) load(userptr, "glVertex3i");
    gladptrs.glad_glVertex3iv = (PFNGLVERTEX3IVPROC) load(userptr, "glVertex3iv");
    gladptrs.glad_glVertex3s = (PFNGLVERTEX3SPROC) load(userptr, "glVertex3s");
    gladptrs.glad_glVertex3sv = (PFNGLVERTEX3SVPROC) load(userptr, "glVertex3sv");
    gladptrs.glad_glVertex4d = (PFNGLVERTEX4DPROC) load(userptr, "glVertex4d");
    gladptrs.glad_glVertex4dv = (PFNGLVERTEX4DVPROC) load(userptr, "glVertex4dv");
    gladptrs.glad_glVertex4f = (PFNGLVERTEX4FPROC) load(userptr, "glVertex4f");
    gladptrs.glad_glVertex4fv = (PFNGLVERTEX4FVPROC) load(userptr, "glVertex4fv");
    gladptrs.glad_glVertex4i = (PFNGLVERTEX4IPROC) load(userptr, "glVertex4i");
    gladptrs.glad_glVertex4iv = (PFNGLVERTEX4IVPROC) load(userptr, "glVertex4iv");
    gladptrs.glad_glVertex4s = (PFNGLVERTEX4SPROC) load(userptr, "glVertex4s");
    gladptrs.glad_glVertex4sv = (PFNGLVERTEX4SVPROC) load(userptr, "glVertex4sv");
    gladptrs.glad_glViewport = (PFNGLVIEWPORTPROC) load(userptr, "glViewport");
}
static void glad_gl_load_GL_VERSION_1_1( GLADuserptrloadfunc load, void* userptr) {
    if(!GLAD_GL_VERSION_1_1) return;
    gladptrs.glad_glAreTexturesResident = (PFNGLARETEXTURESRESIDENTPROC) load(userptr, "glAreTexturesResident");
    gladptrs.glad_glArrayElement = (PFNGLARRAYELEMENTPROC) load(userptr, "glArrayElement");
    gladptrs.glad_glBindTexture = (PFNGLBINDTEXTUREPROC) load(userptr, "glBindTexture");
    gladptrs.glad_glColorPointer = (PFNGLCOLORPOINTERPROC) load(userptr, "glColorPointer");
    gladptrs.glad_glCopyTexImage1D = (PFNGLCOPYTEXIMAGE1DPROC) load(userptr, "glCopyTexImage1D");
    gladptrs.glad_glCopyTexImage2D = (PFNGLCOPYTEXIMAGE2DPROC) load(userptr, "glCopyTexImage2D");
    gladptrs.glad_glCopyTexSubImage1D = (PFNGLCOPYTEXSUBIMAGE1DPROC) load(userptr, "glCopyTexSubImage1D");
    gladptrs.glad_glCopyTexSubImage2D = (PFNGLCOPYTEXSUBIMAGE2DPROC) load(userptr, "glCopyTexSubImage2D");
    gladptrs.glad_glDeleteTextures = (PFNGLDELETETEXTURESPROC) load(userptr, "glDeleteTextures");
    gladptrs.glad_glDisableClientState = (PFNGLDISABLECLIENTSTATEPROC) load(userptr, "glDisableClientState");
    gladptrs.glad_glDrawArrays = (PFNGLDRAWARRAYSPROC) load(userptr, "glDrawArrays");
    gladptrs.glad_glDrawElements = (PFNGLDRAWELEMENTSPROC) load(userptr, "glDrawElements");
    gladptrs.glad_glEdgeFlagPointer = (PFNGLEDGEFLAGPOINTERPROC) load(userptr, "glEdgeFlagPointer");
    gladptrs.glad_glEnableClientState = (PFNGLENABLECLIENTSTATEPROC) load(userptr, "glEnableClientState");
    gladptrs.glad_glGenTextures = (PFNGLGENTEXTURESPROC) load(userptr, "glGenTextures");
    gladptrs.glad_glGetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    gladptrs.glad_glIndexPointer = (PFNGLINDEXPOINTERPROC) load(userptr, "glIndexPointer");
    gladptrs.glad_glIndexub = (PFNGLINDEXUBPROC) load(userptr, "glIndexub");
    gladptrs.glad_glIndexubv = (PFNGLINDEXUBVPROC) load(userptr, "glIndexubv");
    gladptrs.glad_glInterleavedArrays = (PFNGLINTERLEAVEDARRAYSPROC) load(userptr, "glInterleavedArrays");
    gladptrs.glad_glIsTexture = (PFNGLISTEXTUREPROC) load(userptr, "glIsTexture");
    gladptrs.glad_glNormalPointer = (PFNGLNORMALPOINTERPROC) load(userptr, "glNormalPointer");
    gladptrs.glad_glPolygonOffset = (PFNGLPOLYGONOFFSETPROC) load(userptr, "glPolygonOffset");
    gladptrs.glad_glPopClientAttrib = (PFNGLPOPCLIENTATTRIBPROC) load(userptr, "glPopClientAttrib");
    gladptrs.glad_glPrioritizeTextures = (PFNGLPRIORITIZETEXTURESPROC) load(userptr, "glPrioritizeTextures");
    gladptrs.glad_glPushClientAttrib = (PFNGLPUSHCLIENTATTRIBPROC) load(userptr, "glPushClientAttrib");
    gladptrs.glad_glTexCoordPointer = (PFNGLTEXCOORDPOINTERPROC) load(userptr, "glTexCoordPointer");
    gladptrs.glad_glTexSubImage1D = (PFNGLTEXSUBIMAGE1DPROC) load(userptr, "glTexSubImage1D");
    gladptrs.glad_glTexSubImage2D = (PFNGLTEXSUBIMAGE2DPROC) load(userptr, "glTexSubImage2D");
    gladptrs.glad_glVertexPointer = (PFNGLVERTEXPOINTERPROC) load(userptr, "glVertexPointer");
}
static void glad_gl_load_GL_VERSION_1_2( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_1_2) return;
    gladptrs.glad_glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC) load(userptr, "glCopyTexSubImage3D");
    gladptrs.glad_glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) load(userptr, "glDrawRangeElements");
    gladptrs.glad_glTexImage3D = (PFNGLTEXIMAGE3DPROC) load(userptr, "glTexImage3D");
    gladptrs.glad_glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC) load(userptr, "glTexSubImage3D");
}
static void glad_gl_load_GL_VERSION_1_3( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_1_3) return;
    gladptrs.glad_glActiveTexture = (PFNGLACTIVETEXTUREPROC) load(userptr, "glActiveTexture");
    gladptrs.glad_glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC) load(userptr, "glClientActiveTexture");
    gladptrs.glad_glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC) load(userptr, "glCompressedTexImage1D");
    gladptrs.glad_glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) load(userptr, "glCompressedTexImage2D");
    gladptrs.glad_glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC) load(userptr, "glCompressedTexImage3D");
    gladptrs.glad_glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC) load(userptr, "glCompressedTexSubImage1D");
    gladptrs.glad_glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC) load(userptr, "glCompressedTexSubImage2D");
    gladptrs.glad_glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC) load(userptr, "glCompressedTexSubImage3D");
    gladptrs.glad_glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC) load(userptr, "glGetCompressedTexImage");
    gladptrs.glad_glLoadTransposeMatrixd = (PFNGLLOADTRANSPOSEMATRIXDPROC) load(userptr, "glLoadTransposeMatrixd");
    gladptrs.glad_glLoadTransposeMatrixf = (PFNGLLOADTRANSPOSEMATRIXFPROC) load(userptr, "glLoadTransposeMatrixf");
    gladptrs.glad_glMultTransposeMatrixd = (PFNGLMULTTRANSPOSEMATRIXDPROC) load(userptr, "glMultTransposeMatrixd");
    gladptrs.glad_glMultTransposeMatrixf = (PFNGLMULTTRANSPOSEMATRIXFPROC) load(userptr, "glMultTransposeMatrixf");
    gladptrs.glad_glMultiTexCoord1d = (PFNGLMULTITEXCOORD1DPROC) load(userptr, "glMultiTexCoord1d");
    gladptrs.glad_glMultiTexCoord1dv = (PFNGLMULTITEXCOORD1DVPROC) load(userptr, "glMultiTexCoord1dv");
    gladptrs.glad_glMultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC) load(userptr, "glMultiTexCoord1f");
    gladptrs.glad_glMultiTexCoord1fv = (PFNGLMULTITEXCOORD1FVPROC) load(userptr, "glMultiTexCoord1fv");
    gladptrs.glad_glMultiTexCoord1i = (PFNGLMULTITEXCOORD1IPROC) load(userptr, "glMultiTexCoord1i");
    gladptrs.glad_glMultiTexCoord1iv = (PFNGLMULTITEXCOORD1IVPROC) load(userptr, "glMultiTexCoord1iv");
    gladptrs.glad_glMultiTexCoord1s = (PFNGLMULTITEXCOORD1SPROC) load(userptr, "glMultiTexCoord1s");
    gladptrs.glad_glMultiTexCoord1sv = (PFNGLMULTITEXCOORD1SVPROC) load(userptr, "glMultiTexCoord1sv");
    gladptrs.glad_glMultiTexCoord2d = (PFNGLMULTITEXCOORD2DPROC) load(userptr, "glMultiTexCoord2d");
    gladptrs.glad_glMultiTexCoord2dv = (PFNGLMULTITEXCOORD2DVPROC) load(userptr, "glMultiTexCoord2dv");
    gladptrs.glad_glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC) load(userptr, "glMultiTexCoord2f");
    gladptrs.glad_glMultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVPROC) load(userptr, "glMultiTexCoord2fv");
    gladptrs.glad_glMultiTexCoord2i = (PFNGLMULTITEXCOORD2IPROC) load(userptr, "glMultiTexCoord2i");
    gladptrs.glad_glMultiTexCoord2iv = (PFNGLMULTITEXCOORD2IVPROC) load(userptr, "glMultiTexCoord2iv");
    gladptrs.glad_glMultiTexCoord2s = (PFNGLMULTITEXCOORD2SPROC) load(userptr, "glMultiTexCoord2s");
    gladptrs.glad_glMultiTexCoord2sv = (PFNGLMULTITEXCOORD2SVPROC) load(userptr, "glMultiTexCoord2sv");
    gladptrs.glad_glMultiTexCoord3d = (PFNGLMULTITEXCOORD3DPROC) load(userptr, "glMultiTexCoord3d");
    gladptrs.glad_glMultiTexCoord3dv = (PFNGLMULTITEXCOORD3DVPROC) load(userptr, "glMultiTexCoord3dv");
    gladptrs.glad_glMultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC) load(userptr, "glMultiTexCoord3f");
    gladptrs.glad_glMultiTexCoord3fv = (PFNGLMULTITEXCOORD3FVPROC) load(userptr, "glMultiTexCoord3fv");
    gladptrs.glad_glMultiTexCoord3i = (PFNGLMULTITEXCOORD3IPROC) load(userptr, "glMultiTexCoord3i");
    gladptrs.glad_glMultiTexCoord3iv = (PFNGLMULTITEXCOORD3IVPROC) load(userptr, "glMultiTexCoord3iv");
    gladptrs.glad_glMultiTexCoord3s = (PFNGLMULTITEXCOORD3SPROC) load(userptr, "glMultiTexCoord3s");
    gladptrs.glad_glMultiTexCoord3sv = (PFNGLMULTITEXCOORD3SVPROC) load(userptr, "glMultiTexCoord3sv");
    gladptrs.glad_glMultiTexCoord4d = (PFNGLMULTITEXCOORD4DPROC) load(userptr, "glMultiTexCoord4d");
    gladptrs.glad_glMultiTexCoord4dv = (PFNGLMULTITEXCOORD4DVPROC) load(userptr, "glMultiTexCoord4dv");
    gladptrs.glad_glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC) load(userptr, "glMultiTexCoord4f");
    gladptrs.glad_glMultiTexCoord4fv = (PFNGLMULTITEXCOORD4FVPROC) load(userptr, "glMultiTexCoord4fv");
    gladptrs.glad_glMultiTexCoord4i = (PFNGLMULTITEXCOORD4IPROC) load(userptr, "glMultiTexCoord4i");
    gladptrs.glad_glMultiTexCoord4iv = (PFNGLMULTITEXCOORD4IVPROC) load(userptr, "glMultiTexCoord4iv");
    gladptrs.glad_glMultiTexCoord4s = (PFNGLMULTITEXCOORD4SPROC) load(userptr, "glMultiTexCoord4s");
    gladptrs.glad_glMultiTexCoord4sv = (PFNGLMULTITEXCOORD4SVPROC) load(userptr, "glMultiTexCoord4sv");
    gladptrs.glad_glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC) load(userptr, "glSampleCoverage");
}
static void glad_gl_load_GL_VERSION_1_4( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_1_4) return;
    gladptrs.glad_glBlendColor = (PFNGLBLENDCOLORPROC) load(userptr, "glBlendColor");
    gladptrs.glad_glBlendEquation = (PFNGLBLENDEQUATIONPROC) load(userptr, "glBlendEquation");
    gladptrs.glad_glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC) load(userptr, "glBlendFuncSeparate");
    gladptrs.glad_glFogCoordPointer = (PFNGLFOGCOORDPOINTERPROC) load(userptr, "glFogCoordPointer");
    gladptrs.glad_glFogCoordd = (PFNGLFOGCOORDDPROC) load(userptr, "glFogCoordd");
    gladptrs.glad_glFogCoorddv = (PFNGLFOGCOORDDVPROC) load(userptr, "glFogCoorddv");
    gladptrs.glad_glFogCoordf = (PFNGLFOGCOORDFPROC) load(userptr, "glFogCoordf");
    gladptrs.glad_glFogCoordfv = (PFNGLFOGCOORDFVPROC) load(userptr, "glFogCoordfv");
    gladptrs.glad_glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC) load(userptr, "glMultiDrawArrays");
    gladptrs.glad_glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC) load(userptr, "glMultiDrawElements");
    gladptrs.glad_glPointParameterf = (PFNGLPOINTPARAMETERFPROC) load(userptr, "glPointParameterf");
    gladptrs.glad_glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC) load(userptr, "glPointParameterfv");
    gladptrs.glad_glPointParameteri = (PFNGLPOINTPARAMETERIPROC) load(userptr, "glPointParameteri");
    gladptrs.glad_glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC) load(userptr, "glPointParameteriv");
    gladptrs.glad_glSecondaryColor3b = (PFNGLSECONDARYCOLOR3BPROC) load(userptr, "glSecondaryColor3b");
    gladptrs.glad_glSecondaryColor3bv = (PFNGLSECONDARYCOLOR3BVPROC) load(userptr, "glSecondaryColor3bv");
    gladptrs.glad_glSecondaryColor3d = (PFNGLSECONDARYCOLOR3DPROC) load(userptr, "glSecondaryColor3d");
    gladptrs.glad_glSecondaryColor3dv = (PFNGLSECONDARYCOLOR3DVPROC) load(userptr, "glSecondaryColor3dv");
    gladptrs.glad_glSecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC) load(userptr, "glSecondaryColor3f");
    gladptrs.glad_glSecondaryColor3fv = (PFNGLSECONDARYCOLOR3FVPROC) load(userptr, "glSecondaryColor3fv");
    gladptrs.glad_glSecondaryColor3i = (PFNGLSECONDARYCOLOR3IPROC) load(userptr, "glSecondaryColor3i");
    gladptrs.glad_glSecondaryColor3iv = (PFNGLSECONDARYCOLOR3IVPROC) load(userptr, "glSecondaryColor3iv");
    gladptrs.glad_glSecondaryColor3s = (PFNGLSECONDARYCOLOR3SPROC) load(userptr, "glSecondaryColor3s");
    gladptrs.glad_glSecondaryColor3sv = (PFNGLSECONDARYCOLOR3SVPROC) load(userptr, "glSecondaryColor3sv");
    gladptrs.glad_glSecondaryColor3ub = (PFNGLSECONDARYCOLOR3UBPROC) load(userptr, "glSecondaryColor3ub");
    gladptrs.glad_glSecondaryColor3ubv = (PFNGLSECONDARYCOLOR3UBVPROC) load(userptr, "glSecondaryColor3ubv");
    gladptrs.glad_glSecondaryColor3ui = (PFNGLSECONDARYCOLOR3UIPROC) load(userptr, "glSecondaryColor3ui");
    gladptrs.glad_glSecondaryColor3uiv = (PFNGLSECONDARYCOLOR3UIVPROC) load(userptr, "glSecondaryColor3uiv");
    gladptrs.glad_glSecondaryColor3us = (PFNGLSECONDARYCOLOR3USPROC) load(userptr, "glSecondaryColor3us");
    gladptrs.glad_glSecondaryColor3usv = (PFNGLSECONDARYCOLOR3USVPROC) load(userptr, "glSecondaryColor3usv");
    gladptrs.glad_glSecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTERPROC) load(userptr, "glSecondaryColorPointer");
    gladptrs.glad_glWindowPos2d = (PFNGLWINDOWPOS2DPROC) load(userptr, "glWindowPos2d");
    gladptrs.glad_glWindowPos2dv = (PFNGLWINDOWPOS2DVPROC) load(userptr, "glWindowPos2dv");
    gladptrs.glad_glWindowPos2f = (PFNGLWINDOWPOS2FPROC) load(userptr, "glWindowPos2f");
    gladptrs.glad_glWindowPos2fv = (PFNGLWINDOWPOS2FVPROC) load(userptr, "glWindowPos2fv");
    gladptrs.glad_glWindowPos2i = (PFNGLWINDOWPOS2IPROC) load(userptr, "glWindowPos2i");
    gladptrs.glad_glWindowPos2iv = (PFNGLWINDOWPOS2IVPROC) load(userptr, "glWindowPos2iv");
    gladptrs.glad_glWindowPos2s = (PFNGLWINDOWPOS2SPROC) load(userptr, "glWindowPos2s");
    gladptrs.glad_glWindowPos2sv = (PFNGLWINDOWPOS2SVPROC) load(userptr, "glWindowPos2sv");
    gladptrs.glad_glWindowPos3d = (PFNGLWINDOWPOS3DPROC) load(userptr, "glWindowPos3d");
    gladptrs.glad_glWindowPos3dv = (PFNGLWINDOWPOS3DVPROC) load(userptr, "glWindowPos3dv");
    gladptrs.glad_glWindowPos3f = (PFNGLWINDOWPOS3FPROC) load(userptr, "glWindowPos3f");
    gladptrs.glad_glWindowPos3fv = (PFNGLWINDOWPOS3FVPROC) load(userptr, "glWindowPos3fv");
    gladptrs.glad_glWindowPos3i = (PFNGLWINDOWPOS3IPROC) load(userptr, "glWindowPos3i");
    gladptrs.glad_glWindowPos3iv = (PFNGLWINDOWPOS3IVPROC) load(userptr, "glWindowPos3iv");
    gladptrs.glad_glWindowPos3s = (PFNGLWINDOWPOS3SPROC) load(userptr, "glWindowPos3s");
    gladptrs.glad_glWindowPos3sv = (PFNGLWINDOWPOS3SVPROC) load(userptr, "glWindowPos3sv");
}
static void glad_gl_load_GL_VERSION_1_5( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_1_5) return;
    gladptrs.glad_glBeginQuery = (PFNGLBEGINQUERYPROC) load(userptr, "glBeginQuery");
    gladptrs.glad_glBindBuffer = (PFNGLBINDBUFFERPROC) load(userptr, "glBindBuffer");
    gladptrs.glad_glBufferData = (PFNGLBUFFERDATAPROC) load(userptr, "glBufferData");
    gladptrs.glad_glBufferSubData = (PFNGLBUFFERSUBDATAPROC) load(userptr, "glBufferSubData");
    gladptrs.glad_glDeleteBuffers = (PFNGLDELETEBUFFERSPROC) load(userptr, "glDeleteBuffers");
    gladptrs.glad_glDeleteQueries = (PFNGLDELETEQUERIESPROC) load(userptr, "glDeleteQueries");
    gladptrs.glad_glEndQuery = (PFNGLENDQUERYPROC) load(userptr, "glEndQuery");
    gladptrs.glad_glGenBuffers = (PFNGLGENBUFFERSPROC) load(userptr, "glGenBuffers");
    gladptrs.glad_glGenQueries = (PFNGLGENQUERIESPROC) load(userptr, "glGenQueries");
    gladptrs.glad_glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC) load(userptr, "glGetBufferParameteriv");
    gladptrs.glad_glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC) load(userptr, "glGetBufferPointerv");
    gladptrs.glad_glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC) load(userptr, "glGetBufferSubData");
    gladptrs.glad_glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC) load(userptr, "glGetQueryObjectiv");
    gladptrs.glad_glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) load(userptr, "glGetQueryObjectuiv");
    gladptrs.glad_glGetQueryiv = (PFNGLGETQUERYIVPROC) load(userptr, "glGetQueryiv");
    gladptrs.glad_glIsBuffer = (PFNGLISBUFFERPROC) load(userptr, "glIsBuffer");
    gladptrs.glad_glIsQuery = (PFNGLISQUERYPROC) load(userptr, "glIsQuery");
    gladptrs.glad_glMapBuffer = (PFNGLMAPBUFFERPROC) load(userptr, "glMapBuffer");
    gladptrs.glad_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC) load(userptr, "glUnmapBuffer");
}
static void glad_gl_load_GL_VERSION_2_0( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_2_0) return;
    gladptrs.glad_glAttachShader = (PFNGLATTACHSHADERPROC) load(userptr, "glAttachShader");
    gladptrs.glad_glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC) load(userptr, "glBindAttribLocation");
    gladptrs.glad_glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC) load(userptr, "glBlendEquationSeparate");
    gladptrs.glad_glCompileShader = (PFNGLCOMPILESHADERPROC) load(userptr, "glCompileShader");
    gladptrs.glad_glCreateProgram = (PFNGLCREATEPROGRAMPROC) load(userptr, "glCreateProgram");
    gladptrs.glad_glCreateShader = (PFNGLCREATESHADERPROC) load(userptr, "glCreateShader");
    gladptrs.glad_glDeleteProgram = (PFNGLDELETEPROGRAMPROC) load(userptr, "glDeleteProgram");
    gladptrs.glad_glDeleteShader = (PFNGLDELETESHADERPROC) load(userptr, "glDeleteShader");
    gladptrs.glad_glDetachShader = (PFNGLDETACHSHADERPROC) load(userptr, "glDetachShader");
    gladptrs.glad_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) load(userptr, "glDisableVertexAttribArray");
    gladptrs.glad_glDrawBuffers = (PFNGLDRAWBUFFERSPROC) load(userptr, "glDrawBuffers");
    gladptrs.glad_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) load(userptr, "glEnableVertexAttribArray");
    gladptrs.glad_glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC) load(userptr, "glGetActiveAttrib");
    gladptrs.glad_glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC) load(userptr, "glGetActiveUniform");
    gladptrs.glad_glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC) load(userptr, "glGetAttachedShaders");
    gladptrs.glad_glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) load(userptr, "glGetAttribLocation");
    gladptrs.glad_glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) load(userptr, "glGetProgramInfoLog");
    gladptrs.glad_glGetProgramiv = (PFNGLGETPROGRAMIVPROC) load(userptr, "glGetProgramiv");
    gladptrs.glad_glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) load(userptr, "glGetShaderInfoLog");
    gladptrs.glad_glGetShaderSource = (PFNGLGETSHADERSOURCEPROC) load(userptr, "glGetShaderSource");
    gladptrs.glad_glGetShaderiv = (PFNGLGETSHADERIVPROC) load(userptr, "glGetShaderiv");
    gladptrs.glad_glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) load(userptr, "glGetUniformLocation");
    gladptrs.glad_glGetUniformfv = (PFNGLGETUNIFORMFVPROC) load(userptr, "glGetUniformfv");
    gladptrs.glad_glGetUniformiv = (PFNGLGETUNIFORMIVPROC) load(userptr, "glGetUniformiv");
    gladptrs.glad_glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC) load(userptr, "glGetVertexAttribPointerv");
    gladptrs.glad_glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC) load(userptr, "glGetVertexAttribdv");
    gladptrs.glad_glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC) load(userptr, "glGetVertexAttribfv");
    gladptrs.glad_glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC) load(userptr, "glGetVertexAttribiv");
    gladptrs.glad_glIsProgram = (PFNGLISPROGRAMPROC) load(userptr, "glIsProgram");
    gladptrs.glad_glIsShader = (PFNGLISSHADERPROC) load(userptr, "glIsShader");
    gladptrs.glad_glLinkProgram = (PFNGLLINKPROGRAMPROC) load(userptr, "glLinkProgram");
    gladptrs.glad_glShaderSource = (PFNGLSHADERSOURCEPROC) load(userptr, "glShaderSource");
    gladptrs.glad_glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) load(userptr, "glStencilFuncSeparate");
    gladptrs.glad_glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) load(userptr, "glStencilMaskSeparate");
    gladptrs.glad_glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) load(userptr, "glStencilOpSeparate");
    gladptrs.glad_glUniform1f = (PFNGLUNIFORM1FPROC) load(userptr, "glUniform1f");
    gladptrs.glad_glUniform1fv = (PFNGLUNIFORM1FVPROC) load(userptr, "glUniform1fv");
    gladptrs.glad_glUniform1i = (PFNGLUNIFORM1IPROC) load(userptr, "glUniform1i");
    gladptrs.glad_glUniform1iv = (PFNGLUNIFORM1IVPROC) load(userptr, "glUniform1iv");
    gladptrs.glad_glUniform2f = (PFNGLUNIFORM2FPROC) load(userptr, "glUniform2f");
    gladptrs.glad_glUniform2fv = (PFNGLUNIFORM2FVPROC) load(userptr, "glUniform2fv");
    gladptrs.glad_glUniform2i = (PFNGLUNIFORM2IPROC) load(userptr, "glUniform2i");
    gladptrs.glad_glUniform2iv = (PFNGLUNIFORM2IVPROC) load(userptr, "glUniform2iv");
    gladptrs.glad_glUniform3f = (PFNGLUNIFORM3FPROC) load(userptr, "glUniform3f");
    gladptrs.glad_glUniform3fv = (PFNGLUNIFORM3FVPROC) load(userptr, "glUniform3fv");
    gladptrs.glad_glUniform3i = (PFNGLUNIFORM3IPROC) load(userptr, "glUniform3i");
    gladptrs.glad_glUniform3iv = (PFNGLUNIFORM3IVPROC) load(userptr, "glUniform3iv");
    gladptrs.glad_glUniform4f = (PFNGLUNIFORM4FPROC) load(userptr, "glUniform4f");
    gladptrs.glad_glUniform4fv = (PFNGLUNIFORM4FVPROC) load(userptr, "glUniform4fv");
    gladptrs.glad_glUniform4i = (PFNGLUNIFORM4IPROC) load(userptr, "glUniform4i");
    gladptrs.glad_glUniform4iv = (PFNGLUNIFORM4IVPROC) load(userptr, "glUniform4iv");
    gladptrs.glad_glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC) load(userptr, "glUniformMatrix2fv");
    gladptrs.glad_glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC) load(userptr, "glUniformMatrix3fv");
    gladptrs.glad_glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC) load(userptr, "glUniformMatrix4fv");
    gladptrs.glad_glUseProgram = (PFNGLUSEPROGRAMPROC) load(userptr, "glUseProgram");
    gladptrs.glad_glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) load(userptr, "glValidateProgram");
    gladptrs.glad_glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC) load(userptr, "glVertexAttrib1d");
    gladptrs.glad_glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC) load(userptr, "glVertexAttrib1dv");
    gladptrs.glad_glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) load(userptr, "glVertexAttrib1f");
    gladptrs.glad_glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC) load(userptr, "glVertexAttrib1fv");
    gladptrs.glad_glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC) load(userptr, "glVertexAttrib1s");
    gladptrs.glad_glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC) load(userptr, "glVertexAttrib1sv");
    gladptrs.glad_glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC) load(userptr, "glVertexAttrib2d");
    gladptrs.glad_glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC) load(userptr, "glVertexAttrib2dv");
    gladptrs.glad_glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC) load(userptr, "glVertexAttrib2f");
    gladptrs.glad_glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC) load(userptr, "glVertexAttrib2fv");
    gladptrs.glad_glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC) load(userptr, "glVertexAttrib2s");
    gladptrs.glad_glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC) load(userptr, "glVertexAttrib2sv");
    gladptrs.glad_glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC) load(userptr, "glVertexAttrib3d");
    gladptrs.glad_glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC) load(userptr, "glVertexAttrib3dv");
    gladptrs.glad_glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC) load(userptr, "glVertexAttrib3f");
    gladptrs.glad_glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC) load(userptr, "glVertexAttrib3fv");
    gladptrs.glad_glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC) load(userptr, "glVertexAttrib3s");
    gladptrs.glad_glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC) load(userptr, "glVertexAttrib3sv");
    gladptrs.glad_glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC) load(userptr, "glVertexAttrib4Nbv");
    gladptrs.glad_glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC) load(userptr, "glVertexAttrib4Niv");
    gladptrs.glad_glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC) load(userptr, "glVertexAttrib4Nsv");
    gladptrs.glad_glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC) load(userptr, "glVertexAttrib4Nub");
    gladptrs.glad_glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC) load(userptr, "glVertexAttrib4Nubv");
    gladptrs.glad_glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC) load(userptr, "glVertexAttrib4Nuiv");
    gladptrs.glad_glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC) load(userptr, "glVertexAttrib4Nusv");
    gladptrs.glad_glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC) load(userptr, "glVertexAttrib4bv");
    gladptrs.glad_glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC) load(userptr, "glVertexAttrib4d");
    gladptrs.glad_glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC) load(userptr, "glVertexAttrib4dv");
    gladptrs.glad_glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC) load(userptr, "glVertexAttrib4f");
    gladptrs.glad_glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC) load(userptr, "glVertexAttrib4fv");
    gladptrs.glad_glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC) load(userptr, "glVertexAttrib4iv");
    gladptrs.glad_glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC) load(userptr, "glVertexAttrib4s");
    gladptrs.glad_glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC) load(userptr, "glVertexAttrib4sv");
    gladptrs.glad_glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC) load(userptr, "glVertexAttrib4ubv");
    gladptrs.glad_glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC) load(userptr, "glVertexAttrib4uiv");
    gladptrs.glad_glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC) load(userptr, "glVertexAttrib4usv");
    gladptrs.glad_glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC) load(userptr, "glVertexAttribPointer");
}
static void glad_gl_load_GL_VERSION_2_1( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_2_1) return;
    gladptrs.glad_glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC) load(userptr, "glUniformMatrix2x3fv");
    gladptrs.glad_glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC) load(userptr, "glUniformMatrix2x4fv");
    gladptrs.glad_glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC) load(userptr, "glUniformMatrix3x2fv");
    gladptrs.glad_glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC) load(userptr, "glUniformMatrix3x4fv");
    gladptrs.glad_glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC) load(userptr, "glUniformMatrix4x2fv");
    gladptrs.glad_glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC) load(userptr, "glUniformMatrix4x3fv");
}
static void glad_gl_load_GL_VERSION_3_0( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_3_0) return;
    gladptrs.glad_glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC) load(userptr, "glBeginConditionalRender");
    gladptrs.glad_glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC) load(userptr, "glBeginTransformFeedback");
    gladptrs.glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    gladptrs.glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    gladptrs.glad_glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC) load(userptr, "glBindFragDataLocation");
    gladptrs.glad_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC) load(userptr, "glBindFramebuffer");
    gladptrs.glad_glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC) load(userptr, "glBindRenderbuffer");
    gladptrs.glad_glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC) load(userptr, "glBindVertexArray");
    gladptrs.glad_glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC) load(userptr, "glBlitFramebuffer");
    gladptrs.glad_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) load(userptr, "glCheckFramebufferStatus");
    gladptrs.glad_glClampColor = (PFNGLCLAMPCOLORPROC) load(userptr, "glClampColor");
    gladptrs.glad_glClearBufferfi = (PFNGLCLEARBUFFERFIPROC) load(userptr, "glClearBufferfi");
    gladptrs.glad_glClearBufferfv = (PFNGLCLEARBUFFERFVPROC) load(userptr, "glClearBufferfv");
    gladptrs.glad_glClearBufferiv = (PFNGLCLEARBUFFERIVPROC) load(userptr, "glClearBufferiv");
    gladptrs.glad_glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC) load(userptr, "glClearBufferuiv");
    gladptrs.glad_glColorMaski = (PFNGLCOLORMASKIPROC) load(userptr, "glColorMaski");
    gladptrs.glad_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC) load(userptr, "glDeleteFramebuffers");
    gladptrs.glad_glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC) load(userptr, "glDeleteRenderbuffers");
    gladptrs.glad_glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC) load(userptr, "glDeleteVertexArrays");
    gladptrs.glad_glDisablei = (PFNGLDISABLEIPROC) load(userptr, "glDisablei");
    gladptrs.glad_glEnablei = (PFNGLENABLEIPROC) load(userptr, "glEnablei");
    gladptrs.glad_glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC) load(userptr, "glEndConditionalRender");
    gladptrs.glad_glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC) load(userptr, "glEndTransformFeedback");
    gladptrs.glad_glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC) load(userptr, "glFlushMappedBufferRange");
    gladptrs.glad_glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) load(userptr, "glFramebufferRenderbuffer");
    gladptrs.glad_glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC) load(userptr, "glFramebufferTexture1D");
    gladptrs.glad_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC) load(userptr, "glFramebufferTexture2D");
    gladptrs.glad_glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC) load(userptr, "glFramebufferTexture3D");
    gladptrs.glad_glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) load(userptr, "glFramebufferTextureLayer");
    gladptrs.glad_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC) load(userptr, "glGenFramebuffers");
    gladptrs.glad_glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC) load(userptr, "glGenRenderbuffers");
    gladptrs.glad_glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC) load(userptr, "glGenVertexArrays");
    gladptrs.glad_glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC) load(userptr, "glGenerateMipmap");
    gladptrs.glad_glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC) load(userptr, "glGetBooleani_v");
    gladptrs.glad_glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC) load(userptr, "glGetFragDataLocation");
    gladptrs.glad_glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) load(userptr, "glGetFramebufferAttachmentParameteriv");
    gladptrs.glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    gladptrs.glad_glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) load(userptr, "glGetRenderbufferParameteriv");
    gladptrs.glad_glGetStringi = (PFNGLGETSTRINGIPROC) load(userptr, "glGetStringi");
    gladptrs.glad_glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC) load(userptr, "glGetTexParameterIiv");
    gladptrs.glad_glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC) load(userptr, "glGetTexParameterIuiv");
    gladptrs.glad_glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC) load(userptr, "glGetTransformFeedbackVarying");
    gladptrs.glad_glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC) load(userptr, "glGetUniformuiv");
    gladptrs.glad_glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC) load(userptr, "glGetVertexAttribIiv");
    gladptrs.glad_glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC) load(userptr, "glGetVertexAttribIuiv");
    gladptrs.glad_glIsEnabledi = (PFNGLISENABLEDIPROC) load(userptr, "glIsEnabledi");
    gladptrs.glad_glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC) load(userptr, "glIsFramebuffer");
    gladptrs.glad_glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC) load(userptr, "glIsRenderbuffer");
    gladptrs.glad_glIsVertexArray = (PFNGLISVERTEXARRAYPROC) load(userptr, "glIsVertexArray");
    gladptrs.glad_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC) load(userptr, "glMapBufferRange");
    gladptrs.glad_glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC) load(userptr, "glRenderbufferStorage");
    gladptrs.glad_glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) load(userptr, "glRenderbufferStorageMultisample");
    gladptrs.glad_glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC) load(userptr, "glTexParameterIiv");
    gladptrs.glad_glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC) load(userptr, "glTexParameterIuiv");
    gladptrs.glad_glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC) load(userptr, "glTransformFeedbackVaryings");
    gladptrs.glad_glUniform1ui = (PFNGLUNIFORM1UIPROC) load(userptr, "glUniform1ui");
    gladptrs.glad_glUniform1uiv = (PFNGLUNIFORM1UIVPROC) load(userptr, "glUniform1uiv");
    gladptrs.glad_glUniform2ui = (PFNGLUNIFORM2UIPROC) load(userptr, "glUniform2ui");
    gladptrs.glad_glUniform2uiv = (PFNGLUNIFORM2UIVPROC) load(userptr, "glUniform2uiv");
    gladptrs.glad_glUniform3ui = (PFNGLUNIFORM3UIPROC) load(userptr, "glUniform3ui");
    gladptrs.glad_glUniform3uiv = (PFNGLUNIFORM3UIVPROC) load(userptr, "glUniform3uiv");
    gladptrs.glad_glUniform4ui = (PFNGLUNIFORM4UIPROC) load(userptr, "glUniform4ui");
    gladptrs.glad_glUniform4uiv = (PFNGLUNIFORM4UIVPROC) load(userptr, "glUniform4uiv");
    gladptrs.glad_glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC) load(userptr, "glVertexAttribI1i");
    gladptrs.glad_glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC) load(userptr, "glVertexAttribI1iv");
    gladptrs.glad_glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC) load(userptr, "glVertexAttribI1ui");
    gladptrs.glad_glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC) load(userptr, "glVertexAttribI1uiv");
    gladptrs.glad_glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC) load(userptr, "glVertexAttribI2i");
    gladptrs.glad_glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC) load(userptr, "glVertexAttribI2iv");
    gladptrs.glad_glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC) load(userptr, "glVertexAttribI2ui");
    gladptrs.glad_glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC) load(userptr, "glVertexAttribI2uiv");
    gladptrs.glad_glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC) load(userptr, "glVertexAttribI3i");
    gladptrs.glad_glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC) load(userptr, "glVertexAttribI3iv");
    gladptrs.glad_glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC) load(userptr, "glVertexAttribI3ui");
    gladptrs.glad_glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC) load(userptr, "glVertexAttribI3uiv");
    gladptrs.glad_glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC) load(userptr, "glVertexAttribI4bv");
    gladptrs.glad_glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC) load(userptr, "glVertexAttribI4i");
    gladptrs.glad_glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC) load(userptr, "glVertexAttribI4iv");
    gladptrs.glad_glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC) load(userptr, "glVertexAttribI4sv");
    gladptrs.glad_glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC) load(userptr, "glVertexAttribI4ubv");
    gladptrs.glad_glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC) load(userptr, "glVertexAttribI4ui");
    gladptrs.glad_glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC) load(userptr, "glVertexAttribI4uiv");
    gladptrs.glad_glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC) load(userptr, "glVertexAttribI4usv");
    gladptrs.glad_glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC) load(userptr, "glVertexAttribIPointer");
}
static void glad_gl_load_GL_VERSION_3_1( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_3_1) return;
    gladptrs.glad_glBindBufferBase = (PFNGLBINDBUFFERBASEPROC) load(userptr, "glBindBufferBase");
    gladptrs.glad_glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC) load(userptr, "glBindBufferRange");
    gladptrs.glad_glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC) load(userptr, "glCopyBufferSubData");
    gladptrs.glad_glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC) load(userptr, "glDrawArraysInstanced");
    gladptrs.glad_glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC) load(userptr, "glDrawElementsInstanced");
    gladptrs.glad_glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC) load(userptr, "glGetActiveUniformBlockName");
    gladptrs.glad_glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC) load(userptr, "glGetActiveUniformBlockiv");
    gladptrs.glad_glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC) load(userptr, "glGetActiveUniformName");
    gladptrs.glad_glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC) load(userptr, "glGetActiveUniformsiv");
    gladptrs.glad_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC) load(userptr, "glGetIntegeri_v");
    gladptrs.glad_glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC) load(userptr, "glGetUniformBlockIndex");
    gladptrs.glad_glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC) load(userptr, "glGetUniformIndices");
    gladptrs.glad_glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC) load(userptr, "glPrimitiveRestartIndex");
    gladptrs.glad_glTexBuffer = (PFNGLTEXBUFFERPROC) load(userptr, "glTexBuffer");
    gladptrs.glad_glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC) load(userptr, "glUniformBlockBinding");
}
static void glad_gl_load_GL_VERSION_3_2( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_3_2) return;
    gladptrs.glad_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC) load(userptr, "glClientWaitSync");
    gladptrs.glad_glDeleteSync = (PFNGLDELETESYNCPROC) load(userptr, "glDeleteSync");
    gladptrs.glad_glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glDrawElementsBaseVertex");
    gladptrs.glad_glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC) load(userptr, "glDrawElementsInstancedBaseVertex");
    gladptrs.glad_glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC) load(userptr, "glDrawRangeElementsBaseVertex");
    gladptrs.glad_glFenceSync = (PFNGLFENCESYNCPROC) load(userptr, "glFenceSync");
    gladptrs.glad_glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC) load(userptr, "glFramebufferTexture");
    gladptrs.glad_glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC) load(userptr, "glGetBufferParameteri64v");
    gladptrs.glad_glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC) load(userptr, "glGetInteger64i_v");
    gladptrs.glad_glGetInteger64v = (PFNGLGETINTEGER64VPROC) load(userptr, "glGetInteger64v");
    gladptrs.glad_glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC) load(userptr, "glGetMultisamplefv");
    gladptrs.glad_glGetSynciv = (PFNGLGETSYNCIVPROC) load(userptr, "glGetSynciv");
    gladptrs.glad_glIsSync = (PFNGLISSYNCPROC) load(userptr, "glIsSync");
    gladptrs.glad_glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) load(userptr, "glMultiDrawElementsBaseVertex");
    gladptrs.glad_glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC) load(userptr, "glProvokingVertex");
    gladptrs.glad_glSampleMaski = (PFNGLSAMPLEMASKIPROC) load(userptr, "glSampleMaski");
    gladptrs.glad_glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC) load(userptr, "glTexImage2DMultisample");
    gladptrs.glad_glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC) load(userptr, "glTexImage3DMultisample");
    gladptrs.glad_glWaitSync = (PFNGLWAITSYNCPROC) load(userptr, "glWaitSync");
}
static void glad_gl_load_GL_VERSION_3_3( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_3_3) return;
    gladptrs.glad_glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC) load(userptr, "glBindFragDataLocationIndexed");
    gladptrs.glad_glBindSampler = (PFNGLBINDSAMPLERPROC) load(userptr, "glBindSampler");
    gladptrs.glad_glColorP3ui = (PFNGLCOLORP3UIPROC) load(userptr, "glColorP3ui");
    gladptrs.glad_glColorP3uiv = (PFNGLCOLORP3UIVPROC) load(userptr, "glColorP3uiv");
    gladptrs.glad_glColorP4ui = (PFNGLCOLORP4UIPROC) load(userptr, "glColorP4ui");
    gladptrs.glad_glColorP4uiv = (PFNGLCOLORP4UIVPROC) load(userptr, "glColorP4uiv");
    gladptrs.glad_glDeleteSamplers = (PFNGLDELETESAMPLERSPROC) load(userptr, "glDeleteSamplers");
    gladptrs.glad_glGenSamplers = (PFNGLGENSAMPLERSPROC) load(userptr, "glGenSamplers");
    gladptrs.glad_glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC) load(userptr, "glGetFragDataIndex");
    gladptrs.glad_glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC) load(userptr, "glGetQueryObjecti64v");
    gladptrs.glad_glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC) load(userptr, "glGetQueryObjectui64v");
    gladptrs.glad_glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC) load(userptr, "glGetSamplerParameterIiv");
    gladptrs.glad_glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC) load(userptr, "glGetSamplerParameterIuiv");
    gladptrs.glad_glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC) load(userptr, "glGetSamplerParameterfv");
    gladptrs.glad_glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC) load(userptr, "glGetSamplerParameteriv");
    gladptrs.glad_glIsSampler = (PFNGLISSAMPLERPROC) load(userptr, "glIsSampler");
    gladptrs.glad_glMultiTexCoordP1ui = (PFNGLMULTITEXCOORDP1UIPROC) load(userptr, "glMultiTexCoordP1ui");
    gladptrs.glad_glMultiTexCoordP1uiv = (PFNGLMULTITEXCOORDP1UIVPROC) load(userptr, "glMultiTexCoordP1uiv");
    gladptrs.glad_glMultiTexCoordP2ui = (PFNGLMULTITEXCOORDP2UIPROC) load(userptr, "glMultiTexCoordP2ui");
    gladptrs.glad_glMultiTexCoordP2uiv = (PFNGLMULTITEXCOORDP2UIVPROC) load(userptr, "glMultiTexCoordP2uiv");
    gladptrs.glad_glMultiTexCoordP3ui = (PFNGLMULTITEXCOORDP3UIPROC) load(userptr, "glMultiTexCoordP3ui");
    gladptrs.glad_glMultiTexCoordP3uiv = (PFNGLMULTITEXCOORDP3UIVPROC) load(userptr, "glMultiTexCoordP3uiv");
    gladptrs.glad_glMultiTexCoordP4ui = (PFNGLMULTITEXCOORDP4UIPROC) load(userptr, "glMultiTexCoordP4ui");
    gladptrs.glad_glMultiTexCoordP4uiv = (PFNGLMULTITEXCOORDP4UIVPROC) load(userptr, "glMultiTexCoordP4uiv");
    gladptrs.glad_glNormalP3ui = (PFNGLNORMALP3UIPROC) load(userptr, "glNormalP3ui");
    gladptrs.glad_glNormalP3uiv = (PFNGLNORMALP3UIVPROC) load(userptr, "glNormalP3uiv");
    gladptrs.glad_glQueryCounter = (PFNGLQUERYCOUNTERPROC) load(userptr, "glQueryCounter");
    gladptrs.glad_glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC) load(userptr, "glSamplerParameterIiv");
    gladptrs.glad_glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC) load(userptr, "glSamplerParameterIuiv");
    gladptrs.glad_glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC) load(userptr, "glSamplerParameterf");
    gladptrs.glad_glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC) load(userptr, "glSamplerParameterfv");
    gladptrs.glad_glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC) load(userptr, "glSamplerParameteri");
    gladptrs.glad_glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC) load(userptr, "glSamplerParameteriv");
    gladptrs.glad_glSecondaryColorP3ui = (PFNGLSECONDARYCOLORP3UIPROC) load(userptr, "glSecondaryColorP3ui");
    gladptrs.glad_glSecondaryColorP3uiv = (PFNGLSECONDARYCOLORP3UIVPROC) load(userptr, "glSecondaryColorP3uiv");
    gladptrs.glad_glTexCoordP1ui = (PFNGLTEXCOORDP1UIPROC) load(userptr, "glTexCoordP1ui");
    gladptrs.glad_glTexCoordP1uiv = (PFNGLTEXCOORDP1UIVPROC) load(userptr, "glTexCoordP1uiv");
    gladptrs.glad_glTexCoordP2ui = (PFNGLTEXCOORDP2UIPROC) load(userptr, "glTexCoordP2ui");
    gladptrs.glad_glTexCoordP2uiv = (PFNGLTEXCOORDP2UIVPROC) load(userptr, "glTexCoordP2uiv");
    gladptrs.glad_glTexCoordP3ui = (PFNGLTEXCOORDP3UIPROC) load(userptr, "glTexCoordP3ui");
    gladptrs.glad_glTexCoordP3uiv = (PFNGLTEXCOORDP3UIVPROC) load(userptr, "glTexCoordP3uiv");
    gladptrs.glad_glTexCoordP4ui = (PFNGLTEXCOORDP4UIPROC) load(userptr, "glTexCoordP4ui");
    gladptrs.glad_glTexCoordP4uiv = (PFNGLTEXCOORDP4UIVPROC) load(userptr, "glTexCoordP4uiv");
    gladptrs.glad_glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC) load(userptr, "glVertexAttribDivisor");
    gladptrs.glad_glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC) load(userptr, "glVertexAttribP1ui");
    gladptrs.glad_glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC) load(userptr, "glVertexAttribP1uiv");
    gladptrs.glad_glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC) load(userptr, "glVertexAttribP2ui");
    gladptrs.glad_glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC) load(userptr, "glVertexAttribP2uiv");
    gladptrs.glad_glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC) load(userptr, "glVertexAttribP3ui");
    gladptrs.glad_glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC) load(userptr, "glVertexAttribP3uiv");
    gladptrs.glad_glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC) load(userptr, "glVertexAttribP4ui");
    gladptrs.glad_glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC) load(userptr, "glVertexAttribP4uiv");
    gladptrs.glad_glVertexP2ui = (PFNGLVERTEXP2UIPROC) load(userptr, "glVertexP2ui");
    gladptrs.glad_glVertexP2uiv = (PFNGLVERTEXP2UIVPROC) load(userptr, "glVertexP2uiv");
    gladptrs.glad_glVertexP3ui = (PFNGLVERTEXP3UIPROC) load(userptr, "glVertexP3ui");
    gladptrs.glad_glVertexP3uiv = (PFNGLVERTEXP3UIVPROC) load(userptr, "glVertexP3uiv");
    gladptrs.glad_glVertexP4ui = (PFNGLVERTEXP4UIPROC) load(userptr, "glVertexP4ui");
    gladptrs.glad_glVertexP4uiv = (PFNGLVERTEXP4UIVPROC) load(userptr, "glVertexP4uiv");
}
static void glad_gl_load_GL_VERSION_4_0( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_4_0) return;
    gladptrs.glad_glBeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC) load(userptr, "glBeginQueryIndexed");
    gladptrs.glad_glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC) load(userptr, "glBindTransformFeedback");
    gladptrs.glad_glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC) load(userptr, "glBlendEquationSeparatei");
    gladptrs.glad_glBlendEquationi = (PFNGLBLENDEQUATIONIPROC) load(userptr, "glBlendEquationi");
    gladptrs.glad_glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC) load(userptr, "glBlendFuncSeparatei");
    gladptrs.glad_glBlendFunci = (PFNGLBLENDFUNCIPROC) load(userptr, "glBlendFunci");
    gladptrs.glad_glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC) load(userptr, "glDeleteTransformFeedbacks");
    gladptrs.glad_glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC) load(userptr, "glDrawArraysIndirect");
    gladptrs.glad_glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC) load(userptr, "glDrawElementsIndirect");
    gladptrs.glad_glDrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC) load(userptr, "glDrawTransformFeedback");
    gladptrs.glad_glDrawTransformFeedbackStream = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC) load(userptr, "glDrawTransformFeedbackStream");
    gladptrs.glad_glEndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC) load(userptr, "glEndQueryIndexed");
    gladptrs.glad_glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC) load(userptr, "glGenTransformFeedbacks");
    gladptrs.glad_glGetActiveSubroutineName = (PFNGLGETACTIVESUBROUTINENAMEPROC) load(userptr, "glGetActiveSubroutineName");
    gladptrs.glad_glGetActiveSubroutineUniformName = (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC) load(userptr, "glGetActiveSubroutineUniformName");
    gladptrs.glad_glGetActiveSubroutineUniformiv = (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC) load(userptr, "glGetActiveSubroutineUniformiv");
    gladptrs.glad_glGetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC) load(userptr, "glGetProgramStageiv");
    gladptrs.glad_glGetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC) load(userptr, "glGetQueryIndexediv");
    gladptrs.glad_glGetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC) load(userptr, "glGetSubroutineIndex");
    gladptrs.glad_glGetSubroutineUniformLocation = (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC) load(userptr, "glGetSubroutineUniformLocation");
    gladptrs.glad_glGetUniformSubroutineuiv = (PFNGLGETUNIFORMSUBROUTINEUIVPROC) load(userptr, "glGetUniformSubroutineuiv");
    gladptrs.glad_glGetUniformdv = (PFNGLGETUNIFORMDVPROC) load(userptr, "glGetUniformdv");
    gladptrs.glad_glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC) load(userptr, "glIsTransformFeedback");
    gladptrs.glad_glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC) load(userptr, "glMinSampleShading");
    gladptrs.glad_glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC) load(userptr, "glPatchParameterfv");
    gladptrs.glad_glPatchParameteri = (PFNGLPATCHPARAMETERIPROC) load(userptr, "glPatchParameteri");
    gladptrs.glad_glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC) load(userptr, "glPauseTransformFeedback");
    gladptrs.glad_glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC) load(userptr, "glResumeTransformFeedback");
    gladptrs.glad_glUniform1d = (PFNGLUNIFORM1DPROC) load(userptr, "glUniform1d");
    gladptrs.glad_glUniform1dv = (PFNGLUNIFORM1DVPROC) load(userptr, "glUniform1dv");
    gladptrs.glad_glUniform2d = (PFNGLUNIFORM2DPROC) load(userptr, "glUniform2d");
    gladptrs.glad_glUniform2dv = (PFNGLUNIFORM2DVPROC) load(userptr, "glUniform2dv");
    gladptrs.glad_glUniform3d = (PFNGLUNIFORM3DPROC) load(userptr, "glUniform3d");
    gladptrs.glad_glUniform3dv = (PFNGLUNIFORM3DVPROC) load(userptr, "glUniform3dv");
    gladptrs.glad_glUniform4d = (PFNGLUNIFORM4DPROC) load(userptr, "glUniform4d");
    gladptrs.glad_glUniform4dv = (PFNGLUNIFORM4DVPROC) load(userptr, "glUniform4dv");
    gladptrs.glad_glUniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC) load(userptr, "glUniformMatrix2dv");
    gladptrs.glad_glUniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC) load(userptr, "glUniformMatrix2x3dv");
    gladptrs.glad_glUniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC) load(userptr, "glUniformMatrix2x4dv");
    gladptrs.glad_glUniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC) load(userptr, "glUniformMatrix3dv");
    gladptrs.glad_glUniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC) load(userptr, "glUniformMatrix3x2dv");
    gladptrs.glad_glUniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC) load(userptr, "glUniformMatrix3x4dv");
    gladptrs.glad_glUniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC) load(userptr, "glUniformMatrix4dv");
    gladptrs.glad_glUniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC) load(userptr, "glUniformMatrix4x2dv");
    gladptrs.glad_glUniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC) load(userptr, "glUniformMatrix4x3dv");
    gladptrs.glad_glUniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC) load(userptr, "glUniformSubroutinesuiv");
}
static void glad_gl_load_GL_VERSION_4_1( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_4_1) return;
    gladptrs.glad_glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC) load(userptr, "glActiveShaderProgram");
    gladptrs.glad_glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC) load(userptr, "glBindProgramPipeline");
    gladptrs.glad_glClearDepthf = (PFNGLCLEARDEPTHFPROC) load(userptr, "glClearDepthf");
    gladptrs.glad_glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC) load(userptr, "glCreateShaderProgramv");
    gladptrs.glad_glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC) load(userptr, "glDeleteProgramPipelines");
    gladptrs.glad_glDepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC) load(userptr, "glDepthRangeArrayv");
    gladptrs.glad_glDepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC) load(userptr, "glDepthRangeIndexed");
    gladptrs.glad_glDepthRangef = (PFNGLDEPTHRANGEFPROC) load(userptr, "glDepthRangef");
    gladptrs.glad_glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC) load(userptr, "glGenProgramPipelines");
    gladptrs.glad_glGetDoublei_v = (PFNGLGETDOUBLEI_VPROC) load(userptr, "glGetDoublei_v");
    gladptrs.glad_glGetFloati_v = (PFNGLGETFLOATI_VPROC) load(userptr, "glGetFloati_v");
    gladptrs.glad_glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC) load(userptr, "glGetProgramBinary");
    gladptrs.glad_glGetProgramPipelineInfoLog = (PFNGLGETPROGRAMPIPELINEINFOLOGPROC) load(userptr, "glGetProgramPipelineInfoLog");
    gladptrs.glad_glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC) load(userptr, "glGetProgramPipelineiv");
    gladptrs.glad_glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC) load(userptr, "glGetShaderPrecisionFormat");
    gladptrs.glad_glGetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC) load(userptr, "glGetVertexAttribLdv");
    gladptrs.glad_glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC) load(userptr, "glIsProgramPipeline");
    gladptrs.glad_glProgramBinary = (PFNGLPROGRAMBINARYPROC) load(userptr, "glProgramBinary");
    gladptrs.glad_glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC) load(userptr, "glProgramParameteri");
    gladptrs.glad_glProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC) load(userptr, "glProgramUniform1d");
    gladptrs.glad_glProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC) load(userptr, "glProgramUniform1dv");
    gladptrs.glad_glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC) load(userptr, "glProgramUniform1f");
    gladptrs.glad_glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC) load(userptr, "glProgramUniform1fv");
    gladptrs.glad_glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC) load(userptr, "glProgramUniform1i");
    gladptrs.glad_glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC) load(userptr, "glProgramUniform1iv");
    gladptrs.glad_glProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC) load(userptr, "glProgramUniform1ui");
    gladptrs.glad_glProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC) load(userptr, "glProgramUniform1uiv");
    gladptrs.glad_glProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC) load(userptr, "glProgramUniform2d");
    gladptrs.glad_glProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC) load(userptr, "glProgramUniform2dv");
    gladptrs.glad_glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC) load(userptr, "glProgramUniform2f");
    gladptrs.glad_glProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC) load(userptr, "glProgramUniform2fv");
    gladptrs.glad_glProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC) load(userptr, "glProgramUniform2i");
    gladptrs.glad_glProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC) load(userptr, "glProgramUniform2iv");
    gladptrs.glad_glProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC) load(userptr, "glProgramUniform2ui");
    gladptrs.glad_glProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC) load(userptr, "glProgramUniform2uiv");
    gladptrs.glad_glProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC) load(userptr, "glProgramUniform3d");
    gladptrs.glad_glProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC) load(userptr, "glProgramUniform3dv");
    gladptrs.glad_glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC) load(userptr, "glProgramUniform3f");
    gladptrs.glad_glProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC) load(userptr, "glProgramUniform3fv");
    gladptrs.glad_glProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC) load(userptr, "glProgramUniform3i");
    gladptrs.glad_glProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC) load(userptr, "glProgramUniform3iv");
    gladptrs.glad_glProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC) load(userptr, "glProgramUniform3ui");
    gladptrs.glad_glProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC) load(userptr, "glProgramUniform3uiv");
    gladptrs.glad_glProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC) load(userptr, "glProgramUniform4d");
    gladptrs.glad_glProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC) load(userptr, "glProgramUniform4dv");
    gladptrs.glad_glProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC) load(userptr, "glProgramUniform4f");
    gladptrs.glad_glProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC) load(userptr, "glProgramUniform4fv");
    gladptrs.glad_glProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC) load(userptr, "glProgramUniform4i");
    gladptrs.glad_glProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC) load(userptr, "glProgramUniform4iv");
    gladptrs.glad_glProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC) load(userptr, "glProgramUniform4ui");
    gladptrs.glad_glProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC) load(userptr, "glProgramUniform4uiv");
    gladptrs.glad_glProgramUniformMatrix2dv = (PFNGLPROGRAMUNIFORMMATRIX2DVPROC) load(userptr, "glProgramUniformMatrix2dv");
    gladptrs.glad_glProgramUniformMatrix2fv = (PFNGLPROGRAMUNIFORMMATRIX2FVPROC) load(userptr, "glProgramUniformMatrix2fv");
    gladptrs.glad_glProgramUniformMatrix2x3dv = (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC) load(userptr, "glProgramUniformMatrix2x3dv");
    gladptrs.glad_glProgramUniformMatrix2x3fv = (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC) load(userptr, "glProgramUniformMatrix2x3fv");
    gladptrs.glad_glProgramUniformMatrix2x4dv = (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC) load(userptr, "glProgramUniformMatrix2x4dv");
    gladptrs.glad_glProgramUniformMatrix2x4fv = (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC) load(userptr, "glProgramUniformMatrix2x4fv");
    gladptrs.glad_glProgramUniformMatrix3dv = (PFNGLPROGRAMUNIFORMMATRIX3DVPROC) load(userptr, "glProgramUniformMatrix3dv");
    gladptrs.glad_glProgramUniformMatrix3fv = (PFNGLPROGRAMUNIFORMMATRIX3FVPROC) load(userptr, "glProgramUniformMatrix3fv");
    gladptrs.glad_glProgramUniformMatrix3x2dv = (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC) load(userptr, "glProgramUniformMatrix3x2dv");
    gladptrs.glad_glProgramUniformMatrix3x2fv = (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC) load(userptr, "glProgramUniformMatrix3x2fv");
    gladptrs.glad_glProgramUniformMatrix3x4dv = (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC) load(userptr, "glProgramUniformMatrix3x4dv");
    gladptrs.glad_glProgramUniformMatrix3x4fv = (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC) load(userptr, "glProgramUniformMatrix3x4fv");
    gladptrs.glad_glProgramUniformMatrix4dv = (PFNGLPROGRAMUNIFORMMATRIX4DVPROC) load(userptr, "glProgramUniformMatrix4dv");
    gladptrs.glad_glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC) load(userptr, "glProgramUniformMatrix4fv");
    gladptrs.glad_glProgramUniformMatrix4x2dv = (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC) load(userptr, "glProgramUniformMatrix4x2dv");
    gladptrs.glad_glProgramUniformMatrix4x2fv = (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC) load(userptr, "glProgramUniformMatrix4x2fv");
    gladptrs.glad_glProgramUniformMatrix4x3dv = (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC) load(userptr, "glProgramUniformMatrix4x3dv");
    gladptrs.glad_glProgramUniformMatrix4x3fv = (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC) load(userptr, "glProgramUniformMatrix4x3fv");
    gladptrs.glad_glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC) load(userptr, "glReleaseShaderCompiler");
    gladptrs.glad_glScissorArrayv = (PFNGLSCISSORARRAYVPROC) load(userptr, "glScissorArrayv");
    gladptrs.glad_glScissorIndexed = (PFNGLSCISSORINDEXEDPROC) load(userptr, "glScissorIndexed");
    gladptrs.glad_glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC) load(userptr, "glScissorIndexedv");
    gladptrs.glad_glShaderBinary = (PFNGLSHADERBINARYPROC) load(userptr, "glShaderBinary");
    gladptrs.glad_glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC) load(userptr, "glUseProgramStages");
    gladptrs.glad_glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC) load(userptr, "glValidateProgramPipeline");
    gladptrs.glad_glVertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC) load(userptr, "glVertexAttribL1d");
    gladptrs.glad_glVertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC) load(userptr, "glVertexAttribL1dv");
    gladptrs.glad_glVertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC) load(userptr, "glVertexAttribL2d");
    gladptrs.glad_glVertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC) load(userptr, "glVertexAttribL2dv");
    gladptrs.glad_glVertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC) load(userptr, "glVertexAttribL3d");
    gladptrs.glad_glVertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC) load(userptr, "glVertexAttribL3dv");
    gladptrs.glad_glVertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC) load(userptr, "glVertexAttribL4d");
    gladptrs.glad_glVertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC) load(userptr, "glVertexAttribL4dv");
    gladptrs.glad_glVertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC) load(userptr, "glVertexAttribLPointer");
    gladptrs.glad_glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC) load(userptr, "glViewportArrayv");
    gladptrs.glad_glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC) load(userptr, "glViewportIndexedf");
    gladptrs.glad_glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC) load(userptr, "glViewportIndexedfv");
}
static void glad_gl_load_GL_VERSION_4_2( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_4_2) return;
    gladptrs.glad_glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC) load(userptr, "glBindImageTexture");
    gladptrs.glad_glDrawArraysInstancedBaseInstance = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC) load(userptr, "glDrawArraysInstancedBaseInstance");
    gladptrs.glad_glDrawElementsInstancedBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC) load(userptr, "glDrawElementsInstancedBaseInstance");
    gladptrs.glad_glDrawElementsInstancedBaseVertexBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC) load(userptr, "glDrawElementsInstancedBaseVertexBaseInstance");
    gladptrs.glad_glDrawTransformFeedbackInstanced = (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC) load(userptr, "glDrawTransformFeedbackInstanced");
    gladptrs.glad_glDrawTransformFeedbackStreamInstanced = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC) load(userptr, "glDrawTransformFeedbackStreamInstanced");
    gladptrs.glad_glGetActiveAtomicCounterBufferiv = (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC) load(userptr, "glGetActiveAtomicCounterBufferiv");
    gladptrs.glad_glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC) load(userptr, "glGetInternalformativ");
    gladptrs.glad_glMemoryBarrier = (PFNGLMEMORYBARRIERPROC) load(userptr, "glMemoryBarrier");
    gladptrs.glad_glTexStorage1D = (PFNGLTEXSTORAGE1DPROC) load(userptr, "glTexStorage1D");
    gladptrs.glad_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC) load(userptr, "glTexStorage2D");
    gladptrs.glad_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC) load(userptr, "glTexStorage3D");
}
static void glad_gl_load_GL_VERSION_4_3( [[maybe_unused]]GLADuserptrloadfunc load, [[maybe_unused]]void* userptr) {
    if(!GLAD_GL_VERSION_4_3) return;
    gladptrs.glad_glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC) load(userptr, "glBindVertexBuffer");
    gladptrs.glad_glClearBufferData = (PFNGLCLEARBUFFERDATAPROC) load(userptr, "glClearBufferData");
    gladptrs.glad_glClearBufferSubData = (PFNGLCLEARBUFFERSUBDATAPROC) load(userptr, "glClearBufferSubData");
    gladptrs.glad_glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC) load(userptr, "glCopyImageSubData");
    gladptrs.glad_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC) load(userptr, "glDebugMessageCallback");
    gladptrs.glad_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC) load(userptr, "glDebugMessageControl");
    gladptrs.glad_glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC) load(userptr, "glDebugMessageInsert");
    gladptrs.glad_glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC) load(userptr, "glDispatchCompute");
    gladptrs.glad_glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC) load(userptr, "glDispatchComputeIndirect");
    gladptrs.glad_glFramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC) load(userptr, "glFramebufferParameteri");
    gladptrs.glad_glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC) load(userptr, "glGetDebugMessageLog");
    gladptrs.glad_glGetFramebufferParameteriv = (PFNGLGETFRAMEBUFFERPARAMETERIVPROC) load(userptr, "glGetFramebufferParameteriv");
    gladptrs.glad_glGetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC) load(userptr, "glGetInternalformati64v");
    gladptrs.glad_glGetObjectLabel = (PFNGLGETOBJECTLABELPROC) load(userptr, "glGetObjectLabel");
    gladptrs.glad_glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC) load(userptr, "glGetObjectPtrLabel");
    gladptrs.glad_glGetPointerv = (PFNGLGETPOINTERVPROC) load(userptr, "glGetPointerv");
    gladptrs.glad_glGetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC) load(userptr, "glGetProgramInterfaceiv");
    gladptrs.glad_glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC) load(userptr, "glGetProgramResourceIndex");
    gladptrs.glad_glGetProgramResourceLocation = (PFNGLGETPROGRAMRESOURCELOCATIONPROC) load(userptr, "glGetProgramResourceLocation");
    gladptrs.glad_glGetProgramResourceLocationIndex = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC) load(userptr, "glGetProgramResourceLocationIndex");
    gladptrs.glad_glGetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC) load(userptr, "glGetProgramResourceName");
    gladptrs.glad_glGetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC) load(userptr, "glGetProgramResourceiv");
    gladptrs.glad_glInvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC) load(userptr, "glInvalidateBufferData");
    gladptrs.glad_glInvalidateBufferSubData = (PFNGLINVALIDATEBUFFERSUBDATAPROC) load(userptr, "glInvalidateBufferSubData");
    gladptrs.glad_glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC) load(userptr, "glInvalidateFramebuffer");
    gladptrs.glad_glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC) load(userptr, "glInvalidateSubFramebuffer");
    gladptrs.glad_glInvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC) load(userptr, "glInvalidateTexImage");
    gladptrs.glad_glInvalidateTexSubImage = (PFNGLINVALIDATETEXSUBIMAGEPROC) load(userptr, "glInvalidateTexSubImage");
    gladptrs.glad_glMultiDrawArraysIndirect = (PFNGLMULTIDRAWARRAYSINDIRECTPROC) load(userptr, "glMultiDrawArraysIndirect");
    gladptrs.glad_glMultiDrawElementsIndirect = (PFNGLMULTIDRAWELEMENTSINDIRECTPROC) load(userptr, "glMultiDrawElementsIndirect");
    gladptrs.glad_glObjectLabel = (PFNGLOBJECTLABELPROC) load(userptr, "glObjectLabel");
    gladptrs.glad_glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC) load(userptr, "glObjectPtrLabel");
    gladptrs.glad_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC) load(userptr, "glPopDebugGroup");
    gladptrs.glad_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC) load(userptr, "glPushDebugGroup");
    gladptrs.glad_glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC) load(userptr, "glShaderStorageBlockBinding");
    gladptrs.glad_glTexBufferRange = (PFNGLTEXBUFFERRANGEPROC) load(userptr, "glTexBufferRange");
    gladptrs.glad_glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC) load(userptr, "glTexStorage2DMultisample");
    gladptrs.glad_glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC) load(userptr, "glTexStorage3DMultisample");
    gladptrs.glad_glTextureView = (PFNGLTEXTUREVIEWPROC) load(userptr, "glTextureView");
    gladptrs.glad_glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC) load(userptr, "glVertexAttribBinding");
    gladptrs.glad_glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC) load(userptr, "glVertexAttribFormat");
    gladptrs.glad_glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC) load(userptr, "glVertexAttribIFormat");
    gladptrs.glad_glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC) load(userptr, "glVertexAttribLFormat");
    gladptrs.glad_glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC) load(userptr, "glVertexBindingDivisor");
}



#if defined(GL_ES_VERSION_3_0) || defined(GL_VERSION_3_0)
#define GLAD_GL_IS_SOME_NEW_VERSION 1
#else
#define GLAD_GL_IS_SOME_NEW_VERSION 0
#endif

static int glad_gl_get_extensions( int version, const char **out_exts, unsigned int *out_num_exts_i, char ***out_exts_i) {
#if GLAD_GL_IS_SOME_NEW_VERSION
    if(GLAD_VERSION_MAJOR(version) < 3) {
#else
    GLAD_UNUSED(version);
    GLAD_UNUSED(out_num_exts_i);
    GLAD_UNUSED(out_exts_i);
#endif
        if (gladptrs.glad_glGetString == NULL) {
            return 0;
        }
        *out_exts = (const char *)gladptrs.glad_glGetString(GL_EXTENSIONS);
#if GLAD_GL_IS_SOME_NEW_VERSION
    } else {
        unsigned int index = 0;
        unsigned int num_exts_i = 0;
        char **exts_i = NULL;
        if (gladptrs.glad_glGetStringi == NULL || gladptrs.glad_glGetIntegerv == NULL) {
            return 0;
        }
        gladptrs.glad_glGetIntegerv(GL_NUM_EXTENSIONS, (int*) &num_exts_i);
        if (num_exts_i > 0) {
            exts_i = (char **) malloc(num_exts_i * (sizeof *exts_i));
        }
        if (exts_i == NULL) {
            return 0;
        }
        for(index = 0; index < num_exts_i; index++) {
            const char *gl_str_tmp = (const char*) gladptrs.glad_glGetStringi(GL_EXTENSIONS, index);
            size_t len = strlen(gl_str_tmp) + 1;

            char *local_str = (char*) malloc(len * sizeof(char));
            if(local_str != NULL) {
                memcpy(local_str, gl_str_tmp, len * sizeof(char));
            }

            exts_i[index] = local_str;
        }

        *out_num_exts_i = num_exts_i;
        *out_exts_i = exts_i;
    }
#endif
    return 1;
}
static void glad_gl_free_extensions(char **exts_i, unsigned int num_exts_i) {
    if (exts_i != NULL) {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            free((void *) (exts_i[index]));
        }
        free((void *)exts_i);
        exts_i = NULL;
    }
}
static int glad_gl_has_extension(int version, const char *exts, unsigned int num_exts_i, char **exts_i, const char *ext) {
    if(GLAD_VERSION_MAJOR(version) < 3 || !GLAD_GL_IS_SOME_NEW_VERSION) {
        const char *extensions;
        const char *loc;
        const char *terminator;
        extensions = exts;
        if(extensions == NULL || ext == NULL) {
            return 0;
        }
        while(1) {
            loc = strstr(extensions, ext);
            if(loc == NULL) {
                return 0;
            }
            terminator = loc + strlen(ext);
            if((loc == extensions || *(loc - 1) == ' ') &&
                (*terminator == ' ' || *terminator == '\0')) {
                return 1;
            }
            extensions = terminator;
        }
    } else {
        unsigned int index;
        for(index = 0; index < num_exts_i; index++) {
            const char *e = exts_i[index];
            if(strcmp(e, ext) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static GLADapiproc glad_gl_get_proc_from_userptr(void *userptr, const char* name) {
    return (GLAD_GNUC_EXTENSION (GLADapiproc (*)(const char *name)) userptr)(name);
}

static int glad_gl_find_extensions_gl( int version) {
    const char *exts = NULL;
    unsigned int num_exts_i = 0;
    char **exts_i = NULL;
    if (!glad_gl_get_extensions(version, &exts, &num_exts_i, &exts_i)) return 0;

    GLAD_GL_EXT_texture_filter_anisotropic = glad_gl_has_extension(version, exts, num_exts_i, exts_i, "GL_EXT_texture_filter_anisotropic");

    glad_gl_free_extensions(exts_i, num_exts_i);

    return 1;
}

static int glad_gl_find_core_gl(void) {
    int i;
    const char* version;
    const char* prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        "OpenGL SC ",
        NULL
    };
    int major = 0;
    int minor = 0;
    version = (const char*) gladptrs.glad_glGetString(GL_VERSION);
    if (!version) return 0;
    for (i = 0;  prefixes[i];  i++) {
        const size_t length = strlen(prefixes[i]);
        if (strncmp(version, prefixes[i], length) == 0) {
            version += length;
            break;
        }
    }

    GLAD_IMPL_UTIL_SSCANF(version, "%d.%d", &major, &minor);

    GLAD_GL_VERSION_1_0 = (major == 1 && minor >= 0) || major > 1;
    GLAD_GL_VERSION_1_1 = (major == 1 && minor >= 1) || major > 1;
    GLAD_GL_VERSION_1_2 = (major == 1 && minor >= 2) || major > 1;
    GLAD_GL_VERSION_1_3 = (major == 1 && minor >= 3) || major > 1;
    GLAD_GL_VERSION_1_4 = (major == 1 && minor >= 4) || major > 1;
    GLAD_GL_VERSION_1_5 = (major == 1 && minor >= 5) || major > 1;
    GLAD_GL_VERSION_2_0 = (major == 2 && minor >= 0) || major > 2;
    GLAD_GL_VERSION_2_1 = (major == 2 && minor >= 1) || major > 2;
    GLAD_GL_VERSION_3_0 = (major == 3 && minor >= 0) || major > 3;
    GLAD_GL_VERSION_3_1 = (major == 3 && minor >= 1) || major > 3;
    GLAD_GL_VERSION_3_2 = (major == 3 && minor >= 2) || major > 3;
    GLAD_GL_VERSION_3_3 = (major == 3 && minor >= 3) || major > 3;
    GLAD_GL_VERSION_4_0 = (major == 4 && minor >= 0) || major > 4;
    GLAD_GL_VERSION_4_1 = (major == 4 && minor >= 1) || major > 4;
    GLAD_GL_VERSION_4_2 = (major == 4 && minor >= 2) || major > 4;
    GLAD_GL_VERSION_4_3 = (major == 4 && minor >= 3) || major > 4;

    return GLAD_MAKE_VERSION(major, minor);
}

int gladLoadGLUserPtr( GLADuserptrloadfunc load, void *userptr) {
    int version;

    gladptrs.glad_glGetString = (PFNGLGETSTRINGPROC) load(userptr, "glGetString");
    if(gladptrs.glad_glGetString == NULL) return 0;
    if(gladptrs.glad_glGetString(GL_VERSION) == NULL) return 0;
    version = glad_gl_find_core_gl();

    glad_gl_load_GL_VERSION_1_0(load, userptr);
    glad_gl_load_GL_VERSION_1_1(load, userptr);
    glad_gl_load_GL_VERSION_1_2(load, userptr);
    glad_gl_load_GL_VERSION_1_3(load, userptr);
    glad_gl_load_GL_VERSION_1_4(load, userptr);
    glad_gl_load_GL_VERSION_1_5(load, userptr);
    glad_gl_load_GL_VERSION_2_0(load, userptr);
    glad_gl_load_GL_VERSION_2_1(load, userptr);
    glad_gl_load_GL_VERSION_3_0(load, userptr);
    glad_gl_load_GL_VERSION_3_1(load, userptr);
    glad_gl_load_GL_VERSION_3_2(load, userptr);
    glad_gl_load_GL_VERSION_3_3(load, userptr);
    glad_gl_load_GL_VERSION_4_0(load, userptr);
    glad_gl_load_GL_VERSION_4_1(load, userptr);
    glad_gl_load_GL_VERSION_4_2(load, userptr);
    glad_gl_load_GL_VERSION_4_3(load, userptr);

    if (!glad_gl_find_extensions_gl(version)) return 0;



    return version;
}


int gladLoadGL( GLADloadfunc load) {
    return gladLoadGLUserPtr( glad_gl_get_proc_from_userptr, GLAD_GNUC_EXTENSION (void*) load);
}





#ifdef GLAD_GL

#ifndef GLAD_LOADER_LIBRARY_C_
#define GLAD_LOADER_LIBRARY_C_

#include <stddef.h>
#include <stdlib.h>

#if GLAD_PLATFORM_WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif


static void* glad_get_dlopen_handle(const char *lib_names[], int length) {
    void *handle = NULL;
    int i;

    for (i = 0; i < length; ++i) {
#if GLAD_PLATFORM_WIN32
  #if GLAD_PLATFORM_UWP
        size_t buffer_size = (strlen(lib_names[i]) + 1) * sizeof(WCHAR);
        LPWSTR buffer = (LPWSTR) malloc(buffer_size);
        if (buffer != NULL) {
            int ret = MultiByteToWideChar(CP_ACP, 0, lib_names[i], -1, buffer, buffer_size);
            if (ret != 0) {
                handle = (void*) LoadPackagedLibrary(buffer, 0);
            }
            free((void*) buffer);
        }
  #else
        handle = (void*) LoadLibraryA(lib_names[i]);
  #endif
#else
        handle = dlopen(lib_names[i], RTLD_LAZY | RTLD_LOCAL);
#endif
        if (handle != NULL) {
            return handle;
        }
    }

    return NULL;
}

static void glad_close_dlopen_handle(void* handle) {
    if (handle != NULL) {
#if GLAD_PLATFORM_WIN32
        FreeLibrary((HMODULE) handle);
#else
        dlclose(handle);
#endif
    }
}

static GLADapiproc glad_dlsym_handle(void* handle, const char *name) {
    if (handle == NULL) {
        return NULL;
    }

#if GLAD_PLATFORM_WIN32
    return (GLADapiproc) GetProcAddress((HMODULE) handle, name);
#else
    return GLAD_GNUC_EXTENSION (GLADapiproc) dlsym(handle, name);
#endif
}

#endif /* GLAD_LOADER_LIBRARY_C_ */

typedef void* (GLAD_API_PTR *GLADglprocaddrfunc)(const char*);
struct _glad_gl_userptr {
    void *handle;
    GLADglprocaddrfunc gl_get_proc_address_ptr;
};

static GLADapiproc glad_gl_get_proc(void *vuserptr, const char *name) {
    struct _glad_gl_userptr userptr = *(struct _glad_gl_userptr*) vuserptr;
    GLADapiproc result = NULL;

    if(userptr.gl_get_proc_address_ptr != NULL) {
        result = GLAD_GNUC_EXTENSION (GLADapiproc) userptr.gl_get_proc_address_ptr(name);
    }
    if(result == NULL) {
        result = glad_dlsym_handle(userptr.handle, name);
    }

    return result;
}

static void* _glad_GL_loader_handle = NULL;

static void* glad_gl_dlopen_handle(void) {
#if GLAD_PLATFORM_APPLE
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#elif GLAD_PLATFORM_WIN32
    static const char *NAMES[] = {"opengl32.dll"};
#else
    static const char *NAMES[] = {
  #if defined(__CYGWIN__)
        "libGL-1.so",
  #endif
        "libGL.so.1",
        "libGL.so"
    };
#endif

    if (_glad_GL_loader_handle == NULL) {
        _glad_GL_loader_handle = glad_get_dlopen_handle(NAMES, sizeof(NAMES) / sizeof(NAMES[0]));
    }

    return _glad_GL_loader_handle;
}

static struct _glad_gl_userptr glad_gl_build_userptr(void *handle) {
    struct _glad_gl_userptr userptr;

    userptr.handle = handle;
#if GLAD_PLATFORM_APPLE || defined(__HAIKU__)
    userptr.gl_get_proc_address_ptr = NULL;
#elif GLAD_PLATFORM_WIN32
    userptr.gl_get_proc_address_ptr =
        (GLADglprocaddrfunc) glad_dlsym_handle(handle, "wglGetProcAddress");
#else
    userptr.gl_get_proc_address_ptr =
        (GLADglprocaddrfunc) glad_dlsym_handle(handle, "glXGetProcAddressARB");
#endif

    return userptr;
}

int gladLoaderLoadGL(void) {
    int version = 0;
    void *handle;
    int did_load = 0;
    struct _glad_gl_userptr userptr;

    did_load = _glad_GL_loader_handle == NULL;
    handle = glad_gl_dlopen_handle();
    if (handle) {
        userptr = glad_gl_build_userptr(handle);

        version = gladLoadGLUserPtr(glad_gl_get_proc, &userptr);

        if (did_load) {
            gladLoaderUnloadGL();
        }
    }

    return version;
}



void gladLoaderUnloadGL(void) {
    if (_glad_GL_loader_handle != NULL) {
        glad_close_dlopen_handle(_glad_GL_loader_handle);
        _glad_GL_loader_handle = NULL;
    }
}

#endif /* GLAD_GL */

#ifdef __cplusplus
}
#endif
