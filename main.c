//#define USE_GLEW
//#define NO_FIXED_FUNCTION_PIPELINE

#ifdef __EMSCRIPTEN__
#	undef USE_GLEW
#	undef NO_FIXED_FUNCTION_PIPELINE
#	define NO_FIXED_FUNCTION_PIPELINE
#endif //__EMSCRIPTEN__

#ifdef _WIN32
#	include "windows.h"
#	define USE_GLEW
#	include "GL/glew.h"
#	include "GL/glut.h"
#	ifdef __FREEGLUT_STD_H__   
#		include "GL/freeglut_ext.h"
#	endif //__FREEGLUT_STD_H__
#else // _WIN32 
#	ifdef USE_GLEW
#		include "GL/glew.h"
#	else //USE_GLEW
#		define GL_GLEXT_PROTOTYPES
#	endif //USE_GLEW
#	include "GL/glut.h"
#	ifdef __FREEGLUT_STD_H__   
#		ifndef __EMSCRIPTEN__
#			include "GL/freeglut_ext.h"   
#		endif //__EMSCRIPTEN__
#	endif //__FREEGLUT_STD_H__
#endif // _WIN32

#include <stdio.h>
#include <math.h>
#include <string.h>

#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"
#undef MATH_3D_IMPLEMENTATION


// Expressed as float so division returns a float and not 0 (640/480 != 640.0/480.0).
#define RENDER_WIDTH 1024.0 //640.0
#define RENDER_HEIGHT 512.0 //480.0
#define FULLSCREEN_RENDER_WIDTH 0  //640.0
#define FULLSCREEN_RENDER_HEIGHT 0 //480.0

int windowId = 0; 			// window Id when not in fullscreen mode
int gameModeWindowId = 0;	// window Id when in fullscreen mode


mat4_t pMatrix;				// currently not used (the fragment shader calculates it)
mat4_t cameraMatrix,cameraMatrixSlerp;
float cameraMatrixSlerpTimer = 1.f;
float cameraMatrixSlerpTimerSpeed = 1.f;
vec3_t cameraTarget;
float minCameraTargetDistance =  2.f;
float maxCameraTargetDistance = 50.f;
const unsigned FPS_TARGET = 35;
unsigned FPS = 35;
int showFPS = 
#ifdef NO_FIXED_FUNCTION_PIPELINE
0;
#else
1;
#endif


const char ScreenQuadVS[] = 
"#ifdef GL_ES\n"\
"precision highp float;\n"\
"#endif\n"\
"attribute vec3 a_position;\n"\
"\n"\
"void main()	{\n"\
"    gl_Position = vec4( a_position, 1 );\n"\
"}\n";

const char ScreenQuadFS[] = 
"#ifdef GL_ES\n"\
"precision mediump float;\n"\
"uniform lowp sampler2D s_diffuse;\n"\
"#else\n"\
"uniform sampler2D s_diffuse;\n"\
"#endif\n"\
"uniform vec3 screenResAndFactor; // .x and .y in pixels; .z in [0,1]: default: 1\n"\
"\n"\
"void main() {\n"\
"	 vec2 texCoords = (gl_FragCoord.xy/screenResAndFactor.xy)*screenResAndFactor.z;\n"\
"    gl_FragColor = texture2D( s_diffuse, texCoords);\n"\
"    //gl_FragColor = texture2D( s_diffuse, v_texcoord.st );\n"\
"    //gl_FragColor = vec4(1.0,0.0,0.0,1.0);\n"\
"    //vec2 fragCoord = vec2(gl_FragCoord.x/640.0,gl_FragCoord.y/480.0);\n"\
"    //gl_FragColor = vec4(fragCoord.x,fragCoord.y,0.0,1.0);\n"\
"}\n";


GLuint getTextFromFile(char* buffer,int buffer_size,const char* filename);
GLuint loadShaderProgramFromSource(const char* vs,const char* fs);

#define NUM_RENDER_TARGETS (3)	// Must be >0

