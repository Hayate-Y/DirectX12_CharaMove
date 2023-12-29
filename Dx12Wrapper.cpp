#include "Dx12Wrapper.h"
#include<cassert>
#include<d3dx12.h>
#include"Application.h"
#include"PMDRenderer.h"

#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace Microsoft::WRL;
using namespace std;
using namespace DirectX;

namespace {
	///モデルのパスとテクスチャのパスから合成パスを得る
	///@param modelPath アプリケーションから見たpmdモデルのパス
	///@param texPath PMDモデルから見たテクスチャのパス
	///@return アプリケーションから見たテクスチャのパス
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//ファイルのフォルダ区切りは\と/の二種類が使用される可能性があり
		//ともかく末尾の\か/を得られればいいので、双方のrfindをとり比較する
		//int型に代入しているのは見つからなかった場合はrfindがepos(-1→0xffffffff)を返すため
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}

	///ファイル名から拡張子を取得する
	///@param path 対象のパス文字列
	///@return 拡張子
	string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///ファイル名から拡張子を取得する(ワイド文字版)
	///@param path 対象のパス文字列
	///@return 拡張子
	wstring
		GetExtension(const std::wstring& path) {
		int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///テクスチャのパスをセパレータ文字で分離する
	///@param path 対象のパス文字列
	///@param splitter 区切り文字
	///@return 分離前後の文字列ペア
	pair<string, string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		pair<string, string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}

	///string(マルチバイト文字列)からwstring(ワイド文字列)を得る
	///@param str マルチバイト文字列
	///@return 変換されたワイド文字列
	std::wstring
		GetWideStringFromString(const std::string& str) {
		//呼び出し1回目(文字列数を得る)
		auto num1 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, nullptr, 0);

		std::wstring wstr;//stringのwchar_t版
		wstr.resize(num1);//得られた文字列数でリサイズ

		//呼び出し2回目(確保済みのwstrに変換文字列をコピー)
		auto num2 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, &wstr[0], num1);

		assert(num1 == num2);//一応チェック
		return wstr;
	}
	///デバッグレイヤーを有効にする
	void EnableDebugLayer() {
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
	}
}
Dx12Wrapper::Dx12Wrapper(HWND hwnd){
#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif

	auto& app=Application::Instance();
	_winSize = app.GetWindowSize();

	//DirectX12関連初期化
	if (FAILED(InitializeDXGIDevice())) {
		assert(0);
		return;
	}
	if (FAILED(InitializeCommand())) {
		assert(0);
		return;
	}
	if (FAILED(CreateSwapChain(hwnd))) {
		assert(0);
		return;
	}
	if (FAILED(CreateFinalRenderTargets())) {
		assert(0);
		return;
	}

	if (FAILED(CreateSceneView())) {
		assert(0);
		return;
	}

	//テクスチャローダー関連初期化
	CreateTextureLoaderTable();



	//深度バッファ作成
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return ;
	}

	//フェンスの作成
	if (FAILED(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())))) {
		assert(0);
		return ;
	}
	
}

HRESULT 
Dx12Wrapper::CreateDepthStencilView() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	//深度バッファ作成
	//深度バッファの仕様
	//auto depthResDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
	//	desc.Width, desc.Height,
	//	1, 0, 1, 0,
	//	D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);


	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resdesc.DepthOrArraySize = 1;
	resdesc.Width = desc.Width;
	resdesc.Height = desc.Height;
	resdesc.Format = DXGI_FORMAT_D32_FLOAT;
	resdesc.SampleDesc.Count = 1;
	resdesc.SampleDesc.Quality = 0;
	resdesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resdesc.MipLevels = 1;
	resdesc.Alignment = 0;

	//デプス用ヒーププロパティ
	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //デプス書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		//エラー処理
		return result;
	}

	//深度のためのデスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//深度に使うよという事がわかればいい
	dsvHeapDesc.NumDescriptors = 1;//深度ビュー1つのみ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//デプスステンシルビューとして使う
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;


	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//デプス値に32bit使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//フラグは特になし
	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}


