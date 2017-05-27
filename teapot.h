#ifndef TEAPOT_H_
#define TEAPOT_H_

/* LICENSE: Made by @Flix01. MIT license on my part (but I don't know the license of the original Teapot mesh data) */

/* USAGE:
Please see the comments of the functions below and try to follow their order when you call them.
Define TEAPOT_IMPLEMENTATION in one of your .c files before the inclusion of this file.
*/

void Teapot_Init(void);     // In your InitGL() method
void Teapot_Destroy(void);  // In your DestroyGL() method (cleanup)

// In your DrawGL() method:

void Teapot_SetLightDirection(const float lightDirectionViewSpace[3]);    // Sets the directional light in view (=camera) space [Warning: it calls glUseProgram(0); at the end => call it outside Teapot_PreDraw()/Teapot_PostDraw()]

void Teapot_PreDraw(void);  // sets program and buffers for drawing

void Teapot_SetScaling(float scalingX,float scalingY,float scalingZ);   // Optional (last set is used)
void Teapot_SetColor(float R, float G, float B, float A);       // Optional (last set is used)
void Teapot_Draw(const float mvMatrix[16],const float mvpMatrix[16]);   // Hp) mvMatrix CAN'T HAVE any scaling! (optionally repeat this call for multiple draws)

void Teapot_PostDraw(void); // unsets program and buffers for drawing

#endif //TEAPOT_H_

#define TEAPOT_IMPLEMENTATION
#ifdef TEAPOT_IMPLEMENTATION

static const char TeapotVS[] =
        "#ifdef GL_ES\n"\
        "precision highp float;\n"\
        "#endif\n"\
        "attribute vec4 a_vertex;\n"\
        "attribute vec3 a_normal;\n"\
        "uniform mat4 u_mvMatrix;\n"\
        "uniform mat4 u_mvpMatrix;\n"\
        "uniform vec4 u_scaling;\n"\
        "uniform vec3 u_lightVector;\n"\
        "uniform vec4 u_color;\n"\
        "varying vec4 v_color;\n"\
        "\n"\
        "void main()	{\n"\
        "   vec3 modelViewNormal = vec3(u_mvMatrix * vec4(a_normal, 0.0));\n"\
        "   float fDot = max(0.2, dot(modelViewNormal,u_lightVector));\n"\
        "   v_color = vec4(u_color.rgb * fDot,u_color.a);\n"\
        "   gl_Position = u_mvpMatrix * (a_vertex * u_scaling);\n"\
        "}\n";
// 0.2 is the global ambient factor

static const char TeapotFS[] =
        "#ifdef GL_ES\n"\
        "precision mediump float;\n"\
        "#endif\n"\
        "varying vec4 v_color;\n"\
        "\n"\
        "void main() {\n"\
        "    gl_FragColor = v_color;\n"\
        "}\n";

typedef struct {
    GLuint programId;
    GLuint vertexBuffer,normalBuffer,elementBuffer;
    int numInds;

    GLint aLoc_vertex,aLoc_normal;
    GLint uLoc_mvMatrix,uLoc_mvpMatrix,uLoc_scaling,uLoc_lightVector,uLoc_color;
} Teapot_Inner_Struct;
static Teapot_Inner_Struct TIS;


static inline GLuint Teapot_LoadShaderProgramFromSource(const char* vs,const char* fs);

