#include "RenderState.h"
#include "GPUContext.h"
#include "BatchRenderer.h"
#include "stdafx.h"
#include "ZzzOpenglUtil.h"
#include "ZzzTexture.h"
#include "ZzzBMD.h"
#include "ZzzInfomation.h"
#include "zzzObject.h"
#include "zzzcharacter.h"
#include "Zzzinfomation.h"
#include "NewUISystem.h"
#include "CBMu/Render/CBMu_CPUHotspotProfiler.h"
#include "CBMu/Render/CBMu_EffectBudget.h"
#include "CBMu/Render/CBMu_GLDebugCounters.h"
#include "CBMu/Render/CBMu_GLSpriteBatch.h"
#include "CBMu/Render/CBMu_GLStateCache.h"
#include "GPUContext.h"
#include "BatchRenderer.h"

void FlushSpriteBatch();

static inline int ResolveTextureNumber(int Texture)
{
	if (Texture <= 0) return Texture;
	BITMAP_t* pBitmap = Bitmaps.FindTexture(static_cast<GLuint>(Texture));
	if (pBitmap && pBitmap->TextureNumber > 0)
	{
		return static_cast<int>(pBitmap->TextureNumber);
	}
	return Texture;
}

float Distance;
vec3_t CollisionPosition;
int     OpenglWindowX;
int     OpenglWindowY;
int     OpenglWindowWidth;
int     OpenglWindowHeight;
bool    CameraTopViewEnable = false;
float   CameraViewNear = 20.f;
float   CameraViewFar = 2000.f;
float   CameraFOV = 55.f;
vec3_t  CameraPosition;
vec3_t  CameraAngle;
float   CameraMatrix[3][4];
vec3_t  MousePosition;
vec3_t  MouseTarget;
float   g_fCameraCustomDistance = 0.f;
bool    FogEnable = false;
GLfloat FogDensity = 0.00039999999;


CBMu_GLStateCache g_CBMuGLStateCache;
DWORD BlockMouseWheel = 0;
int mMAX_JOIN = 0;
int mMAX_POINTS = 0;
int mMAX_POINTERS = 0;
int mMAX_EFFECTS = 0;
int mMAX_SKILL_EFFECTS = 0;
int mMAX_PARTICLES = 0;
float Zoom3D = 0.0f;
float Camera3DPosY = 0.0f;
float Camera3DPosZ = 0.0f;
bool Camera3DZoom = false;
bool Camera3DSetMove = false;
bool Camera3DEnable = false;
bool Camera3DSet = false;
bool g_RenderEff = true;

int  CachTexture = -1;
bool TextureEnable;
bool DepthTestEnable;
bool CullFaceEnable;
bool DepthMaskEnable;
bool AlphaTestEnable;
int  AlphaBlendType;


unsigned int WindowWidth = 1024;
unsigned int WindowHeight = 768;
int MouseX = WindowWidth / 2;
int MouseY = WindowHeight / 2;
int BackMouseX = MouseX;
int BackMouseY = MouseY;
int MouseRenderX = WindowWidth / 2;
int MouseRenderY = WindowHeight / 2;

bool  MouseLButton;
bool  MouseLButtonPop;
bool  MouseLButtonPush;
bool  MouseRButton;
bool  MouseRButtonPop;
bool  MouseRButtonPush;
bool  MouseLButtonDBClick;
bool  MouseMButton;
bool  MouseMButtonPop;
bool  MouseMButtonPush;
int   MouseWheel;
DWORD MouseRButtonPress = 0;
GLfloat FogColor[4] = { 30 / 256.f, 20 / 256.f, 10 / 256.f, 256.f / 256.f };


#ifdef LDS_ADD_MULTISAMPLEANTIALIASING
BOOL	g_bActivityProcessMSAA = true;
BOOL	g_bSupportedMSAA = FALSE;
BOOL	g_bIsNowRecreationingForMSAA = FALSE;
int		g_iMSAALevel = DEFAULT_MSAAVALUE;
#endif 
void OpenExploper(char* Name, char* para)
{
	ShellExecute(NULL, "open", Name, para, "", SW_SHOW);
}

bool CheckID_HistoryDay(char* Name, WORD day)
{
	typedef struct  __day_history__
	{
		char ID[MAX_ID_SIZE + 1];
		WORD date;
	}dayHistory;

	FILE* fp;
	dayHistory days[100] = { 0, };
	int   count = 0;
	WORD  num = 0;
	bool  sameName = false;
	bool  update = true;

	if ((fp = fopen("dconfig.ini", "rb")) != NULL)
	{
		fread(&num, sizeof(WORD), 1, fp);

		if (num > 100)
		{
			num = 0;
		}
		else
		{
			for (int i = 0; i < num; ++i)
			{
				fread(days[i].ID, sizeof(char), MAX_ID_SIZE + 1, fp);
				fread(&days[i].date, sizeof(WORD), 1, fp);

				if (!strcmp(days[i].ID, Name))
				{
					sameName = true;
					if (days[i].date == day)
					{
						update = false;
						break;
					}
					days[i].date = day;
				}
				count++;
			}
		}
		fclose(fp);
	}

	if (update)
	{
		if (!sameName)
		{
			memcpy(days[num].ID, Name, (MAX_ID_SIZE + 1) * sizeof(char));
			days[num].date = day;

			num++;
		}

		fp = fopen("dconfig.ini", "wb");

		fwrite(&num, sizeof(WORD), 1, fp);
		for (int i = 0; i < num; ++i)
		{
			fwrite(days[i].ID, sizeof(char), MAX_ID_SIZE + 1, fp);
			fwrite(&days[i].date, sizeof(WORD), 1, fp);
		}

		fclose(fp);
	}



	return  update;
}

bool GrabEnable = false;
char GrabFileName[MAX_PATH];
int  GrabScreen = 0;
bool GrabFirst = false;

void SaveScreen()
{
	GrabFirst = true;

	if (GPUContext::Instance().IsInitialized())
	{
		GPUContext::Instance().RequestScreenshot(GrabFileName);
		GrabScreen++;
		GrabScreen %= 10000;
		return;
	}

	unsigned char* Buffer = new unsigned char[(int)WindowWidth * (int)WindowHeight * 3];
	glReadPixels(0, 0, (int)WindowWidth, (int)WindowHeight, GL_RGB, GL_UNSIGNED_BYTE, Buffer);
	WriteJpeg(GrabFileName, (int)WindowWidth, (int)WindowHeight, Buffer, 100);
	SAFE_DELETE_ARRAY(Buffer);

	GrabScreen++;
	GrabScreen %= 10000;
}

float PerspectiveX;
float PerspectiveY;
int   ScreenCenterX;
int   ScreenCenterY;
int   ScreenCenterYFlip;
glm::mat4 g_CurrentProjectionMatrix = glm::mat4(1.0f);

void GetOpenGLMatrix(float Matrix[3][4])
{
	float OpenGLMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, OpenGLMatrix);
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			Matrix[i][j] = OpenGLMatrix[j * 4 + i];
		}
	}
}

void gluPerspective2(float Fov, float Aspect, float ZNear, float ZFar)
{
	gluPerspective(Fov, Aspect, ZNear, ZFar);
	g_CurrentProjectionMatrix = glm::perspective(glm::radians(Fov), Aspect, ZNear, ZFar);

	ScreenCenterX = OpenglWindowX + OpenglWindowWidth / 2;
	ScreenCenterY = OpenglWindowY + OpenglWindowHeight / 2;
	ScreenCenterYFlip = WindowWidth - ScreenCenterY;

	float AspectY = (float)(WindowHeight) / (float)(OpenglWindowHeight);
	PerspectiveX = tanf(Fov * 0.5f * Q_PI / 180.f) / (float)(OpenglWindowWidth / 2) * Aspect;
	PerspectiveY = tanf(Fov * 0.5f * Q_PI / 180.f) / (float)(OpenglWindowHeight / 2) * AspectY;
}

void CreateScreenVector(int sx, int sy, vec3_t Target, bool bFixView, bool bFixScreen)
{
	vec3_t p1, p2;

	if (bFixScreen)
	{
		sx = ConvertNoX(sx);
		sy = ConvertNoY(sy);
	}
	else
	{
		sx = sx * g_fScreenRate_x;
		sy = sy * g_fScreenRate_y;
	}

	if (bFixView)
	{
		p1[0] = (float)(sx - ScreenCenterX) * CameraViewFar * PerspectiveX;
		p1[1] = -(float)(sy - ScreenCenterY) * CameraViewFar * PerspectiveY;
		p1[2] = -CameraViewFar;
	}
	else
	{
		p1[0] = (float)(sx - ScreenCenterX) * RENDER_ITEMVIEW_FAR * PerspectiveX;
		p1[1] = -(float)(sy - ScreenCenterY) * RENDER_ITEMVIEW_FAR * PerspectiveY;
		p1[2] = -RENDER_ITEMVIEW_FAR;
	}

	p2[0] = -CameraMatrix[0][3];
	p2[1] = -CameraMatrix[1][3];
	p2[2] = -CameraMatrix[2][3];
	VectorIRotate(p2, CameraMatrix, MousePosition);
	VectorIRotate(p1, CameraMatrix, p2);
	VectorAdd(MousePosition, p2, Target);
}

