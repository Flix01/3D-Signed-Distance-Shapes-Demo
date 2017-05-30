/**

Math 3D v1.0
By Stephan Soller <stephan.soller@helionweb.de> and Tobias Malmsheimer
Licensed under the MIT license

Math 3D is a compact C99 library meant to be used with OpenGL. It provides basic
3D vector and 4x4 matrix operations as well as functions to create transformation
and projection matrices. The OpenGL binary layout is used so you can just upload
vectors and matrices into shaders and work with them without any conversions.

It's an stb style single header file library. Define MATH_3D_IMPLEMENTATION
before you include this file in *one* C file to create the implementation.


QUICK NOTES

- If not explicitly stated by a parameter name all angles are in radians.
- The matrices use column-major indices. This is the same as in OpenGL and GLSL.
  The matrix documentation below for details.
- Matrices are passed by value. This is probably a bit inefficient but
  simplifies code quite a bit. Most operations will be inlined by the compiler
  anyway so the difference shouldn't matter that much. A matrix fits into 4 of
  the 16 SSE2 registers anyway. If profiling shows significant slowdowns the
  matrix type might change but ease of use is more important than every last
  percent of performance.
- When combining matrices with multiplication the effects apply right to left.
  This is the convention used in mathematics and OpenGL. Source:
  https://en.wikipedia.org/wiki/Transformation_matrix#Composing_and_inverting_transformations
  Direct3D does it differently.
- The `m4_mul_pos()` and `m4_mul_dir()` functions do a correct perspective
  divide (division by w) when necessary. This is a bit slower but ensures that
  the functions will properly work with projection matrices. If profiling shows
  this is a bottleneck special functions without perspective division can be
  added. But the normal multiplications should avoid any surprises.
- The library consistently uses a right-handed coordinate system. The old
  `glOrtho()` broke that rule and `m4_ortho()` has be slightly modified so you
  can always think of right-handed cubes that are projected into OpenGLs
  normalized device coordinates.
- Special care has been taken to document all complex operations and important
  sources. Most code is covered by test cases that have been manually calculated
  and checked on the whiteboard. Since indices and math code is prone to be
  confusing we used pair programming to avoid mistakes.


FURTHER IDEARS

These are ideas for future work on the library. They're implemented as soon as
there is a proper use case and we can find good names for them.

- bool v3_is_null(vec3_t v, float epsilon)
  To check if the length of a vector is smaller than `epsilon`.
- vec3_t v3_length_default(vec3_t v, float default_length, float epsilon)
  Returns `default_length` if the length of `v` is smaller than `epsilon`.
  Otherwise same as `v3_length()`.
- vec3_t v3_norm_default(vec3_t v, vec3_t default_vector, float epsilon)
  Returns `default_vector` if the length of `v` is smaller than `epsilon`.
  Otherwise the same as `v3_norm()`.
- mat4_t m4_invert(mat4_t matrix)
  Matrix inversion that works with arbitrary matrices. `m4_invert_affine()` can
  already invert translation, rotation, scaling, mirroring, reflection and
  shearing matrices. So a general inversion might only be useful to invert
  projection matrices for picking. But with orthographic and perspective
  projection it's probably simpler to calculate the ray into the scene directly
  based on the screen coordinates.


VERSION HISTORY

v1.0  2016-02-15  Initial release


Modified by Flix01. Added some structs and functions mostly ported from
the OgreMath library (www.ogre3d.org) - MIT licensed.
Very little testing has been done. Use it at your own risk.

Ported code back to --std=gnu89 [Basically ansi C with single line comments allowed]
**/

#ifndef MATH_3D_HEADER
#define MATH_3D_HEADER

#include <math.h>
#include <stdio.h>


// Define PI directly because we would need to define the _BSD_SOURCE or
// _XOPEN_SOURCE feature test macros to get it from math.h. That would be a
// rather harsh dependency. So we define it directly if necessary.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_HALF_PI
#define M_HALF_PI (M_PI/2.0)
#endif
#ifndef M_PIOVER180
#define M_PIOVER180 (3.14159265358979323846/180.0)
#endif
#ifndef M_180OVERPI
#define M_180OVERPI (180.0/3.14159265358979323846)
#endif

//
// 3D vectors
// 
// Use the `vec3()` function to create vectors. All other vector functions start
// with the `v3_` prefix.
// 
// The binary layout is the same as in GLSL and everything else (just 3 floats).
// So you can just upload the vectors into shaders as they are.
//

typedef struct { float x, y, z; } vec3_t;
static __inline vec3_t vec3(float x, float y, float z)        { vec3_t v;v.x=x;v.y=y;v.z=z;return v; }

static __inline vec3_t v3_add   (vec3_t a, vec3_t b)          { return vec3( a.x + b.x, a.y + b.y, a.z + b.z ); }
static __inline vec3_t v3_adds  (vec3_t a, float s)           { return vec3( a.x + s,   a.y + s,   a.z + s   ); }
static __inline vec3_t v3_sub   (vec3_t a, vec3_t b)          { return vec3( a.x - b.x, a.y - b.y, a.z - b.z ); }
static __inline vec3_t v3_subs  (vec3_t a, float s)           { return vec3( a.x - s,   a.y - s,   a.z - s   ); }
static __inline vec3_t v3_mul   (vec3_t a, vec3_t b)          { return vec3( a.x * b.x, a.y * b.y, a.z * b.z ); }
static __inline vec3_t v3_muls  (vec3_t a, float s)           { return vec3( a.x * s,   a.y * s,   a.z * s   ); }
static __inline vec3_t v3_div   (vec3_t a, vec3_t b)          { return vec3( a.x / b.x, a.y / b.y, a.z / b.z ); }
static __inline vec3_t v3_divs  (vec3_t a, float s)           { return vec3( a.x / s,   a.y / s,   a.z / s   ); }
static __inline float  v3_length(vec3_t v)                    { return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);          }
static __inline vec3_t v3_norm  (vec3_t v);
static __inline float  v3_dot   (vec3_t a, vec3_t b)          { return a.x*b.x + a.y*b.y + a.z*b.z;                 }
static __inline vec3_t v3_proj  (vec3_t v, vec3_t onto);
static __inline vec3_t v3_cross (vec3_t a, vec3_t b);
static __inline float  v3_angle_between(vec3_t a, vec3_t b);
static __inline vec3_t v3_lerp(vec3_t a, vec3_t b,float t)   { float ct = 1.f-t;return vec3(a.x*ct+b.x*t,a.y*ct+b.y*t,a.z*ct+b.z*t);}
              void   v3_print        (vec3_t v);
              void   v3_printp       (vec3_t v, int width, int precision);
              void   v3_fprint       (FILE* stream, vec3_t v);
              void   v3_fprintp      (FILE* stream, vec3_t v, int width, int precision);
static __inline const float* v3_cvalue_ptr(const vec3_t* v) {return &v->x;}
static __inline       float* v3_value_ptr (vec3_t* v)       {return &v->x;}

//
// quaternions
// 
// Use the `quat()` function to create vectors. All other vector functions start
// with the `qt_` prefix.
// 

typedef struct { float x, y, z, w; } quat_t;
static __inline quat_t quat(float x, float y, float z,float w)        { quat_t q;q.x=x;q.y=y;q.z=z;q.w=w;return q; }

static __inline float  qt_dot   (quat_t a, quat_t b)          { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;        }
static __inline float  qt_length2(quat_t q)                   { return q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; }
static __inline float  qt_length(quat_t q)                    { return sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w); }
static __inline quat_t qt_norm(quat_t q)                      { float len=qt_length(q);return quat(q.x/len,q.y/len,q.z/len,q.w/len);}
//static __inline quat_t qt_look_at_YX(vec3_t source_pos,vec3_t target_pos);
			  quat_t qt_slerp(quat_t qStart,quat_t qEnd,float factor);
              void   qt_print        (quat_t q);
              void   qt_printp       (quat_t q, int width, int precision);
              void   qt_fprint       (FILE* stream, quat_t q);
              void   qt_fprintp      (FILE* stream, quat_t q, int width, int precision);
static __inline const float* qt_cvalue_ptr(const quat_t* q) {return &q->x;}
static __inline       float* qt_value_ptr (quat_t* q)       {return &q->x;}




//
// 3x3 matrices
//
// Use the `mat3()` function to create a matrix. You can write the matrix
// members in the same way as you would write them on paper or on a whiteboard:
// 
// mat3_t m = mat3(
//     1,  0,  0
//     0,  1,  0
//     0,  0,  1
// )
// 
// This creates an identity matrix. All other
// matrix functions start with the `m3_` prefix.
// 
// The matrix is stored in column-major order, just as OpenGL expects. Members
// can be accessed by indices or member names. When you write a matrix on paper
// or on the whiteboard the indices and named members correspond to these
// positions:
// 
// | m00  m10  m20 |
// | m01  m11  m21 |
// | m02  m12  m22 |
// 
// | m00  m10  m20 |
// | m01  m11  m21 |
// | m02  m12  m22 |
// 
// The first index or number in a name denotes the column, the second the row.
// So m[i][j] denotes the member in the ith column and the jth row. This is the
// same as in GLSL (source: GLSL v1.3 specification, 5.6 Matrix Components).
//

typedef union {
	// The first index is the column index, the second the row index. The memory
	// layout of nested arrays in C matches the memory layout expected by OpenGL.
	float m[3][3];
	// OpenGL expects the first 3 floats to be the first column of the matrix.
	// So we need to define the named members column by column for the names to
	// match the memory locations of the array elements.
	struct {
		float m00, m01, m02;
		float m10, m11, m12;
		float m20, m21, m22;
	};
} mat3_t;

static __inline mat3_t mat3(
	float m00, float m10, float m20,
	float m01, float m11, float m21,
	float m02, float m12, float m22
);
static __inline mat3_t mat3_rm(
	float m00, float m01, float m02,
	float m10, float m11, float m12,
	float m20, float m21, float m22
);

static __inline mat3_t m3_identity     ();
static __inline vec3_t m3_get_x_axis   (const mat3_t* matrix);
static __inline vec3_t m3_get_y_axis   (const mat3_t* matrix);
static __inline vec3_t m3_get_z_axis   (const mat3_t* matrix);
static __inline mat3_t m3_invert_XZ_axis(const mat3_t* matrix);
static __inline mat3_t m3_scaling      (vec3_t scale);
static __inline mat3_t m3_rotation_x   (float angle_in_rad);
static __inline mat3_t m3_rotation_y   (float angle_in_rad);
static __inline mat3_t m3_rotation_z   (float angle_in_rad);
              mat3_t m3_rotation     (float angle_in_rad, vec3_t axis);
              quat_t m3_get_quaternion(const mat3_t* matrix);
              void   m3_set_quaternion(mat3_t* matrix,quat_t q);
static __inline mat3_t m3_slerp(const mat3_t* T1,const mat3_t* T2,float t);

