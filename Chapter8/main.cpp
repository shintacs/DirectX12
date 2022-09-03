//ポリゴン表示
#include<Windows.h>
#include<tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<DirectXMath.h>
#include<vector>
#include<string>
#include<DirectXTex.h>
#include<d3dx12.h>

#include<d3dcompiler.h>
#ifdef _DEBUG
#include<iostream>
#endif


#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

#pragma pack(1) // 1バイトパッキングとなり，アライメントは発生しない（？）

// PMDマテリアル読み込み用の構造体
struct PMDMaterial
{
	XMFLOAT3 diffuse; // ディフューズ色
	float alpha; // ディフューズα
	float specularity; // スペキュラの強さ
	XMFLOAT3 specular; // スペキュラの色
	XMFLOAT3 ambient; // アンビエントの色
	unsigned char toonIdx; // トゥーン番号
	unsigned char edgeFlg; // マテリアルごとの輪郭線フラグ
	unsigned int indicesNum; // このマテリアルが割り当てられるインデックス数
	char texFilePath[20]; // テクスチャファイルパス+α
}; // 本来は70バイトだが，パディングによって72バイトになるらしい

#pragma pack() // 元に戻す

// PMD頂点構造体
struct PMDVertex
{
	XMFLOAT3 pos; // 頂点座標（12バイト）
	XMFLOAT3 normal; // 法線ベクトル（12バイト）
	XMFLOAT2 uv; // uv座標（8バイト）
	unsigned short boneno[2]; // ボーン番号（4バイト）
	unsigned char weight; // ボーン影響度(1バイト）
	unsigned char edgeFlg; // 輪郭線フラグ（1バイト）
};

// シェーダー側に渡すための行列データ
struct MatricesData
{
	// 法線ベクトルをモデルの動きに合わせるために定義
	XMMATRIX world; // モデル本体を回転させたり移動させたりするための行列
	XMMATRIX viewproj; // ビューとプロジェクション合成行列
};

// シェーダー側に投げられるマテリアルデータ
struct MaterialForHlsl
{
	XMFLOAT3 diffuse; // ディフーズ色
	float alpha; // ディフューズα
	XMFLOAT3 specular; // スペキュラ色
	float specularity; // スペキュラの強さ（乗算値）
	XMFLOAT3 ambient; // アンビエント色
};

// それ以外のマテリアルデータ
struct AdditionalMaterial
{
	std::string texPath; // テクスチャファイルパス
	int toonIdx; // トゥーン番号
	bool edgeFlg; // マテリアルごとの輪郭線フラグ
};

// 全体をまとめるデータ
struct Material
{
	unsigned int indicesNum; // インデックス数
	MaterialForHlsl material;
	AdditionalMaterial additional;
};


///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//面倒だけど書かなあかんやつ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら呼ばれます
		PostQuitMessage(0);//OSに対して「もうこのアプリは終わるんや」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//規定の処理を行う
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

IDXGIFactory6* _dxgiFactory = nullptr;
ID3D12Device* _dev = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
IDXGISwapChain4* _swapchain = nullptr;


void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)))) {
		debugLayer->EnableDebugLayer();
		debugLayer->Release();
	}
}