void CreateScreenVector(int sx, int sy, vec3_t Target, bool bFixView)
{
	CreateScreenVector(sx, sy, Target, bFixView, false);
}

void Projection(vec3_t Position, int* sx, int* sy)
{
	vec3_t TrasformPosition;
	VectorTransform(Position, CameraMatrix, TrasformPosition);
	*sx = ScreenCenterX - (int)(TrasformPosition[0] / PerspectiveX / TrasformPosition[2]);
	*sy = ScreenCenterY + (int)(TrasformPosition[1] / PerspectiveY / TrasformPosition[2]);
	*sx = *sx / g_fScreenRate_x;
	*sy = *sy / g_fScreenRate_y;
}

void Projection2(vec3_t Position, int* sx, int* sy)
{
	vec3_t TrasformPosition;
	VectorTransform(Position, CameraMatrix, TrasformPosition);
	*sx = -(int)(TrasformPosition[0] / PerspectiveX / TrasformPosition[2]) + ScreenCenterX;
	*sy = (int)(TrasformPosition[1] / PerspectiveY / TrasformPosition[2]) + ScreenCenterY;
}

void TransformPosition(vec3_t Position, vec3_t WorldPosition, int* x, int* y)
{
	vec3_t Temp;
	VectorSubtract(Position, CameraPosition, Temp);
	VectorRotate(Temp, CameraMatrix, WorldPosition);

	*x = (int)(WorldPosition[0] / PerspectiveX / -WorldPosition[2]) + (ScreenCenterX);
	*y = (int)(WorldPosition[1] / PerspectiveY / -WorldPosition[2]) + (ScreenCenterYFlip);
}

bool TestDepthBuffer(vec3_t Position)
{
	vec3_t WorldPosition;
	int x, y;
	TransformPosition(Position, WorldPosition, &x, &y);
	if (x < OpenglWindowX ||
		y < OpenglWindowY ||
		x >= (int)OpenglWindowX + OpenglWindowWidth ||
		y >= (int)OpenglWindowY + OpenglWindowHeight) return false;

	return (WorldPosition[2] < 0.0f);
}





//void InvalidateTextureCache()
//{
//	CachTexture = -1;
//#if CBMu_ENABLE_GL_STATE_CACHE
//	g_CBMuGLStateCache.InvalidateTexture2D();
//#endif
//}

void BindTexture(int tex)
{
	GLuint texture = 0;
	if (tex >= 0)
	{
		BITMAP_t* b = &Bitmaps[tex];
		texture = b->TextureNumber;
	}
	else
	{
		texture = static_cast<GLuint>(-1 * tex);
	}

	if (CachTexture != tex)
	{
		CachTexture = tex;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetTexture2D(texture);
#endif
	}
	else
	{
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetTexture2D(texture);
#endif
	}
}

bool TextureStream = false;

extern  int test;

void BindTextureStream(int tex)
{
}

void EndTextureStream()
{
	TextureStream = false;
}

void EnableDepthTest()
{
	if (!DepthTestEnable)
	{
		FlushSpriteBatch();
		DepthTestEnable = true;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetDepthTest(true);
#else
		glEnable(GL_DEPTH_TEST);
#endif
	}
}

void DisableDepthTest()
{
	if (DepthTestEnable)
	{
		FlushSpriteBatch();
		DepthTestEnable = false;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetDepthTest(false);
#else
		glDisable(GL_DEPTH_TEST);
#endif
	}
}

void EnableDepthMask()
{
	if (!DepthMaskEnable)
	{
		DepthMaskEnable = true;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetDepthWrite(true);
#else
		glDepthMask(true);
#endif
	}
}

void DisableDepthMask()
{
	if (DepthMaskEnable)
	{
		DepthMaskEnable = false;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetDepthWrite(false);
#else
		glDepthMask(false);
#endif
	}
}

void EnableCullFace()
{
	if (!CullFaceEnable)
	{
		CullFaceEnable = true;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetCullFace(true);
#else
		glEnable(GL_CULL_FACE);
#endif
	}
}

void DisableCullFace()
{
	if (CullFaceEnable)
	{
		CullFaceEnable = false;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetCullFace(false);
#else
		glDisable(GL_CULL_FACE);
#endif
	}
}

void DisableTexture(bool AlphaTest)
{
	EnableDepthMask();
	if (AlphaTest == true)
	{
		if (!AlphaTestEnable)
		{
			AlphaTestEnable = true;
			glEnable(GL_ALPHA_TEST);
		}
	}
	else
	{
		if (AlphaTestEnable)
		{
			AlphaTestEnable = false;
			glDisable(GL_ALPHA_TEST);
		}
	}
	if (TextureEnable)
	{
		TextureEnable = false;
		glDisable(GL_TEXTURE_2D);
	}
}

void DisableAlphaBlend()
{
	if (AlphaBlendType != 0)
	{
		FlushSpriteBatch();
		AlphaBlendType = 0;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(false);
#else
		glDisable(GL_BLEND);
#endif
	}
	EnableCullFace();
	EnableDepthMask();
	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glEnable(GL_FOG);
}

void EnableAlphaTest(bool DepthMask)
{
	if (AlphaBlendType != 2)
	{
		FlushSpriteBatch();
		AlphaBlendType = 2;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	}
	DisableCullFace();

	if (DepthMask)
		EnableDepthMask();

	if (!AlphaTestEnable)
	{
		AlphaTestEnable = true;
		glEnable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glEnable(GL_FOG);
}

void EnableAlphaBlend()
{
	if (AlphaBlendType != 3)
	{
		FlushSpriteBatch();
		AlphaBlendType = 3;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_ONE, GL_ONE);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
#endif
	}
	DisableCullFace();
	DisableDepthMask();
	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glDisable(GL_FOG);
}

void EnableAlphaBlendMinus()
{
	if (AlphaBlendType != 4)
	{
		FlushSpriteBatch();
		AlphaBlendType = 4;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
#endif
	}

	DisableCullFace();
	DisableDepthMask();

	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable) glEnable(GL_FOG);
}

void EnableAlphaBlend2()
{
	if (AlphaBlendType != 5)
	{
		AlphaBlendType = 5;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE);
#endif
	}
	DisableCullFace();
	DisableDepthMask();
	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glEnable(GL_FOG);
}

void EnableAlphaBlend3()
{
	if (AlphaBlendType != 6)
	{
		AlphaBlendType = 6;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
	}
	DisableCullFace();
	DisableDepthMask();
	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glEnable(GL_FOG);
}

void EnableAlphaBlend4()
{
	if (AlphaBlendType != 7)
	{
		AlphaBlendType = 7;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
#endif
	}
	DisableCullFace();
	DisableDepthMask();
	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glEnable(GL_FOG);
}

void EnableLightMap()
{
	if (AlphaBlendType != 1)
	{
		AlphaBlendType = 1;
#if CBMu_ENABLE_GL_STATE_CACHE
		g_CBMuGLStateCache.SetBlendEnabled(true);
		g_CBMuGLStateCache.SetBlendFunc(GL_ZERO, GL_SRC_COLOR);
#else
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
#endif

	}

	EnableCullFace();
	EnableDepthMask();

	if (AlphaTestEnable)
	{
		AlphaTestEnable = false;
		glDisable(GL_ALPHA_TEST);
	}
	if (!TextureEnable)
	{
		TextureEnable = true;
		glEnable(GL_TEXTURE_2D);
	}
	if (FogEnable)
		glEnable(GL_FOG);
}

void glViewport2(int x, int y, int Width, int Height)
{
	OpenglWindowX = x;
	OpenglWindowY = y;
	OpenglWindowWidth = Width;
	OpenglWindowHeight = Height;
	glViewport(x, WindowHeight - (y + Height), Width, Height);
}

float ConvertX(float x)
{
	return x * g_fScreenRate_x;
}

float ConvertY(float y)
{
	return y * g_fScreenRate_y;
}

float ConvertNoX(float x)
{
	return (float)((double)WindowWidth * x / 640.0);
}

float ConvertNoY(float y)
{
	return (float)((double)WindowHeight * y / 480.0);
}