static __inline mat3_t m3_transpose    (mat3_t matrix);
static __inline mat3_t m3_mul          (mat3_t a, mat3_t b);
              mat3_t m3_invert       (mat3_t matrix);
              vec3_t m3_mul_dir      (mat3_t matrix, vec3_t direction);
              mat3_t m3_from_euler_XYZ (const vec3_t YPR);
              mat3_t m3_from_euler_XZY (const vec3_t YPR);
              mat3_t m3_from_euler_YXZ (const vec3_t YPR);
              mat3_t m3_from_euler_YZX (const vec3_t YPR);
              mat3_t m3_from_euler_ZXY (const vec3_t YPR);
              mat3_t m3_from_euler_ZYX (const vec3_t YPR);
              int m3_to_euler_XYZ (const mat3_t* m,vec3_t* YPR);
              int m3_to_euler_XZY (const mat3_t* m,vec3_t* YPR);
              int m3_to_euler_YXZ (const mat3_t* m,vec3_t* YPR);
              int m3_to_euler_YZX (const mat3_t* m,vec3_t* YPR);
              int m3_to_euler_ZXY (const mat3_t* m,vec3_t* YPR);
              int m3_to_euler_ZYX (const mat3_t* m,vec3_t* YPR);
              void   m3_print        (mat3_t matrix);
              void   m3_printp       (mat3_t matrix, int width, int precision);
              void   m3_fprint       (FILE* stream, mat3_t matrix);
              void   m3_fprintp      (FILE* stream, mat3_t matrix, int width, int precision);
static __inline const float* m3_cvalue_ptr(const mat3_t* m) {return &m->m00;}
static __inline       float* m3_value_ptr (mat3_t* m)       {return &m->m00;}


//
// 4x4 matrices
//
// Use the `mat4()` function to create a matrix. You can write the matrix
// members in the same way as you would write them on paper or on a whiteboard:
// 
// mat4_t m = mat4(
//     1,  0,  0,  7,
//     0,  1,  0,  5,
//     0,  0,  1,  3,
//     0,  0,  0,  1
// )
// 
// This creates a matrix that translates points by vec3(7, 5, 3). All other
// matrix functions start with the `m4_` prefix. Among them functions to create
// identity, translation, rotation, scaling and projection matrices.
// 
// The matrix is stored in column-major order, just as OpenGL expects. Members
// can be accessed by indices or member names. When you write a matrix on paper
// or on the whiteboard the indices and named members correspond to these
// positions:
// 
// | m00  m10  m20  m30 |
// | m01  m11  m21  m31 |
// | m02  m12  m22  m32 |
// | m03  m13  m23  m33 |
// 
// | m00  m10  m20  m30 |
// | m01  m11  m21  m31 |
// | m02  m12  m22  m32 |
// | m03  m13  m23  m33 |
// 
// The first index or number in a name denotes the column, the second the row.
// So m[i][j] denotes the member in the ith column and the jth row. This is the
// same as in GLSL (source: GLSL v1.3 specification, 5.6 Matrix Components).
//

typedef union {
	// The first index is the column index, the second the row index. The memory
	// layout of nested arrays in C matches the memory layout expected by OpenGL.
	float m[4][4];
	// OpenGL expects the first 4 floats to be the first column of the matrix.
	// So we need to define the named members column by column for the names to
	// match the memory locations of the array elements.
	struct {
		float m00, m01, m02, m03;
		float m10, m11, m12, m13;
		float m20, m21, m22, m23;
		float m30, m31, m32, m33;
	};
} mat4_t;

static __inline mat4_t mat4(
	float m00, float m10, float m20, float m30,
	float m01, float m11, float m21, float m31,
	float m02, float m12, float m22, float m32,
	float m03, float m13, float m23, float m33
);
static __inline mat4_t mat4_rm(
	float m00, float m01, float m02, float m03,
	float m10, float m11, float m12, float m13,
	float m20, float m21, float m22, float m23,
	float m30, float m31, float m32, float m33
);

static __inline mat4_t m4_identity     ();
static __inline void m4_set_identity_mat3(mat4_t* matrix);
static __inline mat3_t m4_get_mat3     (const mat4_t* matrix);
static __inline void m4_set_mat3       (mat4_t* matrix,mat3_t matrix3);
static __inline vec3_t m4_get_x_axis   (const mat4_t* matrix);
static __inline vec3_t m4_get_y_axis   (const mat4_t* matrix);
static __inline vec3_t m4_get_z_axis   (const mat4_t* matrix);
static __inline mat4_t m4_invert_XZ_axis(const mat4_t* matrix);
static __inline vec3_t m4_get_translation(const mat4_t* matrix);
static __inline void   m4_set_translation(mat4_t* matrix,vec3_t translation);
              quat_t m4_get_quaternion(const mat4_t* matrix);
              void   m4_set_quaternion(mat4_t* matrix,quat_t q);
static __inline mat4_t m4_translation  (vec3_t offset);
static __inline mat4_t m4_scaling      (vec3_t scale);
static __inline mat4_t m4_rotation_x   (float angle_in_rad);
static __inline mat4_t m4_rotation_y   (float angle_in_rad);
static __inline mat4_t m4_rotation_z   (float angle_in_rad);
              mat4_t m4_rotation     (float angle_in_rad, vec3_t axis);

              mat4_t m4_ortho_2d     (float left,float right, float bottom, float top);
              mat4_t m4_ortho        (float left,float right, float bottom, float top,float nearVal,float farVal);
              mat4_t m4_ortho_3d     (float cameraTargetDistance,float degFOV,float aspect,float znear,float zfar);
              mat4_t m4_perspective  (float fovy,float aspect, float zNear, float zFar);
              mat4_t m4_frustum      (float left,float right, float bottom, float top,float zNear, float zFar);
              mat4_t m4_look_at      (float eyex,float eyey,float eyez,float centerx,float centery,float centerz,float upx,float upy,float upz);


static __inline mat4_t m4_transpose    (mat4_t matrix);
static __inline mat4_t m4_mul          (mat4_t a, mat4_t b);
              mat4_t m4_invert       (mat4_t matrix);
static __inline mat4_t m4_invert_fast  (mat4_t matrix);
              vec3_t m4_mul_pos      (mat4_t matrix, vec3_t position);
              vec3_t m4_mul_dir      (mat4_t matrix, vec3_t direction);
			  void m4_look_at_YX	 (mat4_t* matrix,vec3_t to,float min_distance_allowed,float max_distance_allowed);
static __inline mat4_t m4_slerp(const mat4_t* T1,const mat4_t* T2,float t);
              void   m4_print        (mat4_t matrix);
              void   m4_printp       (mat4_t matrix, int width, int precision);
              void   m4_fprint       (FILE* stream, mat4_t matrix);
              void   m4_fprintp      (FILE* stream, mat4_t matrix, int width, int precision);
              void   m4_print_as_float_array        (mat4_t matrix);
              void   m4_printp_as_float_array       (mat4_t matrix, int width, int precision);
              void   m4_fprint_as_float_array       (FILE* stream, mat4_t matrix);
              void   m4_fprintp_as_float_array      (FILE* stream, mat4_t matrix, int width, int precision);
static __inline const float* m4_cvalue_ptr(const mat4_t* m) {return &m->m00;}
static __inline       float* m4_value_ptr (mat4_t* m)       {return &m->m00;}



//
// 3D vector functions header implementation
//

static __inline vec3_t v3_norm(vec3_t v) {
	float len = v3_length(v);
	if (len > 0)
		return vec3( v.x / len, v.y / len, v.z / len );
	else
		return vec3( 0, 0, 0);
}

static __inline vec3_t v3_proj(vec3_t v, vec3_t onto) {
	return v3_muls(onto, v3_dot(v, onto) / v3_dot(onto, onto));
}