#ifdef _DEBUG
int main() {
#else
#include<Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	DebugOutputFormatString("Show window test.");
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//ウィンドウクラス生成＆登録
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	w.lpszClassName = _T("DirectXTest");//アプリケーションクラス名(適当でいいです)
	w.hInstance = GetModuleHandle(0);//ハンドルの取得
	RegisterClassEx(&w);//アプリケーションクラス(こういうの作るからよろしくってOSに予告する)

	RECT wrc = { 0,0, window_width, window_height };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//ウィンドウのサイズはちょっと面倒なので関数を使って補正する
	//ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,//クラス名指定
		_T("DX12 単純ポリゴンテスト"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,//タイトルバーと境界線があるウィンドウです
		CW_USEDEFAULT,//表示X座標はOSにお任せします
		CW_USEDEFAULT,//表示Y座標はOSにお任せします
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウハンドル
		nullptr,//メニューハンドル
		w.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ

#ifdef _DEBUG
	//デバッグレイヤーをオンに
	EnableDebugLayer();
#endif
	//DirectX12まわり初期化
	//フィーチャレベル列挙
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
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

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;
	for (auto l : levels) {
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = l;
			break;
		}
	}

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	//_cmdList->Close();
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//タイムアウトなし
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//プライオリティ特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//ここはコマンドリストと合わせてください
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));//コマンドキュー生成

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	result = _dxgiFactory->CreateSwapChainForHwnd(_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビューなので当然RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ガンマ補正あり(sRGB)に設定する
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		//ID3D12Resource* backBuffer = nullptr;
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle); //第２引数にrtvDescのアドレスを入れる
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);//ウィンドウ表示

	// PMDヘッダー構造
	struct PMDHeader
	{
		float version;
		char model_name[20];
		char comment[256];
	};

	// 先頭3バイトの回避
	char signature[3] = {}; // シグネチャ
	PMDHeader pmdheader = {};
	FILE* fp;
	auto err = fopen_s(&fp, "Model/初音ミク.pmd", "rb");

	/*
	if (fp == nullptr) {
		char strerr[256];
		strerror_s(strerr, 256, err);
		MessageBox(hwnd, strerr, "Open Error", MB_ICONERROR);
		return -1;
	}
	*/

	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);


	// ロード内容の確認
	std::cout << "signature: " << pmdheader.version << std::endl;
	std::cout << "pmdheader: " << "version: " << pmdheader.version << std::endl;
	std::cout << "model_name: ";
	for (int i = 0; i < 20; ++i)
		std::cout << pmdheader.model_name[i];
	std::cout << std::endl;
	std::cout << "comment: ";
	for (int i = 0; i < 256; ++i)
		std::cout << pmdheader.comment[i];
	std::cout << std::endl;

	// 頂点数の取得
	unsigned int vertNum; // 頂点数
	fread(&vertNum, sizeof(vertNum), 1, fp);
	std::cout << "vertNum: " << vertNum << std::endl;

	constexpr size_t pmdvertex_size = 38; // 頂点一つあたりのサイズ（頂点が持つ情報量）

	// 頂点データの読み込み
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size); // バッファーの確保（vertNumに頂点サイズ38バイトを掛ける）
	fread(vertices.data(), vertices.size(), 1, fp); // 読み込み


	// UPLOAD
	ID3D12Resource* vertBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), //UPLOADヒープとしてサイズに応じて適切な設定をしてくれる
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // PMDデータのすべての頂点の合計サイズに変換
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff)
	);

	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファの仮想アドレス
	vbView.SizeInBytes = vertices.size();//全バイト数
	vbView.StrideInBytes = pmdvertex_size;//1頂点あたりのバイト数

	//unsigned short indices[] = { 0,1,2, 2,1,3 };	
	std::vector<unsigned short> indices;

	// インデックスデータ読み込みの準備
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	std::cout << "indicesNum: " << indicesNum << std::endl;

	// インデックスデータの読み込み
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// PMDモデルのマテリアルの読み込み
	unsigned int materialNum; // マテリアル数
	fread(&materialNum, sizeof(materialNum), 1, fp); // マテリアル数を読み込む
	std::cout << "マテリアルの数: " << materialNum << std::endl;

	std::vector<PMDMaterial> pmdMaterials(materialNum);

	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp); // 一気に読み込む

	fclose(fp); // ファイルのクローズ

	// マテリアルの設定
	std::vector<Material> materials(pmdMaterials.size());

	// コピー
	for (int i = 0; i < pmdMaterials.size(); ++i)
	{
		materials[i].indicesNum = pmdMaterials[i].indicesNum;
		materials[i].material.diffuse = pmdMaterials[i].diffuse;
		materials[i].material.alpha = pmdMaterials[i].alpha;
		materials[i].material.specular = pmdMaterials[i].specular;
		materials[i].material.specularity = pmdMaterials[i].specularity;
		materials[i].material.ambient = pmdMaterials[i].ambient;
	}

	ID3D12Resource* idxBuff = nullptr;
	//設定は、バッファのサイズ以外頂点バッファの設定を使いまわして
	//OKだと思います。

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//作ったバッファにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//インデックスバッファビューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;

	ID3DBlob* errorBlob = nullptr;
	result = D3DCompileFromFile(L"BasicVertexShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicVS", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_vsBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("ファイルが見当たりません");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//行儀悪いかな…
	}

	// 頂点レイアウトの変更（PMDの頂点データに合わせる）
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// 深度バッファーの作成
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元のテクスチャデータ
	depthResDesc.Width = window_width; // 幅はレンダーターゲットと同じ
	depthResDesc.Height = window_height; // 高さはレンダーターゲットと同じ
	depthResDesc.DepthOrArraySize = 1; // テクスチャ配列でも，3Dテクスチャでもない
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値書き込み用フォーマット
	depthResDesc.SampleDesc.Count = 1; // サンプルは1ピクセル当たり1つ
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // デプスステンシルとして使用（8バイトがステンシルに割り当てられる？）

	// 深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // DEFAULTなのであとはUNKNOWN
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// このクリアバリューが重要な意味を持つ（らしい．まだ良く分かってない）
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f; // 深さ1.0f（最大値）でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // 32ビットfloat値としてクリア

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer)
	);
	std::cout << "深度バッファーの作成確認: " << result << std::endl;

	// 深度バッファービューの作成
	// ディスクリプタヒープを作る（深度バッファービューはこれまでと異なる独立したカテゴリなので，新しくディスクリプタヒープを作る
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // 深度に使うことが分かるようにする
	dsvHeapDesc.NumDescriptors = 1; // 深度ビューは1つ
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // デプスステンシルビューとして使う
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	std::cout << "深度バッファービューの作成確認: " << result << std::endl;

	// デプスステンシルビューの作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 深度値に32ビット使用
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // フラグはなし（そもそも何に使う？）

	_dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//中身は0xffffffff

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//ひとまず加算や乗算やαブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//ひとまず論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.RasterizerState.MultisampleEnable = false;//まだアンチェリは使わない
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	//残り
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// 深度バッファーの設定
	gpipeline.DepthStencilState.DepthEnable = true; // 深度バッファーを使うのでtrueにする
	gpipeline.DepthStencilState.StencilEnable = false;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // マスクの書き込み（これは何のこと？←ピクセルの描画時に深度バッファーに深度値を書き込むことを表している）
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 小さい方を採用
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 32ビットfloatを深度値として使用する

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	gpipeline.NumRenderTargets = 1;//今は１つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低


	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {};

	/*
	// 定数用レジスター0番
	descTblRange[0].NumDescriptors = 1; // 定数は1つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 種別は定数
	descTblRange[0].BaseShaderRegister = 0; //0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// マテリアル用レジスター1番
	descTblRange[1].NumDescriptors = 1; // ディスクリプタヒープは複数だが一度に使うのは一つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 種別は定数
	descTblRange[1].BaseShaderRegister = 1; // 1番スロットから（始める？）
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// テクスチャ用レジスター0番
	descTblRange[2].NumDescriptors = 1; // テクスチャは1つ
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // 種別はテクスチャ
	descTblRange[2].BaseShaderRegister = 0; //0番スロットから
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	*/


	// テクスチャ用レジスター0番
	descTblRange[0].NumDescriptors = 1; // テクスチャは1つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // 種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0; //0番スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 定数用レジスター0番
	descTblRange[1].NumDescriptors = 1; // 定数は1つ
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 種別は定数
	descTblRange[1].BaseShaderRegister = 0; //0番スロットから
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// マテリアル用レジスター1番
	descTblRange[2].NumDescriptors = 1; // ディスクリプタヒープは複数だが一度に使うのは一つ
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // 種別は定数
	descTblRange[2].BaseShaderRegister = 1; // 1番スロットから（始める？）
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータの設定（定数は頂点の座標変換なので，頂点シェーダーから見えるようにしたい）
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	//D3D12_ROOT_PARAMETER rootparam[1] = {};

	/*
	// 定数一つ目のルートパラメタ
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0]; // 配列先頭アドレス
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1; // ディスクリプタレンジ数
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // すべてのシェーダーから見える
	// 定数二つ目のルートパラメタ
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1]; // 配列先頭アドレス
	rootparam[1].DescriptorTable.NumDescriptorRanges =; // ディスクリプタレンジ数
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // すべてのシェーダーから見える
	*/

	// 定数一つ目のルートパラメタ
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0]; // 配列先頭アドレス
	rootparam[0].DescriptorTable.NumDescriptorRanges = 2; // ディスクリプタレンジ数
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // すべてのシェーダーから見える

	// 定数二つ目のルートパラメタ
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[2]; // 配列先頭アドレス
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1; // ディスクリプタレンジ数
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // すべてのシェーダーから見える

	ID3D12RootSignature* rootsignature = nullptr;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam; // ルートパラメータの先頭アドレス
	rootSignatureDesc.NumParameters = 2; // ルートパラメータ数

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //横方向の繰り返し
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //縦方向の繰り返し
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //奥行きの繰り返し
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; //ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //線形補完
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; //ミップマップ最大値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // リサンプリングしない

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	result = _dev->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootsignature));
	std::cout << "CreateRootSignature: " << result << std::endl;
	rootSigBlob->Release();

	gpipeline.pRootSignature = rootsignature;
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));

	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width;//出力先の幅(ピクセル数)
	viewport.Height = window_height;//出力先の高さ(ピクセル数)
	viewport.TopLeftX = 0;//出力先の左上座標X
	viewport.TopLeftY = 0;//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;//深度最大値
	viewport.MinDepth = 0.0f;//深度最小値


	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;//切り抜き上座標
	scissorrect.left = 0;//切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標

	// WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(
		L"C:/Users/belie/Documents/DirectX12/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg
	);
	auto img = scratchImg.GetImage(0, 0, 0); // 生データ抽出


	//WriteToSubresourceで転送する用のヒープ設定
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//特殊な設定なのでdefaultでもuploadでもなく
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//ライトバックで
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//転送がL0つまりCPU側から直で
	texHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	texHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	//リソースの設定
	D3D12_RESOURCE_DESC resDesc = {};

	// 画像に合わせて設定をする
	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width; // 幅
	resDesc.Height = metadata.height; // 高さ
	resDesc.DepthOrArraySize = metadata.arraySize;
	resDesc.MipLevels = metadata.mipLevels;
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.SampleDesc.Count = 1; // 通常テクスチャなのでアンチエイリアシングしない
	resDesc.SampleDesc.Quality = 0; // クオリティは最低
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // レイアウトは決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // 特にフラグなし

	// リソースの生成
	ID3D12Resource* texbuff = nullptr;

	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // 特になし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // テクスチャ用指定
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	// 画像データをGPUに転送する
	result = texbuff->WriteToSubresource(
		0,
		nullptr, // 全領域へコピー
		img->pixels, // 元データアドレス
		img->rowPitch, // 1ラインサイズ
		img->slicePitch // 1枚サイズ 
	);

	// ディスクリプタヒープを作る
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};

	// シェーダーから見えるようにする
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// マスクは0
	descHeapDesc.NodeMask = 0;

	// テクスチャバッファビュー（SRV）と定数バッファービュー（CBV）の二つ
	descHeapDesc.NumDescriptors = 2;

	// シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	// 生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));
	std::cout << "\nCreateDescriptorHeap: " << result << std::endl;

	// シェーダーリソースビューを作る
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA(0.0f-1.0fに正規化)
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // データのRGBAをどのようにマッピングするか
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1; // ミップマップは使用しないため1

	// ハンドルの取得（ディスクリプタヒープの先頭（つまり，この場合シェーダーリソースビューの位置）を示す）
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	// シェーダーリソースビューをヒープ上に作成
	_dev->CreateShaderResourceView(
		texbuff, // ビューと関連付けるバッファー
		&srvDesc, // 上で設定したテクスチャ設定情報
		basicHeapHandle // ヒープのどこに割り当てるか
	);

	// 回転アニメーション用
	XMMATRIX worldMat = XMMatrixIdentity();

	// ビュー行列の定義
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	// XMFLOAT3からXMVECTORへの変換
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// プロジェクション行列の定義
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2, // 画角は90度
		static_cast<float>(window_width) / static_cast<float>(window_height), // アスペクト比
		1.0f, // 近い方（near）
		100.0f // 遠い方（far）
	);

	// 定数バッファーの作成
	ID3D12Resource* constBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff), // バッファーのサイズに合わせて確保する領域を変化させる
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
	std::cout << "定数バッファーの作成確認: " << result << std::endl;

	MatricesData* mapMatrix; // マップ先を示すポインター
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix); // マップ
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;
	//*mapMatrix = worldMat * viewMat * projMat; //行列の内容をコピー

						 // 次の場所に移動
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// 定数バッファービューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	// マテリアルバッファーの作成
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff; // 256アライメントにするための処理

	ID3D12Resource* materialBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	// マテリアルバッファーにデータをコピー
	char* mapMaterial = nullptr;

	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);


	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material; // データのコピー
		mapMaterial += materialBuffSize; // 次のアライメントの位置まで進める
	}
	materialBuff->Unmap(0, nullptr);

	// マテリアル用のディスクリプタヒープ
	ID3D12DescriptorHeap* materialDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum; // ビューはマテリアルの数だけ作る
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(&materialDescHeap)); // 生成
	std::cout << "マテリアル用のディスクリプタヒープの作成確認: " << result << std::endl;

	// マテリアル用のビューの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress(); // バッファーアドレス
	matCBVDesc.SizeInBytes = materialBuffSize; // マテリアル256アラインメントサイズ

	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart(); // ディスクリプタヒープの先頭を記録

	// ループで各マテリアルのビューを作成する
	for (int i = 0; i < materialNum; ++i)
	{
		_dev->CreateConstantBufferView(&matCBVDesc, matDescHeapH);
		matDescHeapH.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		matCBVDesc.BufferLocation += materialBuffSize;
	}

	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;
	while (true) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}

		angle += 0.02f;
		mapMatrix->world = XMMatrixRotationY(angle);
		mapMatrix->viewproj = viewMat * projMat;

		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		_cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET)
		);
		// コメントアウトのところのコードが上の数行で済む！（行数的にはあまり変わらないけど分かりやすい）


		_cmdList->SetPipelineState(_pipelinestate);

		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// レンダーターゲットと深度バッファービューの関連付け
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH); //レンダーターゲットへの書き込みと深度バッファーへの書き込みを同時に行う
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白色
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		// ルートシグネチャの設定
		_cmdList->SetGraphicsRootSignature(rootsignature);

		// ディスクリプタヒープの指定
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);

		_cmdList->SetGraphicsRootDescriptorTable(
			0,
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		);

		// マテリアル用ビューのセット
		
		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(
			1,
			materialDescHeap->GetGPUDescriptorHandleForHeapStart()
		);
		

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		//命令のクローズ
		_cmdList->Close();


		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		////待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//再びコマンドリストをためる準備

		//フリップ
		_swapchain->Present(1, 0);

	}
	//もうクラス使わんから登録解除してや
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}