#pragma once


#include <stdio.h>
#include <windows.h>
#include <DirectXMath.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"DirectXTK12.lib") 

using namespace DirectX;
using namespace std;

#define SafeRelease(x) if(x){x->Release(); x=0;}
#define SafeDelete(x) if(x){delete x; x=0;}
#define SafeDeleteArray(x) if(x){delete[] x; x=0;}

//���_�̍\����
struct MyVertex
{
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
};

//�I���W�i���@�}�e���A���\����
struct MyMaterial
{
	CHAR name[256];
	XMFLOAT4 Ka;//�A���r�G���g
	XMFLOAT4 Kd;//�f�B�t���[�Y
	XMFLOAT4 Ks;//�X�y�L�����[
	CHAR textureName[256];//�e�N�X�`���[�t�@�C����

	ID3D12Resource* pTexture;
	ID3D12DescriptorHeap* pTextureSrvHeap;

	DWORD numFace;//���̃}�e���A���ł���|���S����
	MyMaterial()
	{
		ZeroMemory(this, sizeof(MyMaterial));
	}
	~MyMaterial()
	{
		SafeRelease(pTexture);
	}
};

//
//
//
class Mesh
{
public:
	DWORD m_numVertices;
	DWORD m_numTriangles;
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ID3D12Resource** m_ppIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW* m_indexBufferView;
	DWORD m_numMaterial;
	MyMaterial* m_material;
	XMFLOAT3 m_pos;
	float m_yaw, m_pitch, m_roll;
	float m_scale;
	int m_numTexture;
	int m_MyCbvOffsetOnHeap;//�O���ō쐬�����ł��낤�q�[�v���A�����̃R���X�^���g�o�b�t�@�̃r���[�̃I�t�Z�b�g
	int m_MySrvOffsetOnHeap;//�O���ō쐬�����ł��낤�q�[�v���A�����̃e�N�X�`���[�̃r���[�̃I�t�Z�b�g

	Mesh();
	~Mesh();
	HRESULT Init(ID3D12Device* pDevice, LPSTR FileName, ID3D12CommandAllocator* pAlloc,
		ID3D12CommandQueue* pQueue, ID3D12GraphicsCommandList* pList, ID3D12Fence* pFence,
		UINT64 FenceValue , bool ground);

	//�I�u�W�F�N�g����ۑ�
	//if�Ŏg��
	std::string name;

	//�@���f�[�^���i�[����
	//�@���̏ꏊ
	vector<XMFLOAT3> NVectors;
	vector<XMFLOAT3> NPoints;

private:
	ID3D12Device* m_device;
	ID3D12CommandAllocator* m_alloc;
	ID3D12CommandQueue* m_queue;
	ID3D12GraphicsCommandList* m_list;
	ID3D12Fence* m_fence;
	UINT64 m_fenceValue;
	ID3D12PipelineState* m_pipeline;
	HRESULT LoadStaticMesh(LPSTR FileName , bool ground);
	HRESULT LoadMaterialFromFile(LPSTR FileName, MyMaterial** ppMaterial);
};