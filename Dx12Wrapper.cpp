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
	///���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	///@param modelPath �A�v���P�[�V�������猩��pmd���f���̃p�X
	///@param texPath PMD���f�����猩���e�N�X�`���̃p�X
	///@return �A�v���P�[�V�������猩���e�N�X�`���̃p�X
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		//�t�@�C���̃t�H���_��؂��\��/�̓��ނ��g�p�����\��������
		//�Ƃ�����������\��/�𓾂���΂����̂ŁA�o����rfind���Ƃ��r����
		//int�^�ɑ�����Ă���̂͌�����Ȃ������ꍇ��rfind��epos(-1��0xffffffff)��Ԃ�����
		int pathIndex1 = modelPath.rfind('/');
		int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}

	///�t�@�C��������g���q���擾����
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	string
		GetExtension(const std::string& path) {
		int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///�t�@�C��������g���q���擾����(���C�h������)
	///@param path �Ώۂ̃p�X������
	///@return �g���q
	wstring
		GetExtension(const std::wstring& path) {
		int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	///�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	///@param path �Ώۂ̃p�X������
	///@param splitter ��؂蕶��
	///@return �����O��̕�����y�A
	pair<string, string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		pair<string, string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}

	///string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
	///@param str �}���`�o�C�g������
	///@return �ϊ����ꂽ���C�h������
	std::wstring
		GetWideStringFromString(const std::string& str) {
		//�Ăяo��1���(�����񐔂𓾂�)
		auto num1 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, nullptr, 0);

		std::wstring wstr;//string��wchar_t��
		wstr.resize(num1);//����ꂽ�����񐔂Ń��T�C�Y

		//�Ăяo��2���(�m�ۍς݂�wstr�ɕϊ���������R�s�[)
		auto num2 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, &wstr[0], num1);

		assert(num1 == num2);//�ꉞ�`�F�b�N
		return wstr;
	}
	///�f�o�b�O���C���[��L���ɂ���
	void EnableDebugLayer() {
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
	}
}
Dx12Wrapper::Dx12Wrapper(HWND hwnd){
#ifdef _DEBUG
	//�f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif

	auto& app=Application::Instance();
	_winSize = app.GetWindowSize();

	//DirectX12�֘A������
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

	//�e�N�X�`�����[�_�[�֘A������
	CreateTextureLoaderTable();



	//�[�x�o�b�t�@�쐬
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return ;
	}

	//�t�F���X�̍쐬
	if (FAILED(_dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.ReleaseAndGetAddressOf())))) {
		assert(0);
		return ;
	}
	
}

HRESULT 
Dx12Wrapper::CreateDepthStencilView() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);
	//�[�x�o�b�t�@�쐬
	//�[�x�o�b�t�@�̎d�l
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

	//�f�v�X�p�q�[�v�v���p�e�B
	auto depthHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, //�f�v�X�������݂Ɏg�p
		&depthClearValue,
		IID_PPV_ARGS(_depthBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		//�G���[����
		return result;
	}

	//�[�x�̂��߂̃f�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};//�[�x�Ɏg����Ƃ��������킩��΂���
	dsvHeapDesc.NumDescriptors = 1;//�[�x�r���[1�̂�
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;//�f�v�X�X�e���V���r���[�Ƃ��Ďg��
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;


	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(_dsvHeap.ReleaseAndGetAddressOf()));

	//�[�x�r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;//�f�v�X�l��32bit�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;//�t���O�͓��ɂȂ�
	_dev->CreateDepthStencilView(_depthBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}


Dx12Wrapper::~Dx12Wrapper()
{
}


ComPtr<ID3D12Resource>
Dx12Wrapper::GetTextureByPath(const char* texpath) {
	auto it = _textureTable.find(texpath);
	if (it != _textureTable.end()) {
		//�e�[�u���ɓ��ɂ������烍�[�h����̂ł͂Ȃ��}�b�v����
		//���\�[�X��Ԃ�
		return _textureTable[texpath];
	}
	else {
		return ComPtr<ID3D12Resource>(CreateTextureFromFile(texpath));
	}

}

//�e�N�X�`�����[�_�e�[�u���̍쐬
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

