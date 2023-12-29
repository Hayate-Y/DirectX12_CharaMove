#pragma once

#include<d3d12.h>
#include<DirectXMath.h>
#include<vector>
#include<map>
#include<unordered_map>
#include<wrl.h>

class Dx12Wrapper;
class PMDRenderer;
class PMDActor
{
	friend PMDRenderer;
private:
	unsigned int _duration = 0;
	std::string motionname;

	PMDRenderer& _renderer;
	Dx12Wrapper& _dx12;
	DirectX::XMMATRIX _localMat;
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	
	//頂点関連
	ComPtr<ID3D12Resource> _vb = nullptr;
	ComPtr<ID3D12Resource> _ib = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	ComPtr<ID3D12Resource> _transformMat = nullptr;//座標変換行列(今はワールドのみ)
	ComPtr<ID3D12DescriptorHeap> _transformHeap = nullptr;//座標変換ヒープ

	//シェーダ側に投げられるマテリアルデータ
	struct MaterialForHlsl {
		DirectX::XMFLOAT3 diffuse; //ディフューズ色
		float alpha; // ディフューズα
		DirectX::XMFLOAT3 specular; //スペキュラ色
		float specularity;//スペキュラの強さ(乗算値)
		DirectX::XMFLOAT3 ambient; //アンビエント色
	};
	//それ以外のマテリアルデータ
	struct AdditionalMaterial {
		std::string texPath;//テクスチャファイルパス
		int toonIdx; //トゥーン番号
		bool edgeFlg;//マテリアル毎の輪郭線フラグ
	};
	//まとめたもの
	struct Material {
		unsigned int indicesNum;//インデックス数
		MaterialForHlsl material;
		AdditionalMaterial additional;
	};

	struct Transform {
		//内部に持ってるXMMATRIXメンバが16バイトアライメントであるため
		//Transformをnewする際には16バイト境界に確保する
		void* operator new(size_t size);
		DirectX::XMMATRIX world;
	};

	Transform _transform;
	DirectX::XMMATRIX* _mappedMatrices = nullptr;
	ComPtr<ID3D12Resource> _transformBuff = nullptr;

	//マテリアル関連
	std::vector<Material> _materials;
	ComPtr<ID3D12Resource> _materialBuff = nullptr;
	std::vector<ComPtr<ID3D12Resource>> _textureResources;
	std::vector<ComPtr<ID3D12Resource>> _sphResources;
	std::vector<ComPtr<ID3D12Resource>> _spaResources;
	std::vector<ComPtr<ID3D12Resource>> _toonResources;

	//ボーン関連
	std::vector<DirectX::XMMATRIX> _boneMatrices;

	//失敗、時間があれば後で考える
	//走りと歩きの補完をするための関数など
	std::vector<DirectX::XMMATRIX> _boneMatrices1;
	std::vector<DirectX::XMMATRIX> _boneMatrices2;
	std::vector<DirectX::XMMATRIX> _boneMatrices3;
	std::vector<DirectX::XMMATRIX> _boneMatrices4;
	std::vector<DirectX::XMMATRIX> _boneMatrices5;
	int matrixcount = 0;
	int bonematrixsize;
	//一時的なマトリックス
	void AnimationBlendMatrix();

	struct BoneNode {
		uint32_t boneIdx;//ボーンインデックス
		uint32_t boneType;//ボーン種別
		uint32_t parentBone;
		uint32_t ikParentBone;//IK親ボーン
		DirectX::XMFLOAT3 startPos;//ボーン基準点(回転中心)
		std::vector<BoneNode*> children;//子ノード
	};
	std::unordered_map<std::string, BoneNode> _boneNodeTable;
	std::vector<std::string> _boneNameArray;//インデックスから名前を検索しやすいようにしておく
	std::vector<BoneNode*> _boneNodeAddressArray;//インデックスからノードを検索しやすいようにしておく


	struct PMDIK {
		uint16_t boneIdx;//IK対象のボーンを示す
		uint16_t targetIdx;//ターゲットに近づけるためのボーンのインデックス
		uint16_t iterations;//試行回数
		float limit;//一回当たりの回転制限
		std::vector<uint16_t> nodeIdxes;//間のノード番号
	};
	std::vector<PMDIK> _ikData;
	
	//読み込んだマテリアルをもとにマテリアルバッファを作成
	HRESULT CreateMaterialData();
	
	ComPtr< ID3D12DescriptorHeap> _materialHeap = nullptr;//マテリアルヒープ(5個ぶん)
	//マテリアル＆テクスチャのビューを作成
	HRESULT CreateMaterialAndTextureView();

	//座標変換用ビューの生成
	HRESULT CreateTransformView();