static __inline vec3_t v3_cross(vec3_t a, vec3_t b) {
	return vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

static __inline float v3_angle_between(vec3_t a, vec3_t b) {
	return acosf( v3_dot(a, b) / (v3_length(a) * v3_length(b)) );
}

//
// Quaternion header implementation
//

/*static __inline quat_t qt_look_at_YX(vec3_t source_pos,vec3_t target_pos)	{
	vec3_t D;float Dxz2,Dxz,AY,AX;
    D = target_pos-source_pos;
    Dxz2 = D.x*D.x+D.z*D.z;
    Dxz = sqrt(Dxz2);
    AY = atan2(D.x,D.z);
    AX = -atan2(D.y,Dxz);
	return qt_mul(qt_from_axis_angle(vec3(0.f,1.f,0.f),AY),qt_from_axis_angle(vec3(1.f,0.f,0.f),AX);
}*/						

//
// Matrix3 functions header implementation
//
static __inline mat3_t mat3(
	float m00, float m10, float m20,
	float m01, float m11, float m21,
	float m02, float m12, float m22
) {
	mat3_t m; 
	m.m00=m00; m.m10=m10; m.m20=m20;
	m.m01=m01; m.m11=m11; m.m21=m21;
	m.m02=m02; m.m12=m12; m.m22=m22;
	return m;
}

static __inline mat3_t mat3_rm(
	float m00, float m01, float m02,
	float m10, float m11, float m12,
	float m20, float m21, float m22
) {
	mat3_t m; 
	m.m00=m00; m.m10=m10; m.m20=m20;
	m.m01=m01; m.m11=m11; m.m21=m21;
	m.m02=m02; m.m12=m12; m.m22=m22;
	return m;
}

static __inline mat3_t m3_identity() {
	return mat3(
		 1,  0,  0,
		 0,  1,  0,
		 0,  0,  1
	);
}

static __inline mat3_t m3_scaling(vec3_t scale) {
	float x = scale.x, y = scale.y, z = scale.z;
	return mat3(
		 x,  0,  0,
		 0,  y,  0,
		 0,  0,  z
	);
}

static __inline vec3_t m3_get_x_axis(const mat3_t* matrix)	{
	const mat3_t* m = matrix;
	return vec3(m->m00,m->m01,m->m02);
}

static __inline vec3_t m3_get_y_axis(const mat3_t* matrix)	{
	const mat3_t* m = matrix;
	return vec3(m->m10,m->m11,m->m12);
}

static __inline vec3_t m3_get_z_axis(const mat3_t* matrix)	{
	const mat3_t* m = matrix;
	return vec3(m->m20,m->m21,m->m22);
}

static __inline mat3_t m3_invert_XZ_axis(const mat3_t* matrix) {
	const mat3_t* m = matrix;
	return mat3 (
		-m->m00, m->m10, -m->m20,
		-m->m01, m->m11, -m->m21,
		-m->m02, m->m12, -m->m22
	);
}

/*
	// BASICALLY I WANT TO PORT THIS CODE I USE WITH BULLET MATH
    {
    // The "system" modelview matrix is the inverse of m_cameraT, BUT we must be coherent with the axis conventions...
    // If we choose +Z to be the "forward" axis, and +X the left axis (like we do), we must "spoil" the camera from its own convention,
    // because in openGL, if we assign an identity matrix to the camera, it looks in the -Z direction, and its +X direction
    // is "right", instead of left. 
	// [Note: Having an identity matrix as the camera is like not having a camera at all and just draw on the OpenGL blackboard:
	// we look from +Z to 0 -> -Z direction, and we have +X at our RIGHT].
    // Many people force this convention (-Z = forward and +X = right) for all the objects, so they don't need to do this hack when
    // dealing with the camera, but I strongly advice you not to do so, as most models are modelled with the +Z axis as their
    // forward axis, and you'll end up rotating all the objects in the world, when you can just do some "dirty work" on the camera...
    //
    // P.S. For the ones that are still confused: don't worry, m_cameraT.getBasis().getColumn(2) is still the camera forward axis and that's
    // the important thing to remember.
    static btTransform m_cameraArrangedT;
    m_cameraArrangedT = m_cameraT;
    btMatrix3x3& m_cameraArrangedBasis = m_cameraArrangedT.getBasis();
    //invert the sign of m_cameraArrangedT.getBasis().getColumn(2);	// Z axis
    m_cameraArrangedBasis[0].setZ(- m_cameraArrangedBasis[0][2]);
    m_cameraArrangedBasis[1].setZ(- m_cameraArrangedBasis[1][2]);
    m_cameraArrangedBasis[2].setZ(- m_cameraArrangedBasis[2][2]);
    //invert the sign of m_cameraArrangedT.getBasis().getColumn(0);  // X axis
   	m_cameraArrangedBasis[0].setX( - m_cameraArrangedBasis[0][0]);
    m_cameraArrangedBasis[1].setX( - m_cameraArrangedBasis[1][0]);
    m_cameraArrangedBasis[2].setX( - m_cameraArrangedBasis[2][0]);
    // Here m_cameraArrangedT has been cleaned from the "camera convention"
    m_cameraArrangedT.inverse().getOpenGLMatrix(m_modelviewMatrix);	// very fast (see the implementation of Matrix3x3::inverse())
    }
*/

static __inline mat3_t m3_rotation_x(float angle_in_rad) {
	float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
	return mat3(
		1,  0,  0,
		0,  c, -s,
		0,  s,  c
	);
}

static __inline mat3_t m3_rotation_y(float angle_in_rad) {
	float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
	return mat3(
		 c,  0,  s,
		 0,  1,  0,
		-s,  0,  c
	);
}

static __inline mat3_t m3_rotation_z(float angle_in_rad) {
	float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
	return mat3(
		 c, -s,  0,
		 s,  c,  0,
		 0,  0,  1
	);
}

static __inline mat3_t m3_transpose(mat3_t matrix) {
	return mat3(matrix.m00, matrix.m01, matrix.m02,
                matrix.m10, matrix.m11, matrix.m12,
                matrix.m20, matrix.m21, matrix.m22);
}

static __inline mat3_t m3_slerp(const mat3_t* T1,const mat3_t* T2,float t)	{
	mat3_t m;
	quat_t q = qt_slerp(m3_get_quaternion(T1),m3_get_quaternion(T2),t);
	m3_set_quaternion(&m,q);
	return m;
}

/**
 * Multiplication of two 3x3 matrices.
 * 
 * Implemented by following the row times column rule and illustrating it on a
 * whiteboard with the proper indices in mind.
 * 
 * Further reading: https://en.wikipedia.org/wiki/Matrix_multiplication
 * But note that the article use the first index for rows and the second for
 * columns.
 */
static __inline mat3_t m3_mul(mat3_t a, mat3_t b) {
	mat3_t result;int i,j;
	
	for(i = 0; i < 3; i++) {
		for(j = 0; j < 3; j++) {
			result.m[i][j] = a.m[0][j]*b.m[i][0] +
							 a.m[1][j]*b.m[i][1] +
							 a.m[2][j]*b.m[i][2];
		}
	}	
	return result;
}


//
// Matrix4 functions header implementation
//

static __inline mat4_t mat4(
	float m00, float m10, float m20, float m30,
	float m01, float m11, float m21, float m31,
	float m02, float m12, float m22, float m32,
	float m03, float m13, float m23, float m33
) {
	mat4_t m;
	m.m00=m00; m.m10=m10; m.m20=m20; m.m30=m30;
	m.m01=m01; m.m11=m11; m.m21=m21; m.m31=m31;
	m.m02=m02; m.m12=m12; m.m22=m22; m.m32=m32;
	m.m03=m03; m.m13=m13; m.m23=m23; m.m33=m33;
	return m;
}

static __inline mat4_t mat4_rm(
	float m00, float m01, float m02, float m03,
	float m10, float m11, float m12, float m13,
	float m20, float m21, float m22, float m23,
	float m30, float m31, float m32, float m33
) {
	mat4_t m;
	m.m00=m00; m.m10=m10; m.m20=m20; m.m30=m30;
	m.m01=m01; m.m11=m11; m.m21=m21; m.m31=m31;
	m.m02=m02; m.m12=m12; m.m22=m22; m.m32=m32;
	m.m03=m03; m.m13=m13; m.m23=m23; m.m33=m33;
	return m;
}

static __inline mat4_t m4_identity() {
	return mat4(
		 1,  0,  0,  0,
		 0,  1,  0,  0,
		 0,  0,  1,  0,
		 0,  0,  0,  1
	);
}

static __inline void m4_set_identity_mat3(mat4_t* matrix) {
	mat4_t* m = matrix;
	m->m00 = 1; m->m10 = 0; m->m20 = 0;
	m->m01 = 0; m->m11 = 1; m->m21 = 0;
	m->m02 = 0; m->m12 = 0; m->m22 = 1;
}

static __inline mat3_t m4_get_mat3(const mat4_t* matrix)	{
	const mat4_t* m = matrix;
	return mat3 (
		m->m00, m->m10, m->m20,
		m->m01, m->m11, m->m21,
		m->m02, m->m12, m->m22
    );
}

static __inline void m4_set_mat3(mat4_t* matrix,mat3_t matrix3)	{
	mat4_t* m = matrix;
	m->m00 = matrix3.m00; m->m10 = matrix3.m10; m->m20 = matrix3.m20;
	m->m01 = matrix3.m01; m->m11 = matrix3.m11; m->m21 = matrix3.m21;
	m->m02 = matrix3.m02; m->m12 = matrix3.m12; m->m22 = matrix3.m22;
}

static __inline vec3_t m4_get_x_axis(const mat4_t* matrix)	{
	const mat4_t* m = matrix;
	return vec3(m->m00,m->m01,m->m02);
}

static __inline vec3_t m4_get_y_axis(const mat4_t* matrix)	{
	const mat4_t* m = matrix;
	return vec3(m->m10,m->m11,m->m12);
}

static __inline vec3_t m4_get_z_axis(const mat4_t* matrix)	{
	const mat4_t* m = matrix;
	return vec3(m->m20,m->m21,m->m22);
}

static __inline mat4_t m4_invert_XZ_axis(const mat4_t* matrix) {
	const mat4_t* m = matrix;
	return mat4 (
		-m->m00, m->m10, -m->m20, m->m30,
		-m->m01, m->m11, -m->m21, m->m31,
		-m->m02, m->m12, -m->m22, m->m32,
		 m->m03, m->m13,  m->m23, m->m33
	);
}

static __inline vec3_t m4_get_translation(const mat4_t* matrix)	{
	const mat4_t* m = matrix;
	return vec3(m->m30,m->m31,m->m32);
}

static __inline void m4_set_translation(mat4_t* matrix,vec3_t translation) {
	mat4_t* m = matrix;
	m->m30 = translation.x;
	m->m31 = translation.y;
	m->m32 = translation.z;	
}

static __inline mat4_t m4_translation(vec3_t offset) {
	return mat4(
		 1,  0,  0,  offset.x,
		 0,  1,  0,  offset.y,
		 0,  0,  1,  offset.z,
		 0,  0,  0,  1
	);
}

static __inline mat4_t m4_scaling(vec3_t scale) {
	float x = scale.x, y = scale.y, z = scale.z;
	return mat4(
		 x,  0,  0,  0,
		 0,  y,  0,  0,
		 0,  0,  z,  0,
		 0,  0,  0,  1
	);
}

static __inline mat4_t m4_rotation_x(float angle_in_rad) {
	float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
	return mat4(
		1,  0,  0,  0,
		0,  c, -s,  0,
		0,  s,  c,  0,
		0,  0,  0,  1
	);
}

static __inline mat4_t m4_rotation_y(float angle_in_rad) {
	float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
	return mat4(
		 c,  0,  s,  0,
		 0,  1,  0,  0,
		-s,  0,  c,  0,
		 0,  0,  0,  1
	);
}

static __inline mat4_t m4_rotation_z(float angle_in_rad) {
	float s = sinf(angle_in_rad), c = cosf(angle_in_rad);
	return mat4(
		 c, -s,  0,  0,
		 s,  c,  0,  0,
		 0,  0,  1,  0,
		 0,  0,  0,  1
	);
}

static __inline mat4_t m4_transpose(mat4_t matrix) {
	return mat4(matrix.m00, matrix.m01, matrix.m02, matrix.m03,
                matrix.m10, matrix.m11, matrix.m12, matrix.m13,
                matrix.m20, matrix.m21, matrix.m22, matrix.m23,
                matrix.m30, matrix.m31, matrix.m32, matrix.m33);
}

static __inline mat4_t m4_slerp(const mat4_t* T1,const mat4_t* T2,float t)	{
	mat4_t m = m4_identity();
	quat_t q = qt_slerp(m4_get_quaternion(T1),m4_get_quaternion(T2),t);
	vec3_t l = v3_lerp(m4_get_translation(T1),m4_get_translation(T2),t);
	m4_set_quaternion(&m,q);
	m4_set_translation(&m,l);
	return m;
}

/**
 * Multiplication of two 4x4 matrices.
 * 
 * Implemented by following the row times column rule and illustrating it on a
 * whiteboard with the proper indices in mind.
 * 
 * Further reading: https://en.wikipedia.org/wiki/Matrix_multiplication
 * But note that the article use the first index for rows and the second for
 * columns.
 */
static __inline mat4_t m4_mul(mat4_t a, mat4_t b) {
	mat4_t result;int i,j;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			result.m[i][j] = 
				a.m[0][j] * b.m[i][0] +
				a.m[1][j] * b.m[i][1] +
				a.m[2][j] * b.m[i][2] +
				a.m[3][j] * b.m[i][3];
		}
	}
	return result;
}