typedef struct {
	GLuint frame_buffer[NUM_RENDER_TARGETS];
	GLuint depth_buffer[NUM_RENDER_TARGETS];
	GLuint texture[NUM_RENDER_TARGETS];
	float resolution_factor[NUM_RENDER_TARGETS];	

	GLuint screenQuadProgramId;
	GLint aLoc_APosition;
	GLint uLoc_screenResAndFactor;
	GLint uLoc_SDiffuse;
	int width,height;
	GLint default_frame_buffer;

	int dynamic_resolution_enabled;
	int dynamic_resolution_target_fps;
	
} RenderTarget;
void RenderTarget_Create(RenderTarget* rt) {
	
	rt->screenQuadProgramId = loadShaderProgramFromSource(ScreenQuadVS,ScreenQuadFS);
	//rt->screenQuadProgramId = loadShaderProgramFromSource(DynamicRenderingVS,DynamicRenderingFS);
	if (!rt->screenQuadProgramId) {
		fprintf(stderr,"Error: screenQuadProgramId==0\n");
		exit(0);
	}
	else {
		rt->aLoc_APosition = glGetAttribLocation(rt->screenQuadProgramId, "a_position");
		rt->uLoc_SDiffuse = glGetUniformLocation(rt->screenQuadProgramId,"s_diffuse");
		rt->uLoc_screenResAndFactor = glGetUniformLocation(rt->screenQuadProgramId,"screenResAndFactor");
		if (rt->aLoc_APosition<0) fprintf(stderr,"Error: rt->aLoc_APosition<0\n");	
		if (rt->uLoc_SDiffuse<0) fprintf(stderr,"Error: uLoc_SDiffuse<0\n");	
		if (rt->uLoc_screenResAndFactor<0) fprintf(stderr,"Error: uLoc_screenResAndFactor<0\n");	
	}

	glGenFramebuffers(NUM_RENDER_TARGETS, rt->frame_buffer);
	glGenTextures(NUM_RENDER_TARGETS, rt->texture);
	glGenRenderbuffers(NUM_RENDER_TARGETS, rt->depth_buffer);
	//rt->default_frame_buffer = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &(rt->default_frame_buffer));

	rt->dynamic_resolution_enabled = 1;
	rt->dynamic_resolution_target_fps = FPS_TARGET;

}
void RenderTarget_Destroy(RenderTarget* rt) {
	if (rt->frame_buffer[0]) 	glDeleteBuffers(NUM_RENDER_TARGETS, rt->frame_buffer);
	if (rt->texture[0]) 		glDeleteTextures(NUM_RENDER_TARGETS, rt->texture);
	if (rt->depth_buffer[0])	glDeleteBuffers(NUM_RENDER_TARGETS, rt->depth_buffer);	
	if (rt->screenQuadProgramId) glDeleteProgram(rt->screenQuadProgramId);
	rt->screenQuadProgramId=0;
}
void RenderTarget_Init(RenderTarget* rt,int width, int height) {
	int i;	
	/*	
	In terms of performance, each time you bind, the driver needs to validate the state which costs CPU time. 
	It has been found that you get a speed boost if your textures is the same size and you use 1 FBO for them.
	If you have 10 textures that are 64x64 and 10 textures that are 512x64, make 2 FBOs. One FBO for each group.
	*/
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (rt->screenQuadProgramId==0) return;	

	rt->width = width;
	rt->height = height;


	for (i=0;i<NUM_RENDER_TARGETS;i++)	{
	rt->resolution_factor[i] = 1;
	
	glBindTexture(GL_TEXTURE_2D, rt->texture[i]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#	ifdef __EMSCRIPTEN__	// WebGL 1.0 (in Firefox) seems to accept only this settings (we could use it for non-emscripten builds too)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rt->width, rt->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
#	else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt->width, rt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);	// GL_BGRA, GL_UNSIGNED_BYTE
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt->width, rt->height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);	// GL_BGRA, GL_UNSIGNED_BYTE
#	endif

	//glBindRenderbuffer(GL_RENDERBUFFER, rt->depth_buffer[i]);
	//glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, rt->width, rt->height);	// GL_DEPTH_COMPONENT or GL_DEPTH_COMPONENT16
	
	glBindFramebuffer(GL_FRAMEBUFFER, rt->frame_buffer[i]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->texture[i], 0);
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth_buffer[i]);

	{
   	//Does the GPU support current FBO configuration?
   	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   	if (status!=GL_FRAMEBUFFER_COMPLETE) printf("glCheckFramebufferStatus(...) FAILED.\n");
	}

	
	}
	glBindTexture(GL_TEXTURE_2D, 0);	
	glBindFramebuffer(GL_FRAMEBUFFER,rt->default_frame_buffer);
	
}
RenderTarget render_target;