Dx12Wrapper::~Dx12Wrapper()
{
}


ComPtr<ID3D12Resource>
Dx12Wrapper::GetTextureByPath(const char* texpath) {
	auto it = _textureTable.find(texpath);
	if (it != _textureTable.end()) {
		//テーブルに内にあったらロードするのではなくマップ内の
		//リソースを返す
		return _textureTable[texpath];
	}
	else {
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texpath));
	}

}

//テクスチャローダテーブルの作成
void 
Dx12Wrapper::CreateTextureLoaderTable() {
	_loadLambdaTable["sph"] = _loadLambdaTable["spa"] = _loadLambdaTable["bmp"] = _loadLambdaTable["png"] = _loadLambdaTable["jpg"] = [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	_loadLambdaTable["tga"] = [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	_loadLambdaTable["dds"] = [](const wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

//テクスチャ名からテクスチャバッファ作成、中身をコピー
ID3D12Resource* 
Dx12Wrapper::CreateTextureFromFile(const char* texpath) {
	string texPath = texpath;
	//テクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = GetWideStringFromString(texPath);//テクスチャのファイルパス
	auto ext = GetExtension(texPath);//拡張子を取得
	auto result = _loadLambdaTable[ext](wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return nullptr;
	}
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出

	//WriteToSubresourceで転送する用のヒープ設定
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	if (FAILED(result)) {
		return nullptr;
	}
	result = texbuff->WriteToSubresource(0,
		nullptr,//全領域へコピー
		img->pixels,//元データアドレス
		img->rowPitch,//1ラインサイズ
		img->slicePitch//全サイズ
	);
	if (FAILED(result)) {
		return nullptr;
	}



	return texbuff;
}

HRESULT
Dx12Wrapper::InitializeDXGIDevice() {
	UINT flagsDXGI = 0;
	flagsDXGI |= DXGI_CREATE_FACTORY_DEBUG;
	auto result = CreateDXGIFactory2(flagsDXGI, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
	//DirectX12まわり初期化
	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	if (FAILED(result)) {
		return result;
	}
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}
	result = S_FALSE;
	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (SUCCEEDED(D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(_dev.ReleaseAndGetAddressOf())))) {
			featureLevel = l;
			result = S_OK;
			break;
		}
	}
	return result;
}

///スワップチェイン生成関数
HRESULT
Dx12Wrapper::CreateSwapChain(const HWND& hwnd) {
	RECT rc = {};
	::GetWindowRect(hwnd, &rc);

	
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = _winSize.cx;
	swapchainDesc.Height = _winSize.cy;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	auto result= _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue.Get(),
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)_swapchain.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(result));
	return result;
}

//コマンドまわり初期化
HRESULT 
Dx12Wrapper::InitializeCommand() {
	auto result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_cmdAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator.Get(), nullptr, IID_PPV_ARGS(_cmdList.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//プライオリティ特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//ここはコマンドリストと合わせてください
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));//コマンドキュー生成
	assert(SUCCEEDED(result));
	return result;
}

//ビュープロジェクション用ビューの生成
HRESULT 
Dx12Wrapper::CreateSceneView(){
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	//定数バッファ作成
	result = _dev->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_sceneConstBuff.ReleaseAndGetAddressOf())
	);

	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	_mappedSceneData = nullptr;//マップ先を示すポインタ
	result = _sceneConstBuff->Map(0, nullptr, (void**)&_mappedSceneData);//マップ
	
	XMFLOAT3 eye(0, 15, -30);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//画角は45°
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//アス比
		0.1f,//近い方
		1000.0f//遠い方
	);						
	_mappedSceneData->eye = eye;
	
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//シェーダから見えるように
	descHeapDesc.NodeMask = 0;//マスクは0
	descHeapDesc.NumDescriptors = 1;//
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//デスクリプタヒープ種別
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_sceneDescHeap.ReleaseAndGetAddressOf()));//生成

	////デスクリプタの先頭ハンドルを取得しておく
	auto heapHandle = _sceneDescHeap->GetCPUDescriptorHandleForHeapStart();
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _sceneConstBuff->GetDesc().Width;
	//定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, heapHandle);
	return result;

}

