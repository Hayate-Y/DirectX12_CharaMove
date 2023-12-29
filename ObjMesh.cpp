#include "ObjMesh.h"

#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"d3dCompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"Gdi32.lib") 
#pragma comment(lib,"Ole32.lib") 
#pragma comment(lib,"dxgi.lib")

//
//
//
Mesh::Mesh()
{
	ZeroMemory(this, sizeof(Mesh));
	m_scale = 1.0f;
}

//
//
//
Mesh::~Mesh()
{
	SafeDeleteArray(m_material);
	SafeDeleteArray(m_ppIndexBuffer);
	SafeRelease(m_vertexBuffer);
}
//
//
HRESULT Mesh::Init(ID3D12Device* pDevice, LPSTR FileName, ID3D12CommandAllocator* pAlloc,
	ID3D12CommandQueue* pQueue, ID3D12GraphicsCommandList* pList, ID3D12Fence* pFence,
	UINT64 FenceValue , bool ground)
{
	m_device = pDevice;
	m_alloc = pAlloc;
	m_queue = pQueue;
	m_list = pList;
	m_fence = pFence;
	m_fenceValue = FenceValue;
	//m_pipeline = pPipeline;

	if (FAILED(LoadStaticMesh(FileName,ground)))
	{
		MessageBox(0, "���b�V���쐬���s", NULL, MB_OK);
		return E_FAIL;
	}
	return S_OK;
}
//
//
//
const wchar_t* GetWC(const char* c)
{
	const size_t size = strlen(c) + 1;
	wchar_t* wc = new wchar_t[size];
	mbstowcs(wc, c, size);

	return wc;
}
//
//
HRESULT Mesh::LoadMaterialFromFile(LPSTR FileName, MyMaterial** ppMaterial)
{
	//�}�e���A���t�@�C�����J���ē��e��ǂݍ���
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");
	char key[110] = { 0 };
	XMFLOAT4 v(0, 0, 0, 1);

	//�}�e���A�����𒲂ׂ�
	m_numMaterial = 0;
	while (!feof(fp))
	{
		//�L�[���[�h�ǂݍ���
		fscanf_s(fp, "%s ", key, sizeof(key));
		//�}�e���A����
		if (strcmp(key, "newmtl") == 0)
		{
			m_numMaterial++;
		}
	}
	MyMaterial* pMaterial = new MyMaterial[m_numMaterial];

	//�{�ǂݍ���	
	fseek(fp, SEEK_SET, 0);
	INT matCount = -1;

	while (!feof(fp))
	{
		//�L�[���[�h�ǂݍ���
		int ret = fscanf_s(fp, "%s ", key, sizeof(key));
		if (ret <= 0)
		{
			continue;
		}
		//�}�e���A����
		if (strcmp(key, "newmtl") == 0)
		{
			matCount++;
			fscanf_s(fp, "%s ", key, sizeof(key));
			strcpy_s(pMaterial[matCount].name, key);
		}
		//Ka�@�A���r�G���g
		if (strcmp(key, "Ka") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[matCount].Ka = v;
		}
		//Kd�@�f�B�t���[�Y
		if (strcmp(key, "Kd") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[matCount].Kd = v;
		}
		//Ks�@�X�y�L�����[
		if (strcmp(key, "Ks") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[matCount].Ks = v;
		}
		//map_Kd�@�e�N�X�`���[
		if (strcmp(key, "map_Kd") == 0)
		{
			fscanf_s(fp, "%s", &pMaterial[matCount].textureName, sizeof(pMaterial[matCount].textureName));
			m_numTexture++;
		}
	}
	fclose(fp);

	*ppMaterial = pMaterial;
	return S_OK;
}
//
//
//
HRESULT Mesh::LoadStaticMesh(LPSTR FileName , bool ground)
{
	float x, y, z;
	int v1 = 0, v2 = 0, v3 = 0;
	int vn1 = 0, vn2 = 0, vn3 = 0;
	int vt1 = 0, vt2 = 0, vt3 = 0;
	DWORD vertCount = 0;//�ǂݍ��݃J�E���^�[
	DWORD vnormalCount = 0;//�ǂݍ��݃J�E���^�[
	DWORD vuvCount = 0;//�ǂݍ��݃J�E���^�[
	DWORD faceCount = 0;//�ǂݍ��݃J�E���^�[

	char key[200] = { 0 };
	//OBJ�t�@�C�����J���ē��e��ǂݍ���
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");

	//���O�ɒ��_���A�|���S�����𒲂ׂ�
	while (!feof(fp))
	{
		//�L�[���[�h�ǂݍ���
		fscanf_s(fp, "%s ", key, sizeof(key));
		//�}�e���A���ǂݍ���
		if (strcmp(key, "mtllib") == 0)
		{
			fscanf_s(fp, "%s ", key, sizeof(key));
			LoadMaterialFromFile(key, &m_material);
		}
		//���_
		if (strcmp(key, "v") == 0)
		{
			m_numVertices++;
		}
		//�@��
		if (strcmp(key, "vn") == 0)
		{
			vnormalCount++;
		}
		//�e�N�X�`���[���W
		if (strcmp(key, "vt") == 0)
		{
			vuvCount++;
		}
		//�t�F�C�X�i�|���S���j
		if (strcmp(key, "f") == 0)
		{
			m_numTriangles++;
		}
	}

	//�ꎞ�I�ȃ������m�ہi���_�o�b�t�@�ƃC���f�b�N�X�o�b�t�@�j
	MyVertex* pVertexBuffer = new MyVertex[m_numTriangles * 3];
	XMFLOAT3* pCoord = new XMFLOAT3[m_numVertices];
	XMFLOAT3* pNormal = new XMFLOAT3[vnormalCount];
	XMFLOAT2* pUv = new XMFLOAT2[vuvCount];

	

	//�{�ǂݍ���	
	fseek(fp, SEEK_SET, 0);
	vertCount = 0;
	vnormalCount = 0;
	vuvCount = 0;
	faceCount = 0;

	while (!feof(fp))
	{
		//�L�[���[�h �ǂݍ���
		ZeroMemory(key, sizeof(key));
		fscanf_s(fp, "%s ", key, sizeof(key));

		//���_ �ǂݍ���
		if (strcmp(key, "v") == 0)
		{
			fscanf_s(fp, "%f %f %f", &x, &y, &z);
			pCoord[vertCount].x = x;
			pCoord[vertCount].y = y;
			pCoord[vertCount].z = z;
			vertCount++;
		}

		//�@�� �ǂݍ���
		if (strcmp(key, "vn") == 0)
		{
			fscanf_s(fp, "%f %f %f", &x, &y, &z);
			pNormal[vnormalCount].x = x;
			pNormal[vnormalCount].y = y;
			pNormal[vnormalCount].z = z;
			vnormalCount++;
			if (ground) {
				NVectors.push_back(XMFLOAT3(x, y, z));
				NPoints.push_back(XMFLOAT3(pCoord[vnormalCount-1].x, pCoord[vnormalCount - 1].y, pCoord[vnormalCount - 1].z));
			}
		}

		//�e�N�X�`���[���W �ǂݍ���
		if (strcmp(key, "vt") == 0)
		{
			fscanf_s(fp, "%f %f", &x, &y);
			pUv[vuvCount].x = x;
			pUv[vuvCount].y = 1 - y;//OBJ�t�@�C����Y�������t�Ȃ̂ō��킹��
			vuvCount++;
		}
	}
	if (ground) {
		int i = 0;
	}


	//�}�e���A���̐������C���f�b�N�X�o�b�t�@�[���쐬
	m_ppIndexBuffer = new ID3D12Resource * [m_numMaterial];
	m_indexBufferView = new D3D12_INDEX_BUFFER_VIEW[m_numMaterial];

	//�t�F�C�X�@�ǂݍ��݁@�o���o���Ɏ��^����Ă���\��������̂ŁA�}�e���A�����𗊂�ɂȂ����킹��
	bool boFlag = false;
	int* piFaceBuffer = new int[m_numTriangles * 3];//�R���_�|���S���Ȃ̂ŁA1�t�F�C�X=3���_(3�C���f�b�N�X)
	faceCount = 0;
	DWORD subFaceCount = 0;
	for (DWORD i = 0; i < m_numMaterial; i++)
	{
		subFaceCount = 0;
		fseek(fp, SEEK_SET, 0);

		while (!feof(fp))
		{
			//�L�[���[�h �ǂݍ���
			ZeroMemory(key, sizeof(key));
			fscanf_s(fp, "%s ", key, sizeof(key));

			//�t�F�C�X �ǂݍ��݁����_�C���f�b�N�X��
			if (strcmp(key, "usemtl") == 0)
			{
				fscanf_s(fp, "%s ", key, sizeof(key));
				if (strcmp(key, m_material[i].name) == 0)
				{
					boFlag = true;
				}
				else
				{
					boFlag = false;
				}
			}
			if (strcmp(key, "f") == 0 && boFlag == true)
			{
				if (strlen(m_material[i].textureName) > 0)//�e�N�X�`���[����T�[�t�F�C�X
				{
					fscanf_s(fp, "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				}
				else//�e�N�X�`���[�����T�[�t�F�C�X
				{
					fscanf_s(fp, "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
				}
				//�C���f�b�N�X�o�b�t�@�[
				piFaceBuffer[subFaceCount * 3] = faceCount * 3;
				piFaceBuffer[subFaceCount * 3 + 1] = faceCount * 3 + 1;
				piFaceBuffer[subFaceCount * 3 + 2] = faceCount * 3 + 2;
				//���_�\���̂ɑ��
				pVertexBuffer[faceCount * 3].pos = pCoord[v1 - 1];
				pVertexBuffer[faceCount * 3].normal = pNormal[vn1 - 1];
				pVertexBuffer[faceCount * 3].uv = pUv[vt1 - 1];
				pVertexBuffer[faceCount * 3 + 1].pos = pCoord[v2 - 1];
				pVertexBuffer[faceCount * 3 + 1].normal = pNormal[vn2 - 1];
				pVertexBuffer[faceCount * 3 + 1].uv = pUv[vt2 - 1];
				pVertexBuffer[faceCount * 3 + 2].pos = pCoord[v3 - 1];
				pVertexBuffer[faceCount * 3 + 2].normal = pNormal[vn3 - 1];
				pVertexBuffer[faceCount * 3 + 2].uv = pUv[vt3 - 1];

				subFaceCount++;
				faceCount++;
			}
		}
		if (subFaceCount == 0)//�g�p����Ă��Ȃ��}�e���A���΍�
		{
			m_ppIndexBuffer[i] = NULL;
			continue;
		}
		//�C���f�b�N�X�o�b�t�@�[���쐬
		const UINT indexbufferSize = sizeof(int) * subFaceCount * 3;
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexbufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ppIndexBuffer[i]));
		//�C���f�b�N�X�o�b�t�@�ɃC���f�b�N�X�f�[�^���l�ߍ���
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange2(0, 0);
		m_ppIndexBuffer[i]->Map(0, &readRange2, reinterpret_cast<void**>(&pIndexDataBegin));
		memcpy(pIndexDataBegin, piFaceBuffer, indexbufferSize);
		m_ppIndexBuffer[i]->Unmap(0, NULL);
		//�C���f�b�N�X�o�b�t�@�r���[��������
		m_indexBufferView[i].BufferLocation = m_ppIndexBuffer[i]->GetGPUVirtualAddress();
		m_indexBufferView[i].Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView[i].SizeInBytes = indexbufferSize;

		m_material[i].numFace = subFaceCount;

	}
	delete[] piFaceBuffer;
	fclose(fp);

	//�o�[�e�b�N�X�o�b�t�@�[�쐬
	const UINT vertexbufferSize = sizeof(MyVertex) * m_numTriangles * 3;
	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexbufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));
	//�o�[�e�b�N�X�o�b�t�@�ɒ��_�f�[�^���l�ߍ���
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, pVertexBuffer, vertexbufferSize);
	m_vertexBuffer->Unmap(0, NULL);
	//�o�[�e�b�N�X�o�b�t�@�r���[��������
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(MyVertex);
	m_vertexBufferView.SizeInBytes = vertexbufferSize;

	//�ꎞ�I�ȓ��ꕨ�́A���͂�s�v
	delete pCoord;
	delete pNormal;
	delete pUv;
	delete[] pVertexBuffer;

	return S_OK;
}