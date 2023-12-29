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
		MessageBox(0, "メッシュ作成失敗", NULL, MB_OK);
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
	//マテリアルファイルを開いて内容を読み込む
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");
	char key[110] = { 0 };
	XMFLOAT4 v(0, 0, 0, 1);

	//マテリアル数を調べる
	m_numMaterial = 0;
	while (!feof(fp))
	{
		//キーワード読み込み
		fscanf_s(fp, "%s ", key, sizeof(key));
		//マテリアル名
		if (strcmp(key, "newmtl") == 0)
		{
			m_numMaterial++;
		}
	}
	MyMaterial* pMaterial = new MyMaterial[m_numMaterial];

	//本読み込み	
	fseek(fp, SEEK_SET, 0);
	INT matCount = -1;

	while (!feof(fp))
	{
		//キーワード読み込み
		int ret = fscanf_s(fp, "%s ", key, sizeof(key));
		if (ret <= 0)
		{
			continue;
		}
		//マテリアル名
		if (strcmp(key, "newmtl") == 0)
		{
			matCount++;
			fscanf_s(fp, "%s ", key, sizeof(key));
			strcpy_s(pMaterial[matCount].name, key);
		}
		//Ka　アンビエント
		if (strcmp(key, "Ka") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[matCount].Ka = v;
		}
		//Kd　ディフューズ
		if (strcmp(key, "Kd") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[matCount].Kd = v;
		}
		//Ks　スペキュラー
		if (strcmp(key, "Ks") == 0)
		{
			fscanf_s(fp, "%f %f %f", &v.x, &v.y, &v.z);
			pMaterial[matCount].Ks = v;
		}
		//map_Kd　テクスチャー
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
	DWORD vertCount = 0;//読み込みカウンター
	DWORD vnormalCount = 0;//読み込みカウンター
	DWORD vuvCount = 0;//読み込みカウンター
	DWORD faceCount = 0;//読み込みカウンター

	char key[200] = { 0 };
	//OBJファイルを開いて内容を読み込む
	FILE* fp = NULL;
	fopen_s(&fp, FileName, "rt");

	//事前に頂点数、ポリゴン数を調べる
	while (!feof(fp))
	{
		//キーワード読み込み
		fscanf_s(fp, "%s ", key, sizeof(key));
		//マテリアル読み込み
		if (strcmp(key, "mtllib") == 0)
		{
			fscanf_s(fp, "%s ", key, sizeof(key));
			LoadMaterialFromFile(key, &m_material);
		}
		//頂点
		if (strcmp(key, "v") == 0)
		{
			m_numVertices++;
		}
		//法線
		if (strcmp(key, "vn") == 0)
		{
			vnormalCount++;
		}
		//テクスチャー座標
		if (strcmp(key, "vt") == 0)
		{
			vuvCount++;
		}
		//フェイス（ポリゴン）
		if (strcmp(key, "f") == 0)
		{
			m_numTriangles++;
		}
	}

	//一時的なメモリ確保（頂点バッファとインデックスバッファ）
	MyVertex* pVertexBuffer = new MyVertex[m_numTriangles * 3];
	XMFLOAT3* pCoord = new XMFLOAT3[m_numVertices];
	XMFLOAT3* pNormal = new XMFLOAT3[vnormalCount];
	XMFLOAT2* pUv = new XMFLOAT2[vuvCount];

	

	//本読み込み	
	fseek(fp, SEEK_SET, 0);
	vertCount = 0;
	vnormalCount = 0;
	vuvCount = 0;
	faceCount = 0;

	while (!feof(fp))
	{
		//キーワード 読み込み
		ZeroMemory(key, sizeof(key));
		fscanf_s(fp, "%s ", key, sizeof(key));

		//頂点 読み込み
		if (strcmp(key, "v") == 0)
		{
			fscanf_s(fp, "%f %f %f", &x, &y, &z);
			pCoord[vertCount].x = x;
			pCoord[vertCount].y = y;
			pCoord[vertCount].z = z;
			vertCount++;
		}

		//法線 読み込み
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

		//テクスチャー座標 読み込み
		if (strcmp(key, "vt") == 0)
		{
			fscanf_s(fp, "%f %f", &x, &y);
			pUv[vuvCount].x = x;
			pUv[vuvCount].y = 1 - y;//OBJファイルはY成分が逆なので合わせる
			vuvCount++;
		}
	}
	if (ground) {
		int i = 0;
	}


	//マテリアルの数だけインデックスバッファーを作成
	m_ppIndexBuffer = new ID3D12Resource * [m_numMaterial];
	m_indexBufferView = new D3D12_INDEX_BUFFER_VIEW[m_numMaterial];

	//フェイス　読み込み　バラバラに収録されている可能性があるので、マテリアル名を頼りにつなぎ合わせる
	bool boFlag = false;
	int* piFaceBuffer = new int[m_numTriangles * 3];//３頂点ポリゴンなので、1フェイス=3頂点(3インデックス)
	faceCount = 0;
	DWORD subFaceCount = 0;
	for (DWORD i = 0; i < m_numMaterial; i++)
	{
		subFaceCount = 0;
		fseek(fp, SEEK_SET, 0);

		while (!feof(fp))
		{
			//キーワード 読み込み
			ZeroMemory(key, sizeof(key));
			fscanf_s(fp, "%s ", key, sizeof(key));

			//フェイス 読み込み→頂点インデックスに
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
				if (strlen(m_material[i].textureName) > 0)//テクスチャーありサーフェイス
				{
					fscanf_s(fp, "%d/%d/%d %d/%d/%d %d/%d/%d", &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3);
				}
				else//テクスチャー無しサーフェイス
				{
					fscanf_s(fp, "%d//%d %d//%d %d//%d", &v1, &vn1, &v2, &vn2, &v3, &vn3);
				}
				//インデックスバッファー
				piFaceBuffer[subFaceCount * 3] = faceCount * 3;
				piFaceBuffer[subFaceCount * 3 + 1] = faceCount * 3 + 1;
				piFaceBuffer[subFaceCount * 3 + 2] = faceCount * 3 + 2;
				//頂点構造体に代入
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
		if (subFaceCount == 0)//使用されていないマテリアル対策
		{
			m_ppIndexBuffer[i] = NULL;
			continue;
		}
		//インデックスバッファーを作成
		const UINT indexbufferSize = sizeof(int) * subFaceCount * 3;
		m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexbufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_ppIndexBuffer[i]));
		//インデックスバッファにインデックスデータを詰め込む
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange2(0, 0);
		m_ppIndexBuffer[i]->Map(0, &readRange2, reinterpret_cast<void**>(&pIndexDataBegin));
		memcpy(pIndexDataBegin, piFaceBuffer, indexbufferSize);
		m_ppIndexBuffer[i]->Unmap(0, NULL);
		//インデックスバッファビューを初期化
		m_indexBufferView[i].BufferLocation = m_ppIndexBuffer[i]->GetGPUVirtualAddress();
		m_indexBufferView[i].Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView[i].SizeInBytes = indexbufferSize;

		m_material[i].numFace = subFaceCount;

	}
	delete[] piFaceBuffer;
	fclose(fp);

	//バーテックスバッファー作成
	const UINT vertexbufferSize = sizeof(MyVertex) * m_numTriangles * 3;
	m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexbufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer));
	//バーテックスバッファに頂点データを詰め込む
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, pVertexBuffer, vertexbufferSize);
	m_vertexBuffer->Unmap(0, NULL);
	//バーテックスバッファビューを初期化
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(MyVertex);
	m_vertexBufferView.SizeInBytes = vertexbufferSize;

	//一時的な入れ物は、もはや不要
	delete pCoord;
	delete pNormal;
	delete pUv;
	delete[] pVertexBuffer;

	return S_OK;
}