HRESULT	
Dx12Wrapper::CreateFinalRenderTargets() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		SUCCEEDED(result);
		return result;
	}
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	_backBuffers.resize(swcDesc.BufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (int i = 0; i < swcDesc.BufferCount; ++i) {
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&_backBuffers[i]));
		assert(SUCCEEDED(result));
		rtvDesc.Format = _backBuffers[i]->GetDesc().Format;
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle);
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	_viewport.reset(new CD3DX12_VIEWPORT(_backBuffers[0]));
	_scissorrect.reset(new CD3DX12_RECT(0, 0, desc.Width, desc.Height));
	return result;
}

ComPtr< ID3D12Device> 
Dx12Wrapper::Device() {
	return _dev;
}
ComPtr < ID3D12GraphicsCommandList> 
Dx12Wrapper::CommandList() {
	return _cmdList;
}

void 
Dx12Wrapper::Update() {

}

void
Dx12Wrapper::BeginDraw() {
	//DirectX処理
	//バックバッファのインデックスを取得
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	_cmdList->ResourceBarrier(1, &barrier);


	//レンダーターゲットを指定
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//深度を指定
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


	//画面クリア
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//白色
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	//ビューポート、シザー矩形のセット
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());


}


void 
Dx12Wrapper::SetScene() {
	//現在のシーン(ビュープロジェクション)をセット
	ID3D12DescriptorHeap* sceneheaps[] = { _sceneDescHeap.Get() };
	_cmdList->SetDescriptorHeaps(1, sceneheaps);
	_cmdList->SetGraphicsRootDescriptorTable(0, _sceneDescHeap->GetGPUDescriptorHandleForHeapStart());

}

void
Dx12Wrapper::EndDraw() {

	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	_cmdList->ResourceBarrier(1, &barrier);

	ExecuteCommand();
}

void Dx12Wrapper::ExecuteCommand()
{

	//命令のクローズ
	_cmdList->Close();
	//コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	////待ち
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() < _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//キューをクリア
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);//再びコマンドリストをためる準備
}

ComPtr < IDXGISwapChain4> 
Dx12Wrapper::Swapchain() {
	return _swapchain;
}


