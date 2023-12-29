#pragma once
#include<d3d12.h>
#include<dxgi1_6.h>
#include<map>
#include<DirectXTex.h>
#include<wrl.h>
#include<string>
#include<functional>
#include<memory>
#include<algorithm>
#include"ObjMesh.h"


struct Cbuffer
{
	XMMATRIX w;//ワールド行列
	XMMATRIX wvp;//ワールドから射影までの変換行列
	XMFLOAT4 lightDir;//ライト方向
	XMFLOAT4 eye;//視点位置
	XMFLOAT4 ambient;//環境光
	XMFLOAT4 diffuse;//拡散反射光
	XMFLOAT4 specular;//鏡面反射光

	Cbuffer()
	{
		ZeroMemory(this, sizeof(Cbuffer));
	}
};


class Dx12Wrapper
{
	SIZE _winSize;
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	

	//DirectX12まわり
	ComPtr< ID3D12Device> _dev = nullptr;//デバイス

	ComPtr < ID3D12CommandAllocator> _cmdAllocator = nullptr;//コマンドアロケータ
	ComPtr < ID3D12GraphicsCommandList> _cmdList = nullptr;//コマンドリスト
	ComPtr < ID3D12CommandQueue> _cmdQueue = nullptr;//コマンドキュー

	//表示に関わるバッファ周り
	ComPtr<ID3D12Resource> _depthBuffer = nullptr;//深度バッファ
	std::vector<ID3D12Resource*> _backBuffers;//バックバッファ(2つ以上…スワップチェインが確保)
	ComPtr<ID3D12DescriptorHeap> _rtvHeaps = nullptr;//レンダーターゲット用デスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> _dsvHeap = nullptr;//深度バッファビュー用デスクリプタヒープ
	std::unique_ptr<D3D12_VIEWPORT> _viewport;//ビューポート
	std::unique_ptr<D3D12_RECT> _scissorrect;//シザー矩形
	
	//シーンを構成するバッファまわり
	ComPtr<ID3D12Resource> _sceneConstBuff = nullptr;

	//struct SceneData {
		//DirectX::XMMATRIX view;//ビュー行列
		//DirectX::XMMATRIX proj;//プロジェクション行列
		//DirectX::XMFLOAT3 eye;//視点座標
	//};
	//SceneData* _mappedSceneData;
	//ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;

	//フェンス
	ComPtr<ID3D12Fence> _fence = nullptr;
	UINT64 _fenceVal = 0;

	//最終的なレンダーターゲットの生成
	HRESULT	CreateFinalRenderTargets();
	//デプスステンシルビューの生成
	HRESULT CreateDepthStencilView();

	//スワップチェインの生成
	HRESULT CreateSwapChain(const HWND& hwnd);

	//DXGIまわり初期化
	HRESULT InitializeDXGIDevice();



	//コマンドまわり初期化
	HRESULT InitializeCommand();

	//ビュープロジェクション用ビューの生成
	HRESULT CreateSceneView();

	//ロード用テーブル
	using LoadLambda_t = std::function<HRESULT(const std::wstring& path, DirectX::TexMetadata*, DirectX::ScratchImage&)>;
	std::map < std::string, LoadLambda_t> _loadLambdaTable;
	//テクスチャテーブル
	std::map<std::string,ComPtr<ID3D12Resource>> _textureTable;
	//テクスチャローダテーブルの作成
	void CreateTextureLoaderTable();
	



public:

	//DXGIまわり
	ComPtr < IDXGIFactory4> _dxgiFactory = nullptr;//DXGIインターフェイス
	ComPtr < IDXGISwapChain4> _swapchain = nullptr;//スワップチェイン

	struct SceneData {
		DirectX::XMMATRIX view;//ビュー行列
		DirectX::XMMATRIX proj;//プロジェクション行列
		DirectX::XMFLOAT3 eye;//視点座標
	};

	SceneData* _mappedSceneData;
	ComPtr<ID3D12DescriptorHeap> _sceneDescHeap = nullptr;



	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	
	void Update();
	void BeginDraw();
	void EndDraw();
	void ExecuteCommand();
	///テクスチャパスから必要なテクスチャバッファへのポインタを返す
	///@param texpath テクスチャファイルパス
	ComPtr<ID3D12Resource> GetTextureByPath(const char* texpath);
	//テクスチャ名からテクスチャバッファ作成、中身をコピー
	ID3D12Resource* CreateTextureFromFile(const char* texpath);

	ComPtr< ID3D12Device> Device();//デバイス
	ComPtr < ID3D12GraphicsCommandList> CommandList();//コマンドリスト
	ComPtr < IDXGISwapChain4> Swapchain();//スワップチェイン
	void SetScene();



	//オブジェクト
	void CreateMeshTexture(Mesh* pMesh);
	HRESULT CreateField();
	UINT m_rtvStride;
	UINT m_dsvStride;
	UINT m_cbvSrvStride;
	UINT m_cBSize;
	ID3D12PipelineState* m_pipelineState;
	ID3D12RootSignature* m_rootSignature;
	//RTV, DSV系
	ID3D12Resource* m_renderTargets[2];//////////////画面バッファ数
	ID3D12Resource* m_depthBuffer;
	ID3D12DescriptorHeap* m_rtvHeap;
	ID3D12DescriptorHeap* m_dspHeap;
	//CBV, SRV 系
	ID3D12Resource* m_constantBuffer;
	ID3D12DescriptorHeap* m_descHeap;
	//VBV, IBV 系
	ID3D12Resource* m_vertexBuffer;
	ID3D12Resource* m_indexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	Mesh* m_skyMesh;
	Mesh* m_bldMesh;
	Mesh* m_stair;
	int m_numConstantBuffers;
	int m_numTextures;
	void RenderMesh(Mesh* pMesh);
	void Render();
	void WaitGpu();
	int BufferCount;
	void DestroyD3D();

	//地面の法線のコピー
	//法線の場所
	vector<XMFLOAT3> NVectors;
	vector<XMFLOAT3> NPoints;

	//camera位置
	float CameraPositionY;
	float CameraPositionZ;
	void SetCameraPosition(float Y, float Z);

};