/** Return the inverse of this mat4 
	It works only for translation + rotation, and only
    when rotation can be represented by an unit quaternion
    scaling is discarded
*/
static __inline mat4_t m4_invert_fast(mat4_t matrix)	{ 
	mat4_t inv;vec3_t tra;
	m4_set_mat3(&inv,m3_transpose(m4_get_mat3(&matrix)));
	inv.m30=inv.m31=inv.m32=inv.m03=inv.m13=inv.m23=0.f;inv.m33=1.f;	
	tra = m4_get_translation(&matrix);
	tra.x=-tra.x;tra.y=-tra.y;tra.z=-tra.z;	
	m4_set_translation(&inv,m4_mul_dir(inv,tra));	
	return inv;
}

#endif // MATH_3D_HEADER


#ifdef MATH_3D_IMPLEMENTATION

void v3_print(vec3_t v) {
	v3_fprintp(stdout, v, 6, 2);
}

void v3_printp(vec3_t v, int width, int precision) {
	v3_fprintp(stdout, v, width, precision);
}

void v3_fprint(FILE* stream, vec3_t v) {
	v3_fprintp(stream, v, 6, 2);
}

void v3_fprintp(FILE* stream, vec3_t v, int width, int precision) {
	int w = width, p = precision;
	fprintf(stream, "( %*.*f %*.*f %*.*f )\n",
		w, p, v.x, w, p, v.y, w, p, v.z
	);
}


quat_t qt_slerp(quat_t qStart,quat_t qEnd,float factor) {
	const int normalizeQOutAfterLerp = 1;            // When using Lerp instead of Slerp qOut should be normalized. However some users prefer setting eps small enough so that they can leave the Lerp as it is.
    const float eps=0.0001f;                      	 // In [0 = 100% Slerp,1 = 100% Lerp] Faster but less precise with bigger epsilon (Lerp is used instead of Slerp more often). Users should tune it to achieve a performance boost.
    const int useAcosAndSinInsteadOfAtan2AndSqrt = 0;// Another possible minimal Speed vs Precision tweak (I suggest just changing it here and not in the caller code)

    quat_t qOut;
	float fCos;  
	fCos = qStart.x * qEnd.x + qStart.y * qEnd.y + qStart.z * qEnd.z + qStart.w * qEnd.w;

    // Do we need to invert rotation?
    if(fCos < 0)	//Originally it was if(fCos < static_cast < Real >(0.0) && shortestPath)
	    {fCos = -fCos;qEnd.x = -qEnd.x;qEnd.y = -qEnd.y;qEnd.z = -qEnd.z;qEnd.w = -qEnd.w;}

    if( fCos < 1.f - eps)	// Originally if was "Ogre::Math::Abs(fCos)" instead of "fCos", but we know fCos>0, because we have hard coded shortestPath=true
    {
        // Standard case (slerp)
        float fSin,fAngle;
        if (!useAcosAndSinInsteadOfAtan2AndSqrt)	{
            // Ogre::Quaternion uses this branch by default
            fSin = sqrt(1.f - fCos*fCos);
            fAngle = atan2(fSin, fCos);
        }
        else	{
            // Possible replacement of the two lines above
            // (it's hard to tell if they're faster, but my instinct tells me I should trust atan2 better than acos (geometry geeks needed here...)):
            // But probably sin(...) is faster than (sqrt + 1 subtraction and mult)
            fAngle = acos(fCos);
            fSin = sin(fAngle);
        }

		{
        const float fInvSin = 1.f / fSin;
        const float fCoeff0 = sin((1.f - factor) * fAngle) * fInvSin;
        const float fCoeff1 = sin(factor * fAngle) * fInvSin;

        //qOut =  fCoeff0 * qStart + fCoeff1 * qEnd; //Avoided for maximum portability and conversion of the code
        qOut.x = (fCoeff0 * qStart.x + fCoeff1 * qEnd.x);
        qOut.y = (fCoeff0 * qStart.y + fCoeff1 * qEnd.y);
        qOut.z = (fCoeff0 * qStart.z + fCoeff1 * qEnd.z);
        qOut.w = (fCoeff0 * qStart.w + fCoeff1 * qEnd.w);
		}
    } else
    {
        // There are two situations:
        // 1. "qStart" and "qEnd" are very close (fCos ~= +1), so we can do a linear
        //    interpolation safely.
        // 2. "qStart" and "qEnd" are almost inverse of each other (fCos ~= -1), there
        //    are an infinite number of possibilities interpolation. but we haven't
        //    have method to fix this case, so just use linear interpolation here.
        // IMPORTANT: CASE 2 can't happen anymore because we have hardcoded "shortestPath = true" and now fCos > 0

        const float fCoeff0 = 1.f - factor;
        const float fCoeff1 = factor;

        //qOut =  fCoeff0 * qStart + fCoeff1 * qEnd; //Avoided for maximum portability and conversion of the code
        qOut.x = (fCoeff0 * qStart.x + fCoeff1 * qEnd.x);
        qOut.y = (fCoeff0 * qStart.y + fCoeff1 * qEnd.y);
        qOut.z = (fCoeff0 * qStart.z + fCoeff1 * qEnd.z);
        qOut.w = (fCoeff0 * qStart.w + fCoeff1 * qEnd.w);
        if (normalizeQOutAfterLerp)  qOut = qt_norm(qOut);
    }

   	return qOut;
}

void qt_print(quat_t q) {
	qt_fprintp(stdout, q, 6, 2);
}

void qt_printp(quat_t q, int width, int precision) {
	qt_fprintp(stdout, q, width, precision);
}

void qt_fprint(FILE* stream, quat_t q) {
	qt_fprintp(stream, q, 6, 2);
}

void qt_fprintp(FILE* stream, quat_t q, int width, int precision) {
	int w = width, p = precision;
	fprintf(stream, "( %*.*f %*.*f %*.*f %*.*f )\n",
		w, p, q.x, w, p, q.y, w, p, q.z, w, p, q.w
	);
}

/**
 * Creates a matrix to rotate around an axis by a given angle. The axis doesn't
 * need to be normalized.
 * 
 * Sources:
 * 
 * https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
 */
/*mat3_t m3_rotation(float angle_in_rad, vec3_t axis) {
	vec3_t normalized_axis = v3_norm(axis);
	float x = normalized_axis.x, y = normalized_axis.y, z = normalized_axis.z;
	float c = cosf(angle_in_rad), s = sinf(angle_in_rad);
	
	return mat3(
		c + x*x*(1-c),            x*y*(1-c) - z*s,      x*z*(1-c) + y*s,
		    y*x*(1-c) + z*s,  c + y*y*(1-c),            y*z*(1-c) - x*s,
		    z*x*(1-c) - y*s,      z*y*(1-c) + x*s,  c + z*z*(1-c)
	);
}*/

/**
 * Implementation details:
 * 
 * - Invert the 3x3 matrix to handle rotation, scaling, etc.
 *   correctly (see source).
 * Sources for 3x3 matrix inversion:
 * 
 * https://www.khanacademy.org/math/precalculus/precalc-matrices/determinants-and-inverses-of-large-matrices/v/inverting-3x3-part-2-determinant-and-adjugate-of-a-matrix
 */
mat3_t m3_invert(mat3_t matrix) {
	// Create shorthands to access matrix members
	float m00 = matrix.m00,  m10 = matrix.m10,  m20 = matrix.m20;
	float m01 = matrix.m01,  m11 = matrix.m11,  m21 = matrix.m21;
	float m02 = matrix.m02,  m12 = matrix.m12,  m22 = matrix.m22;
		
	// Calculate cofactor matrix
	float c00 =   m11*m22 - m12*m21,   c10 = -(m01*m22 - m02*m21),  c20 =   m01*m12 - m02*m11;
	float c01 = -(m10*m22 - m12*m20),  c11 =   m00*m22 - m02*m20,   c21 = -(m00*m12 - m02*m10);
	float c02 =   m10*m21 - m11*m20,   c12 = -(m00*m21 - m01*m20),  c22 =   m00*m11 - m01*m10;

	float i00,i10,i20, i01,i11,i21, i02,i12,i22;

	// Caclculate the determinant by using the already calculated determinants
	// in the cofactor matrix.
	// Second sign is already minus from the cofactor matrix.
	float det = m00*c00 + m10*c10 + m20 * c20;
	if (fabs(det) < 0.00001)
		return m3_identity();
		
	// Calcuate inverse by dividing the transposed cofactor matrix by the determinant.
	i00 = c00 / det;  i10 = c01 / det;  i20 = c02 / det;
	i01 = c10 / det;  i11 = c11 / det;  i21 = c12 / det;
	i02 = c20 / det;  i12 = c21 / det;  i22 = c22 / det;

	// Combine the inverted R with the inverted translation
	return mat3(
		i00, i10, i20,
		i01, i11, i21,
		i02, i12, i22
	);		
}

/**
 * Multiplies a 3x3 matrix with a 3D vector representing a direction in 3D space.
 */
vec3_t m3_mul_dir(mat3_t matrix, vec3_t direction) {
    return vec3     (matrix.m00*direction.x + matrix.m10*direction.y + matrix.m20*direction.z,
                     matrix.m01*direction.x + matrix.m11*direction.y + matrix.m21*direction.z,
                     matrix.m02*direction.x + matrix.m12*direction.y + matrix.m22*direction.z);
}


// Not sure here if I have to invert m[i][j] with m[j][i]
quat_t m3_get_quaternion(const mat3_t* matrix) {
	const mat3_t* m = matrix;		
	float trace = m->m00 + m->m11 + m->m22;
	float temp[4];
	if (trace > 0.f) {
		float s = sqrt(trace + 1.0f);
		temp[3]=(s * 0.5f);s = 0.5f / s;
		temp[0]=((m->m12 - m->m21) * s);
		temp[1]=((m->m20 - m->m02) * s);
		temp[2]=((m->m01 - m->m10) * s);
	} 
	else {
		int i = m->m00 < m->m11 ? 
			(m->m11 < m->m22 ? 2 : 1) :
			(m->m00 < m->m22 ? 2 : 0); 
		int j = (i + 1) % 3;  
		int k = (i + 2) % 3;
		float s = sqrt(m->m[i][i] - m->m[j][j] - m->m[k][k] + 1.0f);
		temp[i] = s * 0.5f;s = 0.5f / s;
		temp[3] = (m->m[j][k] - m->m[k][j]) * s;
		temp[j] = (m->m[i][j] + m->m[j][i]) * s;
		temp[k] = (m->m[i][k] + m->m[k][i]) * s;
	}
	return quat(temp[0],temp[1],temp[2],temp[3]);
}
// Not sure here if I have to invert m[i][j] with m[j][i]
void  m3_set_quaternion(mat3_t* matrix,quat_t q)	{
	mat3_t* m = matrix;
	float d = qt_length2(q);
	if (d == 0.0f) {*m = m3_identity();return;}
	{
	float s = 2.0f / d;
	float xs = q.x * s,   ys = q.y * s,   zs = q.z * s;
	float wx = q.w * xs,  wy = q.w * ys,  wz = q.w * zs;
	float xx = q.x * xs,  xy = q.x * ys,  xz = q.x * zs;
	float yy = q.y * ys,  yz = q.y * zs,  zz = q.z * zs;

	m->m00 = 1.0f - (yy + zz); m->m10 = xy - wz;           m->m20 = xz + wy;
	m->m01 = xy + wz;          m->m11 = 1.0f - (xx + zz);  m->m21 = yz - wx;
	m->m02 = xz - wy;          m->m12 = yz + wx;           m->m22 = 1.0f - (xx + yy);
	}
}