const wchar_t* GetWC(const char* c);
//
void Dx12Wrapper::CreateMeshTexture(Mesh* pMesh)
{
	for (int i = 0; i < pMesh->m_numTexture; i++)
	{
		D3D12_SUBRESOURCE_DATA Subres;
		std::unique_ptr<uint8_t[]> decodedData;
		HRESULT hr = LoadWICTextureFromFile(_dev.Get(), GetWC(pMesh->m_material[i].textureName), &pMesh->m_material[i].pTexture, decodedData, Subres);

		//データをもとに自分でテクスチャーを作成する
		//まず空白のテクスチャーを作成する
		D3D12_RESOURCE_DESC tdesc;
		ZeroMemory(&tdesc, sizeof(tdesc));
		tdesc.MipLevels = 1;
		tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		tdesc.Width = Subres.RowPitch / 4;
		tdesc.Height = Subres.SlicePitch / Subres.RowPitch;
		tdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		tdesc.DepthOrArraySize = 1;
		tdesc.SampleDesc.Count = 1;
		tdesc.SampleDesc.Quality = 0;
		tdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		_dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&tdesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&pMesh->m_material[i].pTexture));
		//CPUからGPUへテクスチャーデータを渡すときの中継としてUpload Heapを作る
		ID3D12Resource* pTextureUploadHeap;
		DWORD bufferSize = GetRequiredIntermediateSize(pMesh->m_material[i].pTexture, 0, 1);

		_dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pTextureUploadHeap));

		//読みだしたテクセルデータを空白テクスチャーに流し込み、テクスチャーとして完成させる
		

		//コマンドリストに書き込む前にはコマンドアロケーターをリセットする
		_cmdAllocator->Reset();
		//コマンドリストをリセットする
		_cmdList->Reset(_cmdAllocator.Get(), NULL);

		UpdateSubresources(_cmdList.Get(), pMesh->m_material[i].pTexture, pTextureUploadHeap, 0, 0, 1, &Subres);
		_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pMesh->m_material[i].pTexture,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		//ここで一旦コマンドを閉じる。テクスチャーの転送を開始するため
		_cmdList->Close();
		//コマンドリストの実行　
		ID3D12CommandList* ppCommandLists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		//同期（待機）　テクスチャーの転送が終わるまで待機
		{
			_cmdQueue->Signal(_fence.Get(), _fenceVal);
			//上でセットしたシグナルがGPUから帰ってくるまでストール（この行で待機）
			while (_fence->GetCompletedValue() < _fenceVal);
		}
	}
}


