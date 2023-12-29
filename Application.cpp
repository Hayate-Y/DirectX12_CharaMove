#include "Application.h"
#include"Dx12Wrapper.h"
#include"PMDRenderer.h"
#include"PMDActor.h"

//ウィンドウ定数
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//面倒だけど書かなあかんやつ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

void 
Application::CreateGameWindow(HWND &hwnd, WNDCLASSEX &windowClass) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//ウィンドウクラス生成＆登録
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	windowClass.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	windowClass.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&windowClass);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	//ウィンドウオブジェクトの生成
	hwnd = CreateWindow(windowClass.lpszClassName,//クラス名指定
		_T("DX12リファクタリング"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		windowClass.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ

}

SIZE
Application::GetWindowSize()const {
	SIZE ret;
	ret.cx = window_width;
	ret.cy = window_height;
	return ret;
}



void 
Application::Run() {
	ShowWindow(_hwnd, SW_SHOW);//ウィンドウ表示
	float angle = 0.0f;
	MSG msg = {};
	unsigned int frame = 0;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}

		BYTE keycode[256];
		GetKeyboardState(keycode);
		//float x = 0, y = 0, z = 0;



		//追加

		static float yaw = 0; //横回転
		static float roll = 0.4;//縦回転

		//カメラ設定に必要なもの
		DirectX::XMMATRIX rot, tran;
		DirectX::XMFLOAT3 tmp;
		tmp = DirectX::XMFLOAT3(1, 0, 0);
		DirectX::XMVECTOR axisx = XMLoadFloat3(&tmp);
		tmp = DirectX::XMFLOAT3(0, 1, 0);
		DirectX::XMVECTOR axisy = XMLoadFloat3(&tmp);
		tmp = DirectX::XMFLOAT3(0, 0, 1);
		DirectX::XMVECTOR axisz = XMLoadFloat3(&tmp);

		DirectX::XMFLOAT3 pos = _pmdActor->_pos;
		tran = DirectX::XMMatrixTranslation((pos.x),(pos.y), (pos.z));
		rot = DirectX::XMMatrixRotationY(yaw);
		axisx = XMVector3TransformCoord(axisx, rot);
		axisy = XMVector3TransformCoord(axisy, rot);
		axisz = XMVector3TransformCoord(axisz, rot);
		
		//カメラ移動かどうか
		XMFLOAT3 pulsepos;

		//キャラの移動スピード
		float warkspeed = 0.2;
		float runspeed = 0.4;
		float speed;
		if (run) {
			speed = runspeed;
		}
		else {
			speed = warkspeed;
		}

		//キーボードイベント
		if (keycode['W'] & 0x80) {

			//移動　一度ベクトルにしてから座標を足す
			XMStoreFloat3(&pulsepos, axisz * speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(3.1415f+yaw, false);
			}
		}
		if (keycode['A'] & 0x80) {

			//移動　一度ベクトルにしてから座標を足す
			XMStoreFloat3(&pulsepos, axisx * -speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(3.1415 / 2 + yaw, false);
			}
		}
		if (keycode['S'] & 0x80) {

			//移動　一度ベクトルにしてから座標を足す
			XMStoreFloat3(&pulsepos, axisz * -speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(yaw, false);
			}
		}
		if (keycode['D'] & 0x80) {

			//移動　一度ベクトルにしてから座標を足す
			XMStoreFloat3(&pulsepos, axisx * speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(3.1415 * 1.5 + yaw, false);
			}
		}

		//カメラの左右
		if (GetKeyState(VK_RIGHT) & 0x80) {
			angleplus = true;
			yaw += 0.01f;
			if (InputWASD) {
				_pmdActor->CharaDirection(0.01, true);
			}
		}

		if (GetKeyState(VK_LEFT) & 0x80) {
			angleplus = true;
			yaw -= 0.01f;
			if (InputWASD) {
				_pmdActor->CharaDirection(-0.01, true);
			}
		}
		
		//走りの入力
		if (GetKeyState(VK_SPACE) & 0x80) {
			run = true;
		}
		else {
			if (run) {
				run = false;
			}
		}

		

		//カメラの上下
		// 
		//カメラのYZ軸の座標
		static float CameraPositionY;
		static float CameraPositionZ;

		if (GetKeyState(VK_UP) & 0x80) {
			roll += 0.01f;
		}

		if (GetKeyState(VK_DOWN) & 0x80) {
			roll -= 0.01f;
		}

		if (roll > 1.0) {
			roll = 1.0;
		}

		if (roll < -1.0) {
			roll = -1.0;
		}
		//カメラの座標を取得
		CameraPositionY = 50*sinf(roll);
		CameraPositionZ = 50*cosf(roll);

		//方向転換
		if (!(GetKeyState(VK_LEFT) & 0x80) && !(GetKeyState(VK_RIGHT) & 0x80)) {
			angleplus = false;
		}

		//走り始めと走り終わりで呼ばれる
		if (InputWASD) {
			if (!isruning && run) {
				isruning = true;
				_pmdActor->IsRun = true;
				_pmdActor->WalkOrRun();
			}
			else if (isruning && !run) {
				_pmdActor->IsRun = false;
				isruning = false;
				_pmdActor->WalkOrRun();
			}
		}
		
		//モーションの切り替え
		if (InputWASD && _pmdActor->JadgeMotion())
		{
			_pmdActor->ChangeMotion();
		}
		
		//モーションキャンセル
		if (!(keycode['W'] & 0x80) && !(keycode['A'] & 0x80) && !(keycode['S'] & 0x80) && !(keycode['D'] & 0x80) && InputWASD) {
			_pmdActor->ChancelMotion();
			InputWASD = false;
			isruning = false;
			run = false;
		}


		//ビューをセット
		tmp = DirectX::XMFLOAT3(0.0f, 10.0f+CameraPositionY, -CameraPositionZ);
		DirectX::XMVECTOR m_eye = XMLoadFloat3(&tmp);//カメラ（視点）位置
		tmp = DirectX::XMFLOAT3(0.0f, 15.0f, 0.0f);
		DirectX::XMVECTOR lookat = XMLoadFloat3(&tmp);//注視位置
		tmp = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR vUpVec = XMLoadFloat3(&tmp);//上方位置
		DirectX::XMMATRIX pose;
		pose = rot * tran ;

		m_eye = XMVector3TransformCoord(m_eye, pose);
		lookat = XMVector3TransformCoord(lookat, pose);

		_dx12->_mappedSceneData->view= DirectX::XMMatrixLookAtLH(m_eye, lookat, vUpVec);

		//オブジェクト側のカメラもセット
		_dx12->SetCameraPosition(10.0f + CameraPositionY, -CameraPositionZ);

		//全体の描画準備
		_dx12->BeginDraw();

		//PMD用の描画パイプラインに合わせる
		_dx12->CommandList()->SetPipelineState(_pmdRenderer->GetPipelineState());
		//ルートシグネチャもPMD用に合わせる
		_dx12->CommandList()->SetGraphicsRootSignature(_pmdRenderer->GetRootSignature());



		_dx12->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		_dx12->SetScene();
		
		_pmdActor->Update();

		_pmdActor->Draw();

	   _dx12->EndDraw();

		
		_dx12->Render();

		//フリップ
		_dx12->Swapchain()->Present(0, 0);
	}
}

bool 
Application::Init() {
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);

	//DirectX12ラッパー生成＆初期化
	_dx12.reset(new Dx12Wrapper(_hwnd));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));
	_pmdActor.reset(new PMDActor("Model/初音ミク髪ネクタイIKなし.pmd", *_pmdRenderer));
	//アニメーションをロード
	_pmdActor->LoadVMDFile("motion/待機.vmd", "待機");
	//_dx12->ExecuteCommand();
	_pmdActor->PlayAnimation();

	//フィールド作成
	_dx12->CreateField();
	//地面の法線をpmdに渡す
	_pmdActor->CalNVector(_dx12->NVectors,_dx12->NPoints);


	return true;
}



void
Application::Terminate() {
	//もうクラス使わんから登録解除してや
	UnregisterClass(_windowClass.lpszClassName, _windowClass.hInstance);
}

Application& 
Application::Instance() {
	static Application instance;
	return instance;
}

Application::Application()
{
}


Application::~Application()
{
}