// Not sure here if I have to invert m[i][j] with m[j][i]
quat_t m4_get_quaternion(const mat4_t* matrix) {
	const mat4_t* m = matrix;		
	float trace = m->m00 + m->m11 + m->m22;
	float temp[4];
	if (trace > 0.f) {
		float s = sqrt(trace + 1.0f);
		temp[3]=(s * 0.5f);s = 0.5f / s;
		temp[0]=((m->m12 - m->m21) * s);
		temp[1]=((m->m20 - m->m02) * s);
		temp[2]=((m->m01 - m->m10) * s);
	} 
	else {
		int i = m->m00 < m->m11 ? 
			(m->m11 < m->m22 ? 2 : 1) :
			(m->m00 < m->m22 ? 2 : 0); 
		int j = (i + 1) % 3;  
		int k = (i + 2) % 3;
		float s = sqrt(m->m[i][i] - m->m[j][j] - m->m[k][k] + 1.0f);
		temp[i] = s * 0.5f;s = 0.5f / s;
		temp[3] = (m->m[j][k] - m->m[k][j]) * s;
		temp[j] = (m->m[i][j] + m->m[j][i]) * s;
		temp[k] = (m->m[i][k] + m->m[k][i]) * s;
	}
	return quat(temp[0],temp[1],temp[2],temp[3]);
}
// Not sure here if I have to invert m[i][j] with m[j][i]
void m4_set_quaternion(mat4_t* matrix,quat_t q)	{
	mat4_t* m = matrix;
	float d = qt_length2(q);
	if (d == 0.0f) {m4_set_identity_mat3(m);return;}
	{
	float s = 2.0f / d;
	float xs = q.x * s,   ys = q.y * s,   zs = q.z * s;
	float wx = q.w * xs,  wy = q.w * ys,  wz = q.w * zs;
	float xx = q.x * xs,  xy = q.x * ys,  xz = q.x * zs;
	float yy = q.y * ys,  yz = q.y * zs,  zz = q.z * zs;

	m->m00 = 1.0f - (yy + zz); m->m10 = xy - wz;           m->m20 = xz + wy;
	m->m01 = xy + wz;          m->m11 = 1.0f - (xx + zz);  m->m21 = yz - wx;
	m->m02 = xz - wy;          m->m12 = yz + wx;           m->m22 = 1.0f - (xx + yy);
	}
}

/**
 * Makes "matrix" look at "to", using the YXZ axis order.
 * It must be "matrix"!=NULL,"min_distance_allowed">=0,"max_distance_allowed">0
 * "matrix" must have no scaling applied (its mat3 will be completely re-set)
 * if min_distance_allowed==max_distance_allowed no distance is kept.
*/
void m4_look_at_YX	(mat4_t* matrix,vec3_t to,float min_distance_allowed,float max_distance_allowed)	{
	vec3_t D,T;float Dxz2,Dxz,AY,AX;
	mat3_t basis;
	T = m4_get_translation(matrix);
    D = v3_sub(to,T);
    Dxz2 = D.x*D.x+D.z*D.z;
    Dxz = sqrt(Dxz2);
    AY = atan2(D.x,D.z);
    AX = -atan2(D.y,Dxz);
	basis = m3_from_euler_YXZ(vec3(AY,AX,0));
	m4_set_mat3(matrix,basis);
	if (min_distance_allowed<0) min_distance_allowed=0;
	if (max_distance_allowed<0) max_distance_allowed=0;		
	if (min_distance_allowed<max_distance_allowed)	{
 		float distance=sqrt(Dxz2+D.y*D.y);
		vec3_t zAxis = m4_get_z_axis(matrix);		
        if (distance<min_distance_allowed)
			m4_set_translation(matrix,v3_add(T,v3_muls(zAxis,distance-min_distance_allowed)));
 		else if (distance>max_distance_allowed) 
            m4_set_translation(matrix,v3_add(T,v3_muls(zAxis,distance-max_distance_allowed)));
 	}		
}


/**
 * Creates a 3x3 matrix out of a vec3_t(yaw,pitch,roll) vector with any specified axis
 * convention order (e.g. XYZ,XZY,YXZ,YZX,ZXY,ZYX).
 * The YPR components are all in RADIANS.
 * Please note that, for example, YPR.x is just the yaw of the rotation, and it has
 * nothing to do with the x axis. The same happens for the pitch (YPR.y) and roll (YPR.z) 
 *
 * This set of methods are important because there is no one-to-one (biunique) correspondence
 * between a set of Euler angles and a 3x3 matrix: but if you limit some (or all) the YPR 
 * components in a specific interval of values, you might find that for a particular axis 
 * convention order such corrispondence is still in place.
 *
 * All the Code related to "m3_from_euler_XYZ" and "m3_to_euler_XYZ" adapted from the 
 * Ogre Source Code (OgreMatrix3.cpp), (www.ogre3d.org, MIT License)
*/ 
    mat3_t m3_from_euler_XYZ (const vec3_t YPR)
    {
		mat3_t kXMat,kYMat,kZMat;
        float fCos, fSin;

        fCos = cos(YPR.x);fSin = sin(YPR.x);
        kXMat = mat3(1.0,0.0,0.0,0.0,fCos,-fSin,0.0,fSin,fCos);

        fCos = cos(YPR.y);fSin = sin(YPR.y);
        kYMat = mat3(fCos,0.0,fSin,0.0,1.0,0.0,-fSin,0.0,fCos);

        fCos = cos(YPR.z);fSin = sin(YPR.z);
        kZMat = mat3(fCos,-fSin,0.0,fSin,fCos,0.0,0.0,0.0,1.0);

        return m3_mul(kXMat,m3_mul(kYMat,kZMat));
    }
    mat3_t m3_from_euler_XZY (const vec3_t YPR)
    {
		mat3_t kXMat,kYMat,kZMat;        
        float fCos, fSin;

        fCos = cos(YPR.x);fSin = sin(YPR.x);
        kXMat = mat3(1.0,0.0,0.0,0.0,fCos,-fSin,0.0,fSin,fCos);

        fCos = cos(YPR.y);fSin = sin(YPR.y);
        kZMat = mat3(fCos,-fSin,0.0,fSin,fCos,0.0,0.0,0.0,1.0);

        fCos = cos(YPR.z);fSin = sin(YPR.z);
        kYMat = mat3(fCos,0.0,fSin,0.0,1.0,0.0,-fSin,0.0,fCos);

        return m3_mul(kXMat,m3_mul(kZMat,kYMat));
    }
    mat3_t m3_from_euler_YXZ (const vec3_t YPR)
    {
        mat3_t kXMat,kYMat,kZMat;
        float fCos, fSin;

        fCos = cos(YPR.x);fSin = sin(YPR.x);
        kYMat = mat3(fCos,0.0,fSin,0.0,1.0,0.0,-fSin,0.0,fCos);

        fCos = cos(YPR.y);fSin = sin(YPR.y);
        kXMat = mat3(1.0,0.0,0.0,0.0,fCos,-fSin,0.0,fSin,fCos);

        fCos = cos(YPR.z);fSin = sin(YPR.z);
        kZMat = mat3(fCos,-fSin,0.0,fSin,fCos,0.0,0.0,0.0,1.0);

        return m3_mul(kYMat,m3_mul(kXMat,kZMat));
    }
    mat3_t m3_from_euler_YZX (const vec3_t YPR)
    {
        mat3_t kXMat,kYMat,kZMat;
        float fCos, fSin;

        fCos = cos(YPR.x);fSin = sin(YPR.x);
        kYMat = mat3(fCos,0.0,fSin,0.0,1.0,0.0,-fSin,0.0,fCos);

        fCos = cos(YPR.y);fSin = sin(YPR.y);
        kZMat = mat3(fCos,-fSin,0.0,fSin,fCos,0.0,0.0,0.0,1.0);

        fCos = cos(YPR.z);fSin = sin(YPR.z);
        kXMat = mat3(1.0,0.0,0.0,0.0,fCos,-fSin,0.0,fSin,fCos);

        return m3_mul(kYMat,m3_mul(kZMat,kXMat));
    }
    mat3_t m3_from_euler_ZXY (const vec3_t YPR)
    {
        mat3_t kXMat,kYMat,kZMat;
        float fCos, fSin;

        fCos = cos(YPR.x);fSin = sin(YPR.x);
        kZMat = mat3(fCos,-fSin,0.0,fSin,fCos,0.0,0.0,0.0,1.0);

        fCos = cos(YPR.y);fSin = sin(YPR.y);
        kXMat = mat3(1.0,0.0,0.0,0.0,fCos,-fSin,0.0,fSin,fCos);

        fCos = cos(YPR.z);fSin = sin(YPR.z);
        kYMat = mat3(fCos,0.0,fSin,0.0,1.0,0.0,-fSin,0.0,fCos);

        return m3_mul(kZMat,m3_mul(kXMat,kYMat));
    }
    mat3_t m3_from_euler_ZYX (const vec3_t YPR)
    {
        mat3_t kXMat,kYMat,kZMat;
        float fCos, fSin;

        fCos = cos(YPR.x);fSin = sin(YPR.x);
        kZMat = mat3(fCos,-fSin,0.0,fSin,fCos,0.0,0.0,0.0,1.0);

        fCos = cos(YPR.y);fSin = sin(YPR.y);
        kYMat = mat3(fCos,0.0,fSin,0.0,1.0,0.0,-fSin,0.0,fCos);

        fCos = cos(YPR.z);fSin = sin(YPR.z);
        kXMat = mat3(1.0,0.0,0.0,0.0,fCos,-fSin,0.0,fSin,fCos);

        return m3_mul(kZMat,m3_mul(kYMat,kXMat));
    }

