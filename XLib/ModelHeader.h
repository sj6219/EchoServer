#pragma once



/* Mesh
					Drawing		Decal		Camera		Collision	Pick	Example

Normal(E_)			O			O			X			O			O
Hidden(H_)			X			X			X			O			X		hidden Barricade, Bonding box of fence, Hidden fence
Stair(R_)			X			X			O			O			0		hidden stair, hidden floor
Floor(F_)			0			0			0			0			O		floor
Camera(C_)			O			O			O			O			O		wall, floor, high roof
No Decal(D_)		O			X			X			O			0		semi transparent 
Water(M_)			0			X			0			X			0		water
Transparent(T_)		O			X			X			X			X		leaf of tree, glass
Picking Box(P_)		X			X			X			O			O

A_		: ignore, XRef 
B_		: Bounding box
S_		: Two side
P_		: Picking Box
U_		: Map Bounding Box
V_		: Hull Maker
X_		: ignore, Inner Bounding Box, 
Y_		: ignore, Bounding box
Z_		: ignore

+ N_	: Snow


+ L_	: Land

*/

/* Material

Effect F_

*/
#define XB_HEADER_VERSION 23

enum {
	XMODEL_MAX_BONE	= 200,
	XMODEL_MAX_MESH	= 128,
	XMODEL_MAX_ANIM	= 4,
	XMODEL_MAX_BONEINDEX = 70,
};

enum {
	XMODEL_BONE = 1,
	XMODEL_EFFECT = 2,
	XMODEL_GRID = 8,
	XMODEL_OPTION = 16,
	XMODEL_BILLBOARD = 32,
	XMODEL_BILLBOARD_Y = 64,
};

enum VERTEX_TYPE
{
	XVT_RIGID,					
	XVT_BLEND1,					
	XVT_BLEND2,					
	XVT_BLEND3,					
	XVT_BLEND4,					
	XVT_DOUBLE_RIGID,
	XVT_DOUBLE_BLEND1,					
	XVT_DOUBLE_BLEND2,					
	XVT_DOUBLE_BLEND3,					
	XVT_DOUBLE_BLEND4,					
	XVT_TANGENT_RIGID,					
	XVT_TANGENT_BLEND1,					
	XVT_TANGENT_BLEND2,					
	XVT_TANGENT_BLEND3,					
	XVT_TANGENT_BLEND4,					
	XVT_COMPLEX_RIGID,					
	XVT_COMPLEX_BLEND1,					
	XVT_COMPLEX_BLEND2,					
	XVT_COMPLEX_BLEND3,					
	XVT_COMPLEX_BLEND4,					
	XVT_END,						
};

enum FACE_TYPE
{
	XFT_LIST,
	XFT_STRIP,
	XFT_END
};

enum 
{
	XMATERIAL_TWOSIDED = 1,
	XMATERIAL_MAP_OPACITY = 2,
	XMATERIAL_EXTRAMAP = 4,
	XMATERIAL_SPECULARMAP = 8,
	XMATERIAL_VIDEO = 0x10,
	XMATERIAL_LIGHT = 0x20,
	XMATERIAL_OPTION = 0x80,
	XMATERIAL_LIGHTMAP = 0x100,
	XMATERIAL_TEXTUREBUMP = 0x800,
	XMATERIAL_LAND = 0x1000,
	XMATERIAL_OPACITY = 0x2000,
	XMATERIAL_METHOD = 0x8000,
};
#pragma pack(push, 1)

#define XMODEL_CRC_SEED 0x35bfd8a4L

struct XB_HEADER {
	BYTE	m_version;
	BYTE	m_bone_count;
	BYTE	reserved0;
	BYTE	m_mesh_count;
	DWORD	m_crc;
	DWORD	m_flags;
	WORD	m_material_frame_count;
	WORD	m_bone_index_count;
	WORD	m_keyframe_count;
	WORD	m_script_version;

	DWORD	m_string_size;
	DWORD	m_cls_size;

	WORD	m_anim_data_count;
	BYTE	m_anim_file_count;
	BYTE	m_land_count;


	D3DXVECTOR3	m_minimum;
	D3DXVECTOR3 m_maximum;
	DWORD	m_reserved2[4];
};


struct XB_BONE {
	D3DXMATRIX m_matrix;
	DWORD	m_name;
	BYTE	m_parent;
};

struct XB_MESH_HEADER {
	DWORD	m_name;
	DWORD	m_flags;
	BYTE	m_vertex_type;
	BYTE	m_face_type;
	WORD	m_vertex_count;
	WORD	m_index_count;
	BYTE	m_bone_index_count;
	DWORD	m_frame;
};

struct XB_LAND_HEADER {
	DWORD	m_name;
	DWORD	m_flags;
	BYTE	m_layer;
	FLOAT	m_repeat[5];
	BYTE	m_face_type;
	WORD	m_vertex_count;
	WORD	m_index_count;
};

struct XB_ANIM_HEADER {
	DWORD   m_szoption;
	WORD	m_keyframe_count;
	WORD	m_anim_data_count;
};

struct XB_KEYFRAME {
	WORD	m_time;
	DWORD	m_option;
};


struct XB_MATERIAL_FRAME {
	D3DCOLOR	m_ambient;
	D3DCOLOR	m_diffuse;
	D3DCOLOR	m_specular;
	float		m_opacity;
	D3DXVECTOR2 m_offset;
	D3DXVECTOR3 m_angle;
};

