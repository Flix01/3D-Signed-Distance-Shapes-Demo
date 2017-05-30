#ifndef TEAPOT_H_
#define TEAPOT_H_

/* LICENSE: Made by @Flix01. MIT license on my part.
 * But I don't know the license of the original Teapot mesh data,
 * and of the Bunny and Torus meshes (from the Bullet Physics Engine's GImpact Old Demo)
 * I've made the other meshes myself.
 * You can disable some meshes with something like TEAPOT_NO_MESH_BUNNY (never tested).
*/

/* USAGE:
Please see the comments of the functions below and try to follow their order when you call them.
Define TEAPOT_IMPLEMENTATION in one of your .c files before the inclusion of this file.
*/
// OPTIONAL DEFINITIONS:
//#define TEAPOT_SHADER_SPECULAR // Adds specular light component in the shader

typedef enum {
    TEAPOT_MESH_TEAPOT=0,
    TEAPOT_MESH_BUNNY,
    TEAPOT_MESH_TORUS,
    TEAPOT_MESH_CUBE,
    TEAPOT_MESH_CYLINDER,
    TEAPOT_MESH_CONE,
    TEAPOT_MESH_PYRAMID,
    TEAPOT_MESH_ROOF,
    TEAPOT_MESH_SPHERE1,
    TEAPOT_MESH_SPHERE2,
    TEAPOT_MESH_CYLINDER_LATERAL_SURFACE,
    TEAPOT_MESH_HALF_SPHERE_UP,         // These are not centered
    TEAPOT_MESH_HALF_SPHERE_DOWN,       // These are not centered
    //TEAPOT_MESH_CAPSULE,
    TEAPOT_MESH_COUNT
} TeapotMeshEnum;

void Teapot_Init(void);     // In your InitGL() method
void Teapot_Destroy(void);  // In your DestroyGL() method (cleanup)

// In your InitGL() and ResizeGL() methods:
void Teapot_SetProjectionMatrix(const float pMatrix[16]);                 // Sets the projection matrix [Warning: it calls glUseProgram(0); at the end => call it outside Teapot_PreDraw()/Teapot_PostDraw()]


// In your DrawGL() method:
void Teapot_SetViewMatrixAndLightDirection(const float vMatrix[16],const float lightDirectionWorldSpace[3]);    // vMatrix CAN'T HAVE any scaling! Sets the camera view matrix (= lookAt matrix) and the directional light in world space [Warning: it calls glUseProgram(0); at the end => call it outside Teapot_PreDraw()/Teapot_PostDraw()]

void Teapot_PreDraw(void);  // sets program and buffers for drawing

void Teapot_SetScaling(float scalingX,float scalingY,float scalingZ);   // Optional (last set is used)
void Teapot_SetColor(float R, float G, float B, float A);       // Optional (last set is used)
#ifdef TEAPOT_SHADER_SPECULAR
void Teapot_SetColorSpecular(float R, float G, float B, float SHI); // Optional (last set is used)
#endif //TEAPOT_SHADER_SPECULAR
void Teapot_Draw(const float mMatrix[16],TeapotMeshEnum meshId);   // Hp) mMatrix CAN'T HAVE any scaling! (optionally repeat this call for multiple draws)

void Teapot_PostDraw(void); // unsets program and buffers for drawing

inline void Teapot_GetMeshCenter(TeapotMeshEnum meshId,float center[3]);
inline void Teapot_GetMeshHalfAabb(TeapotMeshEnum meshId,float halfAabb[3]);

#endif //TEAPOT_H_

#define TEAPOT_IMPLEMENTATION
#ifdef TEAPOT_IMPLEMENTATION

#include <math.h>   // sqrt

static const char* TeapotVS[] = {
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif\n"
    "attribute vec4 a_vertex;\n"
    "attribute vec3 a_normal;\n"
    "uniform mat4 u_mvMatrix;\n"
    "uniform mat4 u_pMatrix;\n"
    "uniform vec4 u_scaling;\n"
    "uniform vec3 u_lightVector;\n"
    "uniform vec4 u_color;\n"
#   ifdef TEAPOT_SHADER_SPECULAR
    "uniform vec4 u_colorSpecular;\n"   // R,G,B,SHININESS
#   endif //TEAPOT_SHADER_SPECULAR
    "varying vec4 v_color;\n"
    "\n"
    "void main()	{\n"
    "   vec3 normalEyeSpace = vec3(u_mvMatrix * vec4(a_normal, 0.0));\n"
    "   float fDot = max(0.25, dot(normalEyeSpace,u_lightVector));\n"      // 0.25 is the ambient factor
    "   vec4 vertexScaledEyeSpace = u_mvMatrix*(a_vertex * u_scaling);\n"
    "   vertexScaledEyeSpace.xyz/vertexScaledEyeSpace.w;\n"                 // is this necessary ?
#   ifndef TEAPOT_SHADER_SPECULAR
    "   v_color = vec4(u_color.rgb * fDot,u_color.a);\n"
#   else  // TEAPOT_SHADER_SPECULAR
    "   vec3 E = normalize(-vertexScaledEyeSpace);\n"
    "   vec3 halfVector = normalize(u_lightVector + E);\n"
    "   float nxHalf = max(0.0,dot(normalEyeSpace, halfVector));\n"
    "   vec3 specularColor = u_colorSpecular.rgb * pow(nxHalf,u_colorSpecular.a);\n"
    "   v_color = vec4(u_color.rgb*fDot+specularColor,u_color.a);\n"
#   endif // TEAPOT_SHADER_SPECULAR
    "   gl_Position = u_pMatrix * vertexScaledEyeSpace;\n"
    "}\n"
};
// 0.2 is the global ambient factor

static const char* TeapotFS[] = {
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec4 v_color;\n"
    "\n"
    "void main() {\n"
#   ifdef TEAPOT_SHADER_GAMMA_CORRECTION
    "    gl_FragColor = vec4(pow(v_color.rgb,vec3(0.4545)),v_color.a);\n"
    //"    gl_FragColor = vec4(sqrt(v_color.rgb),v_color.a);\n"    // = pow(col,vec3(0.5)); but it's probably faster than the line above
#   else //TEAPOT_SHADER_GAMMA_CORRECTION
    "    gl_FragColor = v_color;\n"
#   endif //TEAPOT_SHADER_GAMMA_CORRECTION
    "}\n"
};

typedef struct {
    GLuint programId;
    GLuint vertexBuffer,normalBuffer,elementBuffer;
    int startInds[TEAPOT_MESH_COUNT];
    int numInds[TEAPOT_MESH_COUNT];
    float halfExtents[TEAPOT_MESH_COUNT][3];
    float centerPoint[TEAPOT_MESH_COUNT][3];

    GLint aLoc_vertex,aLoc_normal;
    GLint uLoc_mvMatrix,uLoc_pMatrix,uLoc_scaling,uLoc_lightVector,uLoc_color,uLoc_colorSpecular;

    float vMatrix[16];
} Teapot_Inner_Struct;
static Teapot_Inner_Struct TIS;


static inline GLuint Teapot_LoadShaderProgramFromSource(const char* vs,const char* fs);

void Teapot_SetViewMatrixAndLightDirection(const float vMatrix[16],const float lightDirectionWorldSpace[3])   {
    // lightDirectionViewSpace = v3_norm(m4_mul_dir(vMatrix,light_direction));
    memcpy(TIS.vMatrix,vMatrix,16*sizeof(float));
    //int i;for (i=0;i<16;i++) TIS.vMatrix[i]=vMatrix[i];
    float lightDirectionViewSpace[3] = {
        lightDirectionWorldSpace[0]*TIS.vMatrix[0] + lightDirectionWorldSpace[1]*TIS.vMatrix[4] + lightDirectionWorldSpace[2]*TIS.vMatrix[8],
        lightDirectionWorldSpace[0]*TIS.vMatrix[1] + lightDirectionWorldSpace[1]*TIS.vMatrix[5] + lightDirectionWorldSpace[2]*TIS.vMatrix[9],
        lightDirectionWorldSpace[0]*TIS.vMatrix[2] + lightDirectionWorldSpace[1]*TIS.vMatrix[6] + lightDirectionWorldSpace[2]*TIS.vMatrix[10]
    };
    float len = lightDirectionWorldSpace[0]*lightDirectionWorldSpace[0]+lightDirectionWorldSpace[1]*lightDirectionWorldSpace[1]+lightDirectionWorldSpace[2]*lightDirectionWorldSpace[2];
    if (len<0.999f || len>1.001f) {len = sqrt(len);lightDirectionViewSpace[0]/=len;lightDirectionViewSpace[1]/=len;lightDirectionViewSpace[2]/=len;}

    glUseProgram(TIS.programId);
    glUniform3fv(TIS.uLoc_lightVector,1,lightDirectionViewSpace);
    glUseProgram(0);
}
void Teapot_SetProjectionMatrix(const float pMatrix[16])    {
    glUseProgram(TIS.programId);
    glUniformMatrix4fv(TIS.uLoc_pMatrix, 1 /*only setting 1 matrix*/, GL_FALSE /*transpose?*/, pMatrix);
    glUseProgram(0);
}


void Teapot_PreDraw(void)   {
    if (TIS.programId)  {
        glEnableVertexAttribArray(TIS.aLoc_vertex);
        glBindBuffer(GL_ARRAY_BUFFER, TIS.vertexBuffer);
        glVertexAttribPointer(TIS.aLoc_vertex, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, 0);

        glEnableVertexAttribArray(TIS.aLoc_normal);
        glBindBuffer(GL_ARRAY_BUFFER, TIS.normalBuffer);
        glVertexAttribPointer(TIS.aLoc_normal, 3, GL_FLOAT, GL_FALSE, sizeof(float)*3, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TIS.elementBuffer);

        glUseProgram(TIS.programId);
    }
}

void Teapot_SetScaling(float scalingX, float scalingY, float scalingZ)  {glUniform4f(TIS.uLoc_scaling,scalingX,scalingY,scalingZ,1.f);}
void Teapot_SetColor(float R,float G,float B,float A)  {glUniform4f(TIS.uLoc_color,R,G,B,A);}
#ifdef TEAPOT_SHADER_SPECULAR
void Teapot_SetColorSpecular(float R, float G, float B, float SHI)  {glUniform4f(TIS.uLoc_colorSpecular,R,G,B,SHI);}
#endif //TEAPOT_SHADER_SPECULAR


void Teapot_Draw(const float mMatrix[16], TeapotMeshEnum meshId)    {
    // mvMatrix = vMatrix * mMatrix;
    const float* v = TIS.vMatrix;
    const float* m = mMatrix;
    float mvMatrix[16];
    int i,j,j4;
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            j4 = 4*j;
            mvMatrix[i+j4] =
                v[i]    * m[0+j4] +
                v[i+4]  * m[1+j4] +
                v[i+8]  * m[2+j4] +
                v[i+12] * m[3+j4];
        }
    }

    glUniformMatrix4fv(TIS.uLoc_mvMatrix, 1 /*only setting 1 matrix*/, GL_FALSE /*transpose?*/, mvMatrix);

    //if (meshId<TEAPOT_MESH_CAPSULE)
        glDrawElements(GL_TRIANGLES,TIS.numInds[meshId],GL_UNSIGNED_SHORT,(const void*) (TIS.startInds[meshId]*sizeof(unsigned short)));
    /*else if (meshId==TEAPOT_MESH_CAPSULE) {
        // TODO
    }*/
}

void Teapot_PostDraw(void)  {
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glDisableVertexAttribArray(TIS.aLoc_vertex);
    glDisableVertexAttribArray(TIS.aLoc_normal);
}


// Loading shader function
static inline GLhandleARB Teapot_LoadShader(const char* buffer, const unsigned int type)
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