typedef struct {
GLuint programId;
GLint aLoc_APosition;
GLint uLoc_iResolution;
GLint uLoc_iGlobalTime;

GLint uLoc_iCameraMatrix;
} MyShaderStuff;
void MyShaderStuff_Create(MyShaderStuff* p) {
	const char fsFileName[] = "signed_distance_shapes.glsl";
	char fragmentShaderCode[4000000];fragmentShaderCode[0]='\0';
	if (!getTextFromFile(fragmentShaderCode,4000000,fsFileName))	{
		fprintf(stderr,"Error: \"%s\" not found\n",fsFileName);
		return;		
	}
	p->programId = loadShaderProgramFromSource(ScreenQuadVS,fragmentShaderCode);
	//progParams.programId = loadShaderProgram("default.vs",fsFileName);
	if (!p->programId) return;			
	
	p->aLoc_APosition = glGetAttribLocation(p->programId, "a_position");
	p->uLoc_iResolution = glGetUniformLocation(p->programId,"iResolution");
	p->uLoc_iGlobalTime = glGetUniformLocation(p->programId,"iGlobalTime");
	p->uLoc_iCameraMatrix = glGetUniformLocation(p->programId,"iCameraMatrix");
	
}
void MyShaderStuff_Destroy(MyShaderStuff* p) {if (p->programId) glDeleteProgram(p->programId);p->programId=0;}
void MyShaderStuff_SetUniforms(MyShaderStuff* p,int resX,int resY,float globalTime,const mat4_t* m) {
	if (p==NULL) return;
	glUniform2f(p->uLoc_iResolution,resX,resY);
	glUniform1f(p->uLoc_iGlobalTime,globalTime);
	if (m!=NULL) glUniformMatrix4fv(p->uLoc_iCameraMatrix, 1 /*only setting 1 matrix*/, GL_FALSE /*transpose?*/, &m->m[0][0]);
	
}
MyShaderStuff progParams;


GLuint screenQuadVbo = 0;
void ScreenQuadVBO_Init() {	
	const float verts[6] = {-1,-1,3,-1,-1,3};
	glGenBuffers(1,&screenQuadVbo);
	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);
	glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float), verts, GL_STATIC_DRAW);	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void ScreenQuadVBO_Destroy() {if (screenQuadVbo) {glDeleteBuffers(1,&screenQuadVbo);screenQuadVbo=0;}}
void ScreenQuadVBO_Bind() {
	glEnableVertexAttribArray(0);    	
	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);	
}
void ScreenQuadVBO_Draw() {glDrawArrays(GL_TRIANGLES,0,3);}
void ScreenQuadVBO_Unbind() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);    
}


GLuint getTextFromFile(char* buffer,int buffer_size,const char* filename) {
	FILE *pfile;
	memset(buffer,0,buffer_size);

	// This will raise a warning on MS compiler
	pfile = fopen(filename, "rb");
	if(!pfile)
	{
		printf("Sorry, can't open file: '%s'.\n", filename);
		exit(0);
		return 0;
	}
	
	fread(buffer,sizeof(char),buffer_size,pfile);
	//printf("%s\n",buffer);
	
	fclose(pfile);
	return 1;
}


// Loading shader function
GLhandleARB loadShader(const char* buffer, const unsigned int type)
{
	GLhandleARB handle;
	const GLcharARB* files[1];
	
	// shader Compilation variable
	GLint result;				// Compilation code result
	GLint errorLoglength ;
	char* errorLogText;
	GLsizei actualErrorLogLength;
	
	handle = glCreateShader(type);
	if (!handle)
	{
		//We have failed creating the vertex shader object.
		printf("Failed creating vertex shader object.\n");
		exit(0);
	}
	
	files[0] = (const GLcharARB*)buffer;
	glShaderSource(
					  handle, //The handle to our shader
					  1, //The number of files.
					  files, //An array of const char * data, which represents the source code of theshaders
					  NULL);
	
	glCompileShader(handle);

	//Compilation checking.
	glGetShaderiv(handle, GL_COMPILE_STATUS, &result);
	
	// If an error was detected.
	if (!result)
	{
		//We failed to compile.
		printf("Shader failed compilation.\n");
		
		//Attempt to get the length of our error log.
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &errorLoglength);
		
		//Create a buffer to read compilation error message
		errorLogText =(char*) malloc(sizeof(char) * errorLoglength);
		
		//Used to get the final length of the log.
		glGetShaderInfoLog(handle, errorLoglength, &actualErrorLogLength, errorLogText);
		
		// Display errors.
		printf("%s\n",errorLogText);
		
		// Free the buffer malloced earlier
		free(errorLogText);
	}
	
	return handle;
}