HRESULT Dx12Wrapper::CreateField() {

	int kFrameCount = 2;
	m_rtvStride = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvStride = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvStride = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	//レンダーターゲット作成
	{
		//レンダーターゲットビューのヒープ作成
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = kFrameCount;//////////////////////////画面バッファ数
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		_dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

		//レンダーターゲットビュー作成
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < kFrameCount; n++)
		{
			_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			_dev->CreateRenderTargetView(m_renderTargets[n], NULL, rtvHandle);
			rtvHandle.Offset(1, m_rtvStride);
		}
	}


	//メッシュ作成
	m_skyMesh = new Mesh;
	m_bldMesh = new Mesh;
	//m_stair = new Mesh;   この方法で無限にオブジェクトを増やせる
	m_skyMesh->name = "空";
	m_bldMesh->name = "地面";
	//m_stair->name = "階段";　階段断念

	if (FAILED(m_skyMesh->Init(_dev.Get(), (LPSTR)"sky11.obj", _cmdAllocator.Get(),
		_cmdQueue.Get(), _cmdList.Get(), _fence.Get(), _fenceVal,false)))
	{
		return E_FAIL;
	}
	if (FAILED(m_bldMesh->Init(_dev.Get(), (LPSTR)"field_stair.obj", _cmdAllocator.Get(),
		_cmdQueue.Get(), _cmdList.Get(), _fence.Get(), _fenceVal,true)))
	{
		return E_FAIL;
	}
	//if (FAILED(m_stair->Init(_dev.Get(), (LPSTR)"stair10.obj", _cmdAllocator.Get(),
	//	_cmdQueue.Get(), _cmdList.Get(), _fence.Get(), _fenceVal,false)))
	//{
	//	return E_FAIL;
	//}
	
	m_skyMesh->m_pos = XMFLOAT3(0, 0, 0);
	m_bldMesh->m_pos = XMFLOAT3(0, 0, -100);
	//m_stair->m_pos = XMFLOAT3(0, -40, -160);

	//ディスクリプターヒープの作成
	//必要なディスクリプター数を算出 CBVはヒープ内0番から、SRVはCBVの後から
	int numDescriptors = 0;
	m_numConstantBuffers = m_skyMesh->m_numMaterial + m_bldMesh->m_numMaterial;// +m_stair->m_numMaterial;
	m_numTextures = m_skyMesh->m_numTexture + m_bldMesh->m_numTexture;// +m_stair->m_numTexture;
	numDescriptors += m_numConstantBuffers;//CBVの分
	numDescriptors += m_numTextures;//SRVの分

	//ヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	_dev->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_descHeap));

	//コンスタントバッファ作成
	{
		//コンスタントバッファ作成
		m_cBSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * ((sizeof(Cbuffer) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) + 1);
		_dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_cBSize * m_numConstantBuffers),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer));

		//コンスタントバッファビュー作成
		for (int i = 0; i < m_numConstantBuffers; i++)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress() + i * m_cBSize;
			cbvDesc.SizeInBytes = m_cBSize;
			D3D12_CPU_DESCRIPTOR_HANDLE cHandle = m_descHeap->GetCPUDescriptorHandleForHeapStart();
			cHandle.ptr += i * m_cbvSrvStride;
			BufferCount = cHandle.ptr;
			_dev->CreateConstantBufferView(&cbvDesc, cHandle);

		}
	}

	//各メッシュに自分のディスクリプターのオフセットを教えてやる
	m_skyMesh->m_MyCbvOffsetOnHeap = 0;
	m_bldMesh->m_MyCbvOffsetOnHeap = m_skyMesh->m_numMaterial;
	//m_stair->m_MyCbvOffsetOnHeap = m_bldMesh->m_MyCbvOffsetOnHeap + m_bldMesh->m_numMaterial;

	m_skyMesh->m_MySrvOffsetOnHeap = m_skyMesh->m_numMaterial + m_bldMesh->m_numMaterial;// +m_stair->m_numMaterial;
	m_bldMesh->m_MySrvOffsetOnHeap = m_skyMesh->m_MySrvOffsetOnHeap + m_skyMesh->m_numTexture;
	//m_stair->m_MySrvOffsetOnHeap = m_bldMesh->m_MySrvOffsetOnHeap + m_bldMesh->m_numTexture;

	//メッシュ内のテクスチャーをメイン側で作成
	CreateMeshTexture(m_skyMesh);
	CreateMeshTexture(m_bldMesh);
	//CreateMeshTexture(m_stair);

	//メッシュのテクスチャーのビュー（SRV)を作る
	for (int i = 0; i < m_skyMesh->m_numTexture; i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC sdesc = {};
		sdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		sdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		sdesc.Texture2D.MipLevels = 1;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_descHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += (m_skyMesh->m_MySrvOffsetOnHeap + i) * m_cbvSrvStride;
		_dev->CreateShaderResourceView(m_skyMesh->m_material[i].pTexture, &sdesc, handle);
	}
	for (int i = 0; i < m_bldMesh->m_numTexture; i++)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC sdesc = {};
		sdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		sdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		sdesc.Texture2D.MipLevels = 1;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = m_descHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += (m_bldMesh->m_MySrvOffsetOnHeap + i) * m_cbvSrvStride;
		_dev->CreateShaderResourceView(m_bldMesh->m_material[i].pTexture, &sdesc, handle);
	}

	//for (int i = 0; i < m_stair->m_numTexture; i++)
	//{
	//	D3D12_SHADER_RESOURCE_VIEW_DESC sdesc = {};
	//	sdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//	sdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//	sdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    //sdesc.Texture2D.MipLevels = 1;
	//	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_descHeap->GetCPUDescriptorHandleForHeapStart();
	//	handle.ptr += (m_stair->m_MySrvOffsetOnHeap + i) * m_cbvSrvStride;
	//	_dev->CreateShaderResourceView(m_stair->m_material[i].pTexture, &sdesc, handle);
	//}

	//シェーダー作成
	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;
	{
		D3DCompileFromFile(L"Phong.hlsl", NULL, NULL, "VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
		D3DCompileFromFile(L"Phong.hlsl", NULL, NULL, "PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
	}

	//頂点レイアウト作成
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	//ルートシグネチャ作成
	{
		//サンプラーを作成 スタティックサンプラーをルートシグネチャに直接セット
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		//コンスタントバッファビュー、と今回はテクスチャーもテーブルとしてルートに作成
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, m_numConstantBuffers, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_numTextures, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
			1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);
		ID3DBlob* signature;
		ID3DBlob* error;
		D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		_dev->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
	}

	//パイプラインステートオブジェクト作成
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature;
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = false;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
		psoDesc.DepthStencilState.BackFace = defaultStencilOp;
		psoDesc.DepthStencilState.StencilEnable = false;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		_dev->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	}

	NVectors = m_bldMesh->NVectors;
	NPoints = m_bldMesh->NPoints;

	return S_OK;
}