/**
 * Retrieves a vec3_t(yaw,pitch,roll) vector from a 3x3 matrix using any specified axis
 * convention order (e.g. XYZ,XZY,YXZ,YZX,ZXY,ZYX). 
 * vec3_t* YPR can't be NULL, and its components are all in RADIANS.
 * Returns 1 if the returned YPR is not unique in that axis convention order, otherwise 0.
 * Please note that, for example, YPR->x is just the yaw of the rotation, and it has
 * nothing to do with the x axis. The same happens for the pitch (YPR->y) and roll (YPR->z) 
 *
 * This set of methods are important because there is no one-to-one (biunique) correspondence
 * between a set of Euler angles and a 3x3 matrix: but if you limit some (or all) the YPR 
 * components in a specific interval of values, you might find that for a particular axis 
 * convention order such corrispondence is still in place.
 *
 * All the Code related to "m3_from_euler_XYZ" and "m3_to_euler_XYZ" adapted from the 
 * Ogre Source Code (OgreMatrix3.cpp), (www.ogre3d.org, MIT License)
*/ 
    int m3_to_euler_XYZ (const mat3_t* m,vec3_t* YPR)
    {
        // rot =  cy*cz          -cy*sz           sy
        //        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
        //       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

        YPR->y = asin(m->m20);
        if ( YPR->y < M_HALF_PI )
        {
            if ( YPR->y > -M_HALF_PI )
            {
                YPR->x = atan2(-m->m21,m->m22);
                YPR->z = atan2(-m->m10,m->m00);
                return 1;
            }
            else
            {
                // WARNING.  Not a unique solution.
                float fRmY = atan2(m->m01,m->m11);
                YPR->z = 0;  // any angle works
                YPR->x = YPR->z - fRmY;
                return 0;
            }
        }
        else
        {
            // WARNING.  Not a unique solution.
            float fRpY = atan2(m->m10,m->m11);
            YPR->z = 0;  // any angle works
            YPR->x = fRpY - YPR->z;
            return 0;
        }
    }
    int m3_to_euler_XZY (const mat3_t* m,vec3_t* YPR)
    {
        // rot =  cy*cz          -sz              cz*sy
        //        sx*sy+cx*cy*sz  cx*cz          -cy*sx+cx*sy*sz
        //       -cx*sy+cy*sx*sz  cz*sx           cx*cy+sx*sy*sz

        YPR->y = asin(-m->m10);
        if ( YPR->y < M_HALF_PI )
        {
            if ( YPR->y > -M_HALF_PI )
            {
                YPR->x = atan2(m->m12,m->m11);
                YPR->z = atan2(m->m20,m->m00);
                return 1;
            }
            else
            {
                // WARNING.  Not a unique solution.
                float fRmY = atan2(-m->m20,m->m22);
                YPR->z = 0;  // any angle works
                YPR->x = YPR->z - fRmY;
                return 0;
            }
        }
        else
        {
            // WARNING.  Not a unique solution.
            float fRpY = atan2(-m->m02,m->m22);
            YPR->z = 0;  // any angle works
            YPR->x = fRpY - YPR->z;
            return 0;
        }
    }
	int m3_to_euler_YXZ (const mat3_t* m,vec3_t* YPR)
    {
        // rot =  cy*cz+sx*sy*sz  cz*sx*sy-cy*sz  cx*sy
        //        cx*sz           cx*cz          -sx
        //       -cz*sy+cy*sx*sz  cy*cz*sx+sy*sz  cx*cy

        YPR->y = asin(-m->m21);
        if ( YPR->y < M_HALF_PI )
        {
            if ( YPR->y > -M_HALF_PI )
            {
                YPR->x = atan2(m->m20,m->m22);
                YPR->z = atan2(m->m01,m->m11);
                return 1;
            }
            else
            {
                // WARNING.  Not a unique solution.
                float fRmY = atan2(-m->m10,m->m00);
                YPR->z = 0;  // any angle works
                YPR->x = YPR->z - fRmY;
                return 0;
            }
        }
        else
        {
            // WARNING.  Not a unique solution.
            float fRpY = atan2(-m->m10,m->m00);
            YPR->z = 0;  // any angle works
            YPR->x = fRpY - YPR->z;
            return 0;
        }
    }
    int m3_to_euler_YZX (const mat3_t* m,vec3_t* YPR)
    {
        // rot =  cy*cz           sx*sy-cx*cy*sz  cx*sy+cy*sx*sz
        //        sz              cx*cz          -cz*sx
        //       -cz*sy           cy*sx+cx*sy*sz  cx*cy-sx*sy*sz

        YPR->y = asin(m->m01);
        if ( YPR->y < M_HALF_PI )
        {
            if ( YPR->y > -M_HALF_PI )
            {
                YPR->x = atan2(-m->m02,m->m00);
                YPR->z = atan2(-m->m21,m->m11);
                return 1;
            }
            else
            {
                // WARNING.  Not a unique solution.
                float fRmY = atan2(m->m12,m->m22);
                YPR->z = 0;  // any angle works
                YPR->x = YPR->z - fRmY;
                return 0;
            }
        }
        else
        {
            // WARNING.  Not a unique solution.
            float fRpY = atan2(m->m12,m->m22);
            YPR->z = 0;  // any angle works
            YPR->x = fRpY - YPR->z;
            return 0;
        }
    }
    int m3_to_euler_ZXY (const mat3_t* m,vec3_t* YPR)
    {
        // rot =  cy*cz-sx*sy*sz -cx*sz           cz*sy+cy*sx*sz
        //        cz*sx*sy+cy*sz  cx*cz          -cy*cz*sx+sy*sz
        //       -cx*sy           sx              cx*cy

        YPR->y = asin(m->m12);
        if ( YPR->y < M_HALF_PI )
        {
            if ( YPR->y > -M_HALF_PI )
            {
                YPR->x = atan2(-m->m10,m->m11);
                YPR->z = atan2(-m->m02,m->m22);
                return 1;
            }
            else
            {
                // WARNING.  Not a unique solution.
                float fRmY = atan2(m->m20,m->m00);
                YPR->z = 0;  // any angle works
                YPR->x = YPR->z - fRmY;
                return 0;
            }
        }
        else
        {
            // WARNING.  Not a unique solution.
            float fRpY = atan2(m->m20,m->m00);
            YPR->z = 0;  // any angle works
            YPR->x = fRpY - YPR->z;
            return 0;
        }
    }
    int m3_to_euler_ZYX (const mat3_t* m,vec3_t* YPR)
    {
        // rot =  cy*cz           cz*sx*sy-cx*sz  cx*cz*sy+sx*sz
        //        cy*sz           cx*cz+sx*sy*sz -cz*sx+cx*sy*sz
        //       -sy              cy*sx           cx*cy

        YPR->y = asin(-m->m02);
        if ( YPR->y < M_HALF_PI )
        {
            if ( YPR->y > -M_HALF_PI )
            {
                YPR->x = atan2(m->m01,m->m00);
                YPR->z = atan2(m->m12,m->m22);
                return 1;
            }
            else
            {
                // WARNING.  Not a unique solution.
                float fRmY = atan2(-m->m10,m->m20);
                YPR->z = 0;  // any angle works
                YPR->x = YPR->z - fRmY;
                return 0;
            }
        }
        else
        {
            // WARNING.  Not a unique solution.
            float fRpY = atan2(-m->m10,m->m20);
            YPR->z = 0;  // any angle works
            YPR->x = fRpY - YPR->z;
            return 0;
        }
    }


void m3_print(mat3_t matrix) {
	m3_fprintp(stdout, matrix, 6, 2);
}

void m3_printp(mat3_t matrix, int width, int precision) {
	m3_fprintp(stdout, matrix, width, precision);
}

void m3_fprint(FILE* stream, mat3_t matrix) {
	m3_fprintp(stream, matrix, 6, 2);
}

void m3_fprintp(FILE* stream, mat3_t matrix, int width, int precision) {
	int w = width, p = precision, r;
	for(r = 0; r < 3; r++) {
		fprintf(stream, "| %*.*f %*.*f %*.*f |\n",
			w, p, matrix.m[0][r], w, p, matrix.m[1][r], w, p, matrix.m[2][r]
		);
	}
}

/**
 * Creates a matrix to rotate around an axis by a given angle. The axis doesn't
 * need to be normalized.
 * 
 * Sources:
 * 
 * https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
 */