extern float colorworld[4];
void BeginOpengl(int x, int y, int Width, int Height, bool Screen)
{
	CBatchRenderer::Instance().FlushImageBatchesNow();
	CBMu_CPUHotspotBeginFrame();
#if CBMu_ENABLE_GL_STATE_CACHE
	g_CBMuGLStateCache.Reset();
#endif
	if (Screen)
	{
		x = ConvertX(x);
		y = ConvertY(y);
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}
	else
	{
		x = ConvertNoX(x);
		y = ConvertNoY(y);
		Width = ConvertNoX(Width);
		Height = ConvertNoY(Height);
	}

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glViewport2(x, y, Width, Height);

	gluPerspective2(CameraFOV, (float)Width / (float)Height, CameraViewNear, CameraViewFar * 1.4f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRotatef(CameraAngle[1], 0.f, 1.f, 0.f);
	if (CameraTopViewEnable == false)
		glRotatef(CameraAngle[0], 1.f, 0.f, 0.f);
	glRotatef(CameraAngle[2], 0.f, 0.f, 1.f);
	glTranslatef(-CameraPosition[0], -CameraPosition[1], -CameraPosition[2]);

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
#if CBMu_ENABLE_GL_STATE_CACHE
	g_CBMuGLStateCache.SetDepthTest(true);
	g_CBMuGLStateCache.SetCullFace(true);
	g_CBMuGLStateCache.SetDepthWrite(true);
#else
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthMask(true);
#endif
	AlphaTestEnable = false;
	TextureEnable = true;
	DepthTestEnable = true;
	CullFaceEnable = true;
	DepthMaskEnable = true;
	glDepthFunc(GL_LEQUAL);
	glAlphaFunc(GL_GREATER, 0.25f);

	if (FogEnable)
	{
		glEnable(GL_FOG);
		glFogfv(GL_FOG_COLOR, colorworld);
		glFogf(GL_FOG_DENSITY, FogDensity);


		glFogf(GL_FOG_MODE, GL_LINEAR);
		glFogf(GL_FOG_START, CameraViewFar * 0.6f);
		glFogf(GL_FOG_END, CameraViewFar * 1.0f);
	}
	else
	{
		glDisable(GL_FOG);
	}

	GetOpenGLMatrix(CameraMatrix);
	glm::mat4 view(1.0f);
	view = glm::rotate(view, glm::radians(CameraAngle[1]), glm::vec3(0.0f, 1.0f, 0.0f));
	if (CameraTopViewEnable == false)
		view = glm::rotate(view, glm::radians(CameraAngle[0]), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, glm::radians(CameraAngle[2]), glm::vec3(0.0f, 0.0f, 1.0f));
	view = glm::translate(view, glm::vec3(-CameraPosition[0], -CameraPosition[1], -CameraPosition[2]));

	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 4; ++c) {
			CameraMatrix[r][c] = view[c][r];
		}
	}
}

void BeginOpengl(int a, int b, int c, int d)
{
	BeginOpengl(a, b, c, d, false);
}

void EndOpengl()
{
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	CBMu_CPUHotspotEndFrame();
}

void UpdateMousePositionn()
{
	vec3_t vPos;

	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(-CameraPosition[0], -CameraPosition[1], -CameraPosition[2]));
	for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 4; ++c) {
			CameraMatrix[r][c] = view[c][r];
		}
	}

	glLoadIdentity();
	glTranslatef(-CameraPosition[0], -CameraPosition[1], -CameraPosition[2]);

	Vector(-CameraMatrix[0][3], -CameraMatrix[1][3], -CameraMatrix[2][3], vPos);
	VectorIRotate(vPos, CameraMatrix, MousePosition);
}

void TEXCOORD(float* c, float u, float v)
{
	c[0] = u;
	c[1] = v;
}

void RenderBox(float Matrix[3][4])
{
}


struct SpriteBatchVertex {
	float x, y, z;
	float u, v;
	float r, g, b, a;
};

static const int MAX_SPRITE_BATCH_VERTS = 16384;
static SpriteBatchVertex s_SpriteBatchVerts[MAX_SPRITE_BATCH_VERTS];
static int s_SpriteBatchCount = 0;
static int s_SpriteBatchTexture = -1;
static int s_SpriteBatchBlendType = -1;
static bool s_SpriteBatchAlphaTest = false;
static bool s_SpriteBatchDepthMask = true;
static bool s_SpriteBatchDepthTest = true;
static GLuint s_SpriteBatchVBO = 0;

void FlushSpriteBatch()
{
	if (s_SpriteBatchCount == 0)
		return;

	
		std::vector<TerrainVertex_t> vkVerts;
		std::vector<uint32_t> vkIndices;
		vkVerts.reserve(s_SpriteBatchCount);
		vkIndices.reserve(s_SpriteBatchCount / 4 * 6);

		for (int i = 0; i < s_SpriteBatchCount; i += 4)
		{
			uint32_t baseIdx = (uint32_t)vkVerts.size();
			for (int j = 0; j < 4; ++j)
			{
				const auto& sv = s_SpriteBatchVerts[i + j];
				TerrainVertex_t tv;
				tv.pos[0] = sv.x;
				tv.pos[1] = sv.y;
				tv.pos[2] = sv.z;
				tv.uv[0] = sv.u;
				tv.uv[1] = sv.v;
				uint32_t r = (uint32_t)((std::min)((std::max)(sv.r, 0.0f), 1.0f) * 255.0f);
				uint32_t g = (uint32_t)((std::min)((std::max)(sv.g, 0.0f), 1.0f) * 255.0f);
				uint32_t b = (uint32_t)((std::min)((std::max)(sv.b, 0.0f), 1.0f) * 255.0f);
				uint32_t a = (uint32_t)((std::min)((std::max)(sv.a, 0.0f), 1.0f) * 255.0f);
				tv.color = (a << 24) | (b << 16) | (g << 8) | r;
				vkVerts.push_back(tv);
			}
			vkIndices.push_back(baseIdx + 0);
			vkIndices.push_back(baseIdx + 1);
			vkIndices.push_back(baseIdx + 2);
			vkIndices.push_back(baseIdx + 0);
			vkIndices.push_back(baseIdx + 2);
			vkIndices.push_back(baseIdx + 3);
		}

		GPUContext::TerrainMergedBatch batch;
		if (s_SpriteBatchBlendType == 3 || s_SpriteBatchBlendType == 5 || s_SpriteBatchBlendType == 7)
		{
			batch.batchType = s_SpriteBatchDepthTest ? TERRAIN_BATCH_GRASS_ADD : 7; // Additive
		}
		else if (s_SpriteBatchBlendType == 4)
		{
			batch.batchType = 5; // Dark
		}
		else if (s_SpriteBatchBlendType == 0)
		{
			batch.batchType = TERRAIN_BATCH_OPAQUE; // Opaque
		}
		else
		{
			batch.batchType = s_SpriteBatchDepthTest ? TERRAIN_BATCH_ALPHA : 6; // Alpha Blend
		}
		BITMAP_t* pBitmap = Bitmaps.FindTexture(s_SpriteBatchTexture);
		batch.textureIndex = (pBitmap && pBitmap->TextureNumber > 0) ? (int)pBitmap->TextureNumber : s_SpriteBatchTexture;
		batch.renderFlags = s_SpriteBatchDepthTest ? 0 : 1;

		GPUContext::TerrainDrawCmd cmd;
		cmd.firstIndex = 0;
		cmd.indexCount = (uint32_t)vkIndices.size();
		cmd.vertexOffset = 0;
		batch.cmds.push_back(cmd);

		std::vector<GPUContext::TerrainMergedBatch> vkBatches;
		vkBatches.push_back(std::move(batch));

		GPUContext::TerrainVertUBO vkUbo;
		vkUbo.viewMatrix = glm::mat4(1.0f);
		GetActiveProjectionMatrix(&vkUbo.projMatrix[0][0]);
		// Invert row 1 (Y) for Vulkan NDC
		vkUbo.projMatrix[0][1] = -vkUbo.projMatrix[0][1];
		vkUbo.projMatrix[1][1] = -vkUbo.projMatrix[1][1];
		vkUbo.projMatrix[2][1] = -vkUbo.projMatrix[2][1];
		vkUbo.projMatrix[3][1] = -vkUbo.projMatrix[3][1];
		// Remap depth
		vkUbo.projMatrix[0][2] = (vkUbo.projMatrix[0][2] + vkUbo.projMatrix[0][3]) * 0.5f;
		vkUbo.projMatrix[1][2] = (vkUbo.projMatrix[1][2] + vkUbo.projMatrix[1][3]) * 0.5f;
		vkUbo.projMatrix[2][2] = (vkUbo.projMatrix[2][2] + vkUbo.projMatrix[2][3]) * 0.5f;
		vkUbo.projMatrix[3][2] = (vkUbo.projMatrix[3][2] + vkUbo.projMatrix[3][3]) * 0.5f;

		GPUContext::Instance().DrawTerrainMerged(
			vkVerts.data(), (uint32_t)(vkVerts.size() * sizeof(TerrainVertex_t)),
			vkIndices.data(), (uint32_t)(vkIndices.size() * sizeof(uint32_t)),
			vkBatches, vkUbo);

		s_SpriteBatchCount = 0;
}

void BeginSprite()
{
	FlushSpriteBatch();
}

void EndSprite()
{
	FlushSpriteBatch();
}