void Dx12Wrapper::RenderMesh(Mesh* pMesh)
{



	//World View Projection 変換
	XMMATRIX rotMat, world;
	XMMATRIX view;
	XMMATRIX proj;
	rotMat = XMMatrixRotationY(3.14);
	//ワールドトランスフォーム
	world = XMMatrixTranslation(pMesh->m_pos.x, pMesh->m_pos.y, pMesh->m_pos.z);
	// ビュートランスフォーム
	XMFLOAT3 eye(0, CameraPositionY, CameraPositionZ);//カメラ 位置
	XMFLOAT3 at(0, 15, 0);//カメラ　方向
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);//上方位置
	//view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&at), XMLoadFloat3(&up));
	
	//キャラクターのビューをセット
	view = _mappedSceneData->view;

	// プロジェクショントランスフォーム
	//フィールドは深いため別で作成
	proj = XMMatrixPerspectiveFovLH(3.14159 / 4, (FLOAT)1280 / (FLOAT)720, 0.1f, 30000.0f);

	//バーテックスバッファをセット
	_cmdList->IASetVertexBuffers(0, 1, &pMesh->m_vertexBufferView);

	//マテリアルの数だけ、それぞれのマテリアルのインデックスバッファ－を描画
	int texIndex = 0;
	for (DWORD i = 0; i < pMesh->m_numMaterial; i++)
	{
		if (pMesh->m_material[i].numFace == 0)
		{
			continue;
		}
		//コンスタントバッファの内容を更新
		CD3DX12_RANGE readRange(0, 0);
		readRange = CD3DX12_RANGE(0, 0);
		UINT8* pCbvDataBegin;
		m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pCbvDataBegin));
		Cbuffer cb;
		char* ptr = reinterpret_cast<char*>(pCbvDataBegin);
		ptr += (pMesh->m_MyCbvOffsetOnHeap + i) * m_cBSize;

		cb.wvp = XMMatrixTranspose(world * view * proj);//ワールド、カメラ、射影行列を渡す
		cb.w = XMMatrixTranspose(world);//ワールド行列を渡す
		cb.lightDir = XMFLOAT4(1, 1, 1, 0);//ライト方向を渡す
		if (pMesh->name == "階段") {
			cb.lightDir = XMFLOAT4(1, 10, 1, 0);
		}
		cb.eye = XMFLOAT4(eye.x, eye.y, eye.z, 1);//視点を渡す
		cb.ambient = XMFLOAT4(pMesh->m_material[i].Ka.x, pMesh->m_material[i].Ka.y,
			pMesh->m_material[i].Ka.z, pMesh->m_material[i].Ka.w);
		cb.diffuse = XMFLOAT4(pMesh->m_material[i].Kd.x, pMesh->m_material[i].Kd.y,
			pMesh->m_material[i].Kd.z, pMesh->m_material[i].Kd.w);
		cb.specular = XMFLOAT4(pMesh->m_material[i].Ks.x, pMesh->m_material[i].Ks.y,
			pMesh->m_material[i].Ks.z, pMesh->m_material[i].Ks.w);

		memcpy(ptr, &cb, sizeof(Cbuffer));

		//ディスクリプター(CBV)をセット
		D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descHeap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += (pMesh->m_MyCbvOffsetOnHeap + i) * m_cbvSrvStride;
		_cmdList->SetGraphicsRootDescriptorTable(0, handle);
		//ディスクリプター(SRV)をセット
		if (pMesh->m_numTexture > 0)
		{
			handle = m_descHeap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += pMesh->m_MySrvOffsetOnHeap * m_cbvSrvStride;
			_cmdList->SetGraphicsRootDescriptorTable(1, handle);
		}
		//インデックスバッファをセット
		_cmdList->IASetIndexBuffer(&pMesh->m_indexBufferView[i]);
		//draw
		_cmdList->DrawIndexedInstanced(pMesh->m_material[i].numFace * 3, 1, 0, 0, 0);
	}
}