GLuint loadShaderProgramFromSource(const char* vs,const char* fs)	{
	// shader Compilation variable
	GLint result;				// Compilation code result
	GLint errorLoglength ;
	char* errorLogText;
	GLsizei actualErrorLogLength;

	GLhandleARB vertexShaderHandle;
	GLhandleARB fragmentShaderHandle;		
	GLuint programId = 0;

	vertexShaderHandle   = loadShader(vs,GL_VERTEX_SHADER);
	fragmentShaderHandle = loadShader(fs,GL_FRAGMENT_SHADER);
	if (!vertexShaderHandle || !fragmentShaderHandle) return 0;

	programId = glCreateProgram();
	
	glAttachShader(programId,vertexShaderHandle);
	glAttachShader(programId,fragmentShaderHandle);
	glLinkProgram(programId);

	//Link checking.
	glGetProgramiv(programId, GL_LINK_STATUS, &result);
	
	// If an error was detected.
	if (!result)
	{
		//We failed to compile.
		printf("Program failed to link.\n");
		
		//Attempt to get the length of our error log.
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &errorLoglength);
		
		//Create a buffer to read compilation error message
		errorLogText =(char*) malloc(sizeof(char) * errorLoglength);
		
		//Used to get the final length of the log.
		glGetProgramInfoLog(programId, errorLoglength, &actualErrorLogLength, errorLogText);
		
		// Display errors.
		printf("%s\n",errorLogText);
		
		// Free the buffer malloced earlier
		free(errorLogText);
	}

	glDeleteShader(vertexShaderHandle);
	glDeleteShader(fragmentShaderHandle);

	return programId;
}

GLuint loadShaderProgramFromFile(const char* vspath,const char* fspath) {
	char vsbuf[400000];
	char fsbuf[400000];
	vsbuf[0]='\0';fsbuf[0]='\0';
	if (!getTextFromFile(vsbuf,400000,vspath) || !getTextFromFile(fsbuf,400000,fspath)) return 0;
	
	return loadShaderProgramFromSource(vsbuf,fsbuf);
}

void ResizeGL(int w,int h) {
if (h>0)	{
	pMatrix =  //m4_ortho(-10.f, 10.f, -5.f, 5.f, 0.1f, 20.f);
    m4_perspective  (60.f, (float)w/(float)h,0.1f, 500.f);
}

	if (h>0) RenderTarget_Init(&render_target,w,h);

	glViewport(0,0,w,h);

}



void InitGL(void) {
	vec3_t cameraPos;

	glEnable(GL_TEXTURE_2D);
	MyShaderStuff_Create(&progParams);
	RenderTarget_Create(&render_target);
	ScreenQuadVBO_Init();
	// This is important, if not here, FBO's depthbuffer won't be populated.
	//glEnable(GL_DEPTH_TEST);
	glClearColor(0,0,0,1.0f);	
	//glEnable(GL_CULL_FACE);
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);	

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);

	cameraTarget = vec3( -0.5f, -0.4f, 0.5f );
	cameraPos = vec3(-0.5f+3.5*cos(0.1f*15.f + 6.0f*0.f), 1.0f + 2.0f*0.f, 0.5f + 4.0f*sin(0.1f*15.f + 6.0f*0.f));

	cameraMatrix = m4_identity();
	m4_set_translation(&cameraMatrix,cameraPos);
	m4_look_at_YX	(&cameraMatrix,cameraTarget,minCameraTargetDistance,maxCameraTargetDistance);	
	cameraMatrixSlerp = cameraMatrix;	//Mandatory
}

void DestroyGL() {
	ScreenQuadVBO_Destroy();
	RenderTarget_Destroy(&render_target);
	MyShaderStuff_Destroy(&progParams);
}

#ifndef NO_FIXED_FUNCTION_PIPELINE
void DrawGlutText(int x, int y, char *string);
#endif