void RenderPlane3D(float Width, float Height, float Matrix[3][4])
{
	vec3_t BoundingVertices[4];
	Vector(-Width, -Width,  Height, BoundingVertices[3]);
	Vector( Width,  Width,  Height, BoundingVertices[2]);
	Vector( Width,  Width, -Height, BoundingVertices[1]);
	Vector(-Width, -Width, -Height, BoundingVertices[0]);

	const glm::mat4& mv = RenderMatrix::GetCurrentStack().back();
	vec3_t translation = { mv[3][0], mv[3][1], mv[3][2] };

	vec3_t TransformVertices[4];
	vec3_t worldPos[4];
	vec3_t p[4];
	for (int j = 0; j < 4; j++)
	{
		VectorTransform(BoundingVertices[j], Matrix, TransformVertices[j]);
		VectorAdd(TransformVertices[j], translation, worldPos[j]);
		VectorTransform(worldPos[j], CameraMatrix, p[j]);
	}

	float c[4][2] = {
		{ 0.f, 1.f },
		{ 1.f, 1.f },
		{ 1.f, 0.f },
		{ 0.f, 0.f }
	};

	int tex = (CachTexture >= 0) ? CachTexture : 0;
	if (tex != s_SpriteBatchTexture ||
		AlphaBlendType != s_SpriteBatchBlendType ||
		AlphaTestEnable != s_SpriteBatchAlphaTest ||
		DepthMaskEnable != s_SpriteBatchDepthMask ||
		DepthTestEnable != s_SpriteBatchDepthTest ||
		s_SpriteBatchCount + 4 > MAX_SPRITE_BATCH_VERTS)
	{
		FlushSpriteBatch();
		s_SpriteBatchTexture = tex;
		s_SpriteBatchBlendType = AlphaBlendType;
		s_SpriteBatchAlphaTest = AlphaTestEnable;
		s_SpriteBatchDepthMask = DepthMaskEnable;
		s_SpriteBatchDepthTest = DepthTestEnable;
	}

	for (int i = 0; i < 4; i++)
	{
		s_SpriteBatchVerts[s_SpriteBatchCount + i].x = p[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].y = p[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].z = p[i][2];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].u = c[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].v = c[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].r = g_CurrentGLColor[0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].g = g_CurrentGLColor[1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].b = g_CurrentGLColor[2];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].a = g_CurrentGLColor[3];
	}
	s_SpriteBatchCount += 4;
}

void RenderSprite(int Texture, vec3_t Position, float Width, float Height, vec3_t Light, float Rotation, float u, float v, float uWidth, float vHeight)
{

	vec3_t p2;
	VectorTransform(Position, CameraMatrix, p2);

	float x = p2[0];
	float y = p2[1];
	float z = p2[2];

	Width *= 0.5f;
	Height *= 0.5f;

	vec3_t p[4];
	if (Rotation == 0.0f)
	{
		Vector(x - Width, y - Height, z, p[0]);
		Vector(x + Width, y - Height, z, p[1]);
		Vector(x + Width, y + Height, z, p[2]);
		Vector(x - Width, y + Height, z, p[3]);
	}
	else
	{
		float rad = Rotation * (3.14159265358979323846f / 180.0f);
		float cosR = cosf(rad);
		float sinR = sinf(rad);
		float wCos = Width * cosR, wSin = Width * sinR;
		float hCos = Height * cosR, hSin = Height * sinR;
		float rx2 = wCos - hSin, ry2 = wSin + hCos;
		float rx1 = wCos + hSin, ry1 = wSin - hCos;

		p[0][0] = x - rx2; p[0][1] = y - ry2; p[0][2] = z;
		p[1][0] = x + rx1; p[1][1] = y + ry1; p[1][2] = z;
		p[2][0] = x + rx2; p[2][1] = y + ry2; p[2][2] = z;
		p[3][0] = x - rx1; p[3][1] = y - ry1; p[3][2] = z;
	}

	float c[4][2];
	TEXCOORD(c[3], u, v);
	TEXCOORD(c[2], u + uWidth, v);
	TEXCOORD(c[1], u + uWidth, v + vHeight);
	TEXCOORD(c[0], u, v + vHeight);

	vec4_t colors[4];

	for (int i = 0; i < 4; i++)
	{
		VectorCopy(Light, colors[i]);
		if (Bitmaps[Texture].Components == 3)
		{
			colors[i][3] = 1.f;
		}
		else
		{
			if (Texture == BITMAP_BLOOD + 1 || Texture == BITMAP_FONT_HIT)
				colors[i][3] = 1.f;
			else
				colors[i][3] = Light[0];
		}
	}

	if (Texture != s_SpriteBatchTexture ||
		AlphaBlendType != s_SpriteBatchBlendType ||
		AlphaTestEnable != s_SpriteBatchAlphaTest ||
		DepthMaskEnable != s_SpriteBatchDepthMask ||
		DepthTestEnable != s_SpriteBatchDepthTest ||
		s_SpriteBatchCount + 4 > MAX_SPRITE_BATCH_VERTS)
	{
		FlushSpriteBatch();
		s_SpriteBatchTexture = Texture;
		s_SpriteBatchBlendType = AlphaBlendType;
		s_SpriteBatchAlphaTest = AlphaTestEnable;
		s_SpriteBatchDepthMask = DepthMaskEnable;
		s_SpriteBatchDepthTest = DepthTestEnable;
	}

	for (int i = 0; i < 4; i++)
	{
		s_SpriteBatchVerts[s_SpriteBatchCount + i].x = p[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].y = p[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].z = p[i][2];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].u = c[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].v = c[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].r = colors[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].g = colors[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].b = colors[i][2];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].a = colors[i][3];
	}
	s_SpriteBatchCount += 4;
}

void RenderSpriteUV(int Texture, vec3_t Position, float Width, float Height, float(*UV)[2], vec3_t Light[4], float Alpha)
{
	vec3_t p2;
	VectorTransform(Position, CameraMatrix, p2);
	float x = p2[0];
	float y = p2[1];
	float z = p2[2];

	Width *= 0.5f;
	Height *= 0.5f;
	vec3_t p[4];
	Vector(x - Width, y - Height, z, p[0]);
	Vector(x + Width, y - Height, z, p[1]);
	Vector(x + Width, y + Height, z, p[2]);
	Vector(x - Width, y + Height, z, p[3]);
	vec4_t colors[4];
	for (int i = 0; i < 4; i++)
	{
		colors[i][0] = Light[i][0];
		colors[i][1] = Light[i][1];
		colors[i][2] = Light[i][2];
		colors[i][3] = Alpha;
	}

	if (Texture != s_SpriteBatchTexture ||
		AlphaBlendType != s_SpriteBatchBlendType ||
		AlphaTestEnable != s_SpriteBatchAlphaTest ||
		DepthMaskEnable != s_SpriteBatchDepthMask ||
		DepthTestEnable != s_SpriteBatchDepthTest ||
		s_SpriteBatchCount + 4 > MAX_SPRITE_BATCH_VERTS)
	{
		FlushSpriteBatch();
		s_SpriteBatchTexture = Texture;
		s_SpriteBatchBlendType = AlphaBlendType;
		s_SpriteBatchAlphaTest = AlphaTestEnable;
		s_SpriteBatchDepthMask = DepthMaskEnable;
		s_SpriteBatchDepthTest = DepthTestEnable;
	}

	for (int i = 0; i < 4; i++)
	{
		s_SpriteBatchVerts[s_SpriteBatchCount + i].x = p[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].y = p[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].z = p[i][2];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].u = UV[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].v = UV[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].r = colors[i][0];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].g = colors[i][1];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].b = colors[i][2];
		s_SpriteBatchVerts[s_SpriteBatchCount + i].a = colors[i][3];
	}
	s_SpriteBatchCount += 4;
}

void RotateAngleNumber(float& X, float& Y, float Scale)
{
	const float Rad = 0.01745329f;
	float sinTh = sin((double)Rad * CameraAngle[2]);
	float cosTh = cos((double)Rad * CameraAngle[2]);

	X += Scale / 0.7071067f * cosTh / 2;
	Y -= Scale / 0.7071067f * sinTh / 2;
}