//
//シーンを画面にレンダリング
void Dx12Wrapper::Render()
{
	//バックバッファが現在何枚目かを取得
	UINT backBufferIndex = _swapchain->GetCurrentBackBufferIndex();


	//コマンドリストに書き込む前にはコマンドアロケーターをリセットする
	//_cmdAllocator->Reset();
	//コマンドリストをリセットする
	//_cmdList->Reset(_cmdAllocator.Get(), NULL);

	_cmdList->SetPipelineState(m_pipelineState);

	//バックバッファのトランジションをレンダーターゲットモードにする
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[backBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//レンダーターゲットを指定
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//深度を指定
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
	//_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//ビューポートをセット
	CD3DX12_VIEWPORT  viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)1280, (float)720);
	CD3DX12_RECT  scissorRect = CD3DX12_RECT(0, 0, 1280, 720);
	_cmdList->RSSetViewports(1, &viewport);
	_cmdList->RSSetScissorRects(1, &scissorRect);

	//ルートシグネチャをセット
	_cmdList->SetGraphicsRootSignature(m_rootSignature);

	//ヒープ（アプリにただ1つだけ）をセット
	ID3D12DescriptorHeap* ppHeaps[] = { m_descHeap };
	_cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	//ポリゴントポロジーの指定
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	RenderMesh(m_skyMesh);

	//カメラを上に持っていくとキャラの足が埋まってしまうバグを強制的に排除する
	float X = CameraPositionY / 20;
	if (X < 0) {
		X = 0;
	}
	m_bldMesh->m_pos.y = - X -0.3 ;//足が埋まってしまうため0.3浮かせる

	RenderMesh(m_bldMesh);

	//RenderMesh(m_stair);

	//バックバッファのトランジションをPresentモードにする
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[backBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ExecuteCommand();

	//コマンドの書き込みはここで終わり、Closeする
	//_cmdList->Close();

	//コマンドリストの実行
	//ID3D12CommandList* ppCommandLists[] = { _cmdList.Get() };
	//_cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//バックバッファをフロントバッファに切り替えてシーンをモニターに表示
	//_swapchain->Present(1, 0);

	//GPUの処理が完了するまで待つ
	//WaitGpu();

	//60fps固定
	//static unsigned int time60 = 0;
	//while (timeGetTime() - time60 < 16);
	//time60 = timeGetTime();
}

//
//同期処理　Gpuの処理が完了するまで待つ
void Dx12Wrapper::WaitGpu()
{
	//GPUサイドが全て完了したときにGPUサイドから返ってくる値（フェンス値）をセット
	_cmdQueue->Signal(_fence.Get(), _fenceVal);

	//上でセットしたシグナルがGPUから帰ってくるまでストール（この行で待機）
	do
	{
		//GPUの完了を待つ間、ここで何か有意義な事（CPU作業）をやるほど効率が上がる

	} while (_fence->GetCompletedValue() < _fenceVal);

	//ここでフェンス値を更新する 前回より大きな値であればどんな値でもいいわけだが、1足すのが簡単なので1を足す
	_fenceVal++;
}

void Dx12Wrapper::SetCameraPosition(float Y, float Z)
{
	CameraPositionY = Y;
	CameraPositionZ = Z;
}


void Dx12Wrapper::DestroyD3D()
{
	
}