/*mat4_t m4_rotation(float angle_in_rad, vec3_t axis) {
	vec3_t normalized_axis = v3_norm(axis);
	float x = normalized_axis.x, y = normalized_axis.y, z = normalized_axis.z;
	float c = cosf(angle_in_rad), s = sinf(angle_in_rad);
	
	return mat4(
		c + x*x*(1-c),            x*y*(1-c) - z*s,      x*z*(1-c) + y*s,  0,
		    y*x*(1-c) + z*s,  c + y*y*(1-c),            y*z*(1-c) - x*s,  0,
		    z*x*(1-c) - y*s,      z*y*(1-c) + x*s,  c + z*z*(1-c),        0,
		    0,                        0,                    0,            1
	);
}*/

    mat4_t m4_ortho_2d(float left,float right, float bottom, float top) {
        mat4_t res;
		const float eps = 0.0001f;
        float Drl = (right-left);
        float Dtb = (top-bottom);
        if (Drl==0) {right+=eps;left-=eps;Drl=right-left;}
        if (Dtb==0) {top+=eps;bottom-=eps;Dtb=top-bottom;}
        
        res.m00 = 2.f/Drl;
        res.m01 = 0;
        res.m02 = 0;
        res.m03 = 0;

        res.m10 = 0;
        res.m11 = 2.f/Dtb;
        res.m12 = 0;
        res.m13 = 0;

        res.m20 = 0;
        res.m21 = 0;
        res.m22 = -1;
        res.m23 = 0;

        res.m30 = -(right+left)/Drl;
        res.m31 = -(top+bottom)/Dtb;
        res.m32 = 0;
        res.m33 = 1;

        return res;
    }
    // Typically, the matrix mode is GL_PROJECTION, and (left, bottom, -nearVal) and (right,top,-nearVal)
    // specify the points on the near clipping plane that are mapped to the lower left and upper right corners of the window, respectively, assuming that the eye is located at (0, 0, 0).
    // -farVal pecifies the location of the far clipping plane.
    // Both nearVal and farVal can be either positive or negative.
    mat4_t m4_ortho(float left,float right, float bottom, float top,float nearVal,float farVal) {
        mat4_t res;
        const float eps = 0.0001f;
        float Drl = (right-left);
        float Dtb = (top-bottom);
        float Dfn = (farVal-nearVal);
        if (Drl==0) {right+=eps;left-=eps;Drl=right-left;}
        if (Dtb==0) {top+=eps;bottom-=eps;Dtb=top-bottom;}
        if (Dfn==0) {farVal+=eps;nearVal-=eps;Dfn=farVal-nearVal;}

        res.m00 = 2.f/Drl;
        res.m01 = 0;
        res.m02 = 0;
        res.m03 = 0;

        res.m10 = 0;
        res.m11 = 2.f/Dtb;
        res.m12 = 0;
        res.m13 = 0;

        res.m20 = 0;
        res.m21 = 0;
        res.m22 = -2.f/Dfn;
        res.m23 = 0;

        res.m30 = -(right+left)/Drl;
        res.m31 = -(top+bottom)/Dtb;
        res.m32 = (farVal+nearVal)/Dfn;
        res.m33 = 1;

        return res;
    }
    mat4_t m4_perspective(float fovy,float aspect, float zNear, float zFar) {
        mat4_t res;
		const float eps = 0.0001f;
        float f = 1.f/tan(fovy*1.5707963268f/180.0); //cotg
        float Dfn = (zFar-zNear);
        if (Dfn==0) {zFar+=eps;zNear-=eps;Dfn=zFar-zNear;}
		if (aspect==0) aspect = 1.f;
        
        res.m00 = f/aspect;
        res.m01 = 0;
        res.m02 = 0;
        res.m03 = 0;

        res.m10 = 0;
        res.m11 = f;
        res.m12 = 0;
        res.m13 = 0;

        res.m20 = 0;
        res.m21 = 0;
        res.m22 = -(zFar+zNear)/Dfn;	
        res.m23 = -1;

        res.m30 = 0;
        res.m31 = 0;
        res.m32 = -2.f*zFar*zNear/Dfn;	
        res.m33 = 0;

        return res;
    }

    // Typically, the matrix mode is GL_PROJECTION, and (left,bottom,-nearVal) and (right,top,-nearVal) specify the points on the near clipping plane that are mapped to the lower left and upper right corners of the window, assuming that the eye is located at (0, 0, 0).
    // -farVal specifies the location of the far clipping plane.
    // Both nearVal and farVal must be positive (glFrustum(...) seems to return m4_identity() otherwise; we don't).
    mat4_t m4_frustum(float left,float right, float bottom, float top,float zNear, float zFar) {
        mat4_t res;
		const float eps = 0.0001f;        
        float Drl = (right-left);
        float Dtb = (top-bottom);
        float Dfn = (zFar-zNear);
        if (Drl==0) {right+=eps;left-=eps;Drl=right-left;}
        if (Dtb==0) {top+=eps;bottom-=eps;Dtb=top-bottom;}
        if (Dfn==0) {zFar+=eps;zNear-=eps;Dfn=zFar-zNear;}

        res.m00 = 2.f*zNear/Drl;
        res.m01 = 0;
        res.m02 = 0;
        res.m03 = 0;

        res.m10 = 0;
        res.m11 = 2.f*zNear/Dtb;
        res.m12 = 0;
        res.m13 = 0;

        res.m20 = (right+left)/Drl;
        res.m21 = (top+bottom)/Dtb;
        res.m22 = -(zFar+zNear)/Dfn;
        res.m23 = -1;

        res.m30 = 0;
        res.m31 = 0;
        res.m32 = -2.f*zFar*zNear/Dfn;
        res.m33 = 0;

        return res;
    }
 
    mat4_t m4_look_at(float eyex,float eyey,float eyez,float centerx,float centery,float centerz,float upx,float upy,float upz)	{
        mat4_t res;
		const float eps = 0.0001f;        
        
        float F[3] = {eyex-centerx,eyey-centery,eyez-centerz};
        float length = F[0]*F[0]+F[1]*F[1]+F[2]*F[2];	// length2 now
        float up[3] = {upx,upy,upz};

        float S[3] = {up[1]*F[2]-up[2]*F[1],up[2]*F[0]-up[0]*F[2],up[0]*F[1]-up[1]*F[0]};
		float U[3] = {F[1]*S[2]-F[2]*S[1],F[2]*S[0]-F[0]*S[2],F[0]*S[1]-F[1]*S[0]};
        
		if (length==0) length = eps;
        length = sqrt(length); 
        F[0]/=length;F[1]/=length;F[2]/=length;

        length = S[0]*S[0]+S[1]*S[1]+S[2]*S[2];if (length==0) length = eps;
        length = sqrt(length);  //btSqrt ?
        S[0]/=length;S[1]/=length;S[2]/=length;

        length = U[0]*U[0]+U[1]*U[1]+U[2]*U[2];if (length==0) length = eps;
        length = sqrt(length);  
        U[0]/=length;U[1]/=length;U[2]/=length;

        /*
                S0	S1	S2  0		1	0	0	-ex
                U0	U1	U2	0   *   0	1	0	-ey
                F0	F1	F2  0		0	0	1	-ez
                0	0	0	1		0	0	0	1

            */
        res.m00 = S[0];
        res.m01 = U[0];
        res.m02 = F[0];
        res.m03 = 0;

        res.m10 = S[1];
        res.m11 = U[1];
        res.m12 = F[1];
        res.m13 = 0;

        res.m20 = S[2];
        res.m21 = U[2];
        res.m22 = F[2];
        res.m23 = 0;

        res.m30 = -S[0]*eyex -S[1]*eyey -S[2]*eyez;
        res.m31 = -U[0]*eyex -U[1]*eyey -U[2]*eyez;
        res.m32 = -F[0]*eyex -F[1]*eyey -F[2]*eyez;
        res.m33 = 1;

        return res;
    }

/**
 * Untested, by it might be useful in 3d editors that can switch from prjective and ortho mode: 
 * basically in ortho mode we must change the projection matrix
 * every time the distance between the camera and the camera target changes.
*/
mat4_t m4_ortho_3d(float cameraTargetDistance,float degFOV,float aspect,float znear,float zfar)	{
		const float FOVTG=tan(degFOV*1.5707963268f/180.0);
		float y=cameraTargetDistance*FOVTG;//=(zfar-znear)*0.5f;
		float x=y*aspect;
		return m4_ortho( -x, x, -y, y, znear, zfar);
	}

/**
 * Creates an orthographic projection matrix. It maps the right handed cube
 * defined by left, right, bottom, top, back and front onto the screen and
 * z-buffer. You can think of it as a cube you move through world or camera
 * space and everything inside is visible.
 * 
 * This is slightly different from the traditional glOrtho() and from the linked
 * sources. These functions require the user to negate the last two arguments
 * (creating a left-handed coordinate system). We avoid that here so you can
 * think of this function as moving a right-handed cube through world space.
 * 
 * The arguments are ordered in a way that for each axis you specify the minimum
 * followed by the maximum. Thats why it's bottom to top and back to front.
 * 
 * Implementation details:
 * 
 * To be more exact the right-handed cube is mapped into normalized device
 * coordinates, a left-handed cube where (-1 -1) is the lower left corner,
 * (1, 1) the upper right corner and a z-value of -1 is the nearest point and
 * 1 the furthest point. OpenGL takes it from there and puts it on the screen
 * and into the z-buffer.
 * 
 * Sources:
 * 
 * https://msdn.microsoft.com/en-us/library/windows/desktop/dd373965(v=vs.85).aspx
 * https://unspecified.wordpress.com/2012/06/21/calculating-the-gluperspective-matrix-and-other-opengl-matrix-maths/
 */
/*mat4_t m4_ortho(float left, float right, float bottom, float top, float back, float front) {
	float l = left, r = right, b = bottom, t = top, n = front, f = back;
	float tx = -(r + l) / (r - l);
	float ty = -(t + b) / (t - b);
	float tz = -(f + n) / (f - n);
	return mat4(
		 2 / (r - l),  0,            0,            tx,
		 0,            2 / (t - b),  0,            ty,
		 0,            0,            2 / (f - n),  tz,
		 0,            0,            0,            1
	);
}*/

/**
 * Creates a perspective projection matrix for a camera.
 * 
 * The camera is at the origin and looks in the direction of the negative Z axis.
 * `near_view_distance` and `far_view_distance` have to be positive and > 0.
 * They are distances from the camera eye, not values on an axis.
 * 
 * `near_view_distance` can be small but not 0. 0 breaks the projection and
 * everything ends up at the max value (far end) of the z-buffer. Making the
 * z-buffer useless.
 * 
 * The matrix is the same as `gluPerspective()` builds. The view distance is
 * mapped to the z-buffer with a reciprocal function (1/x). Therefore the z-buffer
 * resolution for near objects is very good while resolution for far objects is
 * limited.
 * 
 * Sources:
 * 
 * https://unspecified.wordpress.com/2012/06/21/calculating-the-gluperspective-matrix-and-other-opengl-matrix-maths/
 */
/*mat4_t m4_perspective(float vertical_field_of_view_in_deg, float aspect_ratio, float near_view_distance, float far_view_distance) {
	float fovy_in_rad = vertical_field_of_view_in_deg / 180 * M_PI;
	float f = 1.0f / tan(fovy_in_rad / 2.0f);
	float ar = aspect_ratio;
	float nd = near_view_distance, fd = far_view_distance;
	
	return mat4(
		 f / ar,           0,                0,                0,
		 0,                f,                0,                0,
		 0,                0,               (fd+nd)/(nd-fd),  (2*fd*nd)/(nd-fd),
		 0,                0,               -1,                0
	);
}*/

/**
 * Builds a transformation matrix for a camera that looks from `from` towards
 * `to`. `up` defines the direction that's upwards for the camera. All three
 * vectors are given in world space and `up` doesn't need to be normalized.
 * 
 * Sources: Derived on whiteboard.
 * 
 * Implementation details:
 * 
 * x, y and z are the right-handed base vectors of the cameras subspace.
 * x has to be normalized because the cross product only produces a normalized
 *   output vector if both input vectors are orthogonal to each other. And up
 *   probably isn't orthogonal to z.
 * 
 * These vectors are then used to build a 3x3 rotation matrix. This matrix
 * rotates a vector by the same amount the camera is rotated. But instead we
 * need to rotate all incoming vertices backwards by that amount. That's what a
 * camera matrix is for: To move the world so that the camera is in the origin.
 * So we take the inverse of that rotation matrix and in case of an rotation
 * matrix this is just the transposed matrix. That's why the 3x3 part of the
 * matrix are the x, y and z vectors but written horizontally instead of
 * vertically.
 * 
 * The translation is derived by creating a translation matrix to move the world
 * into the origin (thats translate by minus `from`). The complete lookat matrix
 * is then this translation followed by the rotation. Written as matrix
 * multiplication:
 * 
 *   lookat = rotation * translation
 * 
 * Since we're right-handed this equals to first doing the translation and after
 * that doing the rotation. During that multiplication the rotation 3x3 part
 * doesn't change but the translation vector is multiplied with each rotation
 * axis. The dot product is just a more compact way to write the actual
 * multiplications.
 */
/*mat4_t m4_look_at(vec3_t from, vec3_t to, vec3_t up) {
	vec3_t z = v3_muls(v3_norm(v3_sub(to, from)), -1);
	vec3_t x = v3_norm(v3_cross(up, z));
	vec3_t y = v3_cross(z, x);
	
	return mat4(
		x.x, x.y, x.z, -v3_dot(from, x),
		y.x, y.y, y.z, -v3_dot(from, y),
		z.x, z.y, z.z, -v3_dot(from, z),
		0,   0,   0,    1
	);
}*/