//�e�N�X�`��������e�N�X�`���o�b�t�@�쐬�A���g���R�s�[
ID3D12Resource* 
Dx12Wrapper::CreateTextureFromFile(const char* texpath) {
	string texPath = texpath;
	//�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	auto wtexpath = GetWideStringFromString(texPath);//�e�N�X�`���̃t�@�C���p�X
	auto ext = GetExtension(texPath);//�g���q���擾
	auto result = _loadLambdaTable[ext](wtexpath,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return nullptr;
	}
	auto img = scratchImg.GetImage(0, 0, 0);//���f�[�^���o

	//WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);

	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	if (FAILED(result)) {
		return nullptr;
	}
	result = texbuff->WriteToSubresource(0,
		nullptr,//�S�̈�փR�s�[
		img->pixels,//���f�[�^�A�h���X
		img->rowPitch,//1���C���T�C�Y
		img->slicePitch//�S�T�C�Y
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
	//DirectX12�܂�菉����
	//�t�B�[�`�����x����
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
	//Direct3D�f�o�C�X�̏�����
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

///�X���b�v�`�F�C�������֐�
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

//�R�}���h�܂�菉����
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
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�v���C�I���e�B���Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�����̓R�}���h���X�g�ƍ��킹�Ă�������
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(_cmdQueue.ReleaseAndGetAddressOf()));//�R�}���h�L���[����
	assert(SUCCEEDED(result));
	return result;
}

//�r���[�v���W�F�N�V�����p�r���[�̐���
HRESULT 
Dx12Wrapper::CreateSceneView(){
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(SceneData) + 0xff) & ~0xff);
	//�萔�o�b�t�@�쐬
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

	_mappedSceneData = nullptr;//�}�b�v��������|�C���^
	result = _sceneConstBuff->Map(0, nullptr, (void**)&_mappedSceneData);//�}�b�v
	
	XMFLOAT3 eye(0, 15, -30);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);
	_mappedSceneData->view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
	_mappedSceneData->proj = XMMatrixPerspectiveFovLH(XM_PIDIV4,//��p��45��
		static_cast<float>(desc.Width) / static_cast<float>(desc.Height),//�A�X��
		0.1f,//�߂���
		1000.0f//������
	);						
	_mappedSceneData->eye = eye;
	
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_���猩����悤��
	descHeapDesc.NodeMask = 0;//�}�X�N��0
	descHeapDesc.NumDescriptors = 1;//
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//�f�X�N���v�^�q�[�v���
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(_sceneDescHeap.ReleaseAndGetAddressOf()));//����

	////�f�X�N���v�^�̐擪�n���h�����擾���Ă���
	auto heapHandle = _sceneDescHeap->GetCPUDescriptorHandleForHeapStart();
	
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _sceneConstBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = _sceneConstBuff->GetDesc().Width;
	//�萔�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, heapHandle);
	return result;

}

HRESULT	
Dx12Wrapper::CreateFinalRenderTargets() {
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = _swapchain->GetDesc1(&desc);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//�����_�[�^�[�Q�b�g�r���[�Ȃ̂œ��RRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//���Ɏw��Ȃ�

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeaps.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		SUCCEEDED(result);
		return result;
	}
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	_backBuffers.resize(swcDesc.BufferCount);

	D3D12_CPU_DESCRIPTOR_HANDLE handle = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//SRGB�����_�[�^�[�Q�b�g�r���[�ݒ�
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
	//DirectX����
	//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(_backBuffers[bbIdx],
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	_cmdList->ResourceBarrier(1, &barrier);


	//�����_�[�^�[�Q�b�g���w��
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
	_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


	//��ʃN���A
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//���F
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	//�r���[�|�[�g�A�V�U�[��`�̃Z�b�g
	_cmdList->RSSetViewports(1, _viewport.get());
	_cmdList->RSSetScissorRects(1, _scissorrect.get());


}