	//PMDファイルのロード
	HRESULT LoadPMDFile(const char* path);
	void RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat,bool flg=false);
	
	


	///キーフレーム構造体
	struct KeyFrame {
		unsigned int frameNo;//フレーム№(アニメーション開始からの経過時間)
		DirectX::XMVECTOR quaternion;//クォータニオン
		DirectX::XMFLOAT3 offset;//IKの初期座標からのオフセット情報
		DirectX::XMFLOAT2 p1, p2;//ベジェの中間コントロールポイント
		KeyFrame(
			unsigned int fno, 
			const DirectX::XMVECTOR& q,
			const DirectX::XMFLOAT3& ofst,
			const DirectX::XMFLOAT2& ip1,
			const DirectX::XMFLOAT2& ip2):
			frameNo(fno),
			quaternion(q),
			offset(ofst),
			p1(ip1),
			p2(ip2){}
	};

	



	float GetYFromXOnBezier(float x,const DirectX::XMFLOAT2& a,const DirectX::XMFLOAT2& b, uint8_t n = 12);
	
	std::vector<uint32_t> _kneeIdxes;

	DWORD _startTime;//アニメーション開始時点のミリ秒時刻
	
	void MotionUpdate();

	///CCD-IKによりボーン方向を解決
	///@param ik 対象IKオブジェクト
	void SolveCCDIK(const PMDIK& ik);

	///余弦定理IKによりボーン方向を解決
	///@param ik 対象IKオブジェクト
	void SolveCosineIK(const PMDIK& ik);
	
	///LookAt行列によりボーン方向を解決
	///@param ik 対象IKオブジェクト
	void SolveLookAt(const PMDIK& ik);

	void IKSolve(int frameNo);


	

	//IKオンオフデータ
	struct VMDIKEnable {
		uint32_t frameNo;
		std::unordered_map<std::string, bool> ikEnableTable;
	};
	std::vector<VMDIKEnable> _ikEnableData;

	//DirectX::XMFLOAT3 _pos;
	DirectX::XMMATRIX* _mappedTransform;

	//モーションを切り替えるかどうか
	bool MotionChangeNow=false;

public:

	struct VMDKeyFrame {
		char boneName[15]; // ボーン名
		unsigned int frameNo; // フレーム番号(読込時は現在のフレーム位置を0とした相対位置)
		DirectX::XMFLOAT3 location; // 位置
		DirectX::XMFLOAT4 quaternion; // Quaternion // 回転
		unsigned char bezier[64]; // [4][4][4]  ベジェ補完パラメータ
	};

	std::unordered_map<std::string, std::vector<KeyFrame>> _motiondata;
	DirectX::XMFLOAT3 _pos;
	//キャラの向き
	float _angle;

	//カメラの向き（上下）
	float roll;
	void CameraRollUpdate(float angle);

	//モーション
	bool MotionChancel=false;

	

	PMDActor(const char* filepath,PMDRenderer& renderer);
	~PMDActor();

	void LoadVMDFile(const char* filepath, const char* name);
	void Update();
	void Draw();
	void PlayAnimation();

	void LookAt(float x,float y, float z);

	void Move(float x, float y, float z);
	const DirectX::XMFLOAT3& GetPosition()const;

	void ChangeMotion();
	bool JadgeMotion();
	void VMDFileClear();
	void ChancelMotion();
	void CharaDirection(float angle ,  bool plus);

	//走るか
	bool IsRun;
	//走りと歩きを変更する
	void WalkOrRun();

	//地面の法線を格納する
	DirectX::XMFLOAT3 NVector;
	//法線の場所
	DirectX::XMFLOAT3 NPoint;
	std::vector<DirectX::XMFLOAT3> NVectors;
	std::vector<DirectX::XMFLOAT3> NPoints;


	//法線を格納する関数
	void CalNVector(std::vector<DirectX::XMFLOAT3> N , std::vector<DirectX::XMFLOAT3> NP);
	//高さ更新
	void HeightUpDate();

	//足先までのベクトル
	DirectX::XMFLOAT3 RIK;
	DirectX::XMFLOAT3 LIK;

	//キャラの向き（高さは関係ないのでxzで表現）
	DirectX::XMFLOAT2 FrontVector;
	//キャラの向きベクトルを変える関数
	void UpDateFrontVec();
	//キャラの正面のベクトルと法線の角度
	float Nangle;

	//歩きモーションで特定のフレーム時に足が地面にめり込むのでそれを修正
	bool leftleg;
	bool rightleg;

	float righty;
	float lefty;

	//どこに移動しているか
	//+Zなら＋、-Zなら-
	float DifferentialZ = 0;

	//現在のフレーム数
	int Frame;

};