void DrawGL(void) 
{	
	static char tmp[64] = "";
	static float resolution_factor = 1.0f;
	static int frame = 0;
	static unsigned begin = 0;
	static unsigned cur_time = 0;
	static unsigned display_fps_time = 0;
	static unsigned delta_frames = 0;
	static int render_target_index = 0;
	static unsigned cameraMatrixSlerpTimerBegin = 0;
	int render_target_index2 = 0;
	float current_resolution_factor_fbo,current_resolution_factor_draw;
	unsigned elapsed_time,delta_time;
	if (begin==0) begin = glutGet(GLUT_ELAPSED_TIME);
	elapsed_time = glutGet(GLUT_ELAPSED_TIME) - begin;
	delta_time = elapsed_time - cur_time;
	cur_time = elapsed_time;
	
	// camera stuff
    //mvMatrix = //m4_invert_fast(m4_invert_XZ_axis(&cameraMatrix));	// correct to mimic m4_look_at(...)
				//cameraMatrix;
	
	if (cameraMatrixSlerpTimer<1.f)	{
		if (cameraMatrixSlerpTimerBegin==0) cameraMatrixSlerpTimerBegin = glutGet(GLUT_ELAPSED_TIME);
		cameraMatrixSlerpTimer = (float)(glutGet(GLUT_ELAPSED_TIME) - cameraMatrixSlerpTimerBegin)*0.0001f*cameraMatrixSlerpTimerSpeed;
		if (cameraMatrixSlerpTimer>1.f) {
			cameraMatrixSlerpTimer = 1.f;
			cameraMatrixSlerpTimerBegin = 0;
			cameraMatrix = cameraMatrixSlerp;	
		}	
		else cameraMatrix = m4_slerp(&cameraMatrix,&cameraMatrixSlerp,cameraMatrixSlerpTimer);					
	}

	ScreenQuadVBO_Bind();

	// Render to framebuffer---------------------------------------------------------------------------------------
	if (render_target.dynamic_resolution_enabled)	{
		render_target.resolution_factor[render_target_index] = resolution_factor;
		glViewport(0, 0, (int)(render_target.width * resolution_factor),(int) (render_target.height * resolution_factor));
		glBindFramebuffer(GL_FRAMEBUFFER, render_target.frame_buffer[render_target_index]); //NUM_RENDER_TARGETS	
	}
	else glViewport(0, 0, render_target.width, render_target.height);	

	//Using the raycast shader
	glUseProgram(progParams.programId);
	MyShaderStuff_SetUniforms(&progParams,
			render_target.width *  (render_target.dynamic_resolution_enabled ? resolution_factor : 1.0f),
			render_target.height * (render_target.dynamic_resolution_enabled ? resolution_factor : 1.0f),
			(float)elapsed_time/1000.f,
			&cameraMatrix);
	ScreenQuadVBO_Draw();
	//glUseProgram(0);	

	if (render_target.dynamic_resolution_enabled) 
		glBindFramebuffer(GL_FRAMEBUFFER,render_target.default_frame_buffer);
	//-------------------------------------------------------------------------------------------------------------

	// Draw to screen at resolution_factor: render_target.resolution_factor[render_target_index2]------------------
	if (render_target.dynamic_resolution_enabled)	{	
		glViewport(0, 0, render_target.width, render_target.height);	
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		render_target_index2 = render_target_index + 1;
		if (render_target_index2>=NUM_RENDER_TARGETS) render_target_index2-=NUM_RENDER_TARGETS;	
	
		//printf("%d - %d\n",render_target_index,render_target_index2);	
		glActiveTexture(GL_TEXTURE0);	
		glBindTexture(GL_TEXTURE_2D, render_target.texture[render_target_index2]);

		glUseProgram(render_target.screenQuadProgramId);
		glUniform1i(render_target.uLoc_SDiffuse,0);	
		glUniform3f(render_target.uLoc_screenResAndFactor,render_target.width,render_target.height,render_target.resolution_factor[render_target_index2]);	
		ScreenQuadVBO_Draw();

		//glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	//--------------------------------------------------------------------------------------------------------------

	glUseProgram(0);
	ScreenQuadVBO_Unbind();

#	ifndef NO_FIXED_FUNCTION_PIPELINE
	if (showFPS) DrawGlutText(20,(int) (render_target.height-20), tmp);
#	endif

	// Do FPS count and adjust resolution_factor
	++frame;
	if (++render_target_index>=NUM_RENDER_TARGETS) render_target_index=0;
	display_fps_time+=delta_time;
	++delta_frames;
	if (display_fps_time>2000 && delta_frames>0) {
		//FPS_TARGET = render_target.dynamic_resolution_target_fps;
		FPS = delta_frames*1000/display_fps_time;
		display_fps_time = 0;
		delta_frames = 0;
		if (FPS<FPS_TARGET) {
			resolution_factor-= resolution_factor*(FPS_TARGET-FPS)*0.0175f;
			if (resolution_factor<0.15f) resolution_factor=0.15f;
		}
		else {
			resolution_factor+= resolution_factor*(FPS-FPS_TARGET)*0.0175f;
			if (resolution_factor>1.0f) resolution_factor=1.0f;
		}
		sprintf(tmp,"FPS: %u DYN-RES:%s DRF=%1.3f (%dx%d %s)",FPS,render_target.dynamic_resolution_enabled ? "ON " : "OFF",resolution_factor,render_target.width,render_target.height,windowId ? "windowed" : "fullscreen");		
#		ifdef NO_FIXED_FUNCTION_PIPELINE
		if (showFPS)	{
			//glutSetWindowTitle(tmp);			
			printf("FPS: %u DYN-RES:%s DRF=%1.3f (%dx%d %s)\n",FPS,render_target.dynamic_resolution_enabled ? "ON " : "OFF",resolution_factor,render_target.width,render_target.height,windowId ? "windowed" : "fullscreen");		
		}
#		endif //NO_FIXED_FUNCTION_PIPELINE
	}
}

#ifndef NO_FIXED_FUNCTION_PIPELINE
void DrawGlutText(int x, int y, char *string)
{
	int len, i;mat4_t orthoMatrix;
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	orthoMatrix = m4_ortho_2d(0, render_target.width,0,render_target.height);
	glLoadMatrixf(&(orthoMatrix.m[0][0]));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glColor3f(0.75f, 0.25f, 0.25f);
	glRasterPos2i(x, y);
	len = (int) strlen(string);
	for (i = 0; i < len; i++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
/*
	// Fixed width	
    GLUT_BITMAP_9_BY_15
    GLUT_BITMAP_8_BY_13

	// Variable width
    GLUT_BITMAP_HELVETICA_18
    GLUT_BITMAP_HELVETICA_12
    GLUT_BITMAP_HELVETICA_10
    GLUT_BITMAP_TIMES_ROMAN_24
    GLUT_BITMAP_TIMES_ROMAN_10
*/

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
#endif//NO_FIXED_FUNCTION_PIPELINE

static void GlutDestroyWindow(void);
static void GlutCreateWindow(int fullScreen);


void GlutNormalKeys(unsigned char key, int x, int y) {
	const int mod = glutGetModifiers();    	
	switch (key) {
#	ifndef __EMSCRIPTEN__	
	case 27: 	// esc key
		GlutDestroyWindow();
#		ifdef __FREEGLUT_STD_H__	
		glutLeaveMainLoop();	
#		else
		exit(0);
#		endif
	break;
	case 13:	// return key
	{
		if (mod&GLUT_ACTIVE_CTRL) {
		int fullScreen = gameModeWindowId ? 0 : 1;
		GlutDestroyWindow();
		GlutCreateWindow(fullScreen);		 
		}
	}			
	break;   
#endif //__EMSCRIPTEN__	
	}

}

void MoveCameraAroundTarget(int glut_key,float amount,const mat4_t* cameraIn,mat4_t* cameraOut) {
	vec3_t cameraPos = m4_get_translation(cameraIn);
	float cameraTargetDistance = v3_length(v3_sub(cameraPos,cameraTarget));
	
	vec3_t YPR;
	mat3_t cameraMat3 = m4_get_mat3(cameraIn);
	m3_to_euler_YXZ(&cameraMat3,&YPR);
	YPR.z=0.f;

	if (glut_key==GLUT_KEY_LEFT || glut_key==GLUT_KEY_RIGHT) 
		YPR.x-=(glut_key==GLUT_KEY_RIGHT ? amount : -amount);				
	else if (glut_key==GLUT_KEY_UP || glut_key==GLUT_KEY_DOWN)	{
		YPR.y-=(glut_key==GLUT_KEY_DOWN ? amount : -amount);
		if      (YPR.y < -M_HALF_PI+0.001f) YPR.y = -M_HALF_PI+0.001f;
		else if (YPR.y >  M_HALF_PI-0.001f) YPR.y =  M_HALF_PI-0.001f;			
	}
	m4_set_mat3(cameraOut,m3_from_euler_YXZ(YPR));
		
	// Now we must update the camera position:		
	cameraPos = v3_sub(cameraTarget,v3_muls(m4_get_z_axis(cameraOut),cameraTargetDistance));
	m4_set_translation(cameraOut,cameraPos);	
	
	if (cameraOut==&cameraMatrixSlerp) cameraMatrixSlerpTimer = 0.f;	
}
void ZoomCamera(int glut_key,float amount,const mat4_t* cameraIn,mat4_t* cameraOut) {
	vec3_t cameraPos = m4_get_translation(cameraIn);
	float cameraTargetDistance = v3_length(v3_sub(cameraPos,cameraTarget));
	cameraTargetDistance-=(glut_key==GLUT_KEY_PAGE_UP ? amount : -amount);
	if 		(cameraTargetDistance<minCameraTargetDistance) cameraTargetDistance=minCameraTargetDistance;
	else if (cameraTargetDistance>maxCameraTargetDistance) cameraTargetDistance=maxCameraTargetDistance;
	// Now we must update the camera position:		
	m4_set_translation(cameraOut,v3_sub(cameraTarget,v3_muls(m4_get_z_axis(cameraOut),cameraTargetDistance)));

	if (cameraOut==&cameraMatrixSlerp) cameraMatrixSlerpTimer = 0.f;	
}

void MoveCameraTarget(int glut_key,float amount,const mat4_t* cameraIn,mat4_t* cameraOut) {
	vec3_t cameraPos = m4_get_translation(cameraIn);
	vec3_t deltaTarget = vec3(0,0,0);
	if (cameraOut!=cameraIn) *cameraOut=*cameraIn;
	if 		(glut_key==GLUT_KEY_LEFT) 		deltaTarget.x-=amount;
	else if (glut_key==GLUT_KEY_RIGHT)		deltaTarget.x+=amount;
	else if	(glut_key==GLUT_KEY_UP) 		deltaTarget.z+=amount;
	else if (glut_key==GLUT_KEY_DOWN)		deltaTarget.z-=amount;
	deltaTarget = m4_mul_dir(*cameraIn,deltaTarget);	// Optional, to move not in worls space
	deltaTarget.y = 0.f;	// But we still want the move in +-y to be in world space
	if		(glut_key==GLUT_KEY_PAGE_UP) 	deltaTarget.y+=amount;
	else if (glut_key==GLUT_KEY_PAGE_DOWN)	deltaTarget.y-=amount;

	cameraTarget=v3_add(cameraTarget,deltaTarget);
	cameraPos=v3_add(cameraPos,deltaTarget);
	m4_set_translation(cameraOut,cameraPos);

	if (cameraOut==&cameraMatrixSlerp) cameraMatrixSlerpTimer = 0.f;	
}

void GlutSpecialKeys(int key,int x,int y)
{
	const int mod = glutGetModifiers();

	// We can choose one of these 3 camera smoothing modes:

//	1) None
//	const mat4_t* cameraIn = &cameraMatrix;
//	mat4_t* cameraOut = &cameraMatrix;	

//	2) Moderate
	const mat4_t* cameraIn = &cameraMatrix;
	mat4_t* cameraOut = &cameraMatrixSlerp;	

//	3) Too much
//	const mat4_t* cameraIn = &cameraMatrixSlerp;
//	mat4_t* cameraOut = &cameraMatrixSlerp;

//  But main problem here is that FPS is sampled every two seconds, and even with dynamic resolution on,
//  it's not stable at all on my system... But I guess that on good GPUs it's stable enough..

	if (!(mod&GLUT_ACTIVE_CTRL))	{
    switch (key) {
    case GLUT_KEY_LEFT:
	case GLUT_KEY_RIGHT: 
    case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:	{     		     
		float amount = (35.f/(float)FPS)*((key==GLUT_KEY_LEFT || key==GLUT_KEY_RIGHT) ? 0.025f : 0.01f);
		MoveCameraAroundTarget(key,amount,cameraIn,cameraOut);	
	}
        break;
	case GLUT_KEY_PAGE_UP:
	case GLUT_KEY_PAGE_DOWN:
		ZoomCamera(key,0.1f*35.f/(float)FPS,cameraIn,cameraOut);
		break;
	case GLUT_KEY_F1:	
	{
		render_target.dynamic_resolution_enabled = !render_target.dynamic_resolution_enabled;
		printf("dynamic_resolution_enabled: %s.\n",render_target.dynamic_resolution_enabled?"ON":"OFF");
	}
	break;
	case GLUT_KEY_F2:	
	{
		showFPS = !showFPS;
		printf("showFPS: %s.\n",showFPS?"ON":"OFF");
	}
	break;
	}
	}
	else {
		switch (key) {
    	case GLUT_KEY_LEFT:
		case GLUT_KEY_RIGHT: 
    	case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:	
		case GLUT_KEY_PAGE_UP:
		case GLUT_KEY_PAGE_DOWN:
		{     		     
			float amount = (35.f/(float)FPS)*0.025f;
			MoveCameraTarget(key,amount,cameraIn,cameraOut);	
		}
        break;
	}	
	}
}

void GlutMouse(int a,int b,int c,int d) {

}



static void GlutDrawGL(void)		{DrawGL();glutSwapBuffers();}
static void GlutIdle(void)			{glutPostRedisplay();}
static void GlutFakeDrawGL(void) 	{glutDisplayFunc(GlutDrawGL);}
void GlutDestroyWindow(void) {
	if (gameModeWindowId || windowId)	{	
	DestroyGL();
		
	if (gameModeWindowId) {
		glutLeaveGameMode();
		gameModeWindowId = 0;	
	}
	if (windowId) {
		glutDestroyWindow(windowId);
		windowId=0;
	}
	}
}
void GlutCreateWindow(int fullScreen) {
	GlutDestroyWindow();	
	if (fullScreen)	{
		const int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
		const int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
		char gms[16]="";
		if (FULLSCREEN_RENDER_WIDTH>0 && FULLSCREEN_RENDER_HEIGHT>0)	{
			sprintf(gms,"%dx%d:32",(int)FULLSCREEN_RENDER_WIDTH,(int)FULLSCREEN_RENDER_HEIGHT);
			glutGameModeString(gms);
			if (glutGameModeGet (GLUT_GAME_MODE_POSSIBLE)) gameModeWindowId = glutEnterGameMode();
		}
		if (gameModeWindowId==0)	{
			sprintf(gms,"%dx%d:32",screenWidth,screenHeight);
			glutGameModeString(gms);
			if (glutGameModeGet (GLUT_GAME_MODE_POSSIBLE)) gameModeWindowId = glutEnterGameMode();
		}
	}
	if (!gameModeWindowId) {
		glutInitWindowPosition(100,100);
    	glutInitWindowSize(RENDER_WIDTH,RENDER_HEIGHT);
		windowId = glutCreateWindow("GLSL signed distance shapes");
	}

	glutKeyboardFunc(GlutNormalKeys);
    glutSpecialFunc(GlutSpecialKeys);
	glutMouseFunc(GlutMouse);
    glutIdleFunc(GlutIdle);
    glutReshapeFunc(ResizeGL);
    glutDisplayFunc(GlutFakeDrawGL);   


#ifdef USE_GLEW
	{
    	GLenum err = glewInit();
    	if( GLEW_OK != err ) {
    	    fprintf(stderr, "Error initializing GLEW: %s\n", glewGetErrorString(err) );
    	    return;
    	}
	}
#elif _WIN32
	// This call will grab openGL extension function pointers.
	// This is not necessary for macosx and linux
	getOpenGLFunctionPointers();
#endif //USE_GLEW

#ifdef _DEBUG
    if(glDebugMessageCallback){
        printf("Register OpenGL debug callback\n");
		glEnable(GL_DEBUG_OUTPUT);//
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(openglCallbackFunction, NULL);
        GLuint unusedIds = 0;
        glDebugMessageControl(GL_DONT_CARE,
            GL_DONT_CARE,
            GL_DONT_CARE,
            0,
            &unusedIds,
            1);
    }
    else
        printf("glDebugMessageCallback not available\n");
#endif

	InitGL();
	
}


int main(int argc, char** argv)
{

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);	// GLUT_ALPHA
#ifndef __EMSCRIPTEN__
	//glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
#ifdef __FREEGLUT_STD_H__
    glutSetOption ( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION ) ;
#endif //__FREEGLUT_STD_H__
#endif //__EMSCRIPTEN__

   	GlutCreateWindow(0);

    //OpenGL info
    printf("\nGL Vendor: %s\n", glGetString( GL_VENDOR ));
    printf("GL Renderer : %s\n", glGetString( GL_RENDERER ));
    printf("GL Version (string) : %s\n",  glGetString( GL_VERSION ));
    printf("GLSL Version : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION ));
    //printf("GL Extensions:\n%s\n",(char *) glGetString(GL_EXTENSIONS));

	printf("\nKEYS:\n");
	printf("AROW KEYS + PAGE_UP/PAGE_DOWN:\tmove camera (optionally with CTRL down)\n");
	printf("F1:\t\t\t\ttoggle dynamic resolution on/off\n");
	printf("F2:\t\t\t\ttoggle display FPS on/off\n");
#	ifndef __EMSCRIPTEN__	
	printf("CTRL+RETURN:\t\t\ttoggle fullscreen on/off\n\n");	
#	endif //__EMSCRIPTEN__	

	glutMainLoop();


return 0;
}