void 
Dx12Wrapper::SetScene() {
	//���݂̃V�[��(�r���[�v���W�F�N�V����)���Z�b�g
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

	//���߂̃N���[�Y
	_cmdList->Close();
	//�R�}���h���X�g�̎��s
	ID3D12CommandList* cmdlists[] = { _cmdList.Get() };
	_cmdQueue->ExecuteCommandLists(1, cmdlists);
	////�҂�
	_cmdQueue->Signal(_fence.Get(), ++_fenceVal);

	if (_fence->GetCompletedValue() < _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		_fence->SetEventOnCompletion(_fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	_cmdAllocator->Reset();//�L���[���N���A
	_cmdList->Reset(_cmdAllocator.Get(), nullptr);//�ĂуR�}���h���X�g�����߂鏀��
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

		//�f�[�^�����ƂɎ����Ńe�N�X�`���[���쐬����
		//�܂��󔒂̃e�N�X�`���[���쐬����
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
		//CPU����GPU�փe�N�X�`���[�f�[�^��n���Ƃ��̒��p�Ƃ���Upload Heap�����
		ID3D12Resource* pTextureUploadHeap;
		DWORD bufferSize = GetRequiredIntermediateSize(pMesh->m_material[i].pTexture, 0, 1);

		_dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pTextureUploadHeap));

		//�ǂ݂������e�N�Z���f�[�^���󔒃e�N�X�`���[�ɗ������݁A�e�N�X�`���[�Ƃ��Ċ���������
		

		//�R�}���h���X�g�ɏ������ޑO�ɂ̓R�}���h�A���P�[�^�[�����Z�b�g����
		_cmdAllocator->Reset();
		//�R�}���h���X�g�����Z�b�g����
		_cmdList->Reset(_cmdAllocator.Get(), NULL);

		UpdateSubresources(_cmdList.Get(), pMesh->m_material[i].pTexture, pTextureUploadHeap, 0, 0, 1, &Subres);
		_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pMesh->m_material[i].pTexture,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
		//�����ň�U�R�}���h�����B�e�N�X�`���[�̓]�����J�n���邽��
		_cmdList->Close();
		//�R�}���h���X�g�̎��s�@
		ID3D12CommandList* ppCommandLists[] = { _cmdList.Get() };
		_cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		//�����i�ҋ@�j�@�e�N�X�`���[�̓]�����I���܂őҋ@
		{
			_cmdQueue->Signal(_fence.Get(), _fenceVal);
			//��ŃZ�b�g�����V�O�i����GPU����A���Ă���܂ŃX�g�[���i���̍s�őҋ@�j
			while (_fence->GetCompletedValue() < _fenceVal);
		}
	}
}