void Teapot_SetLightDirection(const float lightDirectionViewSpace[3])   {
    glUseProgram(TIS.programId);
    glUniform3fv(TIS.uLoc_lightVector,1,lightDirectionViewSpace);
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
void Teapot_Draw(const float mvMatrix[16],const float mvpMatrix[16])    {
    glUniformMatrix4fv(TIS.uLoc_mvMatrix, 1 /*only setting 1 matrix*/, GL_FALSE /*transpose?*/, mvMatrix);
    glUniformMatrix4fv(TIS.uLoc_mvpMatrix, 1 /*only setting 1 matrix*/, GL_FALSE /*transpose?*/, mvpMatrix);

    glDrawElements(GL_TRIANGLES,TIS.numInds,GL_UNSIGNED_SHORT,0);
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

void Teapot_Init(void) {

    TIS.vertexBuffer = TIS.normalBuffer = TIS.elementBuffer = TIS.numInds = 0;

    TIS.programId =  Teapot_LoadShaderProgramFromSource(TeapotVS,TeapotFS);
    if (!TIS.programId) return;

    TIS.aLoc_vertex = glGetAttribLocation(TIS.programId, "a_vertex");
    TIS.aLoc_normal = glGetAttribLocation(TIS.programId, "a_normal");
    TIS.uLoc_mvMatrix = glGetUniformLocation(TIS.programId,"u_mvMatrix");
    TIS.uLoc_mvpMatrix = glGetUniformLocation(TIS.programId,"u_mvpMatrix");
    TIS.uLoc_scaling = glGetUniformLocation(TIS.programId,"u_scaling");
    TIS.uLoc_lightVector = glGetUniformLocation(TIS.programId,"u_lightVector");
    TIS.uLoc_color = glGetUniformLocation(TIS.programId,"u_color");

    /*
    if (TIS.aLoc_vertex<0) printf("Error: TIS.aLoc_vertex not found\n");
    //else printf("TIS.aLoc_vertex = %d\n",TIS.aLoc_vertex);
    if (TIS.aLoc_normal<0) printf("Error: TIS.aLoc_normal not found\n");
    //else printf("TIS.aLoc_normal = %d\n",TIS.aLoc_normal);
    if (TIS.uLoc_mvMatrix<0) printf("Error: TIS.uLoc_mvMatrix not found\n");
    if (TIS.uLoc_mvpMatrix<0) printf("Error: TIS.uLoc_mvpMatrix not found\n");
    if (TIS.uLoc_scaling<0) printf("Error: TIS.uLoc_scaling not found\n");
    if (TIS.uLoc_lightVector<0) printf("Error: TIS.uLoc_lightVector not found\n");
    if (TIS.uLoc_color<0) printf("Error: TIS.uLoc_color not found\n");
    */

    {
        const float one[4]={1.f,1.f,1.f,1.f};
        Teapot_SetLightDirection(one);
        glUseProgram(TIS.programId);
        Teapot_SetScaling(1,1,1);
        Teapot_SetColor(1,1,1,1);
        glUseProgram(0);
    }

    {
        const float verts[] = {
            0.041244,0.722391,0.000331,0.000000,0.724815,0.000331,0.030952,0.722391,0.031283,-0.041244,0.722391,0.000331,-0.030952,0.722391,0.031283
            ,0.000000,0.722391,-0.040913,-0.030952,0.722391,-0.030621,0.000000,0.722391,0.041575,0.000000,0.544623,0.350330,0.135459,0.562613,0.318702
            ,0.137375,0.544623,0.323205,0.245034,0.562613,0.245364,0.248500,0.544623,0.248831,0.322875,0.544623,0.137706,0.318373,0.562613,0.135790
            ,0.350000,0.544623,0.000330,0.000000,0.568610,0.351111,0.000000,0.562613,0.345448,0.137682,0.568610,0.323927,0.323599,0.568610,0.138013
            ,0.350785,0.568610,0.000331,0.345120,0.562613,0.000330,0.000000,0.562613,0.362635,0.142205,0.562613,0.334557,0.249056,0.568610,0.249386
            ,0.334229,0.562613,0.142536,0.362308,0.562613,0.000331,0.000000,0.544623,0.375331,0.147187,0.544623,0.346268,0.257238,0.562613,0.257567
            ,0.345937,0.544623,0.147517,0.375000,0.544623,0.000330,0.318373,0.562613,-0.135127,0.322875,0.544623,-0.137044,0.245034,0.562613,-0.244702
            ,0.248500,0.544623,-0.248169,0.137375,0.544623,-0.322545,0.135459,0.562613,-0.318040,0.000000,0.544623,-0.349669,0.323599,0.568610,-0.137351
            ,0.137682,0.568610,-0.323265,0.000000,0.568610,-0.350450,0.000000,0.562613,-0.344786,0.334229,0.562613,-0.141874,0.249056,0.568610,-0.248724
            ,0.142205,0.562613,-0.333896,0.000000,0.562613,-0.361974,0.345937,0.544623,-0.146857,0.257238,0.562613,-0.256906,0.147187,0.544623,-0.345607
            ,0.000000,0.544623,-0.374669,-0.135458,0.562613,-0.318040,-0.137375,0.544623,-0.322544,-0.245033,0.562613,-0.244703,-0.248500,0.544623,-0.248170
            ,-0.322875,0.544623,-0.137044,-0.318371,0.562613,-0.135127,-0.350000,0.544623,0.000331,-0.137681,0.568610,-0.323265,-0.323595,0.568610,-0.137351
            ,-0.350780,0.568610,0.000331,-0.345118,0.562613,0.000331,-0.142204,0.562613,-0.333896,-0.249054,0.568610,-0.248725,-0.334224,0.562613,-0.141874
            ,-0.362303,0.562613,0.000331,-0.147187,0.544623,-0.345607,-0.257235,0.562613,-0.256906,-0.345937,0.544623,-0.146857,-0.375000,0.544623,0.000331
            ,-0.318371,0.562613,0.135790,-0.322875,0.544623,0.137706,-0.245033,0.562613,0.245364,-0.248500,0.544623,0.248831,-0.137375,0.544623,0.323205
            ,-0.135458,0.562613,0.318702,-0.323595,0.568610,0.138013,-0.137681,0.568610,0.323927,-0.334224,0.562613,0.142536,-0.249054,0.568610,0.249386
            ,-0.142204,0.562613,0.334557,-0.345937,0.544623,0.147518,-0.257235,0.562613,0.257567,-0.147187,0.544623,0.346268,0.000000,0.446067,0.421228
            ,0.165203,0.446067,0.388609,0.266250,0.544623,0.266580,0.388279,0.446067,0.165534,0.420898,0.446067,0.000331,0.000000,0.349616,0.461269
            ,0.180918,0.349616,0.425546,0.298838,0.446067,0.299169,0.425215,0.349616,0.181249,0.460938,0.349616,0.000330,0.000000,0.256919,0.489588
            ,0.192034,0.256919,0.451670,0.327266,0.349616,0.327597,0.451340,0.256919,0.192365,0.489258,0.256919,0.000330,0.000000,0.169623,0.500330
            ,0.196250,0.169623,0.461580,0.347373,0.256919,0.347703,0.461250,0.169623,0.196581,0.500000,0.169623,0.000330,0.388279,0.446067,-0.164873
            ,0.266250,0.544623,-0.265920,0.165203,0.446067,-0.387949,0.000000,0.446067,-0.420567,0.425215,0.349616,-0.180587,0.298838,0.446067,-0.298507
            ,0.180918,0.349616,-0.424885,0.000000,0.349616,-0.460608,0.451340,0.256919,-0.191703,0.327266,0.349616,-0.326936,0.192034,0.256919,-0.451009
            ,0.000000,0.256919,-0.488928,0.461250,0.169623,-0.195920,0.347373,0.256919,-0.347043,0.196250,0.169623,-0.460920,0.000000,0.169623,-0.499670
            ,-0.165203,0.446067,-0.387948,-0.266250,0.544623,-0.265920,-0.388279,0.446067,-0.164872,-0.420898,0.446067,0.000331,-0.180918,0.349616,-0.424885
            ,-0.298838,0.446067,-0.298508,-0.425215,0.349616,-0.180587,-0.460938,0.349616,0.000331,-0.192034,0.256919,-0.451009,-0.327266,0.349616,-0.326936
            ,-0.451340,0.256919,-0.191703,-0.489258,0.256919,0.000331,-0.196250,0.169623,-0.460920,-0.347373,0.256919,-0.347043,-0.461250,0.169623,-0.195919
            ,-0.500000,0.169623,0.000331,-0.388279,0.446067,0.165534,-0.266250,0.544623,0.266581,-0.165203,0.446067,0.388609,-0.425215,0.349616,0.181249
            ,-0.298838,0.446067,0.299168,-0.180918,0.349616,0.425546,-0.451340,0.256919,0.192365,-0.327266,0.349616,0.327597,-0.192034,0.256919,0.451670
            ,-0.461250,0.169623,0.196581,-0.347373,0.256919,0.347704,-0.196250,0.169623,0.461580,0.188584,0.095240,0.443562,0.341133,0.095240,0.341464
            ,0.355000,0.169623,0.355330,0.443233,0.095240,0.188914,0.000000,0.095240,0.480799,0.171719,0.040381,0.403924,0.310625,0.040381,0.310955
            ,0.403594,0.040381,0.172050,0.480469,0.095240,0.000330,0.000000,0.040381,0.437831,0.154853,0.001881,0.364285,0.280117,0.001881,0.280447
            ,0.363955,0.001881,0.155184,0.437500,0.040381,0.000331,0.000000,0.001881,0.394862,0.000000,0.001881,0.000330,0.394531,0.001881,0.000331
            ,0.443233,0.095240,-0.188253,0.341133,0.095240,-0.340803,0.355000,0.169623,-0.354669,0.188584,0.095240,-0.442902,0.403594,0.040381,-0.171388
            ,0.310625,0.040381,-0.310295,0.171719,0.040381,-0.403264,0.000000,0.095240,-0.480139,0.363955,0.001881,-0.154523,0.280117,0.001881,-0.279787
            ,0.154853,0.001881,-0.363625,0.000000,0.040381,-0.437169,0.000000,0.001881,-0.394201,-0.188584,0.095240,-0.442901,-0.341133,0.095240,-0.340802
            ,-0.355000,0.169623,-0.354669,-0.443233,0.095240,-0.188253,-0.171719,0.040381,-0.403263,-0.310625,0.040381,-0.310295,-0.403594,0.040381,-0.171388
            ,-0.480469,0.095240,0.000331,-0.154853,0.001881,-0.363624,-0.280117,0.001881,-0.279787,-0.363955,0.001881,-0.154523,-0.437500,0.040381,0.000331
            ,-0.394531,0.001881,0.000331,-0.443233,0.095240,0.188915,-0.341133,0.095240,0.341464,-0.355000,0.169623,0.355330,-0.188584,0.095240,0.443563
            ,-0.403594,0.040381,0.172050,-0.310625,0.040381,0.310955,-0.171719,0.040381,0.403924,-0.363955,0.001881,0.155184,-0.280117,0.001881,0.280447
            ,-0.154853,0.001881,0.364285,0.000000,0.450629,-0.399670,0.042188,0.458351,-0.519946,0.042188,0.459365,-0.395763,0.056251,0.477269,-0.527600
            ,0.056251,0.478583,-0.387169,0.042188,0.496187,-0.535253,0.042188,0.497801,-0.378576,0.000000,0.504786,-0.538732,0.000000,0.506537,-0.374669
            ,0.000000,0.443610,-0.602795,0.000000,0.449752,-0.516467,0.042188,0.451254,-0.611096,0.056251,0.468070,-0.629358,0.042188,0.484886,-0.647619
            ,0.000000,0.426940,-0.656311,0.042188,0.431990,-0.667236,0.056251,0.443101,-0.691272,0.042188,0.454211,-0.715307,0.000000,0.492529,-0.655920
            ,0.000000,0.394477,-0.674670,0.042188,0.394477,-0.686389,0.056251,0.394477,-0.712170,0.042188,0.394477,-0.737951,0.000000,0.459262,-0.726232
            ,-0.042188,0.497801,-0.378576,-0.042188,0.496187,-0.535253,-0.056251,0.478583,-0.387169,-0.056251,0.477269,-0.527600,-0.042188,0.459365,-0.395763
            ,-0.042188,0.458351,-0.519946,-0.042188,0.484886,-0.647619,-0.056251,0.468070,-0.629358,-0.042188,0.451254,-0.611096,-0.042188,0.454211,-0.715307
            ,-0.056251,0.443101,-0.691272,-0.042188,0.431990,-0.667236,-0.042188,0.394477,-0.737951,-0.056251,0.394477,-0.712170,-0.042188,0.394477,-0.686389
            ,0.000000,0.343207,-0.664904,0.042188,0.338309,-0.675555,0.056251,0.327533,-0.698986,0.042188,0.316756,-0.722418,0.000000,0.394477,-0.749670
            ,0.000000,0.281684,-0.634045,0.042188,0.274197,-0.641613,0.056251,0.257727,-0.658264,0.042188,0.241257,-0.674913,0.000000,0.311858,-0.733068
            ,0.000000,0.220343,-0.579748,0.042188,0.211084,-0.582403,0.056251,0.190714,-0.588244,0.042188,0.170345,-0.594085,0.000000,0.233771,-0.682482
            ,0.042188,0.157912,-0.483402,0.056251,0.132147,-0.474809,0.042188,0.106383,-0.466215,0.000000,0.161086,-0.596740,-0.042188,0.316756,-0.722418
            ,-0.056251,0.327533,-0.698986,-0.042188,0.338309,-0.675555,-0.042188,0.241257,-0.674913,-0.056251,0.257727,-0.658264,-0.042188,0.274197,-0.641613
            ,-0.042188,0.170345,-0.594085,-0.056251,0.190714,-0.588244,-0.042188,0.211084,-0.582403,-0.042188,0.106383,-0.466215,-0.056251,0.132147,-0.474808
            ,-0.042188,0.157912,-0.483401,0.000000,0.171168,-0.489748,0.000000,0.300482,0.425330,0.083804,0.304223,0.554410,0.092813,0.268324,0.425330
            ,0.111739,0.251717,0.574550,0.123751,0.197577,0.425330,0.083804,0.199211,0.594690,0.092813,0.126830,0.425330,0.000000,0.175345,0.603850
            ,0.000000,0.094672,0.425330,0.000000,0.328089,0.545251,0.063985,0.380527,0.608920,0.085313,0.349906,0.634701,0.063985,0.319284,0.660490
            ,0.000000,0.305365,0.672210,0.000000,0.394446,0.597200,0.044165,0.469804,0.640000,0.058887,0.458691,0.671420,0.044165,0.447578,0.702840
            ,0.000000,0.442526,0.717130,0.000000,0.544623,0.675330,0.000000,0.474856,0.625720,0.035156,0.544623,0.698770,0.046875,0.544623,0.750330
            ,0.035156,0.544623,0.801890,-0.092813,0.126830,0.425330,-0.083804,0.199211,0.594690,-0.123751,0.197577,0.425330,-0.111739,0.251717,0.574550
            ,-0.092813,0.268324,0.425330,-0.083804,0.304223,0.554410,-0.063985,0.319284,0.660490,-0.085313,0.349906,0.634701,-0.063985,0.380527,0.608920
            ,-0.044165,0.447578,0.702840,-0.058887,0.458691,0.671420,-0.044165,0.469804,0.640000,-0.035156,0.544623,0.801890,-0.046875,0.544623,0.750330
            ,-0.035156,0.544623,0.698770,0.000000,0.554786,0.693300,0.032959,0.555327,0.718400,0.043945,0.556519,0.773620,0.032959,0.557711,0.828840
            ,0.000000,0.544623,0.825330,0.000000,0.558173,0.706580,0.028125,0.559046,0.730140,0.037500,0.560965,0.781970,0.028125,0.562885,0.833800
            ,0.000000,0.558253,0.853940,0.023291,0.555553,0.730490,0.031055,0.557240,0.774500,0.023291,0.558928,0.818510
            ,0.000000,0.559695,0.838510,0.000000,0.563757,0.857360,0.000000,0.554786,0.710490,0.000000,0.444623,0.678909,-0.032959,0.557711,0.828840
            ,-0.043945,0.556519,0.773620,-0.032959,0.555327,0.718400,-0.028125,0.562885,0.833800,-0.037500,0.560965,0.781970,-0.028125,0.559046,0.730140
            ,-0.023291,0.558928,0.818510,-0.031055,0.557240,0.774500,-0.023291,0.555553,0.730490,0.000000,0.719346,0.085486,0.031970,0.689516,0.075309
            ,0.033513,0.719346,0.078917,0.057758,0.689516,0.058089,0.060540,0.719346,0.060871,0.078587,0.719346,0.033844,0.074980,0.689516,0.032301
            ,0.085157,0.719346,0.000331,0.000000,0.689516,0.081581,0.019348,0.652739,0.045745,0.034971,0.652739,0.035303,0.045414,0.652739,0.019679
            ,0.081250,0.689516,0.000331,0.000000,0.652739,0.049550,0.019625,0.619574,0.046456,0.035500,0.619574,0.035831,0.046124,0.619574,0.019956
            ,0.049219,0.652739,0.000331,0.074980,0.689516,-0.031639,0.078587,0.719346,-0.033182,0.057758,0.689516,-0.057427,0.060540,0.719346,-0.060209
            ,0.033513,0.719346,-0.078255,0.031970,0.689516,-0.074649,0.000000,0.719346,-0.084826,0.045414,0.652739,-0.019017,0.034971,0.652739,-0.034641
            ,0.019348,0.652739,-0.045083,0.000000,0.689516,-0.080919,0.046124,0.619574,-0.019294,0.035500,0.619574,-0.035169,0.019625,0.619574,-0.045794
            ,0.000000,0.652739,-0.048888,-0.031970,0.689516,-0.074649,-0.033513,0.719346,-0.078255,-0.057758,0.689516,-0.057427,-0.060540,0.719346,-0.060209
            ,-0.078587,0.719346,-0.033182,-0.074980,0.689516,-0.031639,-0.085157,0.719346,0.000331,-0.019348,0.652739,-0.045083,-0.034971,0.652739,-0.034641
            ,-0.045414,0.652739,-0.019017,-0.081250,0.689516,0.000331,-0.019625,0.619574,-0.045794,-0.035500,0.619574,-0.035169,-0.046124,0.619574,-0.019294
            ,-0.049218,0.652739,0.000331,-0.074980,0.689516,0.032301,-0.078587,0.719346,0.033844,-0.057758,0.689516,0.058089,-0.060540,0.719346,0.060871
            ,-0.033513,0.719346,0.078917,-0.031970,0.689516,0.075309,-0.045414,0.652739,0.019679,-0.034971,0.652739,0.035303,-0.019348,0.652739,0.045745
            ,-0.046124,0.619574,0.019956,-0.035500,0.619574,0.035831,-0.019625,0.619574,0.046456,0.000000,0.596980,0.114393,0.000000,0.619574,0.050331
            ,0.044769,0.596980,0.105554,0.105222,0.596980,0.045101,0.114062,0.596980,0.000331,0.049999,0.619574,0.000331,0.000000,0.581641,0.206581
            ,0.080953,0.581641,0.190597,0.080984,0.596980,0.081315,0.190266,0.581641,0.081284,0.206250,0.581641,0.000331,0.000000,0.566531,0.289392
            ,0.113457,0.566531,0.266990,0.146438,0.581641,0.146768,0.266661,0.566531,0.113788,0.289063,0.566531,0.000330,0.000000,0.544623,0.325330
            ,0.127562,0.544623,0.300142,0.205234,0.566531,0.205565,0.299813,0.544623,0.127893,0.325001,0.544623,0.000330,0.105222,0.596980,-0.044439
            ,0.044769,0.596980,-0.104892,0.000000,0.596980,-0.113732,0.000000,0.619574,-0.049669,0.190266,0.581641,-0.080622,0.080984,0.596980,-0.080653
            ,0.080953,0.581641,-0.189935,0.000000,0.581641,-0.205919,0.266661,0.566531,-0.113127,0.146438,0.581641,-0.146107,0.113457,0.566531,-0.266330
            ,0.000000,0.566531,-0.288731,0.299813,0.544623,-0.127232,0.205234,0.566531,-0.204903,0.127562,0.544623,-0.299483,0.000000,0.544623,-0.324669
            ,-0.044769,0.596980,-0.104892,-0.105222,0.596980,-0.044439,-0.114062,0.596980,0.000331,-0.049999,0.619574,0.000331,-0.080953,0.581641,-0.189935
            ,-0.080984,0.596980,-0.080653,-0.190266,0.581641,-0.080622,-0.206250,0.581641,0.000331,-0.113457,0.566531,-0.266330,-0.146438,0.581641,-0.146107
            ,-0.266661,0.566531,-0.113127,-0.289063,0.566531,0.000331,-0.127562,0.544623,-0.299482,-0.205234,0.566531,-0.204903,-0.299813,0.544623,-0.127232
            ,-0.325001,0.544623,0.000331,-0.105222,0.596980,0.045101,-0.044769,0.596980,0.105554,-0.190266,0.581641,0.081284,-0.080984,0.596980,0.081315
            ,-0.080953,0.581641,0.190597,-0.266661,0.566531,0.113788,-0.146438,0.581641,0.146768,-0.113457,0.566531,0.266991,-0.299813,0.544623,0.127893
            ,-0.205234,0.566531,0.205565,-0.127562,0.544623,0.300143,0.030952,0.722391,-0.030621,-0.230750,0.544623,-0.230419,-0.230750,0.544623,0.231081
            ,0.230750,0.544623,0.231081,0.230750,0.544623,-0.230419,0.000000,0.094672,-0.462308};

        const unsigned short inds[] = {
            0,1,2,1,3,4,1,5,6,7,1,4,8,9,10,10,11,12,11,13,12,14,15,13,16,9,17,18,11,9,11,
            19,14,14,20,21,22,18,16,23,24,18,24,25,19,19,26,20,27,23,22,28,29,23,29,30,25,25,31,26,15,
            32,33,33,34,35,34,36,35,37,38,36,20,32,21,39,34,32,34,40,37,37,41,42,26,39,20,43,44,39,44,
            45,40,40,46,41,31,43,26,47,48,43,48,49,45,45,50,46,38,51,52,52,53,54,53,55,54,56,57,55,41,
            51,42,58,53,51,53,59,56,56,60,61,46,58,41,62,63,58,63,64,59,59,65,60,50,62,46,66,67,62,67,
            68,64,64,69,65,57,70,71,71,72,73,72,74,73,75,8,74,60,70,61,76,72,70,72,77,75,75,16,17,65,
            76,60,78,79,76,79,80,77,77,22,16,69,78,65,81,82,78,82,83,80,80,27,22,84,28,27,85,86,28,86,
            87,30,30,88,31,89,85,84,90,91,85,91,92,87,87,93,88,94,90,89,95,96,90,96,97,92,92,98,93,99,
            95,94,100,101,95,101,102,97,97,103,98,88,47,31,104,105,47,105,106,49,49,107,50,93,104,88,108,109,104,109,
            110,106,106,111,107,98,108,93,112,113,108,113,114,110,110,115,111,103,112,98,116,117,112,117,118,114,114,119,115,107,
            66,50,120,121,66,121,122,68,68,123,69,111,120,107,124,125,120,125,126,122,122,127,123,115,124,111,128,129,124,129,
            130,126,126,131,127,119,128,115,132,133,128,133,134,130,130,135,131,123,81,69,136,137,81,137,138,83,83,84,27,127,
            136,123,139,140,136,140,141,138,138,89,84,131,139,127,142,143,139,143,144,141,141,94,89,135,142,131,145,146,142,146,
            147,144,144,99,94,99,148,100,100,149,150,149,102,150,151,103,102,152,153,148,148,154,149,154,151,149,155,156,151,157,
            158,153,153,159,154,159,155,154,160,161,155,162,163,158,158,163,159,159,163,160,160,163,164,103,165,116,116,166,167,166,
            118,167,168,119,118,156,169,165,165,170,166,170,168,166,171,172,168,161,173,169,169,174,170,174,171,170,175,176,171,164,
            163,173,173,163,174,174,163,175,175,163,177,119,178,132,132,179,180,179,134,180,181,135,134,172,182,178,178,183,179,183,
            181,179,184,185,181,176,186,182,182,187,183,187,184,183,188,189,184,177,163,186,186,163,187,187,163,188,188,163,190,135,
            191,145,145,192,193,192,147,193,194,99,147,185,195,191,191,196,192,196,194,192,197,152,194,189,198,195,195,199,196,199,
            197,196,200,157,197,190,163,198,198,163,199,199,163,200,200,163,162,201,202,203,203,204,205,205,206,207,207,208,209,210,
            202,211,212,204,202,213,206,204,214,208,206,215,212,210,216,213,212,217,214,213,218,219,214,220,216,215,221,217,216,222,
            218,217,223,224,218,208,225,209,226,227,225,228,229,227,230,201,229,208,231,226,226,232,228,228,233,230,230,210,211,219,
            234,231,231,235,232,232,236,233,233,215,210,224,237,234,234,238,235,235,239,236,236,220,215,240,221,220,241,222,221,242,
            223,222,243,244,223,245,241,240,246,242,241,247,243,242,248,249,243,250,246,245,251,247,246,252,248,247,253,254,248,255,
            252,251,256,253,252,257,258,253,244,259,237,237,260,238,238,261,239,239,240,220,249,262,259,259,263,260,260,264,261,261,
            245,240,254,265,262,262,266,263,263,267,264,264,250,245,258,268,265,265,269,266,266,270,267,267,271,250,272,273,274,274,
            275,276,276,277,278,278,279,280,281,282,273,273,283,275,275,284,277,277,285,279,286,287,282,282,288,283,283,289,284,284,
            290,285,291,287,292,293,288,287,294,289,288,295,290,289,279,296,280,297,298,296,299,300,298,301,272,300,285,297,279,302,
            299,297,303,301,299,304,281,301,290,302,285,305,303,302,306,304,303,307,286,304,290,308,305,305,309,306,306,310,307,307,
            291,292,311,293,291,312,294,293,313,295,294,314,315,295,316,312,311,317,313,312,318,314,313,319,320,314,316,321,317,317,
            322,318,318,323,319,319,324,325,326,327,321,321,327,322,322,327,323,323,327,324,315,328,308,308,329,309,309,330,310,310,
            311,291,320,331,328,328,332,329,329,333,330,330,316,311,324,331,325,334,332,331,335,333,332,336,316,333,324,327,334,334,
            327,335,335,327,336,336,327,326,337,338,339,339,340,341,340,342,341,343,344,342,345,346,338,338,347,340,347,343,340,348,
            349,343,350,351,346,346,352,347,352,348,347,353,354,348,344,355,356,356,357,358,357,359,358,360,361,359,349,362,355,355,
            363,357,363,360,357,364,365,360,354,366,362,362,367,363,367,364,363,368,369,364,361,370,371,371,372,373,372,374,373,375,
            376,374,365,377,370,370,378,372,378,375,372,379,380,375,369,381,377,377,382,378,382,379,378,383,384,379,376,385,386,386,
            387,388,387,389,388,390,337,389,380,391,385,385,392,387,392,390,387,393,345,390,384,394,391,391,395,392,395,393,392,396,
            350,393,397,351,398,399,352,351,352,400,353,353,401,402,403,399,397,404,405,399,405,406,400,400,407,401,408,404,403,409,
            410,404,410,411,406,406,412,407,413,409,408,414,415,409,415,416,411,411,417,412,401,366,402,418,367,366,367,419,368,368,
            420,421,407,418,401,422,423,418,423,424,419,419,425,420,412,422,407,426,427,422,427,428,424,424,429,425,417,426,412,430,
            431,426,431,432,428,428,433,429,420,381,421,434,382,381,382,435,383,383,436,437,425,434,420,438,439,434,439,440,435,435,
            441,436,429,438,425,442,443,438,443,444,440,440,445,441,433,442,429,446,447,442,447,448,444,444,449,445,436,394,437,450,
            395,394,395,451,396,396,397,398,441,450,436,452,453,450,453,454,451,451,403,397,445,452,441,455,456,452,456,457,454,454,
            408,403,449,455,445,458,459,455,459,460,457,457,413,408,0,356,461,359,5,461,342,0,2,7,339,2,389,7,4,3,
            386,4,374,3,6,5,371,6,2,341,342,461,358,359,6,373,374,4,388,389,271,251,250,38,446,433,52,462,446,462,
            55,448,448,57,449,57,458,449,71,463,458,463,74,460,460,8,413,8,414,413,10,464,414,464,13,416,416,15,417,15,
            430,417,33,465,430,465,36,432,432,38,433,0,461,1,1,6,3,1,461,5,7,2,1,8,17,9,10,9,11,11,
            14,13,14,21,15,16,18,9,18,24,11,11,24,19,14,19,20,22,23,18,23,29,24,24,29,25,19,25,26,27,
            28,23,28,86,29,29,86,30,25,30,31,15,21,32,33,32,34,34,37,36,37,42,38,20,39,32,39,44,34,34,
            44,40,37,40,41,26,43,39,43,48,44,44,48,45,40,45,46,31,47,43,47,105,48,48,105,49,45,49,50,38,
            42,51,52,51,53,53,56,55,56,61,57,41,58,51,58,63,53,53,63,59,56,59,60,46,62,58,62,67,63,63,
            67,64,59,64,65,50,66,62,66,121,67,67,121,68,64,68,69,57,61,70,71,70,72,72,75,74,75,17,8,60,
            76,70,76,79,72,72,79,77,75,77,16,65,78,76,78,82,79,79,82,80,77,80,22,69,81,78,81,137,82,82,
            137,83,80,83,27,84,85,28,85,91,86,86,91,87,30,87,88,89,90,85,90,96,91,91,96,92,87,92,93,94,
            95,90,95,101,96,96,101,97,92,97,98,99,100,95,100,150,101,101,150,102,97,102,103,88,104,47,104,109,105,105,
            109,106,49,106,107,93,108,104,108,113,109,109,113,110,106,110,111,98,112,108,112,117,113,113,117,114,110,114,115,103,
            116,112,116,167,117,117,167,118,114,118,119,107,120,66,120,125,121,121,125,122,68,122,123,111,124,120,124,129,125,125,
            129,126,122,126,127,115,128,124,128,133,129,129,133,130,126,130,131,119,132,128,132,180,133,133,180,134,130,134,135,123,
            136,81,136,140,137,137,140,138,83,138,84,127,139,136,139,143,140,140,143,141,138,141,89,131,142,139,142,146,143,143,
            146,144,141,144,94,135,145,142,145,193,146,146,193,147,144,147,99,99,152,148,100,148,149,149,151,102,151,156,103,152,
            157,153,148,153,154,154,155,151,155,161,156,157,162,158,153,158,159,159,160,155,160,164,161,103,156,165,116,165,166,166,
            168,118,168,172,119,156,161,169,165,169,170,170,171,168,171,176,172,161,164,173,169,173,174,174,175,171,175,177,176,119,
            172,178,132,178,179,179,181,134,181,185,135,172,176,182,178,182,183,183,184,181,184,189,185,176,177,186,182,186,187,187,
            188,184,188,190,189,135,185,191,145,191,192,192,194,147,194,152,99,185,189,195,191,195,196,196,197,194,197,157,152,189,
            190,198,195,198,199,199,200,197,200,162,157,201,211,202,203,202,204,205,204,206,207,206,208,210,212,202,212,213,204,213,
            214,206,214,219,208,215,216,212,216,217,213,217,218,214,218,224,219,220,221,216,221,222,217,222,223,218,223,244,224,208,
            226,225,226,228,227,228,230,229,230,211,201,208,219,231,226,231,232,228,232,233,230,233,210,219,224,234,231,234,235,232,
            235,236,233,236,215,224,244,237,234,237,238,235,238,239,236,239,220,240,241,221,241,242,222,242,243,223,243,249,244,245,
            246,241,246,247,242,247,248,243,248,254,249,250,251,246,251,252,247,252,253,248,253,258,254,255,256,252,256,257,253,257,
            466,258,244,249,259,237,259,260,238,260,261,239,261,240,249,254,262,259,262,263,260,263,264,261,264,245,254,258,265,262,
            265,266,263,266,267,264,267,250,258,466,268,265,268,269,266,269,270,267,270,271,272,281,273,274,273,275,276,275,277,278,
            277,279,281,286,282,273,282,283,275,283,284,277,284,285,286,292,287,282,287,288,283,288,289,284,289,290,291,293,287,293,
            294,288,294,295,289,295,315,290,279,297,296,297,299,298,299,301,300,301,281,272,285,302,297,302,303,299,303,304,301,304,
            286,281,290,305,302,305,306,303,306,307,304,307,292,286,290,315,308,305,308,309,306,309,310,307,310,291,311,312,293,312,
            313,294,313,314,295,314,320,315,316,317,312,317,318,313,318,319,314,319,325,320,316,326,321,317,321,322,318,322,323,319,
            323,324,315,320,328,308,328,329,309,329,330,310,330,311,320,325,331,328,331,332,329,332,333,330,333,316,324,334,331,334,
            335,332,335,336,333,336,326,316,337,345,338,339,338,340,340,343,342,343,349,344,345,350,346,338,346,347,347,348,343,348,
            354,349,350,398,351,346,351,352,352,353,348,353,402,354,344,349,355,356,355,357,357,360,359,360,365,361,349,354,362,355,
            362,363,363,364,360,364,369,365,354,402,366,362,366,367,367,368,364,368,421,369,361,365,370,371,370,372,372,375,374,375,
            380,376,365,369,377,370,377,378,378,379,375,379,384,380,369,421,381,377,381,382,382,383,379,383,437,384,376,380,385,386,
            385,387,387,390,389,390,345,337,380,384,391,385,391,392,392,393,390,393,350,345,384,437,394,391,394,395,395,396,393,396,
            398,350,397,399,351,399,405,352,352,405,400,353,400,401,403,404,399,404,410,405,405,410,406,400,406,407,408,409,404,409,
            415,410,410,415,411,406,411,412,413,414,409,414,464,415,415,464,416,411,416,417,401,418,366,418,423,367,367,423,419,368,
            419,420,407,422,418,422,427,423,423,427,424,419,424,425,412,426,422,426,431,427,427,431,428,424,428,429,417,430,426,430,
            465,431,431,465,432,428,432,433,420,434,381,434,439,382,382,439,435,383,435,436,425,438,434,438,443,439,439,443,440,435,
            440,441,429,442,438,442,447,443,443,447,444,440,444,445,433,446,442,446,462,447,447,462,448,444,448,449,436,450,394,450,
            453,395,395,453,451,396,451,397,441,452,450,452,456,453,453,456,454,451,454,403,445,455,452,455,459,456,456,459,457,454,
            457,408,449,458,455,458,463,459,459,463,460,457,460,413,0,344,356,359,361,5,342,344,0,7,337,339,389,337,7,3,
            376,386,374,376,3,5,361,371,2,339,341,461,356,358,6,371,373,4,386,388,271,255,251,38,52,446,52,54,462,462,
            54,55,448,55,57,57,71,458,71,73,463,463,73,74,460,74,8,8,10,414,10,12,464,464,12,13,416,13,15,15,
            33,430,33,35,465,465,35,36,432,36,38};

        const int numVerts = sizeof(verts)/(sizeof(verts[0])*3);
        const int numInds = sizeof(inds)/sizeof(inds[0]);
        TIS.numInds = numInds;
        float norms[numVerts*3];
        CalculateVertexNormals(verts,numVerts,inds,numInds,norms);

        glGenBuffers(1, &TIS.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, TIS.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &TIS.normalBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, TIS.normalBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(norms), norms, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &TIS.elementBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TIS.elementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

}


#endif //TEAPOT_IMPLEMENTATION