/**
 * Inverts an affine transformation matrix. That are translation, scaling,
 * mirroring, reflection, rotation and shearing matrices or any combination of
 * them.
 * 
 * Implementation details:
 * 
 * - Invert the 3x3 part of the 4x4 matrix to handle rotation, scaling, etc.
 *   correctly (see source).
 * - Invert the translation part of the 4x4 matrix by multiplying it with the
 *   inverted rotation matrix and negating it.
 * 
 * When a 3D point is multiplied with a transformation matrix it is first
 * rotated and then translated. The inverted transformation matrix is the
 * inverse translation followed by the inverse rotation. Written as a matrix
 * multiplication (remember, the effect applies right to left):
 * 
 *   inv(matrix) = inv(rotation) * inv(translation)
 * 
 * The inverse translation is a translation into the opposite direction, just
 * the negative translation. The rotation part isn't changed by that
 * multiplication but the translation part is multiplied by the inverse rotation
 * matrix. It's the same situation as with `m4_look_at()`. But since we don't
 * store the rotation matrix as 3D vectors we can't use the dot product and have
 * to write the matrix multiplication operations by hand.
 * 
 * Sources for 3x3 matrix inversion:
 * 
 * https://www.khanacademy.org/math/precalculus/precalc-matrices/determinants-and-inverses-of-large-matrices/v/inverting-3x3-part-2-determinant-and-adjugate-of-a-matrix
 */
mat4_t m4_invert(mat4_t matrix) {

	if (matrix.m03 == 0 && matrix.m13 == 0 && matrix.m23 == 0 && matrix.m33 == 1)	{
		// Affine

        float m10 = matrix.m10, m11 = matrix.m11, m12 = matrix.m12;
        float m20 = matrix.m20, m21 = matrix.m21, m22 = matrix.m22;

        float t00 = m22 * m11 - m21 * m12;
        float t10 = m20 * m12 - m22 * m10;
        float t20 = m21 * m10 - m20 * m11;

        float m00 = matrix.m00, m01 = matrix.m01, m02 = matrix.m02;

        float invDet = 1 / (m00 * t00 + m01 * t10 + m02 * t20);
		
        t00 *= invDet; t10 *= invDet; t20 *= invDet;

        m00 *= invDet; m01 *= invDet; m02 *= invDet;
		{
        float r00 = t00;
        float r01 = m02 * m21 - m01 * m22;
        float r02 = m01 * m12 - m02 * m11;

        float r10 = t10;
        float r11 = m00 * m22 - m02 * m20;
        float r12 = m02 * m10 - m00 * m12;

        float r20 = t20;
        float r21 = m01 * m20 - m00 * m21;
        float r22 = m00 * m11 - m01 * m10;

        float m03 = matrix.m03, m13 = matrix.m13, m23 = matrix.m23;

        float r03 = - (r00 * m03 + r01 * m13 + r02 * m23);
        float r13 = - (r10 * m03 + r11 * m13 + r12 * m23);
        float r23 = - (r20 * m03 + r21 * m13 + r22 * m23);

        return mat4_rm(
            r00, r01, r02, r03,
            r10, r11, r12, r13,
            r20, r21, r22, r23,
              0,   0,   0,   1);
		}
	/*// Create shorthands to access matrix members
	float m00 = matrix.m00,  m10 = matrix.m10,  m20 = matrix.m20,  m30 = matrix.m30;
	float m01 = matrix.m01,  m11 = matrix.m11,  m21 = matrix.m21,  m31 = matrix.m31;
	float m02 = matrix.m02,  m12 = matrix.m12,  m22 = matrix.m22,  m32 = matrix.m32;
	
	// Invert 3x3 part of the 4x4 matrix that contains the rotation, etc.
	// That part is called R from here on.
		
		// Calculate cofactor matrix of R
		float c00 =   m11*m22 - m12*m21,   c10 = -(m01*m22 - m02*m21),  c20 =   m01*m12 - m02*m11;
		float c01 = -(m10*m22 - m12*m20),  c11 =   m00*m22 - m02*m20,   c21 = -(m00*m12 - m02*m10);
		float c02 =   m10*m21 - m11*m20,   c12 = -(m00*m21 - m01*m20),  c22 =   m00*m11 - m01*m10;
		
		// Caclculate the determinant by using the already calculated determinants
		// in the cofactor matrix.
		// Second sign is already minus from the cofactor matrix.
		float det = m00*c00 + m10*c10 + m20 * c20;
		if (fabs(det) < 0.00001)
			return m4_identity();
		
		// Calcuate inverse of R by dividing the transposed cofactor matrix by the
		// determinant.
		float i00 = c00 / det,  i10 = c01 / det,  i20 = c02 / det;
		float i01 = c10 / det,  i11 = c11 / det,  i21 = c12 / det;
		float i02 = c20 / det,  i12 = c21 / det,  i22 = c22 / det;
	
	// Combine the inverted R with the inverted translation
	return mat4(
		i00, i10, i20,  -(i00*m30 + i10*m31 + i20*m32),
		i01, i11, i21,  -(i01*m30 + i11*m31 + i21*m32),
		i02, i12, i22,  -(i02*m30 + i12*m31 + i22*m32),
		0,   0,   0,      1
	);*/
	}
	else {
        float m00 = matrix.m00, m01 = matrix.m01, m02 = matrix.m02, m03 = matrix.m03;
        float m10 = matrix.m10, m11 = matrix.m11, m12 = matrix.m12, m13 = matrix.m13;
        float m20 = matrix.m20, m21 = matrix.m21, m22 = matrix.m22, m23 = matrix.m23;
        float m30 = matrix.m30, m31 = matrix.m31, m32 = matrix.m32, m33 = matrix.m33;

        float v0 = m20 * m31 - m21 * m30;
        float v1 = m20 * m32 - m22 * m30;
        float v2 = m20 * m33 - m23 * m30;
        float v3 = m21 * m32 - m22 * m31;
        float v4 = m21 * m33 - m23 * m31;
        float v5 = m22 * m33 - m23 * m32;

        float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
        float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
        float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
        float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

        float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

        float d00 = t00 * invDet;
        float d10 = t10 * invDet;
        float d20 = t20 * invDet;
        float d30 = t30 * invDet;

        float d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
        float d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
        float d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
        float d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

        v0 = m10 * m31 - m11 * m30;
        v1 = m10 * m32 - m12 * m30;
        v2 = m10 * m33 - m13 * m30;
        v3 = m11 * m32 - m12 * m31;
        v4 = m11 * m33 - m13 * m31;
        v5 = m12 * m33 - m13 * m32;
		{
        float d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
        float d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
        float d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
        float d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

        v0 = m21 * m10 - m20 * m11;
        v1 = m22 * m10 - m20 * m12;
        v2 = m23 * m10 - m20 * m13;
        v3 = m22 * m11 - m21 * m12;
        v4 = m23 * m11 - m21 * m13;
        v5 = m23 * m12 - m22 * m13;
		{
        float d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
        float d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
        float d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
        float d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

        return mat4_rm(
            d00, d01, d02, d03,
            d10, d11, d12, d13,
            d20, d21, d22, d23,
            d30, d31, d32, d33);		
		}
		}	
	}

}


/**
 * Multiplies a 4x4 matrix with a 3D vector representing a point in 3D space.
 * 
 * Before the matrix multiplication the vector is first expanded to a 4D vector
 * (x, y, z, 1). After the multiplication the vector is reduced to 3D again by
 * dividing through the 4th component (if it's not 0 or 1).
 */
vec3_t m4_mul_pos(mat4_t matrix, vec3_t position) {
	vec3_t result = vec3(
            position.x*matrix.m00 + position.y*matrix.m10 + position.z*matrix.m20 + matrix.m30,
            position.x*matrix.m01 + position.y*matrix.m11 + position.z*matrix.m21 + matrix.m31,
            position.x*matrix.m02 + position.y*matrix.m12 + position.z*matrix.m22 + matrix.m32);

	float w = position.x*matrix.m03 + position.y*matrix.m13 + position.z*matrix.m23 + matrix.m33;	
	if (w != 0 && w != 1) return vec3(result.x / w, result.y / w, result.z / w);
	
	return result;
}

/**
 * Multiplies a 4x4 matrix with a 3D vector representing a direction in 3D space.
 * 
 * Before the matrix multiplication the vector is first expanded to a 4D vector
 * (x, y, z, 0). For directions the 4th component is set to 0 because directions
 * are only rotated, not translated. After the multiplication the vector is
 * reduced to 3D again by dividing through the 4th component (if it's not 0 or
 * 1). This is necessary because the matrix might contains something other than
 * (0, 0, 0, 1) in the bottom row which might set w to something other than 0
 * or 1.
 */
vec3_t m4_mul_dir(mat4_t matrix, vec3_t direction) {
	vec3_t result = vec3(
            direction.x*matrix.m00 + direction.y*matrix.m10 + direction.z*matrix.m20,
            direction.x*matrix.m01 + direction.y*matrix.m11 + direction.z*matrix.m21,
            direction.x*matrix.m02 + direction.y*matrix.m12 + direction.z*matrix.m22);

	float w = direction.x*matrix.m03 + direction.y*matrix.m13 + direction.z*matrix.m23;	
	if (w != 0 && w != 1) return vec3(result.x / w, result.y / w, result.z / w);
	
	return result;
}

void m4_print(mat4_t matrix) {
	m4_fprintp(stdout, matrix, 6, 2);
}

void m4_printp(mat4_t matrix, int width, int precision) {
	m4_fprintp(stdout, matrix, width, precision);
}

void m4_fprint(FILE* stream, mat4_t matrix) {
	m4_fprintp(stream, matrix, 6, 2);
}

void m4_fprintp(FILE* stream, mat4_t matrix, int width, int precision) {
	int w = width, p = precision, r;
	for(r = 0; r < 4; r++) {
		fprintf(stream, "| %*.*f %*.*f %*.*f %*.*f |\n",
			w, p, matrix.m[0][r], w, p, matrix.m[1][r], w, p, matrix.m[2][r], w, p, matrix.m[3][r]
		);
	}
}

void m4_print_as_float_array(mat4_t matrix) {
    m4_fprintp_as_float_array(stdout, matrix, 0, 4);
}

void m4_printp_as_float_array(mat4_t matrix, int width, int precision) {
    m4_fprintp_as_float_array(stdout, matrix, width, precision);
}

void m4_fprint_as_float_array(FILE* stream, mat4_t matrix) {
    m4_fprintp_as_float_array(stream, matrix, 0, 4);
}

void m4_fprintp_as_float_array(FILE* stream, mat4_t matrix, int width, int precision) {
    const float* m = m4_cvalue_ptr(&matrix);
    int w = width, p = precision, r;
    fprintf(stream, "{");
    for(r = 0; r < 16; r++) {fprintf(stream,"%*.*f",w,p,m[r]);if (r!=15) fprintf(stream,",");}
    fprintf(stream, "};\n");
}



#endif // MATH_3D_IMPLEMENTATION