#if (DAMAGE_RENDER_ENABLE)
void RenderNumber(vec3_t Position, uint64_t Num, vec3_t Color, float Alpha, float Scale)
#else
void RenderNumber(vec3_t Position, int Num, vec3_t Color, float Alpha, float Scale)
#endif
{
	vec3_t p;
	VectorCopy(Position, p);
	vec3_t Light[4];
	VectorCopy(Color, Light[0]);
	VectorCopy(Color, Light[1]);
	VectorCopy(Color, Light[2]);
	VectorCopy(Color, Light[3]);

	if (Num == -1)
	{
		float UV[4][2];
		TEXCOORD(UV[0], 0.f, 32.f / 32.f);
		TEXCOORD(UV[1], 32.f / 256.f, 32.f / 32.f);
		TEXCOORD(UV[2], 32.f / 256.f, 17.f / 32.f);
		TEXCOORD(UV[3], 0.f, 17.f / 32.f);
		RenderSpriteUV(BITMAP_FONT + 1, p, 45, 20, UV, Light, Alpha);
	}
	else if (Num == -2)
	{
		RenderSprite(BITMAP_FONT_HIT, p, 32 * Scale, 20 * Scale, Light[0], 0.f, 0.f, 0.f, 27.f / 32.f, 15.f / 16.f);
	}
	else
	{
		char Text[32];
#if (DAMAGE_RENDER_ENABLE)
		_i64toa(Num, Text, 10);
#else
		itoa(Num, Text, 10);
#endif
		p[0] -= strlen(Text) * 5.f;
		unsigned int Length = strlen(Text);
		p[0] -= Length * Scale * 0.125f;
		p[1] -= Length * Scale * 0.125f;
		for (unsigned int i = 0; i < Length; i++)
		{
			float UV[4][2];
			float u = (float)(Text[i] - 48) * 16.f / 256.f;
			TEXCOORD(UV[0], u, 16.f / 32.f);
			TEXCOORD(UV[1], u + 16.f / 256.f, 16.f / 32.f);
			TEXCOORD(UV[2], u + 16.f / 256.f, 0.f);
			TEXCOORD(UV[3], u, 0.f);
			RenderSpriteUV(BITMAP_FONT + 1, p, Scale, Scale, UV, Light, Alpha);
			RotateAngleNumber(p[0], p[1], Scale);
		}
	}
}

void RenderNumber(vec3_t Position, int64_t Num, vec3_t Color, float Alpha, float Scale)
{
	RenderNumber(Position, static_cast<int>(Num), Color, Alpha, Scale);
}

float RenderNumber2D(float x, float y, int Num, float Width, float Height)
{
	char Text[32];
	itoa(Num, Text, 10);
	int Length = (int)strlen(Text);
	x -= Width * Length / 2;
	for (int i = 0; i < Length; i++)
	{
		float u = (float)(Text[i] - 48) * 16.f / 256.f;

		RenderBitmap(BITMAP_FONT + 1, x, y, Width, Height, u, 0.f, 16.f / 256.f, 16.f / 32.f);
		x += Width * 0.7f;
	}
	return x;
}

float RenderNumberHQ(float x, float y, int Num, float Width, float Height)
{
	char Text[32];
	memset(Text, 0, sizeof(Text));

	itoa(Num, Text, 10);

	for (int i = 0; i < (int)strlen(Text); i++)
	{
		float u = (float)(Text[i] - 48) * 36.f / 512.f;
		RenderBitmap(BITMAP_FONT_POWER, x, y, Width, Height, u, 0.f, 36.f / 512.f, 58.f / 64.f, true, true, 0.0);
		x += Width * 0.75f;
	}
	return x;
}

void BeginBitmap()
{
	FlushSpriteBatch();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	float aspectRatio = static_cast<float>(WindowWidth) / WindowHeight;

	glViewport(0, 0, WindowWidth, WindowHeight);
	gluPerspective(CameraFOV, aspectRatio, CameraViewNear, CameraViewFar);

	glLoadIdentity();
	gluOrtho2D(0, WindowWidth, 0, WindowHeight);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glLoadIdentity();
	DisableDepthTest();
}

