#include "Application.h"
#include"Dx12Wrapper.h"
#include"PMDRenderer.h"
#include"PMDActor.h"

//�E�B���h�E�萔
const unsigned int window_width = 1280;
const unsigned int window_height = 720;

//�ʓ|�����Ǐ����Ȃ�������
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//�E�B���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);//OS�ɑ΂��āu�������̃A�v���͏I�����v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
}

void 
Application::CreateGameWindow(HWND &hwnd, WNDCLASSEX &windowClass) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//�E�B���h�E�N���X�������o�^
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = _T("DirectXTest");//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	windowClass.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&windowClass);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0,0, window_width, window_height };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(windowClass.lpszClassName,//�N���X���w��
		_T("DX12���t�@�N�^�����O"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		windowClass.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^

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
	ShowWindow(_hwnd, SW_SHOW);//�E�B���h�E�\��
	float angle = 0.0f;
	MSG msg = {};
	unsigned int frame = 0;
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�����A�v���P�[�V�������I�����Ď���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}

		BYTE keycode[256];
		GetKeyboardState(keycode);
		//float x = 0, y = 0, z = 0;



		//�ǉ�

		static float yaw = 0; //����]
		static float roll = 0.4;//�c��]

		//�J�����ݒ�ɕK�v�Ȃ���
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
		
		//�J�����ړ����ǂ���
		XMFLOAT3 pulsepos;

		//�L�����̈ړ��X�s�[�h
		float warkspeed = 0.2;
		float runspeed = 0.4;
		float speed;
		if (run) {
			speed = runspeed;
		}
		else {
			speed = warkspeed;
		}

		//�L�[�{�[�h�C�x���g
		if (keycode['W'] & 0x80) {

			//�ړ��@��x�x�N�g���ɂ��Ă�����W�𑫂�
			XMStoreFloat3(&pulsepos, axisz * speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(3.1415f+yaw, false);
			}
		}
		if (keycode['A'] & 0x80) {

			//�ړ��@��x�x�N�g���ɂ��Ă�����W�𑫂�
			XMStoreFloat3(&pulsepos, axisx * -speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(3.1415 / 2 + yaw, false);
			}
		}
		if (keycode['S'] & 0x80) {

			//�ړ��@��x�x�N�g���ɂ��Ă�����W�𑫂�
			XMStoreFloat3(&pulsepos, axisz * -speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(yaw, false);
			}
		}
		if (keycode['D'] & 0x80) {

			//�ړ��@��x�x�N�g���ɂ��Ă�����W�𑫂�
			XMStoreFloat3(&pulsepos, axisx * speed);
			_pmdActor->_pos.x += pulsepos.x;
			_pmdActor->_pos.z += pulsepos.z;

			InputWASD = true;
			if (!angleplus) {
				_pmdActor->CharaDirection(3.1415 * 1.5 + yaw, false);
			}
		}

		//�J�����̍��E
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
		
		//����̓���
		if (GetKeyState(VK_SPACE) & 0x80) {
			run = true;
		}
		else {
			if (run) {
				run = false;
			}
		}

		

		//�J�����̏㉺
		// 
		//�J������YZ���̍��W
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
		//�J�����̍��W���擾
		CameraPositionY = 50*sinf(roll);
		CameraPositionZ = 50*cosf(roll);

		//�����]��
		if (!(GetKeyState(VK_LEFT) & 0x80) && !(GetKeyState(VK_RIGHT) & 0x80)) {
			angleplus = false;
		}

		//����n�߂Ƒ���I���ŌĂ΂��
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
		
		//���[�V�����̐؂�ւ�
		if (InputWASD && _pmdActor->JadgeMotion())
		{
			_pmdActor->ChangeMotion();
		}
		
		//���[�V�����L�����Z��
		if (!(keycode['W'] & 0x80) && !(keycode['A'] & 0x80) && !(keycode['S'] & 0x80) && !(keycode['D'] & 0x80) && InputWASD) {
			_pmdActor->ChancelMotion();
			InputWASD = false;
			isruning = false;
			run = false;
		}


		//�r���[���Z�b�g
		tmp = DirectX::XMFLOAT3(0.0f, 10.0f+CameraPositionY, -CameraPositionZ);
		DirectX::XMVECTOR m_eye = XMLoadFloat3(&tmp);//�J�����i���_�j�ʒu
		tmp = DirectX::XMFLOAT3(0.0f, 15.0f, 0.0f);
		DirectX::XMVECTOR lookat = XMLoadFloat3(&tmp);//�����ʒu
		tmp = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
		DirectX::XMVECTOR vUpVec = XMLoadFloat3(&tmp);//����ʒu
		DirectX::XMMATRIX pose;
		pose = rot * tran ;

		m_eye = XMVector3TransformCoord(m_eye, pose);
		lookat = XMVector3TransformCoord(lookat, pose);

		_dx12->_mappedSceneData->view= DirectX::XMMatrixLookAtLH(m_eye, lookat, vUpVec);

		//�I�u�W�F�N�g���̃J�������Z�b�g
		_dx12->SetCameraPosition(10.0f + CameraPositionY, -CameraPositionZ);

		//�S�̂̕`�揀��
		_dx12->BeginDraw();

		//PMD�p�̕`��p�C�v���C���ɍ��킹��
		_dx12->CommandList()->SetPipelineState(_pmdRenderer->GetPipelineState());
		//���[�g�V�O�l�`����PMD�p�ɍ��킹��
		_dx12->CommandList()->SetGraphicsRootSignature(_pmdRenderer->GetRootSignature());



		_dx12->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		_dx12->SetScene();
		
		_pmdActor->Update();

		_pmdActor->Draw();

	   _dx12->EndDraw();

		
		_dx12->Render();

		//�t���b�v
		_dx12->Swapchain()->Present(0, 0);
	}
}

bool 
Application::Init() {
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
	CreateGameWindow(_hwnd, _windowClass);

	//DirectX12���b�p�[������������
	_dx12.reset(new Dx12Wrapper(_hwnd));
	_pmdRenderer.reset(new PMDRenderer(*_dx12));
	_pmdActor.reset(new PMDActor("Model/�����~�N���l�N�^�CIK�Ȃ�.pmd", *_pmdRenderer));
	//�A�j���[�V���������[�h
	_pmdActor->LoadVMDFile("motion/�ҋ@.vmd", "�ҋ@");
	//_dx12->ExecuteCommand();
	_pmdActor->PlayAnimation();

	//�t�B�[���h�쐬
	_dx12->CreateField();
	//�n�ʂ̖@����pmd�ɓn��
	_pmdActor->CalNVector(_dx12->NVectors,_dx12->NPoints);


	return true;
}



void
Application::Terminate() {
	//�����N���X�g��񂩂�o�^�������Ă�
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