HRESULT Dx12Wrapper::CreateField() {

	int kFrameCount = 2;
	m_rtvStride = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsvStride = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbvSrvStride = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	//�����_�[�^�[�Q�b�g�쐬
	{
		//�����_�[�^�[�Q�b�g�r���[�̃q�[�v�쐬
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = kFrameCount;//////////////////////////��ʃo�b�t�@��
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		_dev->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

		//�����_�[�^�[�Q�b�g�r���[�쐬
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT n = 0; n < kFrameCount; n++)
		{
			_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			_dev->CreateRenderTargetView(m_renderTargets[n], NULL, rtvHandle);
			rtvHandle.Offset(1, m_rtvStride);
		}
	}


	//���b�V���쐬
	m_skyMesh = new Mesh;
	m_bldMesh = new Mesh;
	//m_stair = new Mesh;   ���̕��@�Ŗ����ɃI�u�W�F�N�g�𑝂₹��
	m_skyMesh->name = "��";
	m_bldMesh->name = "�n��";
	//m_stair->name = "�K�i";�@�K�i�f�O

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

	//�f�B�X�N���v�^�[�q�[�v�̍쐬
	//�K�v�ȃf�B�X�N���v�^�[�����Z�o CBV�̓q�[�v��0�Ԃ���ASRV��CBV�̌ォ��
	int numDescriptors = 0;
	m_numConstantBuffers = m_skyMesh->m_numMaterial + m_bldMesh->m_numMaterial;// +m_stair->m_numMaterial;
	m_numTextures = m_skyMesh->m_numTexture + m_bldMesh->m_numTexture;// +m_stair->m_numTexture;
	numDescriptors += m_numConstantBuffers;//CBV�̕�
	numDescriptors += m_numTextures;//SRV�̕�

	//�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	_dev->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_descHeap));

	//�R���X�^���g�o�b�t�@�쐬
	{
		//�R���X�^���g�o�b�t�@�쐬
		m_cBSize = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT * ((sizeof(Cbuffer) / D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) + 1);
		_dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_cBSize * m_numConstantBuffers),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer));

		//�R���X�^���g�o�b�t�@�r���[�쐬
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

	//�e���b�V���Ɏ����̃f�B�X�N���v�^�[�̃I�t�Z�b�g�������Ă��
	m_skyMesh->m_MyCbvOffsetOnHeap = 0;
	m_bldMesh->m_MyCbvOffsetOnHeap = m_skyMesh->m_numMaterial;
	//m_stair->m_MyCbvOffsetOnHeap = m_bldMesh->m_MyCbvOffsetOnHeap + m_bldMesh->m_numMaterial;

	m_skyMesh->m_MySrvOffsetOnHeap = m_skyMesh->m_numMaterial + m_bldMesh->m_numMaterial;// +m_stair->m_numMaterial;
	m_bldMesh->m_MySrvOffsetOnHeap = m_skyMesh->m_MySrvOffsetOnHeap + m_skyMesh->m_numTexture;
	//m_stair->m_MySrvOffsetOnHeap = m_bldMesh->m_MySrvOffsetOnHeap + m_bldMesh->m_numTexture;

	//���b�V�����̃e�N�X�`���[�����C�����ō쐬
	CreateMeshTexture(m_skyMesh);
	CreateMeshTexture(m_bldMesh);
	//CreateMeshTexture(m_stair);

	//���b�V���̃e�N�X�`���[�̃r���[�iSRV)�����
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

	//�V�F�[�_�[�쐬
	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;
	{
		D3DCompileFromFile(L"Phong.hlsl", NULL, NULL, "VS", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
		D3DCompileFromFile(L"Phong.hlsl", NULL, NULL, "PS", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
	}

	//���_���C�A�E�g�쐬
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	//���[�g�V�O�l�`���쐬
	{
		//�T���v���[���쐬 �X�^�e�B�b�N�T���v���[�����[�g�V�O�l�`���ɒ��ڃZ�b�g
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
		//�R���X�^���g�o�b�t�@�r���[�A�ƍ���̓e�N�X�`���[���e�[�u���Ƃ��ă��[�g�ɍ쐬
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

	//�p�C�v���C���X�e�[�g�I�u�W�F�N�g�쐬
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



	//World View Projection �ϊ�
	XMMATRIX rotMat, world;
	XMMATRIX view;
	XMMATRIX proj;
	rotMat = XMMatrixRotationY(3.14);
	//���[���h�g�����X�t�H�[��
	world = XMMatrixTranslation(pMesh->m_pos.x, pMesh->m_pos.y, pMesh->m_pos.z);
	// �r���[�g�����X�t�H�[��
	XMFLOAT3 eye(0, CameraPositionY, CameraPositionZ);//�J���� �ʒu
	XMFLOAT3 at(0, 15, 0);//�J�����@����
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);//����ʒu
	//view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&at), XMLoadFloat3(&up));
	
	//�L�����N�^�[�̃r���[���Z�b�g
	view = _mappedSceneData->view;

	// �v���W�F�N�V�����g�����X�t�H�[��
	//�t�B�[���h�͐[�����ߕʂō쐬
	proj = XMMatrixPerspectiveFovLH(3.14159 / 4, (FLOAT)1280 / (FLOAT)720, 0.1f, 30000.0f);

	//�o�[�e�b�N�X�o�b�t�@���Z�b�g
	_cmdList->IASetVertexBuffers(0, 1, &pMesh->m_vertexBufferView);

	//�}�e���A���̐������A���ꂼ��̃}�e���A���̃C���f�b�N�X�o�b�t�@�|��`��
	int texIndex = 0;
	for (DWORD i = 0; i < pMesh->m_numMaterial; i++)
	{
		if (pMesh->m_material[i].numFace == 0)
		{
			continue;
		}
		//�R���X�^���g�o�b�t�@�̓��e���X�V
		CD3DX12_RANGE readRange(0, 0);
		readRange = CD3DX12_RANGE(0, 0);
		UINT8* pCbvDataBegin;
		m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pCbvDataBegin));
		Cbuffer cb;
		char* ptr = reinterpret_cast<char*>(pCbvDataBegin);
		ptr += (pMesh->m_MyCbvOffsetOnHeap + i) * m_cBSize;

		cb.wvp = XMMatrixTranspose(world * view * proj);//���[���h�A�J�����A�ˉe�s���n��
		cb.w = XMMatrixTranspose(world);//���[���h�s���n��
		cb.lightDir = XMFLOAT4(1, 1, 1, 0);//���C�g������n��
		if (pMesh->name == "�K�i") {
			cb.lightDir = XMFLOAT4(1, 10, 1, 0);
		}
		cb.eye = XMFLOAT4(eye.x, eye.y, eye.z, 1);//���_��n��
		cb.ambient = XMFLOAT4(pMesh->m_material[i].Ka.x, pMesh->m_material[i].Ka.y,
			pMesh->m_material[i].Ka.z, pMesh->m_material[i].Ka.w);
		cb.diffuse = XMFLOAT4(pMesh->m_material[i].Kd.x, pMesh->m_material[i].Kd.y,
			pMesh->m_material[i].Kd.z, pMesh->m_material[i].Kd.w);
		cb.specular = XMFLOAT4(pMesh->m_material[i].Ks.x, pMesh->m_material[i].Ks.y,
			pMesh->m_material[i].Ks.z, pMesh->m_material[i].Ks.w);

		memcpy(ptr, &cb, sizeof(Cbuffer));

		//�f�B�X�N���v�^�[(CBV)���Z�b�g
		D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descHeap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += (pMesh->m_MyCbvOffsetOnHeap + i) * m_cbvSrvStride;
		_cmdList->SetGraphicsRootDescriptorTable(0, handle);
		//�f�B�X�N���v�^�[(SRV)���Z�b�g
		if (pMesh->m_numTexture > 0)
		{
			handle = m_descHeap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += pMesh->m_MySrvOffsetOnHeap * m_cbvSrvStride;
			_cmdList->SetGraphicsRootDescriptorTable(1, handle);
		}
		//�C���f�b�N�X�o�b�t�@���Z�b�g
		_cmdList->IASetIndexBuffer(&pMesh->m_indexBufferView[i]);
		//draw
		_cmdList->DrawIndexedInstanced(pMesh->m_material[i].numFace * 3, 1, 0, 0, 0);
	}
}