struct XB_CLS_HEADER {
	enum {
		PROPERTY = 1,
	};
	WORD	m_vertex_count;
	WORD	m_face_count;
	D3DXVECTOR3	m_minimum;
	D3DXVECTOR3 m_maximum;
	BYTE	m_parent;
	BYTE	m_reserved1;
	BYTE	m_flag;
	BYTE	m_reserved2;
};

namespace XB_CLS_PROPERTY
{
	enum { 
		AIR = -1,
		SNOW = 1, 
	};
}

struct XB_CLS_NODE {
	enum {
		L_LEAF = 1,
		R_LEAF = 2,
		X_MIN = 4,
		X_MAX = 8,
		Y_MIN = 0x10,
		Y_MAX = 0x20,
		Z_MIN = 0x40,
		Z_MAX = 0x80,
		L_HIDDEN = 0x100,
		R_HIDDEN = 0x200,
		L_CAMERA = 0x400,
		R_CAMERA = 0x800,
		L_WATER	= 0x500,
		R_WATER = 0xA00,
		L_NODECAL = 0x1000,
		R_NODECAL = 0x2000,
		L_PICK	  = 0x1100,
		R_PICK    = 0x2200,
		L_STAIR = 0x1400,
		R_STAIR = 0x2800,
		L_FLOOR = 0x4000,
		R_FLOOR = 0x8000,
		L_MASK = 0x5500,
		R_MASK = 0xAA00,
	};
	WORD	m_flag;
	BYTE	m_x_min, m_y_min, m_z_min;
	BYTE	m_x_max, m_y_max, m_z_max;
	WORD	m_left, m_right;

	void MinMax(D3DXVECTOR3 *pLMin, D3DXVECTOR3 *pRMin,  D3DXVECTOR3 *pLMax, D3DXVECTOR3 *pRMax, 
		const D3DXVECTOR3 &vvMin,  const D3DXVECTOR3& vvMax)
	{
		D3DXVECTOR3 e =  (1 / 255.0f) * ( vvMax - vvMin);
		*pLMin = *pRMin =  vvMin;
		if (m_flag & X_MIN)
			pLMin->x += m_x_min * e.x;
		else
			pRMin->x += m_x_min * e.x;
		if (m_flag & Y_MIN)
			pLMin->y += m_y_min * e.y;
		else
			pRMin->y += m_y_min * e.y;
		if (m_flag & Z_MIN)
			pLMin->z += m_z_min * e.z;
		else
			pRMin->z += m_z_min * e.z;
		*pLMax = *pRMax = vvMax;
		if (m_flag & X_MAX)
			pLMax->x -= m_x_max * e.x;
		else
			pRMax->x -= m_x_max * e.x;
		if (m_flag & Y_MAX)
			pLMax->y -= m_y_max * e.y;
		else
			pRMax->y -= m_y_max * e.y;
		if (m_flag & Z_MAX)
			pLMax->z -= m_z_max * e.z;
		else
			pRMax->z -= m_z_max * e.z;
	}

};


struct XB_VERTEX_LAND {
	D3DXVECTOR3	m_v;
	D3DXVECTOR3 m_n;
	D3DCOLOR	m_color0;
	D3DCOLOR	m_color1;
	D3DXVECTOR2	m_t0;
};

struct XB_VERTEX_RIGID {
	D3DXVECTOR3 m_v;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
};

struct XB_VERTEX_BLEND1 {
	D3DXVECTOR3 m_v;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
};

struct XB_VERTEX_BLEND2 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
};

struct XB_VERTEX_BLEND3 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
};

struct XB_VERTEX_BLEND4 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
};

struct XB_VERTEX_DOUBLE_RIGID {
	D3DXVECTOR3 m_v;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
};

struct XB_VERTEX_DOUBLE_BLEND1 {
	D3DXVECTOR3 m_v;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
};

struct XB_VERTEX_DOUBLE_BLEND2 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
};

struct XB_VERTEX_DOUBLE_BLEND3 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
};

struct XB_VERTEX_DOUBLE_BLEND4 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
};

struct XB_VERTEX_TANGENT_RIGID {
	D3DXVECTOR3 m_v;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_TANGENT_BLEND1 {
	D3DXVECTOR3 m_v;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_TANGENT_BLEND2 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_TANGENT_BLEND3 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_TANGENT_BLEND4 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_COMPLEX_RIGID {
	D3DXVECTOR3 m_v;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_COMPLEX_BLEND1 {
	D3DXVECTOR3 m_v;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_COMPLEX_BLEND2 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_COMPLEX_BLEND3 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
	D3DXVECTOR4 m_t;
};

struct XB_VERTEX_COMPLEX_BLEND4 {
	D3DXVECTOR3 m_v;
	D3DCOLOR	m_blend;
	DWORD	m_indices;
	D3DXVECTOR3 m_n;
	D3DXVECTOR2	m_t0;
	D3DXVECTOR2	m_t1;
	D3DXVECTOR4 m_t;
};


struct XB_ANIM {
	D3DXVECTOR3 m_pos;
	D3DXQUATERNION m_quat;
	D3DXVECTOR3 m_scale;
};

struct GR_HEADER {
	BYTE	version;
	BYTE	flags;
	WORD	reserved0;
	DWORD	crc;
	DWORD	spt_size;
	DWORD	cls_size;
	D3DXVECTOR3	minimum;
	D3DXVECTOR3 maximum;
	DWORD	reserved1[4];
};
#pragma pack(pop)