static inline GLuint Teapot_LoadShaderProgramFromSource(const char* vs,const char* fs)	{
    // shader Compilation variable
    GLint result;				// Compilation code result
    GLint errorLoglength ;
    char* errorLogText;
    GLsizei actualErrorLogLength;

    GLhandleARB vertexShaderHandle;
    GLhandleARB fragmentShaderHandle;
    GLuint programId = 0;

    vertexShaderHandle   = Teapot_LoadShader(vs,GL_VERTEX_SHADER);
    fragmentShaderHandle = Teapot_LoadShader(fs,GL_FRAGMENT_SHADER);
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

// numVerts is the number of vertices (= number of 3 floats)
// numInds is the number of triangle indices (= 3 * num triangles)
// pNormsOut must be the same size as pVerts
inline static void CalculateVertexNormals(const float* pVerts,int numVerts,const unsigned short* pInds,int numInds,float* pNormsOut)   {
    if (!pVerts || !pNormsOut || !pInds || numInds<3 || numVerts<3) return;
    // Calculate vertex normals
    {
        int i,f,iSz;float v21[3],v01[3],vcr[3];float len;
        for (i=0;i<numVerts*3;i++)	pNormsOut[i] = 0.f;

        for (f = 0;f < numInds;f=f+3)	{
            const int id1 = 3*(int)pInds[f];
            const int id2 = 3*(int)pInds[f+1];
            const int id3 = 3*(int)pInds[f+2];

            const float* v0 = &pVerts[id1];
            const float* v1 = &pVerts[id2];
            const float* v2 = &pVerts[id3];

            float* n0 = &pNormsOut[id1];
            float* n1 = &pNormsOut[id2];
            float* n2 = &pNormsOut[id3];

            v21[0] = v2[0]-v1[0];   v21[1] = v2[1]-v1[1];   v21[2] = v2[2]-v1[2];
            v01[0] = v0[0]-v1[0];   v01[1] = v0[1]-v1[1];   v01[2] = v0[2]-v1[2];

            //vcr = v21.cross(v01);
            vcr[0] = v21[1] * v01[2] - v21[2] * v01[1];
            vcr[1] = v21[2] * v01[0] - v21[0] * v01[2];
            vcr[2] = v21[0] * v01[1] - v21[1] * v01[0];

            n0[0]+=vcr[0];  n0[1]+=vcr[1];  n0[2]+=vcr[2];
            n1[0]+=vcr[0];  n1[1]+=vcr[1];  n1[2]+=vcr[2];
            n2[0]+=vcr[0];  n2[1]+=vcr[1];  n2[2]+=vcr[2];
        }
        for (i=0,iSz=numVerts*3;i<iSz;i+=3)	{
            float* n = &pNormsOut[i];
            len = n[0]*n[0]+n[1]*n[1]+n[2]*n[2];
            if (len>0.00001f)   {len = sqrt(len);n[0]/=len;n[1]/=len;n[2]/=len;}
            else {n[0]=0.f;n[1]=1.f;n[2]=0.f;}
        }
    }
}

// numVerts is the number of vertices (= number of 3 floats)
inline static void GetAabbHalfExtentsAndCenter(const float* pVerts,const int numVerts,float halfExtentsOut[3],float centerPointOut[3])	{
    float minValue[3];float maxValue[3];float tempVert[3];int i,i3;
    if (pVerts && numVerts>0)	{
        minValue[0]=maxValue[0]=pVerts[0];minValue[1]=maxValue[1]=pVerts[1];minValue[2]=maxValue[2]=pVerts[2];
    }
    else minValue[0]=maxValue[0]=minValue[1]=maxValue[1]=minValue[2]=maxValue[2]=0;
    for (i=1; i<numVerts; i++)	{
        i3 = i*3;tempVert[0]=pVerts[i3];tempVert[1]=pVerts[i3+1];tempVert[2]=pVerts[i3+2];
        if (minValue[0]>tempVert[0])        minValue[0]=tempVert[0];
        else if (maxValue[0]<tempVert[0])   maxValue[0]=tempVert[0];
        if (minValue[1]>tempVert[1])        minValue[1]=tempVert[1];
        else if (maxValue[1]<tempVert[1])   maxValue[1]=tempVert[1];
        if (minValue[2]>tempVert[2])        minValue[2]=tempVert[2];
        else if (maxValue[2]<tempVert[2])   maxValue[2]=tempVert[2];
    }
    for (i=0;i<3;i++) {
        halfExtentsOut[i]=(maxValue[i]-minValue[i])*0.5;
        centerPointOut[i]=(maxValue[i]+minValue[i])*0.5;
    }
}

void Teapot_Destroy(void) {
    if (TIS.vertexBuffer) {
        glDeleteBuffers(1,&TIS.vertexBuffer);
        TIS.vertexBuffer = 0;
    }
    if (TIS.normalBuffer) {
        glDeleteBuffers(1,&TIS.normalBuffer);
        TIS.normalBuffer = 0;
    }
    if (TIS.elementBuffer) {
        glDeleteBuffers(1,&TIS.elementBuffer);
        TIS.elementBuffer = 0;
    }
    if (TIS.programId) {
        glDeleteProgram(TIS.programId);TIS.programId=0;
    }
}

static void AddMeshVertsAndInds(float* totVerts,const int MAX_TOTAL_VERTS,int* numTotVerts,unsigned short* totInds,const int MAX_TOTAL_INDS,int* numTotInds,
                                const float* verts,int numVerts,const unsigned short* inds,int numInds,TeapotMeshEnum meshId) {
    int i,i3;
    float* pTotVerts = &totVerts[(*numTotVerts)*3];
    unsigned short* pTotInds = &totInds[*numTotInds];
    if (*numTotVerts+numVerts>MAX_TOTAL_VERTS) {
        fprintf(stderr,"Error: MAX_TOTAL_VERTS must be at least: %d\n",*numTotVerts+numVerts);
        exit(0);
    }
    if (*numTotInds+numInds>MAX_TOTAL_INDS) {
        fprintf(stderr,"Error: MAX_TOTAL_INDS must be at least: %d\n",*numTotInds+numInds);
        exit(0);
    }
    TIS.startInds[meshId] = *numTotInds;
    TIS.numInds[meshId] = numInds;

    // HERE we can do stuff on verts/inds---------------------------------------
    for (i=0;i<numVerts;i++) {
        i3=i*3;
        pTotVerts[0] = verts[i3];
        pTotVerts[1] = verts[i3+1];
#       ifndef  TEAPOT_INVERT_MESHES_Z_AXIS
        pTotVerts[2] = verts[i3+2];
#       else //TEAPOT_INVERT_MESHES_Z_AXIS
        pTotVerts[2] = -verts[i3+2];
#       endif //TEAPOT_INVERT_MESHES_Z_AXIS

        pTotVerts+=3;
    }

     for (i=0;i<numInds;i+=3) {
        pTotInds[0] = inds[i]+(*numTotVerts);
#       ifndef  TEAPOT_INVERT_MESHES_Z_AXIS
        pTotInds[1] = inds[i+1]+(*numTotVerts);
        pTotInds[2] = inds[i+2]+(*numTotVerts);
#       else //TEAPOT_INVERT_MESHES_Z_AXIS
        pTotInds[2] = inds[i+1]+(*numTotVerts);
        pTotInds[1] = inds[i+2]+(*numTotVerts);
#       endif //TEAPOT_INVERT_MESHES_Z_AXIS
        pTotInds+=3;
    }
    //---------------------------------------------------------------------------

    GetAabbHalfExtentsAndCenter(&totVerts[(*numTotVerts)*3],numVerts,&TIS.halfExtents[meshId][0],&TIS.centerPoint[meshId][0]);

    /*fprintf(stderr,"%d) centerPoint:%1.2f,%1.2f,%1.2f   halfExtents:%1.2f,%1.2f,%1.2f\n",meshId,
            TIS.centerPoint[meshId][0],TIS.centerPoint[meshId][1],TIS.centerPoint[meshId][2],
            TIS.halfExtents[meshId][0],TIS.halfExtents[meshId][1],TIS.halfExtents[meshId][2]
            );*/

#   ifdef TEAPOT_CENTER_MESHES_ON_FLOOR
    if (meshId<=TEAPOT_MESH_SPHERE2)    {
        pTotVerts = &totVerts[(*numTotVerts)*3];
        for (i=1;i<numVerts*3;i+=3) {
            pTotVerts[i] += TIS.halfExtents[meshId][1];
        }
    }
    TIS.centerPoint[meshId][1] = 0;
#   endif //TEAPOT_CENTER_MESHES_ON_FLOOR

    *numTotVerts+=numVerts;
    *numTotInds+=numInds;
}

inline void Teapot_GetMeshCenter(TeapotMeshEnum meshId,float center[3]) {
    center[0] = TIS.centerPoint[meshId][0];center[1] = TIS.centerPoint[meshId][1];center[2] = TIS.centerPoint[meshId][2];
}
inline void Teapot_GetMeshHalfAabb(TeapotMeshEnum meshId,float halfAabb[3]) {
    halfAabb[0] = TIS.halfExtents[meshId][0];halfAabb[1] = TIS.halfExtents[meshId][1];halfAabb[2] = TIS.halfExtents[meshId][2];
}


void Teapot_Init(void) {
    int i,j;
    TIS.vertexBuffer = TIS.normalBuffer = TIS.elementBuffer = 0;
    for (i=0;i<TEAPOT_MESH_COUNT;i++) {
        TIS.startInds[i] = TIS.numInds[i] = 0;
        TIS.halfExtents[i][0]=TIS.halfExtents[i][1]=TIS.halfExtents[i][2]=0;
        TIS.centerPoint[i][0]=TIS.centerPoint[i][1]=TIS.centerPoint[i][2]=0;
    }

    TIS.programId =  Teapot_LoadShaderProgramFromSource(*TeapotVS,*TeapotFS);
    if (!TIS.programId) return;

    TIS.aLoc_vertex = glGetAttribLocation(TIS.programId, "a_vertex");
    TIS.aLoc_normal = glGetAttribLocation(TIS.programId, "a_normal");
    TIS.uLoc_mvMatrix = glGetUniformLocation(TIS.programId,"u_mvMatrix");
    TIS.uLoc_pMatrix = glGetUniformLocation(TIS.programId,"u_pMatrix");
    TIS.uLoc_scaling = glGetUniformLocation(TIS.programId,"u_scaling");
    TIS.uLoc_lightVector = glGetUniformLocation(TIS.programId,"u_lightVector");
    TIS.uLoc_color = glGetUniformLocation(TIS.programId,"u_color");
    TIS.uLoc_colorSpecular = glGetUniformLocation(TIS.programId,"u_colorSpecular");

    /*
    if (TIS.aLoc_vertex<0) printf("Error: TIS.aLoc_vertex not found\n");
    //else printf("TIS.aLoc_vertex = %d\n",TIS.aLoc_vertex);
    if (TIS.aLoc_normal<0) printf("Error: TIS.aLoc_normal not found\n");
    //else printf("TIS.aLoc_normal = %d\n",TIS.aLoc_normal);
    if (TIS.uLoc_mvMatrix<0) printf("Error: TIS.uLoc_mvMatrix not found\n");
    if (TIS.uLoc_pMatrix<0) printf("Error: TIS.uLoc_pMatrix not found\n");
    if (TIS.uLoc_scaling<0) printf("Error: TIS.uLoc_scaling not found\n");
    if (TIS.uLoc_lightVector<0) printf("Error: TIS.uLoc_lightVector not found\n");
    if (TIS.uLoc_color<0) printf("Error: TIS.uLoc_color not found\n");
    if (TIS.uLoc_colorSpecular<0) printf("Error: TIS.uLoc_colorSpecular not found\n");
    */

    {
        const float one[4]={1.f,1.f,1.f,1.f};
        const float pMatrix[16] = {1.3580,0.0000,0.0000,0.0000,0.0000,2.4142,0.0000,0.0000,0.0000,0.0000,-1.0004,-1.0000,0.0000,0.0000,-0.2000,0.0000};
        const float vMatrix[16] = {1.0000,0.0000,0.0000,0.0000,0.0000,0.8944,0.4472,0.0000,0.0000,-0.4472,0.8944,0.0000,-0.0000,0.0000,-22.3607,1.0000};
        Teapot_SetProjectionMatrix(pMatrix);    // m4_perspective(45.0f, 16.f/9.f,0.1f,500.f);
        Teapot_SetViewMatrixAndLightDirection(vMatrix,one);   // m4_look_at(0,10,20,0,0,0,0,1,0);
        glUseProgram(TIS.programId);
        Teapot_SetScaling(1,1,1);
        Teapot_SetColor(1,1,1,1);
#       ifdef TEAPOT_SHADER_SPECULAR
        Teapot_SetColorSpecular(0.8,0.8,0.8,20.0);
#       endif //TEAPOT_SHADER_SPECULAR
        glUseProgram(0);
    }

    {
        // numTotVerts = 1672 numTotInds = 9300
        const unsigned MAX_TOTAL_VERTS = 1672;//10000;
        const unsigned MAX_TOTAL_INDS = 9300;//65535;

        float totVerts[MAX_TOTAL_VERTS*3];
        float norms[MAX_TOTAL_VERTS*3];
        unsigned short totInds[MAX_TOTAL_INDS];

        int numTotVerts = 0;
        int numTotInds = 0;

        for (i=0;i<TEAPOT_MESH_COUNT;i++)    {
            switch (i)  {
            case TEAPOT_MESH_TEAPOT: {
#               ifndef TEAPOT_NO_MESH_TEAPOT
                {


                    const float verts[] = {
                        0.057040,0.496550,-0.074009,0.000000,0.499902,-0.074009,0.042806,0.496550,-0.031203,-0.057040,0.496550,-0.074009,-0.042806,0.496550,-0.031203,0.000000,0.496550,-0.131049,-0.042806,0.496550,-0.116815,0.000000,0.496550,-0.016969,
                        0.000000,0.250700,0.410034,0.187337,0.275580,0.366292,0.189988,0.250700,0.372519,0.338877,0.275580,0.264867,0.343671,0.250700,0.269662,0.446530,0.250700,0.115979,0.440304,0.275580,0.113328,0.484044,0.250700,-0.074010,
                        0.000000,0.283874,0.411113,0.000000,0.275580,0.403281,0.190412,0.283874,0.373518,0.447531,0.283874,0.116403,0.485129,0.283874,-0.074009,0.477295,0.275580,-0.074010,0.000000,0.275580,0.427050,0.196667,0.275580,0.388220,
                        0.344440,0.283874,0.270429,0.462233,0.275580,0.122658,0.501065,0.275580,-0.074009,0.000000,0.250700,0.444610,0.203557,0.250700,0.404416,0.355755,0.275580,0.281743,0.478425,0.250700,0.129547,0.518619,0.250700,-0.074010,
                        0.440304,0.275580,-0.261345,0.446530,0.250700,-0.263996,0.338877,0.275580,-0.412885,0.343671,0.250700,-0.417680,0.189988,0.250700,-0.520540,0.187337,0.275580,-0.514310,0.000000,0.250700,-0.558052,0.447531,0.283874,-0.264421,
                        0.190412,0.283874,-0.521536,0.000000,0.283874,-0.559132,0.000000,0.275580,-0.551299,0.462233,0.275580,-0.270676,0.344440,0.283874,-0.418447,0.196667,0.275580,-0.536239,0.000000,0.275580,-0.575070,0.478425,0.250700,-0.277568,
                        0.355755,0.275580,-0.429763,0.203557,0.250700,-0.552434,0.000000,0.250700,-0.592626,-0.187336,0.275580,-0.514310,-0.189988,0.250700,-0.520539,-0.338876,0.275580,-0.412886,-0.343671,0.250700,-0.417681,-0.446530,0.250700,-0.263996,
                        -0.440301,0.275580,-0.261345,-0.484044,0.250700,-0.074009,-0.190410,0.283874,-0.521536,-0.447526,0.283874,-0.264421,-0.485122,0.283874,-0.074009,-0.477292,0.275580,-0.074009,-0.196665,0.275580,-0.536239,-0.344437,0.283874,-0.418449,
                        -0.462226,0.275580,-0.270676,-0.501058,0.275580,-0.074009,-0.203557,0.250700,-0.552434,-0.355751,0.275580,-0.429763,-0.478425,0.250700,-0.277568,-0.518619,0.250700,-0.074009,-0.440301,0.275580,0.113328,-0.446530,0.250700,0.115979,
                        -0.338876,0.275580,0.264867,-0.343671,0.250700,0.269662,-0.189988,0.250700,0.372519,-0.187336,0.275580,0.366292,-0.447526,0.283874,0.116403,-0.190410,0.283874,0.373518,-0.462226,0.275580,0.122658,-0.344437,0.283874,0.270429,
                        -0.196665,0.275580,0.388220,-0.478425,0.250700,0.129548,-0.355751,0.275580,0.281743,-0.203557,0.250700,0.404416,0.000000,0.114399,0.508084,0.228473,0.114399,0.462973,0.368219,0.250700,0.294209,0.536983,0.114399,0.154464,
                        0.582094,0.114399,-0.074009,0.000000,-0.018991,0.563460,0.250207,-0.018991,0.514055,0.413288,0.114399,0.339279,0.588064,-0.018991,0.176198,0.637469,-0.018991,-0.074010,0.000000,-0.147190,0.602624,0.265579,-0.147190,0.550184,
                        0.452603,-0.018991,0.378594,0.624195,-0.147190,0.191571,0.676635,-0.147190,-0.074010,0.000000,-0.267919,0.617480,0.271410,-0.267919,0.563890,0.480410,-0.147190,0.406400,0.637900,-0.267919,0.197401,0.691490,-0.267919,-0.074010,
                        0.536983,0.114399,-0.302483,0.368219,0.250700,-0.442230,0.228473,0.114399,-0.610992,0.000000,0.114399,-0.656103,0.588064,-0.018991,-0.324215,0.413288,0.114399,-0.487296,0.250207,-0.018991,-0.662075,0.000000,-0.018991,-0.711478,
                        0.624195,-0.147190,-0.339588,0.452603,-0.018991,-0.526614,0.265579,-0.147190,-0.698204,0.000000,-0.147190,-0.750645,0.637900,-0.267918,-0.345420,0.480410,-0.147190,-0.554421,0.271410,-0.267918,-0.711911,0.000000,-0.267918,-0.765501,
                        -0.228473,0.114399,-0.610991,-0.368219,0.250700,-0.442230,-0.536983,0.114399,-0.302482,-0.582094,0.114399,-0.074009,-0.250207,-0.018991,-0.662075,-0.413288,0.114399,-0.487297,-0.588064,-0.018991,-0.324215,-0.637469,-0.018991,-0.074009,
                        -0.265579,-0.147190,-0.698204,-0.452603,-0.018991,-0.526614,-0.624195,-0.147190,-0.339588,-0.676635,-0.147190,-0.074009,-0.271410,-0.267918,-0.711911,-0.480410,-0.147190,-0.554421,-0.637900,-0.267918,-0.345419,-0.691490,-0.267919,-0.074009,
                        -0.536983,0.114399,0.154464,-0.368219,0.250700,0.294210,-0.228473,0.114399,0.462973,-0.588064,-0.018991,0.176198,-0.413288,0.114399,0.339277,-0.250207,-0.018991,0.514055,-0.624195,-0.147190,0.191571,-0.452603,-0.018991,0.378594,
                        -0.265579,-0.147190,0.550184,-0.637900,-0.267919,0.197401,-0.480410,-0.147190,0.406401,-0.271410,-0.267919,0.563890,0.260808,-0.370789,0.538971,0.471781,-0.370789,0.397772,0.490958,-0.267919,0.416948,0.612983,-0.370789,0.186797,
                        0.000000,-0.370789,0.590469,0.237484,-0.446657,0.484152,0.429588,-0.446657,0.355578,0.558163,-0.446657,0.163476,0.664480,-0.370789,-0.074010,0.000000,-0.446657,0.531045,0.214158,-0.499902,0.429333,0.387397,-0.499902,0.313386,
                        0.503343,-0.499902,0.140150,0.605054,-0.446657,-0.074009,0.000000,-0.499902,0.471620,0.000000,-0.499902,-0.074010,0.545629,-0.499902,-0.074009,0.612983,-0.370789,-0.334817,0.471781,-0.370788,-0.545791,0.490958,-0.267918,-0.564967,
                        0.260808,-0.370788,-0.686992,0.558163,-0.446657,-0.311493,0.429588,-0.446657,-0.503599,0.237484,-0.446657,-0.632173,0.000000,-0.370788,-0.738490,0.503343,-0.499902,-0.288169,0.387397,-0.499902,-0.461407,0.214158,-0.499902,-0.577353,
                        0.000000,-0.446657,-0.679063,0.000000,-0.499902,-0.619640,-0.260808,-0.370788,-0.686991,-0.471781,-0.370788,-0.545790,-0.490958,-0.267918,-0.564967,-0.612983,-0.370789,-0.334817,-0.237484,-0.446657,-0.632172,-0.429588,-0.446657,-0.503599,
                        -0.558163,-0.446657,-0.311493,-0.664480,-0.370789,-0.074009,-0.214158,-0.499902,-0.577351,-0.387397,-0.499902,-0.461407,-0.503343,-0.499902,-0.288169,-0.605054,-0.446657,-0.074009,-0.545629,-0.499902,-0.074009,-0.612983,-0.370789,0.186799,
                        -0.471781,-0.370789,0.397772,-0.490958,-0.267919,0.416948,-0.260808,-0.370789,0.538973,-0.558163,-0.446657,0.163476,-0.429588,-0.446657,0.355578,-0.237484,-0.446657,0.484152,-0.503343,-0.499902,0.140150,-0.387397,-0.499902,0.313386,
                        -0.214158,-0.499902,0.429333,0.000000,0.120708,-0.627202,0.058345,0.131388,-0.793542,0.058345,0.132790,-0.621799,0.077794,0.157551,-0.804127,0.077794,0.159368,-0.609914,0.058345,0.183714,-0.814712,0.058345,0.185946,-0.598031,
                        0.000000,0.195607,-0.819523,0.000000,0.198028,-0.592626,0.000000,0.111001,-0.908121,0.000000,0.119495,-0.788730,0.058345,0.121573,-0.919601,0.077794,0.144829,-0.944857,0.058345,0.168085,-0.970112,0.000000,0.087947,-0.982133,
                        0.058345,0.094931,-0.997242,0.077794,0.110297,-1.030483,0.058345,0.125662,-1.063723,0.000000,0.178655,-0.981592,0.000000,0.043050,-1.007523,0.058345,0.043050,-1.023730,0.077794,0.043050,-1.059384,0.058345,0.043050,-1.095039,
                        0.000000,0.132647,-1.078832,-0.058345,0.185946,-0.598031,-0.058345,0.183714,-0.814712,-0.077794,0.159368,-0.609914,-0.077794,0.157551,-0.804127,-0.058345,0.132790,-0.621799,-0.058345,0.131388,-0.793542,-0.058345,0.168085,-0.970112,
                        -0.077794,0.144829,-0.944857,-0.058345,0.121573,-0.919601,-0.058345,0.125662,-1.063723,-0.077794,0.110297,-1.030483,-0.058345,0.094931,-0.997242,-0.058345,0.043050,-1.095039,-0.077794,0.043050,-1.059384,-0.058345,0.043050,-1.023730,
                        0.000000,-0.027854,-0.994017,0.058345,-0.034629,-1.008747,0.077794,-0.049531,-1.041151,0.058345,-0.064436,-1.073557,0.000000,0.043050,-1.111246,0.000000,-0.112939,-0.951339,0.058345,-0.123294,-0.961806,0.077794,-0.146072,-0.984833,
                        0.058345,-0.168850,-1.007859,0.000000,-0.071209,-1.088285,0.000000,-0.197773,-0.876247,0.058345,-0.210578,-0.879919,0.077794,-0.238750,-0.887997,0.058345,-0.266920,-0.896075,0.000000,-0.179203,-1.018327,0.058345,-0.284114,-0.743002,
                        0.077794,-0.319747,-0.731119,0.058345,-0.355378,-0.719234,0.000000,-0.279725,-0.899747,-0.058345,-0.064436,-1.073557,-0.077794,-0.049531,-1.041151,-0.058345,-0.034629,-1.008747,-0.058345,-0.168850,-1.007859,-0.077794,-0.146072,-0.984833,
                        -0.058345,-0.123294,-0.961806,-0.058345,-0.266920,-0.896075,-0.077794,-0.238750,-0.887997,-0.058345,-0.210578,-0.879919,-0.058345,-0.355378,-0.719234,-0.077794,-0.319747,-0.731117,-0.058345,-0.284114,-0.743001,0.000000,-0.265781,-0.751779,
                        0.000000,-0.086943,0.513757,0.115900,-0.081769,0.692272,0.128358,-0.131416,0.513757,0.154533,-0.154384,0.720126,0.171145,-0.229259,0.513757,0.115900,-0.226999,0.747979,0.128358,-0.327100,0.513757,0.000000,-0.260004,0.760646,
                        0.000000,-0.371574,0.513757,0.000000,-0.048762,0.679605,0.088490,0.023758,0.767659,0.117986,-0.018590,0.803313,0.088490,-0.060940,0.838979,0.000000,-0.080189,0.855188,0.000000,0.043007,0.751450,0.061079,0.147227,0.810641,
                        0.081440,0.131858,0.854095,0.061079,0.116488,0.897548,0.000000,0.109502,0.917311,0.000000,0.250700,0.859502,0.000000,0.154213,0.790893,0.048620,0.250700,0.891919,0.064827,0.250700,0.963226,0.048620,0.250700,1.034532,
                        -0.128358,-0.327100,0.513757,-0.115900,-0.226999,0.747979,-0.171145,-0.229259,0.513757,-0.154533,-0.154384,0.720126,-0.128358,-0.131416,0.513757,-0.115900,-0.081769,0.692272,-0.088490,-0.060940,0.838979,-0.117986,-0.018590,0.803313,
                        -0.088490,0.023758,0.767659,-0.061079,0.116488,0.897548,-0.081440,0.131858,0.854095,-0.061079,0.147227,0.810641,-0.048620,0.250700,1.034532,-0.064827,0.250700,0.963226,-0.048620,0.250700,0.891919,0.000000,0.264755,0.884354,
                        0.045581,0.265503,0.919067,0.060775,0.267151,0.995435,0.045581,0.268800,1.071804,0.000000,0.250700,1.066949,0.000000,0.269440,0.902720,0.038897,0.270646,0.935303,0.051861,0.273301,1.006984,0.038897,0.275955,1.078663,
                        0.000000,0.269549,1.106517,0.032211,0.265816,0.935787,0.042949,0.268149,0.996652,0.032211,0.270483,1.057518,0.000000,0.271544,1.085177,0.000000,0.277162,1.111246,0.000000,0.264755,0.908128,0.000000,0.112402,0.864452,
                        -0.045581,0.268800,1.071804,-0.060775,0.267151,0.995435,-0.045581,0.265503,0.919067,-0.038897,0.275955,1.078663,-0.051861,0.273301,1.006984,-0.038897,0.270646,0.935303,-0.032211,0.270483,1.057518,-0.042949,0.268149,0.996652,
                        -0.032211,0.265816,0.935787,0.000000,0.492338,0.043759,0.044214,0.451085,0.029684,0.046348,0.492338,0.034674,0.079879,0.451085,0.005870,0.083726,0.492338,0.009717,0.108685,0.492338,-0.027661,0.103696,0.451085,-0.029795,
                        0.117770,0.492338,-0.074009,0.000000,0.451085,0.038358,0.026758,0.400222,-0.011202,0.048364,0.400222,-0.025643,0.062807,0.400222,-0.047251,0.112367,0.451085,-0.074009,0.000000,0.400222,-0.005940,0.027141,0.354356,-0.010219,
                        0.049096,0.354356,-0.024913,0.063789,0.354356,-0.046868,0.068069,0.400222,-0.074009,0.103696,0.451085,-0.118223,0.108685,0.492338,-0.120356,0.079879,0.451085,-0.153887,0.083726,0.492338,-0.157735,0.046348,0.492338,-0.182692,
                        0.044214,0.451085,-0.177705,0.000000,0.492338,-0.191779,0.062807,0.400222,-0.100767,0.048364,0.400222,-0.122375,0.026758,0.400222,-0.136816,0.000000,0.451085,-0.186376,0.063789,0.354356,-0.101150,0.049096,0.354356,-0.123105,
                        0.027141,0.354356,-0.137798,0.000000,0.400222,-0.142078,-0.044214,0.451085,-0.177705,-0.046348,0.492338,-0.182692,-0.079879,0.451085,-0.153887,-0.083726,0.492338,-0.157735,-0.108685,0.492338,-0.120356,-0.103696,0.451085,-0.118223,
                        -0.117770,0.492338,-0.074009,-0.026758,0.400222,-0.136816,-0.048364,0.400222,-0.122375,-0.062807,0.400222,-0.100767,-0.112367,0.451085,-0.074009,-0.027141,0.354356,-0.137798,-0.049096,0.354356,-0.123105,-0.063789,0.354356,-0.101150,
                        -0.068067,0.400222,-0.074009,-0.103696,0.451085,-0.029795,-0.108685,0.492338,-0.027661,-0.079879,0.451085,0.005870,-0.083726,0.492338,0.009717,-0.046348,0.492338,0.034674,-0.044214,0.451085,0.029684,-0.062807,0.400222,-0.047251,
                        -0.048364,0.400222,-0.025643,-0.026758,0.400222,-0.011202,-0.063789,0.354356,-0.046868,-0.049096,0.354356,-0.024913,-0.027141,0.354356,-0.010219,0.000000,0.323109,0.083737,0.000000,0.354356,-0.004860,0.061915,0.323109,0.071513,
                        0.145520,0.323109,-0.012093,0.157746,0.323109,-0.074009,0.069148,0.354356,-0.074009,0.000000,0.301895,0.211231,0.111957,0.301895,0.189126,0.112000,0.323109,0.037991,0.263135,0.301895,0.037948,0.285240,0.301895,-0.074009,
                        0.000000,0.280998,0.325758,0.156909,0.280998,0.294775,0.202521,0.301895,0.128511,0.368787,0.280998,0.082900,0.399768,0.280998,-0.074010,0.000000,0.250700,0.375459,0.176415,0.250700,0.340624,0.283835,0.280998,0.209826,
                        0.414636,0.250700,0.102407,0.449471,0.250700,-0.074010,0.145520,0.323109,-0.135925,0.061915,0.323109,-0.219531,0.000000,0.323109,-0.231756,0.000000,0.354356,-0.143158,0.263135,0.301895,-0.185966,0.112000,0.323109,-0.186008,
                        0.111957,0.301895,-0.337143,0.000000,0.301895,-0.359249,0.368787,0.280998,-0.230920,0.202521,0.301895,-0.276530,0.156909,0.280998,-0.442796,0.000000,0.280998,-0.473776,0.414636,0.250700,-0.250426,0.283835,0.280998,-0.357844,
                        0.176415,0.250700,-0.488646,0.000000,0.250700,-0.523477,-0.061915,0.323109,-0.219531,-0.145520,0.323109,-0.135925,-0.157746,0.323109,-0.074009,-0.069148,0.354356,-0.074009,-0.111957,0.301895,-0.337143,-0.112000,0.323109,-0.186008,
                        -0.263135,0.301895,-0.185966,-0.285240,0.301895,-0.074009,-0.156909,0.280998,-0.442796,-0.202521,0.301895,-0.276530,-0.368787,0.280998,-0.230920,-0.399768,0.280998,-0.074009,-0.176415,0.250700,-0.488645,-0.283835,0.280998,-0.357844,
                        -0.414636,0.250700,-0.250426,-0.449471,0.250700,-0.074009,-0.145520,0.323109,-0.012093,-0.061915,0.323109,0.071513,-0.263135,0.301895,0.037948,-0.112000,0.323109,0.037991,-0.111957,0.301895,0.189126,-0.368787,0.280998,0.082900,
                        -0.202521,0.301895,0.128511,-0.156909,0.280998,0.294777,-0.414636,0.250700,0.102407,-0.283835,0.280998,0.209826,-0.176415,0.250700,0.340625,0.042806,0.496550,-0.116815,-0.319123,0.250700,-0.393132,-0.319123,0.250700,0.245114,
                        0.319123,0.250700,0.245114,0.319123,0.250700,-0.393132,0.000000,-0.371574,-0.713830};

                    const unsigned short inds[] = {
                        0,1,2,1,3,4,1,5,6,7,1,4,8,9,10,10,11,12,11,13,12,14,15,13,16,9,17,18,11,9,11,19,14,14,20,21,22,18,16,23,24,18,24,25,19,19,26,20,27,23,22,28,29,23,29,30,25,25,31,26,15,32,33,
                        33,34,35,34,36,35,37,38,36,20,32,21,39,34,32,34,40,37,37,41,42,26,39,20,43,44,39,44,45,40,40,46,41,31,43,26,47,48,43,48,49,45,45,50,46,38,51,52,52,53,54,53,55,54,56,57,55,41,51,42,58,53,51,
                        53,59,56,56,60,61,46,58,41,62,63,58,63,64,59,59,65,60,50,62,46,66,67,62,67,68,64,64,69,65,57,70,71,71,72,73,72,74,73,75,8,74,60,70,61,76,72,70,72,77,75,75,16,17,65,76,60,78,79,76,79,80,77,
                        77,22,16,69,78,65,81,82,78,82,83,80,80,27,22,84,28,27,85,86,28,86,87,30,30,88,31,89,85,84,90,91,85,91,92,87,87,93,88,94,90,89,95,96,90,96,97,92,92,98,93,99,95,94,100,101,95,101,102,97,97,103,98,
                        88,47,31,104,105,47,105,106,49,49,107,50,93,104,88,108,109,104,109,110,106,106,111,107,98,108,93,112,113,108,113,114,110,110,115,111,103,112,98,116,117,112,117,118,114,114,119,115,107,66,50,120,121,66,121,122,68,68,123,69,111,120,107,
                        124,125,120,125,126,122,122,127,123,115,124,111,128,129,124,129,130,126,126,131,127,119,128,115,132,133,128,133,134,130,130,135,131,123,81,69,136,137,81,137,138,83,83,84,27,127,136,123,139,140,136,140,141,138,138,89,84,131,139,127,142,143,139,
                        143,144,141,141,94,89,135,142,131,145,146,142,146,147,144,144,99,94,99,148,100,100,149,150,149,102,150,151,103,102,152,153,148,148,154,149,154,151,149,155,156,151,157,158,153,153,159,154,159,155,154,160,161,155,162,163,158,158,163,159,159,163,160,
                        160,163,164,103,165,116,116,166,167,166,118,167,168,119,118,156,169,165,165,170,166,170,168,166,171,172,168,161,173,169,169,174,170,174,171,170,175,176,171,164,163,173,173,163,174,174,163,175,175,163,177,119,178,132,132,179,180,179,134,180,181,135,134,
                        172,182,178,178,183,179,183,181,179,184,185,181,176,186,182,182,187,183,187,184,183,188,189,184,177,163,186,186,163,187,187,163,188,188,163,190,135,191,145,145,192,193,192,147,193,194,99,147,185,195,191,191,196,192,196,194,192,197,152,194,189,198,195,
                        195,199,196,199,197,196,200,157,197,190,163,198,198,163,199,199,163,200,200,163,162,201,202,203,203,204,205,205,206,207,207,208,209,210,202,211,212,204,202,213,206,204,214,208,206,215,212,210,216,213,212,217,214,213,218,219,214,220,216,215,221,217,216,
                        222,218,217,223,224,218,208,225,209,226,227,225,228,229,227,230,201,229,208,231,226,226,232,228,228,233,230,230,210,211,219,234,231,231,235,232,232,236,233,233,215,210,224,237,234,234,238,235,235,239,236,236,220,215,240,221,220,241,222,221,242,223,222,
                        243,244,223,245,241,240,246,242,241,247,243,242,248,249,243,250,246,245,251,247,246,252,248,247,253,254,248,255,252,251,256,253,252,257,258,253,244,259,237,237,260,238,238,261,239,239,240,220,249,262,259,259,263,260,260,264,261,261,245,240,254,265,262,
                        262,266,263,263,267,264,264,250,245,258,268,265,265,269,266,266,270,267,267,271,250,272,273,274,274,275,276,276,277,278,278,279,280,281,282,273,273,283,275,275,284,277,277,285,279,286,287,282,282,288,283,283,289,284,284,290,285,291,287,292,293,288,287,
                        294,289,288,295,290,289,279,296,280,297,298,296,299,300,298,301,272,300,285,297,279,302,299,297,303,301,299,304,281,301,290,302,285,305,303,302,306,304,303,307,286,304,290,308,305,305,309,306,306,310,307,307,291,292,311,293,291,312,294,293,313,295,294,
                        314,315,295,316,312,311,317,313,312,318,314,313,319,320,314,316,321,317,317,322,318,318,323,319,319,324,325,326,327,321,321,327,322,322,327,323,323,327,324,315,328,308,308,329,309,309,330,310,310,311,291,320,331,328,328,332,329,329,333,330,330,316,311,
                        324,331,325,334,332,331,335,333,332,336,316,333,324,327,334,334,327,335,335,327,336,336,327,326,337,338,339,339,340,341,340,342,341,343,344,342,345,346,338,338,347,340,347,343,340,348,349,343,350,351,346,346,352,347,352,348,347,353,354,348,344,355,356,
                        356,357,358,357,359,358,360,361,359,349,362,355,355,363,357,363,360,357,364,365,360,354,366,362,362,367,363,367,364,363,368,369,364,361,370,371,371,372,373,372,374,373,375,376,374,365,377,370,370,378,372,378,375,372,379,380,375,369,381,377,377,382,378,
                        382,379,378,383,384,379,376,385,386,386,387,388,387,389,388,390,337,389,380,391,385,385,392,387,392,390,387,393,345,390,384,394,391,391,395,392,395,393,392,396,350,393,397,351,398,399,352,351,352,400,353,353,401,402,403,399,397,404,405,399,405,406,400,
                        400,407,401,408,404,403,409,410,404,410,411,406,406,412,407,413,409,408,414,415,409,415,416,411,411,417,412,401,366,402,418,367,366,367,419,368,368,420,421,407,418,401,422,423,418,423,424,419,419,425,420,412,422,407,426,427,422,427,428,424,424,429,425,
                        417,426,412,430,431,426,431,432,428,428,433,429,420,381,421,434,382,381,382,435,383,383,436,437,425,434,420,438,439,434,439,440,435,435,441,436,429,438,425,442,443,438,443,444,440,440,445,441,433,442,429,446,447,442,447,448,444,444,449,445,436,394,437,
                        450,395,394,395,451,396,396,397,398,441,450,436,452,453,450,453,454,451,451,403,397,445,452,441,455,456,452,456,457,454,454,408,403,449,455,445,458,459,455,459,460,457,457,413,408,0,356,461,359,5,461,342,0,2,7,339,2,389,7,4,3,386,4,
                        374,3,6,5,371,6,2,341,342,461,358,359,6,373,374,4,388,389,271,251,250,38,446,433,52,462,446,462,55,448,448,57,449,57,458,449,71,463,458,463,74,460,460,8,413,8,414,413,10,464,414,464,13,416,416,15,417,15,430,417,33,465,430,
                        465,36,432,432,38,433,0,461,1,1,6,3,1,461,5,7,2,1,8,17,9,10,9,11,11,14,13,14,21,15,16,18,9,18,24,11,11,24,19,14,19,20,22,23,18,23,29,24,24,29,25,19,25,26,27,28,23,28,86,29,29,86,30,
                        25,30,31,15,21,32,33,32,34,34,37,36,37,42,38,20,39,32,39,44,34,34,44,40,37,40,41,26,43,39,43,48,44,44,48,45,40,45,46,31,47,43,47,105,48,48,105,49,45,49,50,38,42,51,52,51,53,53,56,55,56,61,57,
                        41,58,51,58,63,53,53,63,59,56,59,60,46,62,58,62,67,63,63,67,64,59,64,65,50,66,62,66,121,67,67,121,68,64,68,69,57,61,70,71,70,72,72,75,74,75,17,8,60,76,70,76,79,72,72,79,77,75,77,16,65,78,76,
                        78,82,79,79,82,80,77,80,22,69,81,78,81,137,82,82,137,83,80,83,27,84,85,28,85,91,86,86,91,87,30,87,88,89,90,85,90,96,91,91,96,92,87,92,93,94,95,90,95,101,96,96,101,97,92,97,98,99,100,95,100,150,101,
                        101,150,102,97,102,103,88,104,47,104,109,105,105,109,106,49,106,107,93,108,104,108,113,109,109,113,110,106,110,111,98,112,108,112,117,113,113,117,114,110,114,115,103,116,112,116,167,117,117,167,118,114,118,119,107,120,66,120,125,121,121,125,122,
                        68,122,123,111,124,120,124,129,125,125,129,126,122,126,127,115,128,124,128,133,129,129,133,130,126,130,131,119,132,128,132,180,133,133,180,134,130,134,135,123,136,81,136,140,137,137,140,138,83,138,84,127,139,136,139,143,140,140,143,141,138,141,89,
                        131,142,139,142,146,143,143,146,144,141,144,94,135,145,142,145,193,146,146,193,147,144,147,99,99,152,148,100,148,149,149,151,102,151,156,103,152,157,153,148,153,154,154,155,151,155,161,156,157,162,158,153,158,159,159,160,155,160,164,161,103,156,165,
                        116,165,166,166,168,118,168,172,119,156,161,169,165,169,170,170,171,168,171,176,172,161,164,173,169,173,174,174,175,171,175,177,176,119,172,178,132,178,179,179,181,134,181,185,135,172,176,182,178,182,183,183,184,181,184,189,185,176,177,186,182,186,187,
                        187,188,184,188,190,189,135,185,191,145,191,192,192,194,147,194,152,99,185,189,195,191,195,196,196,197,194,197,157,152,189,190,198,195,198,199,199,200,197,200,162,157,201,211,202,203,202,204,205,204,206,207,206,208,210,212,202,212,213,204,213,214,206,
                        214,219,208,215,216,212,216,217,213,217,218,214,218,224,219,220,221,216,221,222,217,222,223,218,223,244,224,208,226,225,226,228,227,228,230,229,230,211,201,208,219,231,226,231,232,228,232,233,230,233,210,219,224,234,231,234,235,232,235,236,233,236,215,
                        224,244,237,234,237,238,235,238,239,236,239,220,240,241,221,241,242,222,242,243,223,243,249,244,245,246,241,246,247,242,247,248,243,248,254,249,250,251,246,251,252,247,252,253,248,253,258,254,255,256,252,256,257,253,257,466,258,244,249,259,237,259,260,
                        238,260,261,239,261,240,249,254,262,259,262,263,260,263,264,261,264,245,254,258,265,262,265,266,263,266,267,264,267,250,258,466,268,265,268,269,266,269,270,267,270,271,272,281,273,274,273,275,276,275,277,278,277,279,281,286,282,273,282,283,275,283,284,
                        277,284,285,286,292,287,282,287,288,283,288,289,284,289,290,291,293,287,293,294,288,294,295,289,295,315,290,279,297,296,297,299,298,299,301,300,301,281,272,285,302,297,302,303,299,303,304,301,304,286,281,290,305,302,305,306,303,306,307,304,307,292,286,
                        290,315,308,305,308,309,306,309,310,307,310,291,311,312,293,312,313,294,313,314,295,314,320,315,316,317,312,317,318,313,318,319,314,319,325,320,316,326,321,317,321,322,318,322,323,319,323,324,315,320,328,308,328,329,309,329,330,310,330,311,320,325,331,
                        328,331,332,329,332,333,330,333,316,324,334,331,334,335,332,335,336,333,336,326,316,337,345,338,339,338,340,340,343,342,343,349,344,345,350,346,338,346,347,347,348,343,348,354,349,350,398,351,346,351,352,352,353,348,353,402,354,344,349,355,356,355,357,
                        357,360,359,360,365,361,349,354,362,355,362,363,363,364,360,364,369,365,354,402,366,362,366,367,367,368,364,368,421,369,361,365,370,371,370,372,372,375,374,375,380,376,365,369,377,370,377,378,378,379,375,379,384,380,369,421,381,377,381,382,382,383,379,
                        383,437,384,376,380,385,386,385,387,387,390,389,390,345,337,380,384,391,385,391,392,392,393,390,393,350,345,384,437,394,391,394,395,395,396,393,396,398,350,397,399,351,399,405,352,352,405,400,353,400,401,403,404,399,404,410,405,405,410,406,400,406,407,
                        408,409,404,409,415,410,410,415,411,406,411,412,413,414,409,414,464,415,415,464,416,411,416,417,401,418,366,418,423,367,367,423,419,368,419,420,407,422,418,422,427,423,423,427,424,419,424,425,412,426,422,426,431,427,427,431,428,424,428,429,417,430,426,
                        430,465,431,431,465,432,428,432,433,420,434,381,434,439,382,382,439,435,383,435,436,425,438,434,438,443,439,439,443,440,435,440,441,429,442,438,442,447,443,443,447,444,440,444,445,433,446,442,446,462,447,447,462,448,444,448,449,436,450,394,450,453,395,
                        395,453,451,396,451,397,441,452,450,452,456,453,453,456,454,451,454,403,445,455,452,455,459,456,456,459,457,454,457,408,449,458,455,458,463,459,459,463,460,457,460,413,0,344,356,359,361,5,342,344,0,7,337,339,389,337,7,3,376,386,374,376,3,
                        5,361,371,2,339,341,461,356,358,6,371,373,4,386,388,271,255,251,38,52,446,52,54,462,462,54,55,448,55,57,57,71,458,71,73,463,463,73,74,460,74,8,8,10,414,10,12,464,464,12,13,416,13,15,15,33,430,33,35,465,465,35,36,
                        432,36,38};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_BUNNY: {
#               ifndef TEAPOT_NO_MESH_BUNNY
                {


                    const float verts[] = {
                        -0.005326,-0.147601,0.625450,-0.005546,-0.139797,0.663273,-0.060312,-0.097053,0.658934,0.049399,-0.097219,0.659606,-0.005563,-0.067678,0.683914,0.124974,0.032568,0.537897,0.118797,0.064318,0.519466,0.119200,0.058246,0.544451,
                        -0.134000,0.032960,0.536311,-0.113666,0.008720,0.555753,-0.120334,0.037640,0.562773,0.110999,0.037291,0.564190,0.104331,0.008391,0.557089,-0.128230,0.058620,0.542936,-0.118585,0.003714,0.539146,0.122726,0.015638,0.514850,
                        0.118861,0.050850,0.497373,-0.127502,0.064690,0.517958,-0.131521,0.016022,0.513293,0.109437,0.003370,0.540543,-0.127335,0.051222,0.495865,0.136776,0.292841,0.384904,0.130910,0.285500,0.342629,0.137174,0.325360,0.311767,
                        0.146967,0.352588,0.353097,-0.168932,0.414408,0.301237,-0.190199,0.434949,0.244993,-0.154896,0.355935,0.289227,-0.136779,0.285904,0.340990,-0.143141,0.293264,0.383189,-0.152761,0.353040,0.351262,-0.142543,0.325782,0.310054,
                        0.163935,0.413905,0.303276,0.149871,0.355475,0.291093,0.185951,0.434380,0.247296,0.145330,0.175559,0.427091,0.162452,0.267513,0.325697,-0.152564,0.176009,0.425267,-0.168166,0.268013,0.323672,-0.188663,0.394252,0.241677,
                        0.184333,0.393688,0.243960,0.180278,0.327873,0.268389,-0.185106,0.328425,0.266151,0.061550,0.153321,-0.530988,0.076663,0.176933,-0.572437,0.002538,0.212315,-0.569922,0.002236,0.177176,-0.529208,0.080589,0.084343,-0.678480,
                        0.031129,0.041548,-0.679584,0.062379,0.140834,-0.657305,0.003036,0.157284,-0.664987,0.002973,0.046359,-0.681928,-0.025224,0.041634,-0.679930,-0.056445,0.141014,-0.658033,-0.120976,-0.030224,-0.630149,-0.127903,0.029817,-0.584012,
                        -0.119430,0.032701,-0.657161,0.002554,-0.026922,-0.631542,-0.035336,-0.028018,-0.631098,-0.064204,0.036900,-0.677897,-0.074832,-0.029121,-0.630623,0.097432,0.108237,-0.598019,0.090001,0.113064,-0.545099,0.070066,0.036697,-0.677075,
                        0.079915,-0.029355,-0.629675,0.040432,-0.028132,-0.630634,-0.074564,0.084577,-0.679430,-0.085522,0.113329,-0.546173,-0.092319,0.108523,-0.599181,-0.071657,0.177157,-0.573346,0.125021,0.032332,-0.655664,0.126047,-0.030598,-0.628636,
                        0.132589,0.029424,-0.582416,-0.057125,0.153501,-0.531715,-0.281047,0.054272,-0.326657,-0.289679,0.075437,-0.212368,-0.184106,0.209299,-0.222753,-0.171339,0.186400,-0.322165,-0.164703,0.199932,-0.140366,-0.141192,0.159338,0.045313,
                        -0.001190,0.252155,0.048661,-0.000037,0.290949,-0.130016,-0.174526,0.138986,-0.454281,0.001191,0.264893,-0.337106,0.001862,0.222300,-0.457061,0.000730,0.296135,-0.254187,-0.245046,0.065927,-0.121983,-0.234104,0.053911,0.023500,
                        -0.237908,-0.066725,-0.482486,-0.152235,-0.126037,-0.633994,-0.115314,0.126627,0.123694,-0.001876,0.182151,0.143412,-0.177588,0.026660,0.144913,0.244173,0.065189,-0.118987,0.289939,0.074562,-0.208819,0.184905,0.208743,-0.220494,
                        0.164466,0.199435,-0.138349,0.241050,-0.067448,-0.479553,0.157061,-0.126503,-0.632100,0.177949,0.138453,-0.452124,0.250122,-0.177727,0.123656,0.196050,-0.199430,-0.000811,0.242635,-0.129938,0.083742,0.199003,-0.230305,-0.062507,
                        0.205738,-0.233363,-0.137763,0.257917,-0.108345,-0.126910,0.257603,-0.102724,-0.024105,0.232539,-0.433881,-0.339609,0.265654,-0.475224,-0.259346,0.199283,-0.466998,-0.425337,0.127566,-0.205535,0.333254,-0.003891,-0.259620,0.363483,
                        -0.003352,-0.295387,0.266658,0.127772,-0.237416,0.274235,0.062728,-0.328777,-0.050642,0.079827,-0.336149,-0.105039,0.000198,-0.352302,-0.327120,0.085926,-0.441323,-0.322447,0.051360,-0.327746,-0.172376,0.265290,-0.471302,-0.169849,
                        0.260760,-0.451076,-0.240370,0.233744,-0.407344,-0.163808,0.039899,-0.336571,-0.049210,-0.001117,-0.356544,-0.113517,0.124334,-0.391360,0.208957,0.155125,-0.296774,0.183402,0.075680,-0.352388,0.192189,0.190897,-0.498582,0.269451,
                        0.177128,-0.498543,0.306824,0.139141,-0.498427,0.335966,0.082624,-0.498260,0.314472,0.035934,-0.498119,0.240176,-0.004366,-0.186127,0.459205,0.046942,-0.187283,0.463321,0.032034,-0.177715,0.509900,-0.004648,-0.171651,0.508707,
                        0.171696,0.040099,0.442045,0.171679,0.023802,0.447166,0.205390,-0.001310,0.416503,-0.005120,-0.173144,0.585529,0.013197,-0.184091,0.553572,0.013787,-0.166117,0.596733,0.051115,-0.156804,0.622179,0.087701,-0.110714,0.636777,
                        0.086082,-0.063101,0.636396,0.040761,-0.012835,0.656814,0.097011,-0.015912,0.594033,0.103024,0.048542,0.597409,0.079517,0.058508,0.617072,0.114271,0.037621,0.577971,0.104448,0.001223,0.574035,0.137325,0.075374,0.462314,
                        0.135830,0.245557,0.453480,0.105194,0.163531,0.489091,0.129234,0.050857,0.488433,0.213282,0.499256,0.194086,0.181515,0.485771,0.329617,0.056237,0.236086,0.451189,0.070471,0.277036,0.436495,0.035845,0.268199,0.377224,
                        0.109290,0.371312,0.238490,0.106326,0.333005,0.243748,0.084608,0.361313,0.316120,0.162810,0.326732,0.446023,0.181065,0.406156,0.399669,0.100215,0.385059,0.367970,0.083064,0.332158,0.405988,0.162999,0.177038,0.389596,
                        0.152055,0.108612,0.410181,0.068624,0.180391,0.326281,0.063008,0.220484,0.309278,0.193006,-0.051877,0.198542,0.173332,0.026130,0.147062,0.188527,0.048564,0.289780,0.053456,-0.499274,-0.439014,0.082043,-0.497446,-0.321138,
                        0.063861,-0.443539,-0.440244,0.057765,-0.499291,-0.221558,0.074159,-0.499343,-0.147787,0.081066,-0.453984,-0.149058,0.064705,-0.443079,-0.222830,0.192162,-0.232395,-0.481292,0.144180,-0.247934,-0.603770,0.079513,-0.499353,-0.503828,
                        0.083082,-0.442371,-0.508676,0.133881,-0.457891,-0.519984,0.131514,-0.499512,-0.515121,0.168328,-0.414539,-0.424309,0.121277,-0.405106,-0.471814,0.103696,-0.341721,-0.571274,0.152673,-0.349502,-0.528593,-0.166915,-0.414033,-0.426363,
                        -0.183343,-0.325647,-0.444820,-0.149789,-0.349046,-0.530445,-0.177186,-0.419475,-0.406013,-0.062350,-0.443349,-0.441016,-0.052129,-0.499114,-0.439660,-0.077391,-0.499116,-0.504788,-0.080728,-0.442124,-0.509679,-0.095674,-0.121240,-0.683914,
                        0.002334,-0.101605,-0.614195,0.001762,-0.232855,-0.552971,-0.035458,-0.238852,-0.628699,-0.038556,-0.123049,-0.673901,-0.191261,-0.499120,-0.429739,-0.259685,-0.499121,-0.264573,-0.266438,-0.474421,-0.262604,-0.198014,-0.466397,-0.427770,
                        -0.181108,-0.499120,-0.475449,-0.129250,-0.499118,-0.516718,-0.082152,-0.497198,-0.322144,-0.194459,0.049143,0.287434,-0.080790,0.125935,0.247442,-0.198028,0.249360,0.290466,-0.097024,0.269580,0.274922,-0.068671,0.220683,0.308471,
                        -0.113626,0.371648,0.237124,-0.218670,0.426640,0.178958,-0.216681,0.499905,0.191454,-0.077521,0.277260,0.435589,-0.169818,0.327235,0.443986,-0.143178,0.245979,0.451772,-0.074616,0.180607,0.325404,-0.003350,0.235827,0.397391,
                        -0.042197,0.268317,0.376746,-0.064074,0.312509,0.346878,-0.089572,0.332419,0.404930,-0.187264,0.406712,0.397413,-0.186617,0.486327,0.327363,-0.134306,0.006169,0.507234,-0.159285,0.109082,0.408274,-0.145295,0.075801,0.460583,
                        -0.169769,0.177541,0.387557,-0.106873,-0.015605,0.592785,-0.114013,0.001553,0.572697,-0.121838,-0.013177,0.545736,-0.117158,-0.036849,0.557649,-0.089512,0.135121,0.574423,-0.062717,0.054062,0.636786,-0.005158,0.073721,0.652625,
                        -0.004641,0.158550,0.589155,-0.098373,-0.110433,0.635637,-0.096606,-0.062825,0.635277,-0.061751,-0.156634,0.621487,-0.023078,-0.184037,0.553350,-0.024143,-0.166059,0.596501,-0.188526,0.095747,0.328633,-0.213025,-0.000678,0.413941,
                        -0.179522,0.040630,0.439894,-0.055724,-0.187128,0.462692,-0.103550,-0.155305,0.463592,-0.118620,-0.146677,0.402488,-0.056350,-0.186626,0.411631,-0.090513,-0.497747,0.312389,-0.042917,-0.497749,0.238670,-0.036598,-0.406081,0.250420,
                        -0.132928,-0.367311,0.269885,-0.147290,-0.497743,0.333189,-0.235495,-0.406635,-0.166682,-0.261705,-0.450287,-0.243570,-0.267159,-0.470498,-0.173109,-0.083152,-0.453736,-0.150064,-0.065855,-0.442881,-0.223629,-0.113383,-0.368585,-0.222894,
                        -0.146576,-0.394012,-0.112895,-0.052782,-0.327589,-0.173014,-0.081421,-0.303510,-0.150225,-0.207285,-0.232739,-0.140292,-0.201464,-0.229700,-0.064958,-0.190682,-0.218929,-0.018404,-0.199174,-0.198833,-0.003231,-0.230331,-0.106290,0.041787,
                        -0.260143,-0.101942,-0.027276,-0.191371,-0.430642,0.159888,-0.203467,-0.239652,0.055062,-0.197084,-0.334939,0.019019,-0.180289,-0.497752,0.178537,-0.279229,-0.255122,-0.280191,-0.320756,-0.102652,-0.235287,-0.311994,-0.120316,-0.342527,
                        -0.246580,-0.129199,0.080746,-0.254700,-0.176965,0.120565,-0.209549,-0.145738,0.236471,-0.198126,-0.051286,0.196146,-0.244589,-0.269896,-0.374270,-0.189501,-0.231819,-0.483629,-0.160800,-0.296297,0.181467,-0.196407,-0.403209,0.254206,
                        -0.184918,-0.497746,0.303584,-0.185459,-0.227219,0.005551,-0.170806,-0.360279,-0.187845,-0.085849,-0.441064,-0.323499,-0.000554,-0.341289,-0.201640,-0.082095,-0.335904,-0.106031,-0.081638,-0.352150,0.191226,-0.002377,-0.369012,0.089170,
                        -0.002885,-0.345467,0.178067,-0.134384,-0.237019,0.272630,-0.020657,-0.431673,0.169698,-0.041415,-0.497750,0.206240,-0.004097,-0.199710,0.411820,-0.041360,-0.177604,0.509452,-0.004808,-0.173531,0.534301,-0.115960,-0.103451,0.601792,
                        -0.096219,-0.152051,0.588691,-0.051387,-0.012696,0.656250,-0.089438,0.058764,0.616037,-0.112733,0.048868,0.596087,-0.123773,0.037980,0.576514,-0.113229,0.163861,0.487752,-0.131533,0.073086,0.546432,-0.136044,0.077254,0.522242,
                        -0.211983,0.331671,0.229941,-0.063591,0.236266,0.450455,-0.115513,0.419500,0.320538,-0.121951,0.421298,0.268127,-0.091861,0.148905,0.281343,-0.205977,-0.122629,0.270677,-0.134805,-0.205139,0.331647,-0.162760,-0.122236,0.387249,
                        -0.201695,-0.042184,0.348775,-0.187861,-0.456200,-0.473480,-0.140069,-0.247505,-0.605510,-0.088689,-0.241529,-0.642742,-0.100270,-0.341413,-0.572523,-0.071600,-0.395452,-0.467013,-0.119259,-0.404743,-0.473287,-0.056064,-0.337101,-0.558613,
                        0.178331,-0.420012,-0.403836,0.185247,-0.326204,-0.442564,0.101130,-0.121538,-0.682709,0.043888,-0.123174,-0.673396,0.039887,-0.238966,-0.628237,0.258851,-0.499905,-0.261398,0.192454,-0.499700,-0.427389,0.258475,-0.499905,-0.171901,
                        0.231474,-0.499824,-0.071233,0.155526,-0.499591,-0.044702,0.075588,0.125699,0.248400,0.182229,0.095186,0.330904,0.117384,0.420936,0.269592,0.058219,0.312324,0.347628,0.127325,0.076856,0.523855,0.122505,0.072703,0.547987,
                        0.120612,0.097839,0.566866,0.052531,0.053888,0.637492,0.105723,-0.103785,0.603150,0.119919,-0.079262,0.568615,0.107661,-0.037189,0.559026,0.085996,-0.152326,0.589806,0.034008,-0.166103,0.563629,0.169029,-0.095405,0.506545,
                        0.146009,-0.024881,0.514847,0.155089,-0.122716,0.389195,0.128765,-0.134192,0.486288,0.094848,-0.155604,0.464807,0.110692,-0.147023,0.403893,0.088450,-0.391918,0.036306,0.193277,-0.335528,0.021410,0.174032,-0.498539,0.181729,
                        0.014720,-0.431978,0.170936,0.029748,-0.406432,0.251850,0.125950,-0.367953,0.272492,0.144211,-0.394451,-0.111114,0.219802,-0.388971,-0.241891,0.112443,-0.368926,-0.221511,0.079791,-0.303753,-0.149237,0.182143,-0.227774,0.007802,
                        0.199506,-0.240260,0.057530,0.185545,-0.431463,0.163220,0.189721,-0.456770,-0.471168,0.203650,-0.146362,0.239001,0.199729,-0.123242,0.273161,0.320756,-0.103621,-0.231358,0.313254,-0.121260,-0.338699,0.282642,0.053421,-0.323205,
                        0.111627,0.126285,0.125085,0.138561,0.158916,0.047026,0.231414,0.053208,0.026350,0.189507,-0.404043,0.257592,0.151897,-0.274032,-0.046093,0.177514,-0.234453,-0.021832,0.034829,-0.498116,0.207730,0.048193,-0.186784,0.412271,
                        0.080332,0.134865,0.575463,0.112559,-0.013531,0.547172,0.125556,0.005776,0.508825,0.215205,0.425985,0.181616,0.110299,0.419159,0.321922,0.207605,0.331037,0.232510,0.086311,0.148636,0.282434,0.000638,-0.386198,-0.407391,
                        0.073573,-0.395671,-0.466124,0.238375,-0.470133,-0.075830,0.093277,-0.241804,-0.641628,0.059337,-0.337275,-0.557907,-0.131431,-0.457491,-0.521608,-0.076398,-0.499116,-0.148709,-0.059102,-0.499114,-0.222274,-0.106097,0.385370,0.366706,
                        -0.089928,0.361577,0.315051,-0.110842,0.333333,0.242419,-0.137599,0.051260,0.486799,-0.179616,0.024333,0.445015,-0.043957,-0.165985,0.563151,-0.178054,-0.094880,0.504419,-0.154924,-0.024427,0.513004,-0.137663,-0.133790,0.484657,
                        -0.092618,-0.391644,0.035196,-0.042856,-0.336447,-0.049716,-0.220543,-0.388305,-0.244588,-0.232218,-0.433180,-0.342455,-0.154695,-0.273569,-0.047971,-0.130604,-0.390724,0.206373,-0.259216,-0.107564,-0.130077,-0.198228,-0.497743,0.266045,
                        -0.180486,-0.233912,-0.024024,-0.274469,-0.376654,-0.315745,-0.241394,-0.469408,-0.078767,-0.165775,-0.460771,-0.047984,-0.065642,-0.328583,-0.051428,-0.129660,-0.078885,0.567087,-0.109942,-0.128015,0.528800,-0.129796,0.098218,0.565333,
                        -0.186505,0.317381,0.419948,-0.004005,0.215761,0.499482,-0.002819,0.227905,0.308580,0.182862,-0.499670,-0.473220,0.194734,-0.042783,0.351201,0.091917,0.269294,0.276078,0.132048,0.104912,0.522245,-0.005419,-0.006328,0.675304,
                        0.072536,-0.153572,0.515188,0.100524,-0.128333,0.530089,-0.001521,-0.349590,-0.045783,0.274630,-0.377483,-0.312382,0.169459,-0.360793,-0.185761,0.226933,-0.106981,0.044587,0.173288,0.185880,-0.320055,0.245793,-0.270637,-0.371267,
                        0.279321,-0.255966,-0.276771,0.187685,-0.219501,-0.016086,0.179761,0.316827,0.422191,-0.002602,0.168040,0.258478,-0.002406,0.150857,0.222217,0.192662,0.248769,0.292859,0.162412,-0.461266,-0.045974,0.001109,-0.332804,-0.470974,
                        -0.260405,-0.499121,-0.175078,-0.234640,-0.499120,-0.074087,-0.140663,0.105325,0.520575,-0.081849,-0.153339,0.514243,-0.159022,-0.499116,-0.046627};

                    const unsigned short inds[] = {
                        0,1,2,3,4,1,2,1,4,0,3,1,5,6,7,8,9,10,5,7,11,5,11,12,8,10,13,8,14,9,5,15,16,8,13,17,8,18,14,5,16,6,5,19,15,8,17,20,8,20,18,5,12,19,21,22,23,21,23,24,25,26,27,
                        28,29,30,28,30,31,32,33,34,35,36,22,27,31,30,27,30,25,37,28,38,23,33,32,23,32,24,35,22,21,27,26,39,37,29,28,33,40,34,22,36,41,22,41,23,42,31,27,42,27,39,38,28,31,38,31,42,23,41,40,23,40,33,
                        43,44,45,43,45,46,47,48,49,50,51,52,50,52,53,54,55,56,51,57,58,51,58,52,54,59,60,61,47,49,61,49,44,61,44,43,43,62,61,63,64,65,63,65,48,52,59,66,67,68,56,67,56,55,66,68,69,66,69,53,70,71,63,
                        71,64,63,44,49,50,44,50,45,61,62,72,61,72,70,47,63,48,56,66,59,56,68,66,53,69,45,53,45,50,54,56,59,51,50,49,51,49,48,70,47,61,70,63,47,66,53,52,69,73,46,69,46,45,68,73,69,68,67,73,60,59,52,
                        60,52,58,57,51,48,57,48,65,71,70,72,74,75,76,74,76,77,78,79,80,78,80,81,82,77,83,82,83,84,73,67,82,77,76,85,77,85,83,86,87,79,86,79,78,88,74,82,54,89,88,54,88,82,54,82,55,75,86,78,75,78,76,
                        79,90,91,79,91,80,82,84,46,82,46,73,67,55,82,76,78,81,76,81,85,87,92,90,87,90,79,74,77,82,93,94,95,93,95,96,97,98,71,97,71,72,97,72,99,100,101,102,103,104,105,103,105,106,107,108,109,110,111,112,110,112,113,
                        114,115,103,116,117,118,119,120,121,115,114,122,115,122,123,124,125,126,127,128,129,127,129,130,127,130,131,132,133,134,132,134,135,136,137,138,139,140,141,142,143,3,3,144,145,146,147,148,149,146,150,151,152,153,151,35,152,150,12,11,150,11,149,
                        16,15,154,32,34,155,32,155,156,157,158,159,160,161,162,163,164,165,163,165,166,167,168,169,167,169,170,171,172,173,174,175,117,174,117,176,177,178,179,177,179,180,98,97,181,98,181,182,183,184,185,183,185,186,187,188,189,187,189,190,191,192,193,
                        191,194,192,195,196,197,195,197,198,54,60,199,200,201,202,200,202,203,204,205,206,204,206,207,204,208,209,204,209,197,204,197,196,205,204,196,205,196,210,92,211,212,213,214,215,216,217,218,219,220,221,222,215,223,219,224,225,219,225,226,30,227,228,
                        30,228,25,14,18,229,230,231,37,230,37,232,233,234,235,233,235,236,237,238,239,237,239,240,241,2,242,243,0,2,244,139,245,230,246,247,230,247,248,249,250,251,249,251,252,253,254,255,253,255,256,253,256,257,258,259,260,261,262,263,261,263,264,
                        265,266,267,268,269,270,268,270,271,268,271,272,273,274,275,275,276,273,277,278,279,280,281,282,280,282,283,192,284,279,192,279,285,280,283,92,274,286,281,259,206,260,256,287,288,256,288,257,270,269,289,290,267,277,291,263,262,292,123,293,294,295,296,
                        112,297,286,298,255,254,298,254,299,249,252,300,300,132,249,301,135,302,301,302,244,241,303,304,2,305,242,233,306,307,307,308,233,231,309,221,221,37,231,13,310,311,234,9,14,234,14,235,42,39,217,42,217,312,313,223,224,215,214,225,215,225,224,
                        218,228,314,218,314,315,211,246,316,211,316,212,317,318,319,317,319,320,208,204,207,208,207,321,57,200,203,199,60,58,199,58,203,322,323,324,322,324,193,325,326,324,325,324,327,328,190,329,174,176,184,174,184,183,71,330,64,201,200,331,201,331,332,
                        333,334,109,333,109,108,334,333,175,334,175,174,175,333,335,175,335,336,175,336,337,175,337,178,175,178,177,172,338,173,169,168,339,160,340,155,166,152,163,166,158,152,170,169,223,159,158,166,159,166,341,164,24,32,164,32,156,6,16,154,6,154,342,
                        151,136,168,343,344,147,343,147,149,146,148,345,346,347,348,349,142,141,349,141,350,351,138,137,351,137,352,353,354,355,353,355,356,357,358,359,360,361,362,360,362,124,357,295,122,363,121,364,363,364,365,118,292,116,115,366,103,367,368,358,369,368,125,
                        107,370,328,371,171,372,373,374,375,373,375,94,62,43,99,96,95,85,96,85,81,376,377,80,376,80,91,99,72,62,106,105,93,106,93,378,368,100,125,107,109,370,379,369,124,379,124,362,380,381,367,366,104,103,118,180,365,107,364,121,107,121,120,
                        122,114,357,296,126,125,361,360,382,361,382,131,133,300,383,133,132,300,135,134,140,135,140,302,349,143,142,143,144,3,345,384,240,345,240,239,150,146,348,150,348,385,151,168,167,151,167,35,15,19,385,15,385,386,34,40,387,34,387,155,165,388,162,
                        223,159,170,388,165,156,161,160,389,339,390,169,172,376,338,176,391,392,336,335,119,336,119,393,370,187,328,71,98,330,332,394,189,332,189,395,328,187,190,326,191,193,326,193,324,198,197,209,198,209,396,88,89,322,88,322,285,397,398,262,397,262,261,
                        210,196,195,210,195,291,283,211,92,230,232,215,230,215,222,227,220,226,227,226,399,216,400,401,224,219,313,26,25,228,26,228,218,20,402,229,9,234,308,9,308,10,403,402,231,403,231,248,308,234,233,238,237,306,233,303,241,233,241,242,243,304,404,
                        243,404,245,247,405,406,247,406,403,407,319,251,407,251,250,408,299,276,294,296,112,408,409,295,410,411,259,410,259,258,291,265,263,266,293,268,412,275,289,273,287,256,273,256,413,411,194,321,321,207,411,282,317,283,414,272,87,414,87,86,278,414,86,
                        278,86,75,274,281,270,274,270,289,279,284,277,415,287,273,415,273,276,289,269,416,410,290,277,410,277,417,418,419,264,418,264,258,409,420,293,409,293,123,408,295,298,408,298,299,420,408,275,420,275,412,319,407,405,406,405,421,406,421,236,244,245,404,
                        421,422,304,421,304,303,236,421,303,233,238,306,423,310,308,423,308,307,231,230,248,20,17,311,20,311,402,37,424,29,313,425,223,214,401,400,214,400,225,316,222,426,228,227,399,215,232,213,252,318,111,318,251,319,208,321,396,208,396,209,398,210,291,
                        398,291,262,279,74,88,321,191,326,321,326,396,284,194,417,188,392,395,188,395,189,394,182,190,394,190,189,64,330,331,64,331,65,57,65,331,334,427,370,334,370,109,110,372,428,110,428,353,339,173,338,339,338,390,156,155,340,156,340,388,429,170,159,
                        429,159,341,157,159,223,40,41,389,40,389,387,12,150,385,12,385,19,342,343,7,384,153,157,385,348,352,344,430,153,344,153,384,3,145,431,3,431,4,142,3,0,350,432,433,138,339,136,354,433,432,354,432,355,428,173,339,428,339,138,126,360,124,
                        295,434,122,435,364,107,118,365,364,118,364,436,103,380,114,128,127,379,107,120,108,125,100,371,125,371,113,106,378,172,106,172,437,375,99,438,172,378,377,172,377,376,94,375,438,94,438,95,439,329,181,439,181,374,100,102,171,100,171,371,373,440,439,
                        368,369,358,369,359,358,101,441,103,101,103,106,101,106,437,118,104,366,180,179,363,180,363,365,121,393,119,361,130,129,361,129,362,130,361,131,355,133,383,355,383,356,339,168,136,140,350,141,433,347,346,433,346,349,346,146,144,346,144,143,345,148,384,
                        149,147,146,154,137,136,154,136,151,157,240,384,15,386,154,35,21,442,163,442,164,388,340,162,443,390,338,443,338,444,389,445,429,389,429,161,376,444,338,176,117,116,176,116,391,337,336,393,337,393,446,201,332,395,201,395,447,188,185,184,188,184,392,
                        328,329,439,391,325,327,391,327,447,323,202,327,323,327,324,199,89,54,321,194,191,448,449,418,448,418,260,391,195,325,212,90,92,230,222,246,222,316,246,218,315,216,219,226,220,215,224,223,399,400,314,39,26,218,39,218,217,229,235,14,231,402,311,
                        231,311,450,229,402,403,307,306,237,307,237,423,242,305,238,2,241,243,404,304,422,247,403,248,249,132,135,249,135,301,257,288,415,257,415,254,257,254,253,298,286,413,409,123,434,418,258,260,116,265,291,420,412,268,111,318,297,111,297,112,206,259,411,
                        267,268,272,267,272,414,281,280,270,193,192,285,193,285,322,92,271,280,282,297,318,282,318,317,413,286,274,289,416,412,412,416,269,412,269,268,266,265,292,261,264,419,293,266,292,294,298,295,112,286,294,299,254,415,299,415,276,451,250,249,451,249,301,
                        139,0,245,304,243,241,305,2,4,305,4,431,450,423,237,450,237,309,406,236,235,313,309,237,17,13,311,37,221,220,37,220,424,232,37,38,232,38,213,399,226,225,399,225,400,223,426,222,217,216,312,444,91,90,318,252,251,205,448,260,205,260,206,
                        57,203,58,323,199,203,323,203,202,411,417,194,436,104,118,435,328,439,187,370,185,187,185,188,97,375,374,175,177,180,175,180,117,370,427,186,370,186,185,110,353,356,111,110,383,170,429,445,160,155,387,426,169,390,161,429,341,161,341,162,157,223,425,
                        21,24,164,164,442,21,11,7,343,11,343,149,352,137,154,430,151,153,144,146,345,348,146,346,140,134,432,140,432,350,351,347,433,351,433,354,351,353,428,351,428,138,382,357,359,126,295,360,292,366,115,179,446,363,366,292,118,381,380,103,381,103,441,
                        358,380,367,125,124,369,104,440,373,104,373,105,102,101,437,329,190,182,329,182,181,95,438,83,95,83,85,377,96,81,377,81,80,438,99,84,438,84,83,43,46,84,43,84,99,437,172,102,172,171,102,113,371,372,113,372,110,120,119,108,379,362,129,
                        379,129,128,101,367,441,436,440,104,118,117,180,115,123,292,126,296,295,125,113,112,125,112,296,131,382,359,131,359,127,355,432,134,355,134,133,139,141,0,349,346,143,144,345,145,148,147,344,148,344,384,386,352,154,154,151,430,154,430,342,152,35,442,
                        152,442,163,36,445,389,36,389,41,162,340,160,426,390,443,165,164,156,170,445,167,111,383,300,372,171,173,372,173,428,174,183,186,174,186,427,174,427,334,392,184,176,178,337,446,178,446,179,97,374,181,98,182,394,98,394,330,435,439,440,392,391,447,
                        392,447,395,194,284,192,396,326,325,396,325,198,202,201,447,202,447,327,449,452,419,449,419,418,291,195,391,291,391,116,90,212,444,213,312,401,213,401,214,316,443,444,316,444,212,314,400,315,220,227,424,29,227,30,29,424,227,229,18,20,425,313,237,
                        425,237,240,406,235,229,450,311,310,450,310,423,238,305,431,238,431,239,245,0,243,301,244,404,301,404,451,421,405,407,421,407,422,319,405,247,319,247,320,276,275,408,255,298,413,255,413,256,408,420,409,258,264,263,258,263,410,265,116,292,268,267,266,
                        275,274,289,413,274,273,277,267,414,277,414,278,280,271,270,279,278,75,279,75,74,87,272,271,87,271,92,281,286,297,281,297,282,411,207,206,288,287,415,420,268,293,263,265,290,263,290,410,417,411,410,295,409,434,298,294,286,211,320,247,211,247,246,
                        422,407,250,422,250,451,302,139,244,422,451,404,236,303,233,238,233,242,450,309,231,403,406,229,13,10,308,13,308,310,221,309,313,221,313,219,213,38,42,213,42,312,400,216,315,426,443,316,399,314,228,401,312,216,111,300,252,283,317,320,283,320,211,
                        210,398,397,210,397,452,210,452,449,210,449,448,210,448,205,195,198,325,452,397,261,452,261,419,88,285,279,322,89,199,322,199,323,417,277,284,290,265,267,107,328,435,330,394,332,330,332,331,200,57,331,335,333,108,335,108,119,110,356,383,376,91,444,
                        387,389,160,223,169,426,166,165,162,166,162,341,35,167,445,35,445,36,153,152,158,153,158,157,7,6,342,157,425,240,352,386,385,342,430,344,342,344,343,145,345,239,145,239,431,141,142,0,350,433,349,302,140,139,351,352,348,351,348,347,353,351,354,
                        357,114,380,357,380,358,295,357,382,295,382,360,122,434,123,446,393,121,446,121,363,436,364,435,436,435,440,367,381,441,379,127,359,379,359,369,439,374,373,100,368,367,100,367,101,105,373,94,105,94,93,375,97,99,378,93,96,378,96,377};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_TORUS: {
#               ifndef TEAPOT_NO_MESH_TORUS
                {


                    const float verts[] = {
                        0.502782,0.000000,0.000000,0.483676,0.000000,-0.059127,0.433398,0.000000,-0.095730,0.371053,0.000000,-0.095730,0.320775,0.000000,-0.059127,0.301669,0.000000,0.000000,0.320775,0.000000,0.059127,0.371053,0.000000,0.095730,
                        0.433398,0.000000,0.095730,0.483676,0.000000,0.059127,0.491721,0.104579,0.000000,0.473017,0.100556,-0.059127,0.423745,0.090099,-0.095730,0.363009,0.077227,-0.095730,0.313937,0.066769,-0.059127,0.295032,0.062747,0.000000,
                        0.313937,0.066769,0.059127,0.363009,0.077227,0.095730,0.423745,0.090099,0.095730,0.473017,0.100556,0.059127,0.459342,0.204532,0.000000,0.441845,0.196688,-0.059127,0.395790,0.176175,-0.095730,0.339076,0.151036,-0.095730,
                        0.293222,0.130522,-0.059127,0.275525,0.122679,0.000000,0.293222,0.130522,0.059127,0.339076,0.151036,0.095730,0.395790,0.176175,0.095730,0.441845,0.196688,0.059127,0.406851,0.295435,0.000000,0.391164,0.284172,-0.059127,
                        0.350540,0.254609,-0.095730,0.300261,0.218207,-0.095730,0.259637,0.188644,-0.059127,0.244151,0.177382,0.000000,0.259637,0.188644,0.059127,0.300261,0.218207,0.095730,0.350540,0.254609,0.095730,0.391164,0.284172,0.059127,
                        0.336462,0.373668,0.000000,0.323590,0.359389,-0.059127,0.290005,0.321982,-0.095730,0.248374,0.275726,-0.095730,0.214788,0.238520,-0.059127,0.201917,0.224241,0.000000,0.214788,0.238520,0.059127,0.248374,0.275726,0.095730,
                        0.290005,0.321982,0.095730,0.323590,0.359389,0.059127,0.251391,0.435409,0.000000,0.241738,0.418717,-0.059127,0.216598,0.375276,-0.095730,0.185627,0.321378,-0.095730,0.160488,0.277938,-0.059127,0.150835,0.261246,0.000000,
                        0.160488,0.277938,0.059127,0.185627,0.321378,0.095730,0.216598,0.375276,0.095730,0.241738,0.418717,0.059127,0.155460,0.478246,0.000000,0.149427,0.459945,-0.059127,0.133941,0.412080,-0.095730,0.114634,0.352953,-0.095730,
                        0.099149,0.305088,-0.059127,0.093316,0.286988,0.000000,0.099149,0.305088,0.059127,0.114634,0.352953,0.095730,0.133941,0.412080,0.095730,0.149427,0.459945,0.059127,0.052490,0.499966,0.000000,0.050479,0.480861,-0.059127,
                        0.045250,0.430985,-0.095730,0.038815,0.369042,-0.095730,0.033586,0.319166,-0.059127,0.031575,0.300060,0.000000,0.033586,0.319166,0.059127,0.038815,0.369042,0.095730,0.045250,0.430985,0.095730,0.050479,0.480861,0.059127,
                        -0.052490,0.499966,0.000000,-0.050479,0.480861,-0.059127,-0.045250,0.430985,-0.095730,-0.038815,0.369042,-0.095730,-0.033586,0.319166,-0.059127,-0.031575,0.300060,0.000000,-0.033586,0.319166,0.059127,-0.038815,0.369042,0.095730,
                        -0.045250,0.430985,0.095730,-0.050479,0.480861,0.059127,-0.155460,0.478246,0.000000,-0.149427,0.459945,-0.059127,-0.133941,0.412080,-0.095730,-0.114634,0.352953,-0.095730,-0.099149,0.305088,-0.059127,-0.093316,0.286988,0.000000,
                        -0.099149,0.305088,0.059127,-0.114634,0.352953,0.095730,-0.133941,0.412080,0.095730,-0.149427,0.459945,0.059127,-0.251391,0.435409,0.000000,-0.241738,0.418717,-0.059127,-0.216598,0.375276,-0.095730,-0.185627,0.321378,-0.095730,
                        -0.160488,0.277938,-0.059127,-0.150835,0.261246,0.000000,-0.160488,0.277938,0.059127,-0.185627,0.321378,0.095730,-0.216598,0.375276,0.095730,-0.241738,0.418717,0.059127,-0.336462,0.373668,0.000000,-0.323590,0.359389,-0.059127,
                        -0.290005,0.321982,-0.095730,-0.248374,0.275726,-0.095730,-0.214788,0.238520,-0.059127,-0.201917,0.224241,0.000000,-0.214788,0.238520,0.059127,-0.248374,0.275726,0.095730,-0.290005,0.321982,0.095730,-0.323590,0.359389,0.059127,
                        -0.406851,0.295435,0.000000,-0.391164,0.284172,-0.059127,-0.350540,0.254609,-0.095730,-0.300261,0.218207,-0.095730,-0.259637,0.188644,-0.059127,-0.244151,0.177382,0.000000,-0.259637,0.188644,0.059127,-0.300261,0.218207,0.095730,
                        -0.350540,0.254609,0.095730,-0.391164,0.284172,0.059127,-0.459342,0.204532,0.000000,-0.441845,0.196688,-0.059127,-0.395790,0.176175,-0.095730,-0.339076,0.151036,-0.095730,-0.293222,0.130522,-0.059127,-0.275525,0.122679,0.000000,
                        -0.293222,0.130522,0.059127,-0.339076,0.151036,0.095730,-0.395790,0.176175,0.095730,-0.441845,0.196688,0.059127,-0.491721,0.104579,0.000000,-0.473017,0.100556,-0.059127,-0.423745,0.090099,-0.095730,-0.363009,0.077227,-0.095730,
                        -0.313937,0.066769,-0.059127,-0.295032,0.062747,0.000000,-0.313937,0.066769,0.059127,-0.363009,0.077227,0.095730,-0.423745,0.090099,0.095730,-0.473017,0.100556,0.059127,-0.502782,0.000000,0.000000,-0.483676,0.000000,-0.059127,
                        -0.433398,0.000000,-0.095730,-0.371053,0.000000,-0.095730,-0.320775,0.000000,-0.059127,-0.301669,0.000000,0.000000,-0.320775,0.000000,0.059127,-0.371053,0.000000,0.095730,-0.433398,0.000000,0.095730,-0.483676,0.000000,0.059127,
                        -0.491721,-0.104579,0.000000,-0.473017,-0.100556,-0.059127,-0.423745,-0.090099,-0.095730,-0.363009,-0.077227,-0.095730,-0.313937,-0.066769,-0.059127,-0.295032,-0.062747,0.000000,-0.313937,-0.066769,0.059127,-0.363009,-0.077227,0.095730,
                        -0.423745,-0.090099,0.095730,-0.473017,-0.100556,0.059127,-0.459342,-0.204532,0.000000,-0.441845,-0.196688,-0.059127,-0.395790,-0.176175,-0.095730,-0.339076,-0.151036,-0.095730,-0.293222,-0.130522,-0.059127,-0.275525,-0.122679,0.000000,
                        -0.293222,-0.130522,0.059127,-0.339076,-0.151036,0.095730,-0.395790,-0.176175,0.095730,-0.441845,-0.196688,0.059127,-0.406851,-0.295435,0.000000,-0.391164,-0.284172,-0.059127,-0.350540,-0.254609,-0.095730,-0.300261,-0.218207,-0.095730,
                        -0.259637,-0.188644,-0.059127,-0.244151,-0.177381,0.000000,-0.259637,-0.188644,0.059127,-0.300261,-0.218207,0.095730,-0.350540,-0.254609,0.095730,-0.391164,-0.284172,0.059127,-0.336462,-0.373668,0.000000,-0.323590,-0.359389,-0.059127,
                        -0.290005,-0.321982,-0.095730,-0.248374,-0.275726,-0.095730,-0.214788,-0.238520,-0.059127,-0.201917,-0.224241,0.000000,-0.214788,-0.238520,0.059127,-0.248374,-0.275726,0.095730,-0.290005,-0.321982,0.095730,-0.323590,-0.359389,0.059127,
                        -0.251391,-0.435409,0.000000,-0.241738,-0.418717,-0.059127,-0.216598,-0.375276,-0.095730,-0.185627,-0.321378,-0.095730,-0.160488,-0.277938,-0.059127,-0.150835,-0.261246,0.000000,-0.160488,-0.277938,0.059127,-0.185627,-0.321378,0.095730,
                        -0.216598,-0.375276,0.095730,-0.241738,-0.418717,0.059127,-0.155460,-0.478246,0.000000,-0.149427,-0.459945,-0.059127,-0.133941,-0.412080,-0.095730,-0.114634,-0.352953,-0.095730,-0.099149,-0.305088,-0.059127,-0.093316,-0.286988,0.000000,
                        -0.099149,-0.305088,0.059127,-0.114634,-0.352953,0.095730,-0.133941,-0.412080,0.095730,-0.149427,-0.459945,0.059127,-0.052490,-0.499966,0.000000,-0.050479,-0.480861,-0.059127,-0.045250,-0.430985,-0.095730,-0.038815,-0.369042,-0.095730,
                        -0.033586,-0.319166,-0.059127,-0.031575,-0.300060,0.000000,-0.033586,-0.319166,0.059127,-0.038815,-0.369042,0.095730,-0.045250,-0.430985,0.095730,-0.050479,-0.480861,0.059127,0.052490,-0.499966,0.000000,0.050479,-0.480861,-0.059127,
                        0.045250,-0.430985,-0.095730,0.038815,-0.369042,-0.095730,0.033586,-0.319166,-0.059127,0.031575,-0.300060,0.000000,0.033586,-0.319166,0.059127,0.038815,-0.369042,0.095730,0.045250,-0.430985,0.095730,0.050479,-0.480861,0.059127,
                        0.155460,-0.478246,0.000000,0.149427,-0.459945,-0.059127,0.133941,-0.412080,-0.095730,0.114634,-0.352953,-0.095730,0.099149,-0.305088,-0.059127,0.093316,-0.286988,0.000000,0.099149,-0.305088,0.059127,0.114634,-0.352953,0.095730,
                        0.133941,-0.412080,0.095730,0.149427,-0.459945,0.059127,0.251391,-0.435409,0.000000,0.241738,-0.418717,-0.059127,0.216598,-0.375276,-0.095730,0.185627,-0.321378,-0.095730,0.160488,-0.277938,-0.059127,0.150835,-0.261246,0.000000,
                        0.160488,-0.277938,0.059127,0.185627,-0.321378,0.095730,0.216598,-0.375276,0.095730,0.241738,-0.418717,0.059127,0.336462,-0.373668,0.000000,0.323590,-0.359389,-0.059127,0.290005,-0.321982,-0.095730,0.248374,-0.275726,-0.095730,
                        0.214788,-0.238520,-0.059127,0.201917,-0.224241,0.000000,0.214788,-0.238520,0.059127,0.248374,-0.275726,0.095730,0.290005,-0.321982,0.095730,0.323590,-0.359389,0.059127,0.406851,-0.295435,0.000000,0.391164,-0.284172,-0.059127,
                        0.350540,-0.254609,-0.095730,0.300261,-0.218207,-0.095730,0.259637,-0.188644,-0.059127,0.244151,-0.177381,0.000000,0.259637,-0.188644,0.059127,0.300261,-0.218207,0.095730,0.350540,-0.254609,0.095730,0.391164,-0.284172,0.059127,
                        0.459342,-0.204532,0.000000,0.441845,-0.196688,-0.059127,0.395790,-0.176175,-0.095730,0.339076,-0.151036,-0.095730,0.293222,-0.130522,-0.059127,0.275525,-0.122679,0.000000,0.293222,-0.130522,0.059127,0.339076,-0.151036,0.095730,
                        0.395790,-0.176175,0.095730,0.441845,-0.196688,0.059127,0.491721,-0.104579,0.000000,0.473017,-0.100556,-0.059127,0.423745,-0.090099,-0.095730,0.363009,-0.077227,-0.095730,0.313937,-0.066769,-0.059127,0.295032,-0.062747,0.000000,
                        0.313937,-0.066769,0.059127,0.363009,-0.077227,0.095730,0.423745,-0.090099,0.095730,0.473017,-0.100556,0.059127};

                    const unsigned short inds[] = {
                        0,1,11,1,2,12,2,3,13,3,4,14,4,5,15,5,6,16,6,7,17,7,8,18,8,9,19,9,0,10,10,11,21,11,12,22,12,13,23,13,14,24,14,15,25,15,16,26,16,17,27,17,18,28,18,19,29,19,10,20,20,21,31,
                        21,22,32,22,23,33,23,24,34,24,25,35,25,26,36,26,27,37,27,28,38,28,29,39,29,20,30,30,31,41,31,32,42,32,33,43,33,34,44,34,35,45,35,36,46,36,37,47,37,38,48,38,39,49,39,30,40,40,41,51,41,42,52,
                        42,43,53,43,44,54,44,45,55,45,46,56,46,47,57,47,48,58,48,49,59,49,40,50,50,51,61,51,52,62,52,53,63,53,54,64,54,55,65,55,56,66,56,57,67,57,58,68,58,59,69,59,50,60,60,61,71,61,62,72,62,63,73,
                        63,64,74,64,65,75,65,66,76,66,67,77,67,68,78,68,69,79,69,60,70,70,71,81,71,72,82,72,73,83,73,74,84,74,75,85,75,76,86,76,77,87,77,78,88,78,79,89,79,70,80,80,81,91,81,82,92,82,83,93,83,84,94,
                        84,85,95,85,86,96,86,87,97,87,88,98,88,89,99,89,80,90,90,91,101,91,92,102,92,93,103,93,94,104,94,95,105,95,96,106,96,97,107,97,98,108,98,99,109,99,90,100,100,101,111,101,102,112,102,103,113,103,104,114,104,105,115,
                        105,106,116,106,107,117,107,108,118,108,109,119,109,100,110,110,111,121,111,112,122,112,113,123,113,114,124,114,115,125,115,116,126,116,117,127,117,118,128,118,119,129,119,110,120,120,121,131,121,122,132,122,123,133,123,124,134,124,125,135,125,126,136,
                        126,127,137,127,128,138,128,129,139,129,120,130,130,131,141,131,132,142,132,133,143,133,134,144,134,135,145,135,136,146,136,137,147,137,138,148,138,139,149,139,130,140,140,141,151,141,142,152,142,143,153,143,144,154,144,145,155,145,146,156,146,147,157,
                        147,148,158,148,149,159,149,140,150,150,151,161,151,152,162,152,153,163,153,154,164,154,155,165,155,156,166,156,157,167,157,158,168,158,159,169,159,150,160,160,161,171,161,162,172,162,163,173,163,164,174,164,165,175,165,166,176,166,167,177,167,168,178,
                        168,169,179,169,160,170,170,171,181,171,172,182,172,173,183,173,174,184,174,175,185,175,176,186,176,177,187,177,178,188,178,179,189,179,170,180,180,181,191,181,182,192,182,183,193,183,184,194,184,185,195,185,186,196,186,187,197,187,188,198,188,189,199,
                        189,180,190,190,191,201,191,192,202,192,193,203,193,194,204,194,195,205,195,196,206,196,197,207,197,198,208,198,199,209,199,190,200,200,201,211,201,202,212,202,203,213,203,204,214,204,205,215,205,206,216,206,207,217,207,208,218,208,209,219,209,200,210,
                        210,211,221,211,212,222,212,213,223,213,214,224,214,215,225,215,216,226,216,217,227,217,218,228,218,219,229,219,210,220,220,221,231,221,222,232,222,223,233,223,224,234,224,225,235,225,226,236,226,227,237,227,228,238,228,229,239,229,220,230,230,231,241,
                        231,232,242,232,233,243,233,234,244,234,235,245,235,236,246,236,237,247,237,238,248,238,239,249,239,230,240,240,241,251,241,242,252,242,243,253,243,244,254,244,245,255,245,246,256,246,247,257,247,248,258,248,249,259,249,240,250,250,251,261,251,252,262,
                        252,253,263,253,254,264,254,255,265,255,256,266,256,257,267,257,258,268,258,259,269,259,250,260,260,261,271,261,262,272,262,263,273,263,264,274,264,265,275,265,266,276,266,267,277,267,268,278,268,269,279,269,260,270,270,271,281,271,272,282,272,273,283,
                        273,274,284,274,275,285,275,276,286,276,277,287,277,278,288,278,279,289,279,270,280,280,281,291,281,282,292,282,283,293,283,284,294,284,285,295,285,286,296,286,287,297,287,288,298,288,289,299,289,280,290,290,291,1,291,292,2,292,293,3,293,294,4,
                        294,295,5,295,296,6,296,297,7,297,298,8,298,299,9,299,290,0,0,11,10,1,12,11,2,13,12,3,14,13,4,15,14,5,16,15,6,17,16,7,18,17,8,19,18,9,10,19,10,21,20,11,22,21,12,23,22,13,24,23,14,25,24,
                        15,26,25,16,27,26,17,28,27,18,29,28,19,20,29,20,31,30,21,32,31,22,33,32,23,34,33,24,35,34,25,36,35,26,37,36,27,38,37,28,39,38,29,30,39,30,41,40,31,42,41,32,43,42,33,44,43,34,45,44,35,46,45,
                        36,47,46,37,48,47,38,49,48,39,40,49,40,51,50,41,52,51,42,53,52,43,54,53,44,55,54,45,56,55,46,57,56,47,58,57,48,59,58,49,50,59,50,61,60,51,62,61,52,63,62,53,64,63,54,65,64,55,66,65,56,67,66,
                        57,68,67,58,69,68,59,60,69,60,71,70,61,72,71,62,73,72,63,74,73,64,75,74,65,76,75,66,77,76,67,78,77,68,79,78,69,70,79,70,81,80,71,82,81,72,83,82,73,84,83,74,85,84,75,86,85,76,87,86,77,88,87,
                        78,89,88,79,80,89,80,91,90,81,92,91,82,93,92,83,94,93,84,95,94,85,96,95,86,97,96,87,98,97,88,99,98,89,90,99,90,101,100,91,102,101,92,103,102,93,104,103,94,105,104,95,106,105,96,107,106,97,108,107,98,109,108,
                        99,100,109,100,111,110,101,112,111,102,113,112,103,114,113,104,115,114,105,116,115,106,117,116,107,118,117,108,119,118,109,110,119,110,121,120,111,122,121,112,123,122,113,124,123,114,125,124,115,126,125,116,127,126,117,128,127,118,129,128,119,120,129,
                        120,131,130,121,132,131,122,133,132,123,134,133,124,135,134,125,136,135,126,137,136,127,138,137,128,139,138,129,130,139,130,141,140,131,142,141,132,143,142,133,144,143,134,145,144,135,146,145,136,147,146,137,148,147,138,149,148,139,140,149,140,151,150,
                        141,152,151,142,153,152,143,154,153,144,155,154,145,156,155,146,157,156,147,158,157,148,159,158,149,150,159,150,161,160,151,162,161,152,163,162,153,164,163,154,165,164,155,166,165,156,167,166,157,168,167,158,169,168,159,160,169,160,171,170,161,172,171,
                        162,173,172,163,174,173,164,175,174,165,176,175,166,177,176,167,178,177,168,179,178,169,170,179,170,181,180,171,182,181,172,183,182,173,184,183,174,185,184,175,186,185,176,187,186,177,188,187,178,189,188,179,180,189,180,191,190,181,192,191,182,193,192,
                        183,194,193,184,195,194,185,196,195,186,197,196,187,198,197,188,199,198,189,190,199,190,201,200,191,202,201,192,203,202,193,204,203,194,205,204,195,206,205,196,207,206,197,208,207,198,209,208,199,200,209,200,211,210,201,212,211,202,213,212,203,214,213,
                        204,215,214,205,216,215,206,217,216,207,218,217,208,219,218,209,210,219,210,221,220,211,222,221,212,223,222,213,224,223,214,225,224,215,226,225,216,227,226,217,228,227,218,229,228,219,220,229,220,231,230,221,232,231,222,233,232,223,234,233,224,235,234,
                        225,236,235,226,237,236,227,238,237,228,239,238,229,230,239,230,241,240,231,242,241,232,243,242,233,244,243,234,245,244,235,246,245,236,247,246,237,248,247,238,249,248,239,240,249,240,251,250,241,252,251,242,253,252,243,254,253,244,255,254,245,256,255,
                        246,257,256,247,258,257,248,259,258,249,250,259,250,261,260,251,262,261,252,263,262,253,264,263,254,265,264,255,266,265,256,267,266,257,268,267,258,269,268,259,260,269,260,271,270,261,272,271,262,273,272,263,274,273,264,275,274,265,276,275,266,277,276,
                        267,278,277,268,279,278,269,270,279,270,281,280,271,282,281,272,283,282,273,284,283,274,285,284,275,286,285,276,287,286,277,288,287,278,289,288,279,280,289,280,291,290,281,292,291,282,293,292,283,294,293,284,295,294,285,296,295,286,297,296,287,298,297,
                        288,299,298,289,290,299,290,1,0,291,2,1,292,3,2,293,4,3,294,5,4,295,6,5,296,7,6,297,8,7,298,9,8,299,0,9};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_CUBE: {
#               ifndef TEAPOT_NO_MESH_CUBE
                {
                    const float verts[] = {
                        -0.500000,-0.500000,-0.500000,-0.500000,-0.500000,0.500000,0.500000,-0.500000,-0.500000,0.500000,-0.500000,0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.500000,0.500000,0.500000,0.500000,-0.500000,0.500000,0.500000,0.500000,
                        -0.500000,-0.500000,0.500000,-0.500000,0.500000,0.500000,0.500000,-0.500000,0.500000,0.500000,0.500000,0.500000,-0.500000,-0.500000,-0.500000,-0.500000,-0.500000,0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.500000,0.500000,
                        -0.500000,-0.500000,-0.500000,-0.500000,0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.500000,0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.500000,-0.500000,0.500000,0.500000,0.500000,-0.500000,0.500000,0.500000,0.500000};

                    const unsigned short inds[] = {
                        3,0,2,5,6,4,9,10,11,13,14,12,19,16,17,23,20,22,3,1,0,5,7,6,9,8,10,13,15,14,19,18,16,23,21,20};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_CYLINDER: {
#               ifndef TEAPOT_NO_MESH_CYLINDER
                {

                    const float verts[] = {
                        0.000000,-0.500000,0.000000,-0.133934,-0.500000,-0.499848,0.133934,-0.500000,-0.499848,0.365914,-0.500000,-0.365914,0.499848,-0.500000,-0.133934,0.499848,-0.500000,0.133934,0.365914,-0.500000,0.365914,0.133934,-0.500000,0.499848,
                        -0.133934,-0.500000,0.499848,-0.365914,-0.500000,0.365914,-0.499847,-0.500000,0.133934,-0.499848,-0.500000,-0.133933,-0.365914,-0.500000,-0.365914,0.000000,0.500000,0.000000,-0.133934,0.500000,-0.499848,0.133934,0.500000,-0.499848,
                        0.365914,0.500000,-0.365914,0.499848,0.500000,-0.133934,0.499848,0.500000,0.133934,0.365914,0.500000,0.365914,0.133934,0.500000,0.499848,-0.133934,0.500000,0.499848,-0.365914,0.500000,0.365914,-0.499847,0.500000,0.133934,
                        -0.499848,0.500000,-0.133933,-0.365914,0.500000,-0.365914,-0.133934,-0.500000,-0.499848,-0.133934,0.500000,-0.499848,0.133934,-0.500000,-0.499848,0.133934,0.500000,-0.499848,0.365914,-0.500000,-0.365914,0.365914,0.500000,-0.365914,
                        0.499848,-0.500000,-0.133934,0.499848,0.500000,-0.133934,0.499848,-0.500000,0.133934,0.499848,0.500000,0.133934,0.365914,-0.500000,0.365914,0.365914,0.500000,0.365914,0.133934,-0.500000,0.499848,0.133934,0.500000,0.499848,
                        -0.133934,-0.500000,0.499848,-0.133934,0.500000,0.499848,-0.365914,-0.500000,0.365914,-0.365914,0.500000,0.365914,-0.499847,-0.500000,0.133934,-0.499847,0.500000,0.133934,-0.499848,-0.500000,-0.133933,-0.499848,0.500000,-0.133933,
                        -0.365914,-0.500000,-0.365914,-0.365914,0.500000,-0.365914};

                    const unsigned short inds[] = {
                        0,1,2,0,2,3,0,3,4,0,4,5,0,5,6,0,6,7,0,7,8,0,8,9,0,9,10,0,10,11,0,11,12,0,12,1,13,15,14,13,16,15,13,17,16,13,18,17,13,19,18,13,20,19,13,21,20,13,22,21,13,23,22,
                        13,24,23,13,25,24,13,14,25,27,28,26,29,30,28,31,32,30,33,34,32,35,36,34,37,38,36,39,40,38,41,42,40,43,44,42,45,46,44,47,48,46,49,26,48,27,29,28,29,31,30,31,33,32,33,35,34,35,37,36,37,39,38,
                        39,41,40,41,43,42,43,45,44,45,47,46,47,49,48,49,27,26};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_CONE: {
#               ifndef TEAPOT_NO_MESH_CONE
                {

                    const float verts[] = {
                        0.000000,0.500000,0.000000,0.365992,-0.500000,-0.365992,0.499955,-0.500000,-0.133962,0.133962,-0.500000,-0.499955,-0.133963,-0.500000,-0.499955,-0.365993,-0.500000,-0.365992,-0.499955,-0.500000,-0.133962,-0.499955,-0.500000,0.133963,
                        -0.365992,-0.500000,0.365992,-0.133962,-0.500000,0.499955,0.133963,-0.500000,0.499955,0.365992,-0.500000,0.365992,0.499955,-0.500000,0.133963,0.000000,-0.500000,0.000000,0.133962,-0.500000,-0.499955,0.365992,-0.500000,-0.365992,
                        0.499955,-0.500000,-0.133962,0.499955,-0.500000,0.133963,0.365992,-0.500000,0.365992,0.133963,-0.500000,0.499955,-0.133962,-0.500000,0.499955,-0.365992,-0.500000,0.365992,-0.499955,-0.500000,0.133963,-0.499955,-0.500000,-0.133962,
                        -0.365993,-0.500000,-0.365992,-0.133963,-0.500000,-0.499955};

                    const unsigned short inds[] = {
                        1,0,2,3,0,1,4,0,3,5,0,4,6,0,5,7,0,6,8,0,7,9,0,8,10,0,9,11,0,10,12,0,11,2,0,12,13,14,15,13,15,16,13,16,17,13,17,18,13,18,19,13,19,20,13,20,21,13,21,22,13,22,23,
                        13,23,24,13,24,25,13,25,14};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_PYRAMID: {
#               ifndef TEAPOT_NO_MESH_PYRAMID
                {

                    const float verts[] = {
                        0.500000,-0.500000,0.500000,-0.500000,-0.500000,-0.500000,0.500000,-0.500000,-0.500000,-0.500000,-0.500000,0.500000,0.000000,0.500000,0.000000,-0.500000,-0.500000,-0.500000,-0.500000,-0.500000,0.500000,0.000000,0.500000,0.000000,
                        -0.500000,-0.500000,0.500000,0.500000,-0.500000,0.500000,0.000000,0.500000,0.000000,0.500000,-0.500000,-0.500000,-0.500000,-0.500000,-0.500000,0.000000,0.500000,0.000000,0.500000,-0.500000,0.500000,0.500000,-0.500000,-0.500000};

                    const unsigned short inds[] = {
                        0,1,2,3,4,5,0,6,1,7,8,9,10,11,12,13,14,15};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_ROOF: {
#               ifndef TEAPOT_NO_MESH_ROOF
                {

                    const float verts[] = {
                        0.500000,-0.500000,0.500000,-0.500000,-0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.000000,0.500000,0.500000,0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.500000,0.000000,0.500000,-0.500000,-0.500000,-0.500000,-0.500000,
                        0.000000,0.500000,-0.500000,-0.500000,-0.500000,-0.500000,0.000000,0.500000,0.500000,0.500000,-0.500000,-0.500000,0.000000,0.500000,-0.500000,-0.500000,-0.500000,0.500000,-0.500000,-0.500000,0.500000,0.000000,0.500000,0.500000,
                        0.500000,-0.500000,-0.500000,0.500000,-0.500000,0.500000};

                    const unsigned short inds[] = {
                        0,1,2,5,6,7,10,11,12,0,13,1,3,14,4,5,15,6,8,16,9,10,17,11};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_SPHERE1: {
#               ifndef TEAPOT_NO_MESH_SPHERE1
                {

                    const float verts[] = {
                        0.000000,0.000000,-0.500000,0.361804,-0.262863,-0.223610,-0.138194,-0.425325,-0.223610,-0.447213,0.000000,-0.223608,-0.138194,0.425325,-0.223610,0.361804,0.262863,-0.223610,0.138194,-0.425325,0.223610,-0.361804,-0.262863,0.223610,
                        -0.361804,0.262863,0.223610,0.138194,0.425325,0.223610,0.447213,-0.000000,0.223608,0.000000,-0.000000,0.500000,-0.081228,-0.249998,-0.425327,0.212661,-0.154506,-0.425327,0.131434,-0.404506,-0.262869,0.425324,0.000000,-0.262868,
                        0.212661,0.154506,-0.425327,-0.262865,0.000000,-0.425326,-0.344095,-0.249998,-0.262868,-0.081228,0.249998,-0.425327,-0.344095,0.249998,-0.262868,0.131434,0.404506,-0.262869,0.475529,-0.154506,-0.000000,0.475529,0.154506,0.000000,
                        0.000000,-0.500000,-0.000000,0.293893,-0.404508,-0.000000,-0.475529,-0.154506,-0.000000,-0.293893,-0.404508,-0.000000,-0.293893,0.404508,0.000000,-0.475529,0.154506,0.000000,0.293893,0.404508,0.000000,0.000000,0.500000,0.000000,
                        0.344095,-0.249998,0.262868,-0.131434,-0.404506,0.262869,-0.425324,-0.000000,0.262868,-0.131434,0.404506,0.262869,0.344095,0.249998,0.262868,0.081228,-0.249998,0.425327,0.262865,-0.000000,0.425326,-0.212661,-0.154506,0.425327,
                        -0.212661,0.154506,0.425327,0.081228,0.249998,0.425327};

                    const unsigned short inds[] = {
                        0,13,12,1,13,15,0,12,17,0,17,19,0,19,16,1,15,22,2,14,24,3,18,26,4,20,28,5,21,30,1,22,25,2,24,27,3,26,29,4,28,31,5,30,23,6,32,37,7,33,39,8,34,40,9,35,41,10,36,38,38,41,11,
                        38,36,41,36,9,41,41,40,11,41,35,40,35,8,40,40,39,11,40,34,39,34,7,39,39,37,11,39,33,37,33,6,37,37,38,11,37,32,38,32,10,38,23,36,10,23,30,36,30,9,36,31,35,9,31,28,35,28,8,35,29,34,8,
                        29,26,34,26,7,34,27,33,7,27,24,33,24,6,33,25,32,6,25,22,32,22,10,32,30,31,9,30,21,31,21,4,31,28,29,8,28,20,29,20,3,29,26,27,7,26,18,27,18,2,27,24,25,6,24,14,25,14,1,25,22,23,10,
                        22,15,23,15,5,23,16,21,5,16,19,21,19,4,21,19,20,4,19,17,20,17,3,20,17,18,3,17,12,18,12,2,18,15,16,5,15,13,16,13,0,16,12,14,2,12,13,14,13,1,14};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_SPHERE2: {
#               ifndef TEAPOT_NO_MESH_SPHERE2
                {

                    const float verts[] = {
                        0.000000,0.000000,-0.500000,0.361804,-0.262863,-0.223610,-0.138194,-0.425325,-0.223610,-0.447213,0.000000,-0.223608,-0.138194,0.425325,-0.223610,0.361804,0.262863,-0.223610,0.138194,-0.425325,0.223610,-0.361804,-0.262863,0.223610,
                        -0.361804,0.262863,0.223610,0.138194,0.425325,0.223610,0.447213,-0.000000,0.223608,0.000000,-0.000000,0.500000,-0.116411,-0.358282,-0.328760,-0.081228,-0.249998,-0.425327,-0.038803,-0.119426,-0.483975,0.101590,-0.073809,-0.483975,
                        0.212661,-0.154506,-0.425327,0.304773,-0.221428,-0.328759,0.265970,-0.340856,-0.251151,0.131434,-0.404506,-0.262869,-0.014820,-0.432092,-0.251151,0.406365,0.147619,-0.251150,0.425324,0.000000,-0.262868,0.406365,-0.147619,-0.251150,
                        0.101590,0.073809,-0.483975,0.212661,0.154506,-0.425327,0.304773,0.221428,-0.328759,-0.376721,0.000000,-0.328757,-0.262865,0.000000,-0.425326,-0.125573,0.000000,-0.483974,-0.241986,-0.358282,-0.251151,-0.344095,-0.249998,-0.262868,
                        -0.415525,-0.119427,-0.251149,-0.116411,0.358282,-0.328760,-0.081228,0.249998,-0.425327,-0.038803,0.119426,-0.483975,-0.415525,0.119427,-0.251149,-0.344095,0.249998,-0.262868,-0.241986,0.358282,-0.251151,-0.014820,0.432092,-0.251151,
                        0.131434,0.404506,-0.262869,0.265970,0.340856,-0.251151,0.478313,-0.073809,0.125575,0.475529,-0.154506,-0.000000,0.430349,-0.221429,-0.125575,0.430349,0.221429,-0.125575,0.475529,0.154506,0.000000,0.478313,0.073809,0.125575,
                        0.077608,-0.477711,0.125576,0.000000,-0.500000,-0.000000,-0.077608,-0.477711,-0.125576,0.343579,-0.340858,-0.125576,0.293893,-0.404508,-0.000000,0.218003,-0.432094,0.125576,-0.430349,-0.221429,0.125575,-0.475529,-0.154506,-0.000000,
                        -0.478313,-0.073809,-0.125575,-0.218003,-0.432094,-0.125576,-0.293893,-0.404508,-0.000000,-0.343579,-0.340858,0.125576,-0.343579,0.340858,0.125576,-0.293893,0.404508,0.000000,-0.218003,0.432094,-0.125576,-0.478313,0.073809,-0.125575,
                        -0.475529,0.154506,0.000000,-0.430349,0.221429,0.125575,0.218003,0.432094,0.125576,0.293893,0.404508,0.000000,0.343579,0.340858,-0.125576,-0.077608,0.477711,-0.125576,0.000000,0.500000,0.000000,0.077608,0.477711,0.125576,
                        0.415525,-0.119427,0.251149,0.344095,-0.249998,0.262868,0.241986,-0.358282,0.251151,0.014820,-0.432092,0.251151,-0.131434,-0.404506,0.262869,-0.265970,-0.340856,0.251151,-0.406365,-0.147619,0.251150,-0.425324,-0.000000,0.262868,
                        -0.406365,0.147619,0.251150,-0.265970,0.340856,0.251151,-0.131434,0.404506,0.262869,0.014820,0.432092,0.251151,0.241986,0.358282,0.251151,0.344095,0.249998,0.262868,0.415525,0.119427,0.251149,0.038803,-0.119426,0.483975,
                        0.081228,-0.249998,0.425327,0.116411,-0.358282,0.328760,0.376721,-0.000000,0.328757,0.262865,-0.000000,0.425326,0.125573,-0.000000,0.483974,-0.101590,-0.073809,0.483975,-0.212661,-0.154506,0.425327,-0.304773,-0.221428,0.328759,
                        -0.101590,0.073809,0.483975,-0.212661,0.154506,0.425327,-0.304773,0.221428,0.328759,0.038803,0.119426,0.483975,0.081228,0.249998,0.425327,0.116411,0.358282,0.328760,0.180900,0.131431,0.447215,0.319097,0.131432,0.361805,
                        0.223605,0.262864,0.361806,-0.069099,0.212660,0.447215,-0.026395,0.344092,0.361806,-0.180902,0.293889,0.361806,-0.223605,-0.000000,0.447215,-0.335408,0.081229,0.361805,-0.335408,-0.081229,0.361805,-0.069099,-0.212660,0.447215,
                        -0.180902,-0.293889,0.361806,-0.026395,-0.344092,0.361806,0.180900,-0.131431,0.447215,0.223605,-0.262864,0.361806,0.319097,-0.131432,0.361805,0.430902,0.212661,0.138198,0.404510,0.293891,0.000000,0.335410,0.344095,0.138199,
                        -0.069099,0.475528,0.138199,-0.154508,0.475528,-0.000000,-0.223608,0.425324,0.138199,-0.473607,0.081229,0.138198,-0.500000,-0.000000,0.000000,-0.473606,-0.081229,0.138198,-0.223608,-0.425324,0.138198,-0.154509,-0.475528,-0.000000,
                        -0.069100,-0.475528,0.138198,0.335410,-0.344095,0.138198,0.404509,-0.293891,-0.000001,0.430902,-0.212662,0.138197,0.154509,0.475528,0.000000,0.223608,0.425324,-0.138199,0.069100,0.475527,-0.138199,-0.404509,0.293892,0.000000,
                        -0.335409,0.344095,-0.138199,-0.430902,0.212662,-0.138198,-0.404509,-0.293892,-0.000000,-0.430902,-0.212662,-0.138198,-0.335410,-0.344095,-0.138199,0.154509,-0.475528,-0.000000,0.069100,-0.475527,-0.138199,0.223608,-0.425324,-0.138199,
                        0.500000,0.000000,0.000000,0.473607,-0.081229,-0.138198,0.473607,0.081229,-0.138198,0.180902,0.293890,-0.361806,0.069098,0.212661,-0.447215,0.026395,0.344093,-0.361805,-0.223605,0.262864,-0.361806,-0.180901,0.131431,-0.447214,
                        -0.319097,0.131431,-0.361805,-0.319097,-0.131432,-0.361805,-0.180901,-0.131432,-0.447214,-0.223605,-0.262864,-0.361805,0.335409,0.081228,-0.361805,0.335409,-0.081229,-0.361805,0.223605,-0.000000,-0.447214,0.026395,-0.344093,-0.361806,
                        0.069099,-0.212660,-0.447215,0.180902,-0.293890,-0.361805};

                    const unsigned short inds[] = {
                        0,15,14,1,17,23,0,14,29,0,29,35,0,35,24,1,23,44,2,20,50,3,32,56,4,38,62,5,41,68,1,44,51,2,50,57,3,56,63,4,62,69,5,68,45,6,74,89,7,77,95,8,80,98,9,83,101,10,86,90,92,99,11,
                        91,102,92,90,103,91,92,102,99,102,100,99,91,103,102,103,104,102,102,104,100,104,101,100,90,86,103,86,85,103,103,85,104,85,84,104,104,84,101,84,9,101,99,96,11,100,105,99,101,106,100,99,105,96,105,97,96,100,106,105,106,107,105,
                        105,107,97,107,98,97,101,83,106,83,82,106,106,82,107,82,81,107,107,81,98,81,8,98,96,93,11,97,108,96,98,109,97,96,108,93,108,94,93,97,109,108,109,110,108,108,110,94,110,95,94,98,80,109,80,79,109,109,79,110,79,78,110,
                        110,78,95,78,7,95,93,87,11,94,111,93,95,112,94,93,111,87,111,88,87,94,112,111,112,113,111,111,113,88,113,89,88,95,77,112,77,76,112,112,76,113,76,75,113,113,75,89,75,6,89,87,92,11,88,114,87,89,115,88,87,114,92,
                        114,91,92,88,115,114,115,116,114,114,116,91,116,90,91,89,74,115,74,73,115,115,73,116,73,72,116,116,72,90,72,10,90,47,86,10,46,117,47,45,118,46,47,117,86,117,85,86,46,118,117,118,119,117,117,119,85,119,84,85,45,68,118,
                        68,67,118,118,67,119,67,66,119,119,66,84,66,9,84,71,83,9,70,120,71,69,121,70,71,120,83,120,82,83,70,121,120,121,122,120,120,122,82,122,81,82,69,62,121,62,61,121,121,61,122,61,60,122,122,60,81,60,8,81,65,80,8,
                        64,123,65,63,124,64,65,123,80,123,79,80,64,124,123,124,125,123,123,125,79,125,78,79,63,56,124,56,55,124,124,55,125,55,54,125,125,54,78,54,7,78,59,77,7,58,126,59,57,127,58,59,126,77,126,76,77,58,127,126,127,128,126,
                        126,128,76,128,75,76,57,50,127,50,49,127,127,49,128,49,48,128,128,48,75,48,6,75,53,74,6,52,129,53,51,130,52,53,129,74,129,73,74,52,130,129,130,131,129,129,131,73,131,72,73,51,44,130,44,43,130,130,43,131,43,42,131,
                        131,42,72,42,10,72,66,71,9,67,132,66,68,133,67,66,132,71,132,70,71,67,133,132,133,134,132,132,134,70,134,69,70,68,41,133,41,40,133,133,40,134,40,39,134,134,39,69,39,4,69,60,65,8,61,135,60,62,136,61,60,135,65,
                        135,64,65,61,136,135,136,137,135,135,137,64,137,63,64,62,38,136,38,37,136,136,37,137,37,36,137,137,36,63,36,3,63,54,59,7,55,138,54,56,139,55,54,138,59,138,58,59,55,139,138,139,140,138,138,140,58,140,57,58,56,32,139,
                        32,31,139,139,31,140,31,30,140,140,30,57,30,2,57,48,53,6,49,141,48,50,142,49,48,141,53,141,52,53,49,142,141,142,143,141,141,143,52,143,51,52,50,20,142,20,19,142,142,19,143,19,18,143,143,18,51,18,1,51,42,47,10,
                        43,144,42,44,145,43,42,144,47,144,46,47,43,145,144,145,146,144,144,146,46,146,45,46,44,23,145,23,22,145,145,22,146,22,21,146,146,21,45,21,5,45,26,41,5,25,147,26,24,148,25,26,147,41,147,40,41,25,148,147,148,149,147,
                        147,149,40,149,39,40,24,35,148,35,34,148,148,34,149,34,33,149,149,33,39,33,4,39,33,38,4,34,150,33,35,151,34,33,150,38,150,37,38,34,151,150,151,152,150,150,152,37,152,36,37,35,29,151,29,28,151,151,28,152,28,27,152,
                        152,27,36,27,3,36,27,32,3,28,153,27,29,154,28,27,153,32,153,31,32,28,154,153,154,155,153,153,155,31,155,30,31,29,14,154,14,13,154,154,13,155,13,12,155,155,12,30,12,2,30,21,26,5,22,156,21,23,157,22,21,156,26,
                        156,25,26,22,157,156,157,158,156,156,158,25,158,24,25,23,17,157,17,16,157,157,16,158,16,15,158,158,15,24,15,0,24,12,20,2,13,159,12,14,160,13,12,159,20,159,19,20,13,160,159,160,161,159,159,161,19,161,18,19,14,15,160,
                        15,16,160,160,16,161,16,17,161,161,17,18,17,1,18};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_CYLINDER_LATERAL_SURFACE: {
#               ifndef TEAPOT_NO_MESH_CYLINDER
                {
                    TIS.startInds[i] = TIS.startInds[TEAPOT_MESH_CYLINDER]+24;
                    TIS.numInds[i] = TIS.numInds[TEAPOT_MESH_CYLINDER]-24;
                    for (j=0;j<3;j++) {
                        TIS.halfExtents[i][j] = TIS.halfExtents[TEAPOT_MESH_CYLINDER][j];
                        TIS.centerPoint[i][j] = TIS.centerPoint[TEAPOT_MESH_CYLINDER][j];
                    }
                }
#               endif
            }
                break;
            case TEAPOT_MESH_HALF_SPHERE_UP: {
#               ifndef TEAPOT_MESH_HALF_SPHERE_UP
                {

                    const float verts[] = {
                        0.040667,0.461940,-0.191323,0.075143,0.353553,-0.353519,0.098179,0.191342,-0.461895,0.106268,-0.000000,-0.499951,-0.000000,0.500000,-0.000000,0.119652,0.461940,-0.154731,0.221087,0.353553,-0.285906,0.288864,0.191342,-0.373554,
                        0.312665,-0.000000,-0.404332,0.174938,0.461940,-0.087493,0.323243,0.353553,-0.161666,0.422337,0.191342,-0.211227,0.457134,-0.000000,-0.228631,0.195575,0.461940,-0.002926,0.361376,0.353553,-0.005407,0.472161,0.191342,-0.007064,
                        0.511063,-0.000000,-0.007646,0.177477,0.461940,0.082221,0.327934,0.353553,0.151924,0.428467,0.191342,0.198498,0.463769,-0.000000,0.214853,0.124227,0.461940,0.151083,0.229541,0.353553,0.279164,0.299910,0.191342,0.364746,
                        0.324620,-0.000000,0.394798,0.046372,0.461940,0.190021,0.085685,0.353553,0.351112,0.111952,0.191342,0.458751,0.121176,-0.000000,0.496548,-0.040667,0.461940,0.191323,-0.075143,0.353553,0.353519,-0.098179,0.191342,0.461894,
                        -0.106268,-0.000000,0.499951,-0.119652,0.461940,0.154731,-0.221087,0.353553,0.285906,-0.288865,0.191342,0.373554,-0.312665,-0.000000,0.404332,-0.174938,0.461940,0.087493,-0.323243,0.353553,0.161666,-0.422337,0.191342,0.211227,
                        -0.457134,-0.000000,0.228631,-0.195575,0.461940,0.002926,-0.361376,0.353553,0.005406,-0.472161,0.191342,0.007064,-0.511063,-0.000000,0.007646,-0.177477,0.461940,-0.082221,-0.327934,0.353553,-0.151924,-0.428467,0.191342,-0.198499,
                        -0.463769,-0.000000,-0.214853,-0.124227,0.461940,-0.151083,-0.229541,0.353553,-0.279164,-0.299910,0.191342,-0.364746,-0.324620,-0.000000,-0.394798,-0.046372,0.461940,-0.190021,-0.085685,0.353553,-0.351113,-0.111952,0.191342,-0.458751,
                        -0.121176,-0.000000,-0.496548};

                    const unsigned short inds[] = {
                        1,7,2,0,4,5,2,8,3,1,5,6,5,4,9,7,12,8,5,10,6,6,11,7,11,16,12,10,13,14,10,15,11,9,4,13,14,19,15,13,4,17,15,20,16,13,18,14,18,23,19,17,4,21,19,24,20,17,22,18,24,27,28,
                        22,25,26,22,27,23,21,4,25,28,31,32,25,30,26,26,31,27,25,4,29,31,34,35,29,4,33,32,35,36,30,33,34,34,39,35,33,4,37,36,39,40,33,38,34,39,44,40,38,41,42,38,43,39,37,4,41,42,45,46,42,47,43,
                        41,4,45,43,48,44,46,51,47,45,4,49,47,52,48,45,50,46,49,4,53,51,56,52,50,53,54,50,55,51,56,2,3,53,1,54,54,2,55,53,4,0,1,6,7,2,7,8,1,0,5,7,11,12,5,9,10,6,10,11,11,15,16,
                        10,9,13,10,14,15,14,18,19,15,19,20,13,17,18,18,22,23,19,23,24,17,21,22,24,23,27,22,21,25,22,26,27,28,27,31,25,29,30,26,30,31,31,30,34,32,31,35,30,29,33,34,38,39,36,35,39,33,37,38,39,43,44,
                        38,37,41,38,42,43,42,41,45,42,46,47,43,47,48,46,50,51,47,51,52,45,49,50,51,55,56,50,49,53,50,54,55,56,55,2,53,0,1,54,1,2};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            case TEAPOT_MESH_HALF_SPHERE_DOWN: {
#               ifndef TEAPOT_MESH_HALF_SPHERE_DOWN
                {

                    const float verts[] = {
                        -0.040667,-0.461940,-0.191323,-0.075143,-0.353553,-0.353519,-0.098179,-0.191342,-0.461895,-0.106268,0.000000,-0.499951,-0.000000,-0.500000,-0.000000,-0.119652,-0.461940,-0.154731,-0.221087,-0.353553,-0.285906,-0.288865,-0.191342,-0.373554,
                        -0.312665,0.000000,-0.404332,-0.174938,-0.461940,-0.087493,-0.323243,-0.353553,-0.161666,-0.422337,-0.191342,-0.211227,-0.457134,0.000000,-0.228631,-0.195575,-0.461940,-0.002926,-0.361376,-0.353553,-0.005407,-0.472161,-0.191342,-0.007064,
                        -0.511063,0.000000,-0.007646,-0.177477,-0.461940,0.082221,-0.327935,-0.353553,0.151924,-0.428467,-0.191342,0.198498,-0.463769,0.000000,0.214853,-0.124227,-0.461940,0.151083,-0.229541,-0.353553,0.279164,-0.299910,-0.191342,0.364746,
                        -0.324620,0.000000,0.394798,-0.046372,-0.461940,0.190021,-0.085685,-0.353553,0.351112,-0.111953,-0.191342,0.458751,-0.121176,0.000000,0.496548,0.040667,-0.461940,0.191323,0.075143,-0.353553,0.353519,0.098179,-0.191342,0.461894,
                        0.106268,-0.000000,0.499951,0.119651,-0.461940,0.154731,0.221087,-0.353553,0.285906,0.288864,-0.191342,0.373554,0.312665,-0.000000,0.404332,0.174938,-0.461940,0.087493,0.323243,-0.353554,0.161666,0.422337,-0.191342,0.211227,
                        0.457134,-0.000000,0.228631,0.195575,-0.461940,0.002926,0.361376,-0.353554,0.005406,0.472161,-0.191342,0.007064,0.511063,-0.000000,0.007646,0.177477,-0.461940,-0.082221,0.327934,-0.353554,-0.151924,0.428467,-0.191342,-0.198499,
                        0.463769,-0.000000,-0.214853,0.124227,-0.461940,-0.151083,0.229541,-0.353553,-0.279164,0.299910,-0.191342,-0.364746,0.324620,-0.000000,-0.394798,0.046372,-0.461940,-0.190021,0.085684,-0.353553,-0.351113,0.111952,-0.191342,-0.458751,
                        0.121176,-0.000000,-0.496548};

                    const unsigned short inds[] = {
                        1,7,2,0,4,5,2,8,3,1,5,6,5,4,9,7,12,8,5,10,6,6,11,7,11,16,12,10,13,14,10,15,11,9,4,13,14,19,15,13,4,17,15,20,16,13,18,14,18,23,19,17,4,21,19,24,20,17,22,18,24,27,28,
                        22,25,26,22,27,23,21,4,25,28,31,32,25,30,26,26,31,27,25,4,29,31,34,35,29,4,33,32,35,36,30,33,34,34,39,35,33,4,37,36,39,40,33,38,34,39,44,40,38,41,42,38,43,39,37,4,41,42,45,46,42,47,43,
                        41,4,45,43,48,44,46,51,47,45,4,49,47,52,48,45,50,46,49,4,53,51,56,52,50,53,54,50,55,51,56,2,3,53,1,54,54,2,55,53,4,0,1,6,7,2,7,8,1,0,5,7,11,12,5,9,10,6,10,11,11,15,16,
                        10,9,13,10,14,15,14,18,19,15,19,20,13,17,18,18,22,23,19,23,24,17,21,22,24,23,27,22,21,25,22,26,27,28,27,31,25,29,30,26,30,31,31,30,34,32,31,35,30,29,33,34,38,39,36,35,39,33,37,38,39,43,44,
                        38,37,41,38,42,43,42,41,45,42,46,47,43,47,48,46,50,51,47,51,52,45,49,50,51,55,56,50,49,53,50,54,55,56,55,2,53,0,1,54,1,2};

                    AddMeshVertsAndInds(totVerts,MAX_TOTAL_VERTS,&numTotVerts,totInds,MAX_TOTAL_INDS,&numTotInds,verts,sizeof(verts)/(sizeof(verts[0])*3),inds,sizeof(inds)/sizeof(inds[0]),i);
                }
#               endif
            }
                break;
            }
        }

        printf("Teapot_init(): numTotVerts = %d numTotInds = %d",numTotVerts,numTotInds);

        CalculateVertexNormals(totVerts,numTotVerts,totInds,numTotInds,norms);

        glGenBuffers(1, &TIS.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, TIS.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*numTotVerts, totVerts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &TIS.normalBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, TIS.normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*numTotVerts, norms, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &TIS.elementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TIS.elementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short)*numTotInds, totInds, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

}


#endif //TEAPOT_IMPLEMENTATION