//
//�V�[������ʂɃ����_�����O
void Dx12Wrapper::Render()
{
	//�o�b�N�o�b�t�@�����݉����ڂ����擾
	UINT backBufferIndex = _swapchain->GetCurrentBackBufferIndex();


	//�R�}���h���X�g�ɏ������ޑO�ɂ̓R�}���h�A���P�[�^�[�����Z�b�g����
	//_cmdAllocator->Reset();
	//�R�}���h���X�g�����Z�b�g����
	//_cmdList->Reset(_cmdAllocator.Get(), NULL);

	_cmdList->SetPipelineState(m_pipelineState);

	//�o�b�N�o�b�t�@�̃g�����W�V�����������_�[�^�[�Q�b�g���[�h�ɂ���
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[backBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�����_�[�^�[�Q�b�g���w��
	auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
	auto rtvH = _rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//�[�x���w��
	auto dsvH = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);
	//_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//�r���[�|�[�g���Z�b�g
	CD3DX12_VIEWPORT  viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)1280, (float)720);
	CD3DX12_RECT  scissorRect = CD3DX12_RECT(0, 0, 1280, 720);
	_cmdList->RSSetViewports(1, &viewport);
	_cmdList->RSSetScissorRects(1, &scissorRect);

	//���[�g�V�O�l�`�����Z�b�g
	_cmdList->SetGraphicsRootSignature(m_rootSignature);

	//�q�[�v�i�A�v���ɂ���1�����j���Z�b�g
	ID3D12DescriptorHeap* ppHeaps[] = { m_descHeap };
	_cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	//�|���S���g�|���W�[�̎w��
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	RenderMesh(m_skyMesh);

	//�J��������Ɏ����Ă����ƃL�����̑������܂��Ă��܂��o�O�������I�ɔr������
	float X = CameraPositionY / 20;
	if (X < 0) {
		X = 0;
	}
	m_bldMesh->m_pos.y = - X -0.3 ;//�������܂��Ă��܂�����0.3��������

	RenderMesh(m_bldMesh);

	//RenderMesh(m_stair);

	//�o�b�N�o�b�t�@�̃g�����W�V������Present���[�h�ɂ���
	_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[backBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ExecuteCommand();

	//�R�}���h�̏������݂͂����ŏI���AClose����
	//_cmdList->Close();

	//�R�}���h���X�g�̎��s
	//ID3D12CommandList* ppCommandLists[] = { _cmdList.Get() };
	//_cmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//�o�b�N�o�b�t�@���t�����g�o�b�t�@�ɐ؂�ւ��ăV�[�������j�^�[�ɕ\��
	//_swapchain->Present(1, 0);

	//GPU�̏�������������܂ő҂�
	//WaitGpu();

	//60fps�Œ�
	//static unsigned int time60 = 0;
	//while (timeGetTime() - time60 < 16);
	//time60 = timeGetTime();
}

//
//���������@Gpu�̏�������������܂ő҂�
void Dx12Wrapper::WaitGpu()
{
	//GPU�T�C�h���S�Ċ��������Ƃ���GPU�T�C�h����Ԃ��Ă���l�i�t�F���X�l�j���Z�b�g
	_cmdQueue->Signal(_fence.Get(), _fenceVal);

	//��ŃZ�b�g�����V�O�i����GPU����A���Ă���܂ŃX�g�[���i���̍s�őҋ@�j
	do
	{
		//GPU�̊�����҂ԁA�����ŉ����L�Ӌ`�Ȏ��iCPU��Ɓj�����قǌ������オ��

	} while (_fence->GetCompletedValue() < _fenceVal);

	//�����Ńt�F���X�l���X�V���� �O����傫�Ȓl�ł���΂ǂ�Ȓl�ł������킯�����A1�����̂��ȒP�Ȃ̂�1�𑫂�
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