void EndBitmap()
{
	FlushSpriteBatch();
	CBatchRenderer::Instance().FlushImageBatchesNow();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

static inline int GetCurrentAlphaBlendRenderFlag()
{
	switch (AlphaBlendType)
	{
	case 0: return RENDER_ALPHA_BLEND_TYPE_NONE;
	case 1:
	case 4: return RENDER_ALPHA_BLEND_TYPE_SUB;
	case 3: // GL_ONE, GL_ONE
	case 5: // GL_SRC_COLOR, GL_ONE
	case 6: // GL_SRC_ALPHA, GL_ONE
	case 7: // GL_ONE, GL_ONE_MINUS_SRC_COLOR
		return RENDER_ALPHA_BLEND_TYPE_ADD;
	default:
		return RENDER_ALPHA_BLEND_TYPE_NORMAL;
	}
}

void RenderColor(float x, float y, float Width, float Height, float Alpha, int Flag, bool Scale)
{
	if (Scale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}

	if (GPUContext::Instance().IsFrameActive())
	{
		ImageInstance_t img{};
		img.Texture = -1;
		img.x = x;
		img.y = y;
		img.width = Width;
		img.height = Height;
		img.u = 0.0f;
		img.v = 0.0f;
		img.uWidth = 1.0f;
		img.vHeight = 1.0f;
		if (Flag == 0)
		{
			img.color[0] = g_CurrentGLColor[0];
			img.color[1] = g_CurrentGLColor[1];
			img.color[2] = g_CurrentGLColor[2];
			img.color[3] = (Alpha > 0.0f) ? Alpha : g_CurrentGLColor[3];
		}
		else
		{
			img.color[0] = 0.0f;
			img.color[1] = 0.0f;
			img.color[2] = 0.0f;
			img.color[3] = (Alpha > 0.0f) ? Alpha : g_CurrentGLColor[3];
		}
		img.rotation = 0.0f;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	}
}

void RenderColor(float x, float y, float Width, float Height, float Alpha, int Flag)
{
	RenderColor(x, y, Width, Height, Alpha, Flag, true);
}

inline float Clamp(float value, float min, float max)
{
	return (value < min) ? min : (value > max) ? max : value;
}

void RenderCooldownPie(float x, float y, float Width, float Height, float percent)
{
}

void RenderNoColor(float x, float y, float Width, float Height, float Alpha, int Flag)
{
	x = ConvertNoX(x);
	y = ConvertNoY(y);
	Width = ConvertNoX(Width);
	Height = ConvertNoY(Height);

	
		ImageInstance_t img{};
		img.Texture = -1;
		img.x = x;
		img.y = y;
		img.width = Width;
		img.height = Height;
		img.u = 0.0f;
		img.v = 0.0f;
		img.uWidth = 1.0f;
		img.vHeight = 1.0f;
		if (Flag == 0)
		{
			img.color[0] = g_CurrentGLColor[0];
			img.color[1] = g_CurrentGLColor[1];
			img.color[2] = g_CurrentGLColor[2];
			img.color[3] = (Alpha > 0.0f) ? Alpha : g_CurrentGLColor[3];
		}
		else
		{
			img.color[0] = 0.0f;
			img.color[1] = 0.0f;
			img.color[2] = 0.0f;
			img.color[3] = (Alpha > 0.0f) ? Alpha : g_CurrentGLColor[3];
		}
		img.rotation = 0.0f;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void EndRenderColor()
{
	EnableAlphaTest();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void RenderColorBitmap(int Texture, float x, float y, float Width, float Height, float u, float v, float uWidth, float vHeight, unsigned int color)
{
	x = ConvertX(x);
	y = ConvertY(y);

	Width = ConvertX(Width);
	Height = ConvertY(Height);

	
		float r = static_cast<float>(color & 0xff) / 255.0f;
		float g = static_cast<float>((color >> 8) & 0xff) / 255.0f;
		float b = static_cast<float>((color >> 16) & 0xff) / 255.0f;
		float a = static_cast<float>((color >> 24) & 0xff) / 255.0f;
		if (a <= 0.0f) a = 1.0f;

		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x;
		img.y = y;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = r;
		img.color[1] = g;
		img.color[2] = b;
		img.color[3] = a;
		img.rotation = 0.0f;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderBitmap(int Texture, float x, float y, float Width, float Height, float u, float v, float uWidth, float vHeight, bool Scale, bool StartScale, float Alpha)
{
	if (StartScale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
	}
	if (Scale)
	{
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}

	
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x;
		img.y = y;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = (Alpha > 0.0f) ? Alpha : g_CurrentGLColor[3];
		img.rotation = 0.0f;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderNoBitmap(int Texture, float x, float y, float Width, float Height, float u, float v, float uWidth, float vHeight, bool Scale, bool StartScale, float Alpha)
{
	if (StartScale)
	{
		x = ConvertNoX(x);
		y = ConvertNoY(y);
	}
	if (Scale)
	{
		Width = ConvertNoX(Width);
		Height = ConvertNoY(Height);
	}

	
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x;
		img.y = y;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = (Alpha > 0.0f) ? Alpha : g_CurrentGLColor[3];
		img.rotation = 0.0f;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderBitmapRotate(int Texture, float x, float y, float Width, float Height, float Rotate, float u, float v, float uWidth, float vHeight, bool Scale)
{
	if (Scale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}

	
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x - Width * 0.5f;
		img.y = y - Height * 0.5f;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = g_CurrentGLColor[3];
		img.rotation = Rotate * (3.14159265358979323846f / 180.0f);
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderBitmapRotate(int Texture, float x, float y, float Width, float Height, float Rotate, float u, float v, float uWidth, float vHeight)
{
	RenderBitmapRotate(Texture, x, y, Width, Height, Rotate, u, v, uWidth, vHeight, false);
}

void RenderBitRotate(int Texture, float x, float y, float Width, float Height, float Rotate)
{
	RenderBitmapRotate(Texture, x, y, Width, Height, Rotate, 0.f, 0.f, 1.f, 1.f);
}

void RenderPointRotate(int Texture, float ix, float iy, float iWidth, float iHeight, float x, float y, float Width, float Height, float Rotate, float Rotate_Loc, float uWidth, float vHeight, int Num)
{
}

void RenderBitmapLocalRotate(int Texture, float x, float y, float Width, float Height, float Rotate, float u, float v, float uWidth, float vHeight)
{
	
		x = ConvertX(x);
		y = ConvertY(y);
		Width = ConvertX(Width);
		Height = ConvertY(Height);

		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x - Width * 0.5f;
		img.y = y - Height * 0.5f;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = g_CurrentGLColor[3];
		img.rotation = Rotate; // Already in radians in this function (uses cosf/sinf(Rotate))
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderNoBitmapLocalRotate(int Texture, float x, float y, float Width, float Height, float Rotate, float u, float v, float uWidth, float vHeight)
{
	x = ConvertNoX(x);
	y = ConvertNoY(y);
	Width = ConvertNoX(Width);
	Height = ConvertNoY(Height);

	
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x - Width * 0.5f;
		img.y = y - Height * 0.5f;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = g_CurrentGLColor[3];
		img.rotation = Rotate;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderBitmapLocalRotate(int Texture, float x, float y, float Width, float Height, float Rotate, float u, float v, float uWidth, float vHeight, bool Scale)
{
	if (Scale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}

	
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x;
		img.y = y;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = g_CurrentGLColor[3];
		img.rotation = Rotate * (3.14159265358979323846f / 180.0f);
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}

void RenderBitmapLocalRotate2(int Texture, float x, float y, float Width, float Height, float Rotate, float u, float v, float uWidth, float vHeight, bool Scale)
{
	if (Scale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
		Width = ConvertX(Width);
		Height = ConvertY(Height);
	}

	
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = x - Width * 0.5f;
		img.y = y - Height * 0.5f;
		img.width = Width;
		img.height = Height;
		img.u = u;
		img.v = v;
		img.uWidth = uWidth;
		img.vHeight = vHeight;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = g_CurrentGLColor[3];
		img.rotation = Rotate * (3.14159265358979323846f / 180.0f);
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;

		g_BatchRenderer.AddImage(img);
		return;
	
}


void RenderBitmapLocalProjection(int Texture, float x, float y, float w, float h, vec3_t Angle, float su, float sv, float uw, float uh, bool Scale)
{
	float Matrix[3][4];
	vec3_t sp[4], vertex[4];

	if (Scale)
	{
		x = ConvertX(x);
		y = ConvertY(y);
		w = ConvertX(w);
		h = ConvertY(h);
	}

	if (GPUContext::Instance().IsFrameActive())
	{
		std::vector<TerrainVertex_t> vkVerts(4);
		std::vector<uint32_t> vkIndices = { 0, 1, 2, 0, 2, 3 };

		Vector(0.0, 0.0, 0.f, sp[0]);
		Vector(0.0, -h, 0.0, sp[1]);
		Vector(w, -h, 0.0, sp[2]);
		Vector(w, 0.0, 0.0, sp[3]);

		float coord2[4][2];
		coord2[0][0] = su;      coord2[0][1] = sv;
		coord2[1][0] = su;      coord2[1][1] = sv + uh;
		coord2[2][0] = su + uw; coord2[2][1] = sv + uh;
		coord2[3][0] = su + uw; coord2[3][1] = sv;

		AngleMatrix(Angle, Matrix);

		float cr = g_CurrentGLColor[0], cg = g_CurrentGLColor[1], cb = g_CurrentGLColor[2], ca = g_CurrentGLColor[3];
		if (ca <= 0.0f || (cr == 0.0f && cg == 0.0f && cb == 0.0f)) { cr = cg = cb = ca = 1.0f; }
		uint32_t r = (uint32_t)((std::min)((std::max)(cr, 0.0f), 1.0f) * 255.0f);
		uint32_t g = (uint32_t)((std::min)((std::max)(cg, 0.0f), 1.0f) * 255.0f);
		uint32_t b = (uint32_t)((std::min)((std::max)(cb, 0.0f), 1.0f) * 255.0f);
		uint32_t a = (uint32_t)((std::min)((std::max)(ca, 0.0f), 1.0f) * 255.0f);
		uint32_t color = (a << 24) | (b << 16) | (g << 8) | r;

		for (int n = 0; n < 4; n++)
		{
			VectorRotate(sp[n], Matrix, vertex[n]);
			vkVerts[n].pos[0] = x + vertex[n][0];
			vkVerts[n].pos[1] = y - vertex[n][1];
			vkVerts[n].pos[2] = 0.0f;
			vkVerts[n].uv[0] = coord2[n][0];
			vkVerts[n].uv[1] = coord2[n][1];
			vkVerts[n].color = color;
		}

		GPUContext::TerrainMergedBatch batch;
		batch.batchType = (AlphaBlendType == 3 || AlphaBlendType == 5 || AlphaBlendType == 7) ? TERRAIN_BATCH_GRASS_ADD : ((AlphaBlendType == 2 || AlphaBlendType == 6) ? TERRAIN_BATCH_ALPHA : TERRAIN_BATCH_OPAQUE);
		BITMAP_t* pBitmap = Bitmaps.FindTexture(Texture);
		batch.textureIndex = (pBitmap && pBitmap->TextureNumber > 0) ? (int)pBitmap->TextureNumber : Texture;
		batch.renderFlags = 0;

		GPUContext::TerrainDrawCmd cmd;
		cmd.firstIndex = 0;
		cmd.indexCount = 6;
		cmd.vertexOffset = 0;
		batch.cmds.push_back(cmd);

		std::vector<GPUContext::TerrainMergedBatch> vkBatches;
		vkBatches.push_back(std::move(batch));

		GPUContext::TerrainVertUBO vkUbo;
		vkUbo.viewMatrix = glm::mat4(1.0f);
		vkUbo.projMatrix = glm::ortho(0.0f, (float)WindowWidth, (float)WindowHeight, 0.0f, -1.0f, 1.0f);
		vkUbo.projMatrix[0][1] = -vkUbo.projMatrix[0][1];
		vkUbo.projMatrix[1][1] = -vkUbo.projMatrix[1][1];
		vkUbo.projMatrix[2][1] = -vkUbo.projMatrix[2][1];
		vkUbo.projMatrix[3][1] = -vkUbo.projMatrix[3][1];
		vkUbo.projMatrix[0][2] = (vkUbo.projMatrix[0][2] + vkUbo.projMatrix[0][3]) * 0.5f;
		vkUbo.projMatrix[1][2] = (vkUbo.projMatrix[1][2] + vkUbo.projMatrix[1][3]) * 0.5f;
		vkUbo.projMatrix[2][2] = (vkUbo.projMatrix[2][2] + vkUbo.projMatrix[2][3]) * 0.5f;
		vkUbo.projMatrix[3][2] = (vkUbo.projMatrix[3][2] + vkUbo.projMatrix[3][3]) * 0.5f;

		GPUContext::Instance().DrawTerrainMerged(
			vkVerts.data(), (uint32_t)(vkVerts.size() * sizeof(TerrainVertex_t)),
			vkIndices.data(), (uint32_t)(vkIndices.size() * sizeof(uint32_t)),
			vkBatches, vkUbo);
		return;
	}
}

void RenderBitmapAlpha(int Texture, float sx, float sy, float Width, float Height)
{
	if (GPUContext::Instance().IsFrameActive())
	{
		ImageInstance_t img{};
		img.Texture = ResolveTextureNumber(Texture);
		img.x = ConvertX(sx);
		img.y = ConvertY(sy);
		img.width = ConvertX(Width);
		img.height = ConvertY(Height);
		img.u = 0.0f;
		img.v = 0.0f;
		img.uWidth = 1.0f;
		img.vHeight = 1.0f;
		img.color[0] = g_CurrentGLColor[0];
		img.color[1] = g_CurrentGLColor[1];
		img.color[2] = g_CurrentGLColor[2];
		img.color[3] = g_CurrentGLColor[3];
		img.rotation = 0.0f;
		img.layer = 0;
		img.RenderFlags = GetCurrentAlphaBlendRenderFlag();
		img.grayscale = false;
		g_BatchRenderer.AddImage(img);
		return;
	}
}

void RenderBitmapUV(int Texture, float x, float y, float Width, float Height, float u, float v, float uWidth, float vHeight)
{
	x = ConvertX(x);
	y = ConvertY(y);
	Width = ConvertX(Width);
	Height = ConvertY(Height);

	if (GPUContext::Instance().IsFrameActive())
	{
		std::vector<TerrainVertex_t> vkVerts(4);
		std::vector<uint32_t> vkIndices = { 0, 1, 2, 0, 2, 3 };

		float p[4][2];
		p[0][0] = x;         p[0][1] = y;
		p[1][0] = x;         p[1][1] = y + Height;
		p[2][0] = x + Width; p[2][1] = y + Height;
		p[3][0] = x + Width; p[3][1] = y;

		float c[4][2];
		TEXCOORD(c[0], u, v + vHeight * 0.25f);
		TEXCOORD(c[1], u, v + vHeight - vHeight * 0.25f);
		TEXCOORD(c[2], u + uWidth, v + vHeight);
		TEXCOORD(c[3], u + uWidth, v);

		float cr = g_CurrentGLColor[0], cg = g_CurrentGLColor[1], cb = g_CurrentGLColor[2], ca = g_CurrentGLColor[3];
		if (ca <= 0.0f || (cr == 0.0f && cg == 0.0f && cb == 0.0f)) { cr = cg = cb = ca = 1.0f; }
		uint32_t r = (uint32_t)((std::min)((std::max)(cr, 0.0f), 1.0f) * 255.0f);
		uint32_t g = (uint32_t)((std::min)((std::max)(cg, 0.0f), 1.0f) * 255.0f);
		uint32_t b = (uint32_t)((std::min)((std::max)(cb, 0.0f), 1.0f) * 255.0f);
		uint32_t a = (uint32_t)((std::min)((std::max)(ca, 0.0f), 1.0f) * 255.0f);
		uint32_t color = (a << 24) | (b << 16) | (g << 8) | r;

		for (int i = 0; i < 4; ++i)
		{
			vkVerts[i].pos[0] = p[i][0];
			vkVerts[i].pos[1] = p[i][1];
			vkVerts[i].pos[2] = 0.0f;
			vkVerts[i].uv[0] = c[i][0];
			vkVerts[i].uv[1] = c[i][1];
			vkVerts[i].color = color;
		}

		GPUContext::TerrainMergedBatch batch;
		batch.batchType = (AlphaBlendType == 3 || AlphaBlendType == 5 || AlphaBlendType == 7) ? TERRAIN_BATCH_GRASS_ADD : ((AlphaBlendType == 2 || AlphaBlendType == 6) ? TERRAIN_BATCH_ALPHA : TERRAIN_BATCH_OPAQUE);
		BITMAP_t* pBitmap = Bitmaps.FindTexture(Texture);
		batch.textureIndex = (pBitmap && pBitmap->TextureNumber > 0) ? (int)pBitmap->TextureNumber : Texture;
		batch.renderFlags = 0;

		GPUContext::TerrainDrawCmd cmd;
		cmd.firstIndex = 0;
		cmd.indexCount = 6;
		cmd.vertexOffset = 0;
		batch.cmds.push_back(cmd);

		std::vector<GPUContext::TerrainMergedBatch> vkBatches;
		vkBatches.push_back(std::move(batch));

		GPUContext::TerrainVertUBO vkUbo;
		vkUbo.viewMatrix = glm::mat4(1.0f);
		vkUbo.projMatrix = glm::ortho(0.0f, (float)WindowWidth, (float)WindowHeight, 0.0f, -1.0f, 1.0f);
		// Invert row 1 (Y) for Vulkan NDC
		vkUbo.projMatrix[0][1] = -vkUbo.projMatrix[0][1];
		vkUbo.projMatrix[1][1] = -vkUbo.projMatrix[1][1];
		vkUbo.projMatrix[2][1] = -vkUbo.projMatrix[2][1];
		vkUbo.projMatrix[3][1] = -vkUbo.projMatrix[3][1];
		// Remap depth
		vkUbo.projMatrix[0][2] = (vkUbo.projMatrix[0][2] + vkUbo.projMatrix[0][3]) * 0.5f;
		vkUbo.projMatrix[1][2] = (vkUbo.projMatrix[1][2] + vkUbo.projMatrix[1][3]) * 0.5f;
		vkUbo.projMatrix[2][2] = (vkUbo.projMatrix[2][2] + vkUbo.projMatrix[2][3]) * 0.5f;
		vkUbo.projMatrix[3][2] = (vkUbo.projMatrix[3][2] + vkUbo.projMatrix[3][3]) * 0.5f;

		GPUContext::Instance().DrawTerrainMerged(
			vkVerts.data(), (uint32_t)(vkVerts.size() * sizeof(TerrainVertex_t)),
			vkIndices.data(), (uint32_t)(vkIndices.size() * sizeof(uint32_t)),
			vkBatches, vkUbo);
		return;
	}
}





float absf(float a)
{
	if (a < 0.f) return -a;
	return a;
}

float minf(float a, float b)
{
	if (a > b)
		return b;
	return a;
}

float maxf(float a, float b)
{
	if (a > b) return a;
	return b;
}

int InsideTest(float x, float y, float z, int n, float* v1, float* v2, float* v3, float* v4, int flag, float type)
{
	if (type > 0.f)
		flag <<= 3;

	int i;
	vec3_t* vtx[4];
	vtx[0] = (vec3_t*)v1;
	vtx[1] = (vec3_t*)v2;
	vtx[2] = (vec3_t*)v3;
	vtx[3] = (vec3_t*)v4;

	int j = n - 1;
	switch (flag)
	{
	case 1:
		for (i = 0; i < n; j = i, i++)
		{
			float d = ((*vtx[i])[1] - y) * ((*vtx[j])[2] - z) - ((*vtx[j])[1] - y) * ((*vtx[i])[2] - z);
			if (d <= 0.f)
				return false;
		}
		break;
	case 2:
		for (i = 0; i < n; j = i, i++)
		{
			float d = ((*vtx[i])[2] - z) * ((*vtx[j])[0] - x) - ((*vtx[j])[2] - z) * ((*vtx[i])[0] - x);
			if (d <= 0.f)
				return false;
		}
		break;
	case 4:
		for (i = 0; i < n; j = i, i++)
		{
			float d = ((*vtx[i])[0] - x) * ((*vtx[j])[1] - y) - ((*vtx[j])[0] - x) * ((*vtx[i])[1] - y);
			if (d <= 0.f)
				return false;
		}
		break;
	case 8:
		for (i = 0; i < n; j = i, i++)
		{
			float d = ((*vtx[i])[1] - y) * ((*vtx[j])[2] - z) - ((*vtx[j])[1] - y) * ((*vtx[i])[2] - z);
			if (d >= 0.f)
				return false;
		}
		break;
	case 16:
		for (i = 0; i < n; j = i, i++)
		{
			float d = ((*vtx[i])[2] - z) * ((*vtx[j])[0] - x) - ((*vtx[j])[2] - z) * ((*vtx[i])[0] - x);
			if (d >= 0.f)
				return false;
		}
		break;
	case 32:
		for (i = 0; i < n; j = i, i++)
		{
			float d = ((*vtx[i])[0] - x) * ((*vtx[j])[1] - y) - ((*vtx[j])[0] - x) * ((*vtx[i])[1] - y);
			if (d >= 0.f)
				return false;
		}
		break;
	}

	return true;
}

void InitCollisionDetectLineToFace()
{
	Distance = 9999999.f;
}

bool CollisionDetectLineToFace(vec3_t Position, vec3_t Target, int Polygon, float* v1, float* v2, float* v3, float* v4, vec3_t Normal, bool Collision)
{
	vec3_t Direction;
	VectorSubtract(Target, Position, Direction);
	float a = DotProduct(Direction, Normal);
	if (a >= 0.f) return false;
	float b = DotProduct(Position, Normal) - DotProduct(v1, Normal);
	float t = -b / a;
	if (t >= 0.f && t <= Distance)
	{
		float X = Direction[0] * t + Position[0];
		float Y = Direction[1] * t + Position[1];
		float Z = Direction[2] * t + Position[2];
		int Count = 0;
		float MIN = minf(minf(absf(Direction[0]), absf(Direction[1])), absf(Direction[2]));
		if (MIN == absf(Direction[0]))
		{
			if ((Y >= minf(Position[1], Target[1]) && Y <= maxf(Position[1], Target[1])) &&
				(Z >= minf(Position[2], Target[2]) && Z <= maxf(Position[2], Target[2]))) Count++;
		}
		else if (MIN == absf(Direction[1]))
		{
			if ((Z >= minf(Position[2], Target[2]) && Z <= maxf(Position[2], Target[2])) &&
				(X >= minf(Position[0], Target[0]) && X <= maxf(Position[0], Target[0]))) Count++;
		}
		else
		{
			if ((X >= minf(Position[0], Target[0]) && X <= maxf(Position[0], Target[0])) &&
				(Y >= minf(Position[1], Target[1]) && Y <= maxf(Position[1], Target[1]))) Count++;
		}
		if (Count == 0) return false;
		Count = 0;
		if (Normal[0] <= -0.5f || Normal[0] >= 0.5f)
		{
			Count += InsideTest(X, Y, Z, Polygon, v1, v2, v3, v4, 1, Normal[0]);
		}
		else if (Normal[1] <= -0.5f || Normal[1] >= 0.5f)
		{
			Count += InsideTest(X, Y, Z, Polygon, v1, v2, v3, v4, 2, Normal[1]);
		}
		else
		{
			Count += InsideTest(X, Y, Z, Polygon, v1, v2, v3, v4, 4, Normal[2]);
		}
		if (Count == 0) return false;
		if (Collision)
		{
			Distance = t;
			Vector(X, Y, Z, CollisionPosition);
		}
		return true;
	}
	return false;
}

bool ProjectLineBox(vec3_t ax, vec3_t p1, vec3_t p2, OBB_t obb)
{
	float P1 = DotProduct(ax, p1);
	float P2 = DotProduct(ax, p2);

	float mx1 = maxf(P1, P2);
	float mn1 = minf(P1, P2);

	float ST = DotProduct(ax, obb.StartPos);
	float Q1 = DotProduct(ax, obb.XAxis);
	float Q2 = DotProduct(ax, obb.YAxis);
	float Q3 = DotProduct(ax, obb.ZAxis);

	float mx2 = ST;
	float mn2 = ST;

	if (Q1 > 0)	mx2 += Q1; else mn2 += Q1;
	if (Q2 > 0)	mx2 += Q2; else mn2 += Q2;
	if (Q3 > 0) mx2 += Q3; else mn2 += Q3;

	if (mn1 > mx2) return false;
	if (mn2 > mx1) return false;

	return true;
}

bool CollisionDetectLineToOBB(vec3_t p1, vec3_t p2, OBB_t obb)
{
	vec3_t e1;
	vec3_t eq11, eq12, eq13;

	VectorSubtract(p2, p1, e1);

	CrossProduct(e1, obb.XAxis, eq11);
	CrossProduct(e1, obb.YAxis, eq12);
	CrossProduct(e1, obb.ZAxis, eq13);

	if (!ProjectLineBox(eq11, p1, p2, obb)) return false;
	if (!ProjectLineBox(eq12, p1, p2, obb)) return false;
	if (!ProjectLineBox(eq13, p1, p2, obb)) return false;

	if (!ProjectLineBox(obb.XAxis, p1, p2, obb)) return false;
	if (!ProjectLineBox(obb.YAxis, p1, p2, obb)) return false;
	if (!ProjectLineBox(obb.ZAxis, p1, p2, obb)) return false;

	return true;
}


void CollisionDetectRotate(float centerX, float centerY, float angle, float& x, float& y)
{
	static float DEG_TO_RAD = (Q_PI / 180.0f);

	float rad = angle * DEG_TO_RAD;
	float cosA = cos(rad);
	float sinA = sin(rad);

	float translatedX = x - centerX;
	float translatedY = y - centerY;

	float rotatedX = translatedX * cosA + translatedY * sinA;
	float rotatedY = -translatedX * sinA + translatedY * cosA;

	x = rotatedX + centerX;
	y = rotatedY + centerY;
}

#ifdef V_SYNCRONIZE

bool _isVSyncEnabled = false;
bool _isVSyncAvailable = true;

void InitVSync()
{
	_isVSyncAvailable = true;
}

void SetVSync(bool enable)
{
	_isVSyncEnabled = enable;
}

bool IsVSyncAvailable()
{
	return _isVSyncAvailable;
}

bool IsVSyncEnabled()
{
	return _isVSyncEnabled;
}

void EnableVSync()
{
	SetVSync(true);
}

void DisableVSync()
{
	SetVSync(false);
}

int GetFPSLimit()
{
	return GetDeviceCaps(g_hDC, VREFRESH);
}

#endif 

void GetDrawCircle(int ID, float X, float Y, float W, float CurrenX, float CurrenY, float SetScale, int ScaleSize, int ScalePosicion, float Alpha)
{
	if (ScalePosicion)
	{
		X = ConvertX(X);
		Y = ConvertY(Y);
	}
	if (ScaleSize)
	{
		W = ConvertX(W);
	}

	ImageInstance_t img{};
	img.Texture = ResolveTextureNumber(ID);
	img.x = X;
	img.y = Y;
	img.width = W;
	img.height = W;
	img.u = CurrenX - SetScale * 0.5f;
	img.v = CurrenY - SetScale * 0.5f;
	img.uWidth = SetScale;
	img.vHeight = SetScale;
	img.color[0] = 1.0f;
	img.color[1] = 1.0f;
	img.color[2] = 1.0f;
	img.color[3] = (Alpha > 0.0f) ? Alpha : 1.0f;
	img.rotation = 0.0f;
	img.layer = 0;
	img.RenderFlags = RENDER_ALPHA_BLEND_TYPE_NORMAL;
	img.grayscale = false;

	g_BatchRenderer.AddImage(img);
}

// ============================================================================
// MU Custom UI Functions
// ============================================================================
void SetLineColor(int iType, float fAlphaRate = 1.0f);

void BRenderTabLine(float iPos_x, float iPos_y, float iTabWidth, float iTabHeight, int iTabNum, int iSelectNum)
{
	for (int i = 0; i < iTabNum; ++i)
	{
		SetLineColor(2);
		float fRPos_x = float(iPos_x + i * iTabWidth);
		if (i == iSelectNum)
		{
			RenderColor((float)fRPos_x, (float)iPos_y, (float)iTabWidth, (float)1);
			RenderColor((float)fRPos_x - 1, (float)iPos_y, (float)1, (float)iTabHeight);
			RenderColor((float)fRPos_x + iTabWidth - 1, (float)iPos_y, (float)1, (float)iTabHeight);
			SetLineColor(5);
			RenderColor((float)fRPos_x, (float)iPos_y + 1, (float)iTabWidth - 1, (float)iTabHeight - 1);
		}
		else
		{
			RenderColor((float)fRPos_x, (float)iPos_y + 1, (float)iTabWidth-1, (float)1);
			RenderColor((float)fRPos_x, (float)iPos_y + iTabHeight - 1, (float)iTabWidth-1, (float)1);
			SetLineColor(6);
			RenderColor((float)fRPos_x, (float)iPos_y + 2, (float)iTabWidth - 1, (float)iTabHeight - 3);
		}
	}
}

void BDrawOutLine(int iPos_x, int iPos_y, int iWidth, int iHeight)
{
	SetLineColor(5, 0.5f);
	RenderColor((float)iPos_x + 4, (float)iPos_y + 4, (float)iWidth - 8, (float)iHeight - 8);

	SetLineColor(0);
	RenderColor((float)iPos_x, (float)iPos_y, (float)iWidth, (float)1);
	SetLineColor(1);
	RenderColor((float)iPos_x, (float)iPos_y + 1, (float)iWidth, (float)3);
	SetLineColor(2);
	RenderColor((float)iPos_x, (float)iPos_y + 4, (float)iWidth, (float)1);

	SetLineColor(2);
	RenderColor((float)iPos_x, (float)iPos_y + iHeight - 5, (float)iWidth, (float)1);
	SetLineColor(1);
	RenderColor((float)iPos_x, (float)iPos_y + iHeight - 4, (float)iWidth, (float)3);
	SetLineColor(0);
	RenderColor((float)iPos_x, (float)iPos_y + iHeight - 1, (float)iWidth, (float)1);

	SetLineColor(0);
	RenderColor((float)iPos_x, (float)iPos_y, (float)1, (float)iHeight);
	SetLineColor(1);
	RenderColor((float)iPos_x + 1, (float)iPos_y + 5, (float)3, (float)iHeight - 10);
	SetLineColor(2);
	RenderColor((float)iPos_x + 4, (float)iPos_y + 1, (float)1, (float)iHeight - 2);

	SetLineColor(2);
	RenderColor((float)iPos_x + iWidth - 5, (float)iPos_y + 1, (float)1, (float)iHeight - 2);
	SetLineColor(1);
	RenderColor((float)iPos_x + iWidth - 4, (float)iPos_y + 5, (float)3, (float)iHeight - 10);
	SetLineColor(0);
	RenderColor((float)iPos_x + iWidth - 1, (float)iPos_y, (float)1, (float)iHeight);

	SetLineColor(4);
	RenderColor((float)iPos_x + 2, (float)iPos_y + 2, (float)1, (float)1);
	RenderColor((float)iPos_x + iWidth - 3, (float)iPos_y + 2, (float)1, (float)1);
	RenderColor((float)iPos_x + iWidth - 3, (float)iPos_y + iHeight - 3, (float)1, (float)1);
	RenderColor((float)iPos_x + 2, (float)iPos_y + iHeight - 3, (float)1, (float)1);
}

extern bool g_RenderEff;
bool GetRenderEffect()
{
	return g_RenderEff;
}
