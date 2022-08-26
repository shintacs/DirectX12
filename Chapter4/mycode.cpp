//動かない原因が特定できないのでいったん放置

#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h> //DXGIの制御
#include <vector>
#include <DirectXMath.h>
#include <d3dcompiler.h> //シェーダーのコンパイル用
#include <string>


#ifdef _DEBUG
#include <iostream>
#endif

//ライブラリのリンク
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib") //シェーダーのコンパイル用

using namespace std;
using namespace DirectX;

// @brief コンソール画面にフォーマット付き文字列を表示
// @param formatフォーマット
// @param 可変長引数
// @remarks デバッグ用の変数


void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//ウィンドウプロシージャ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//ウィンドウが破棄された時に呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); //OSにアプリの終了を伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); //既定の処理を行う（？）（既定とは？）
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

//コマンドアロケータの宣言
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
//コマンドキューの設定
ID3D12CommandQueue* _cmdQueue = nullptr;

// デバッグレイヤーの有効化
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(
		IID_PPV_ARGS(&debugLayer)
	);
	if (SUCCEEDED(result)) {
		debugLayer->EnableDebugLayer(); //デバッグプレイヤーを有効化する
		debugLayer->Release(); //有効化したらインターフェイスを解散する
	}
}

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	DebugOutputFormatString("Show window test.");
	//ウィンドウクラスの生成と登録
	WNDCLASSEX w = {};

	//確認用
	//cout << _dev << " : " << typeid(_dev).name() << endl;
	//cout << _dxgiFactory << " : " << typeid(_dxgiFactory).name() << endl;

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; //コールバック関数（？）の指定
	w.lpszClassName = _T("Dx12Sample"); //アプリケーションクラス名
	w.hInstance = GetModuleHandle(nullptr); //ハンドルの取得（？）

	RegisterClassEx(&w); //アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）

	RECT wrc = { 0, 0, window_width, window_height }; // ウィンドウサイズを決める
	//関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウオブジェクト（？）の生成
	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("Dx12のテスト"),		//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	//タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,			//表示x座標はOSに任せる
		CW_USEDEFAULT,			//表示y座標はOSに任せる
		wrc.right - wrc.left,	//ウィンドウ幅
		wrc.bottom - wrc.top,	//ウィンドウ高
		nullptr,				//親ウィンドウハンドル（？）
		nullptr,				//メニューハンドル
		w.hInstance,			//呼び出しアプリケーションハンドル
		nullptr);				//追加パラメーター

#ifdef _DEBUG
	EnableDebugLayer();
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
	// DirectX12まわりの初期化
	//フィーチャーレベルの配列
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	auto result = CreateDXGIFactory(IID_PPV_ARGS(&_dxgiFactory));

	//アダプターを列挙するための変数 
	vector <IDXGIAdapter*> adapters;

	//特定の名前を持つアダプターオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;

	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	//アダプターを識別するための情報(DXGI_ADAPTER_DESC構造体）を取得
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); //アダプターの説明オブジェクト取得

		wstring strDesc = adesc.Description;

		//探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != string::npos)
		{
			tmpAdapter = adpt; //NVIDIAという名前が含まれるアダプターオブジェクトを格納
			break;
		}
	}

	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;
		}
	}



	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	cout << endl;
	cout << "command allocator result: " << result << endl;

	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
	cout << "command list result: " << result << endl;


	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};


	//タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	//アダプターを1つしか使わない時は0でよい
	cmdQueueDesc.NodeMask = 0;

	//プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	//コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	//キュー生成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	cout << endl;
	cout << "cmdQueue result: " << result << endl;


	//スワップチェーン生成プログラム
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width; //画像の幅
	swapchainDesc.Height = window_height; //画像の高さ
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //ピックセルフォーマット
	swapchainDesc.Stereo = false; //ステレオ表示フラグ
	swapchainDesc.SampleDesc.Count = 1; //マルチサンプルの指定
	swapchainDesc.SampleDesc.Quality = 0; //同上
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2; //バッファーの数（ダブルバッファーなら2）

	//スケーリング可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	//フリップ後は破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	//ウィンドウとフルスクリーンの切り替えを可能にする
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);
	cout << endl;
	cout << "SwapChain result:" << result << endl;

	//cout << _dev << " : " << typeid(_dev).name() << endl;
	//cout << _dxgiFactory << " : " << typeid(_dxgiFactory).name() << endl;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //レンダーターゲットビュー（RTV）
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //表と裏の2つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* rtvHeaps = nullptr;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	cout << "descriptor heap result: " << result << endl;

	DXGI_SWAP_CHAIN_DESC swcDesc = {};

	result = _swapchain->GetDesc(&swcDesc);

	vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])); //スワップチェーン上のバックバッファーを取得
		D3D12_CPU_DESCRIPTOR_HANDLE handle
			= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		//ポインターをずらす
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//レンダーターゲットビューの生成
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}

	// フェンスの作成
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;

	result = _dev->CreateFence(
		_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)
	);
	cout << "CreateFence: " << result << endl;

	//頂点の設定
	XMFLOAT3 vertices[] =
	{
		{-1.0f, -1.0f, 0.0f}, //左下
		{-1.0f,  1.0f, 0.0f}, //左上
		{ 1.0f, -1.0f, 0.0f}, //右上
	};

	//頂点バッファーの生成
	D3D12_HEAP_PROPERTIES heapprop = {};
	
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices); //頂点情報が入るサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));

	cout << "CommitedResource result: " << result << endl;

	//頂点情報のコピー
	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	
	copy(begin(vertices), end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr); //マップを解除する（アンマップ）

	//頂点バッファービューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); //バッファーの仮想アドレス
	vbView.SizeInBytes = sizeof(vertices); //全バイト数
	vbView.StrideInBytes = sizeof(vertices[0]); // 1頂点辺りのバイト数

	//シェーダーオブジェクトを保持するための変数
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	//頂点シェーダーの読み込み
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl", //シェーダー名
		nullptr, //defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, //デフォルトにする
		"BasicVS", "vs_5_0", //関数はBasicVS, 対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用及び最適化なし
		0,
		&_vsBlob, &errorBlob //エラー時はerrorBlobにメッセージが入る
	);

	//エラーが起こった場合の処理
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("ファイルが見当たりません");
			return 0;
		}
		else
		{
			string errstr;
			errstr.resize(errorBlob->GetBufferSize());

			copy_n((char*)errorBlob->GetBufferPointer(),
				errorBlob->GetBufferSize(),
				errstr.begin());
			errstr += "\n";

			::OutputDebugStringA(errstr.c_str());
		}
	}
	
	cout << "Compile Vertex Shader: " << result << endl;

	//ピクセルシェーダーの読み込み
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl", //シェーダー名
		nullptr, //defineはなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE, //デフォルトにする
		"BasicPS", "ps_5_0", //関数はBasicVS, 対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //デバッグ用及び最適化なし
		0,
		&_psBlob, &errorBlob //エラー時はerrorBlobにメッセージが入る
	);

	cout << "Compile Pixel Shader: " << result << endl;

	//頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
	};

	// シェーダーのセット
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;

	//頂点シェーダーとピクセルシェーダーだけ初期化
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	// ブレンドステートの初期化
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// 追加分
	gpipeline.RasterizerState.MultisampleEnable = false;//まだアンチェリは使わない
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	gpipeline.DepthStencilState.DepthEnable = false;
	gpipeline.DepthStencilState.StencilEnable = false;

	// 入力レイアウトの初期化
	gpipeline.InputLayout.pInputElementDescs = inputLayout;	// レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // レイアウト配列の要素数
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // トライアングルストリップを使わない

	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形で構成する

	gpipeline.NumRenderTargets = 1; // 今は一つのみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0から1に正規化されたRGBA

	// アンチエイリアシングの設定
	gpipeline.SampleDesc.Count = 1; // サンプリングは1ピクセルにつき1
	gpipeline.SampleDesc.Quality = 0; // クオリティは最低

	// ルートシグネチャの設定
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // 頂点情報が存在することを示す

	// ルートシグネチャのバイナリコードの作成
	ID3DBlob* rootSigBlob = nullptr;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ルートシグネチャのバージョン
		&rootSigBlob,
		&errorBlob
	);

	// ルートシグネチャオブジェクトの作成
	result = _dev->CreateRootSignature(
		0, // nodemask
		rootSigBlob->GetBufferPointer(), // シェーダーの時と同様
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootsignature)
	);
	rootSigBlob->Release();

	// ルートシグネチャの設定
	gpipeline.pRootSignature = rootsignature;

	// グラフィックスパイプラインステート
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	cout << "CreateGraphicsPipelineState: " << result << endl; //ルートシグネチャを設定していない場合，エラーをはく

	// ビューポートの作成
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width; // 出力先の幅
	viewport.Height = window_height; // 出力先の高さ
	viewport.TopLeftX = 0; // 出力先の左上座標X
	viewport.TopLeftY = 0; // 出力先の左上座標Y
	viewport.MaxDepth = 1.0f; // 深度最大値
	viewport.MinDepth = 0.0f; // 深度最小値

	// シザー矩形
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0; // 切り抜き上座標
	scissorrect.left = 0; // 切り抜き左座標
	scissorrect.right = scissorrect.left + window_width; // 切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height; // 切り抜き下座標

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);

	// ウィンドウの表示を継続させる
	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) //Q:（ここは何をしている？）
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//アプリケーションが終わるとき，messageがWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}

		// DirectX処理

		//現在のバックバッファーを指すインデックスの表示
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
		cout << "bbIdx: " << bbIdx << endl;

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; //遷移
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; //特に指定なし
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx]; //バックバッファーリソース
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; //直前はpresent状態
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; //直後からレンダーターゲット状態

		_cmdList->ResourceBarrier(1, &BarrierDesc); //バリア指定実行

		cout << "pipelinestate: " << _pipelinestate << endl;
		_cmdList->SetPipelineState(_pipelinestate); // パイプラインステートの設定
		_cmdList->SetGraphicsRootSignature(rootsignature); // ルートシグネチャの設定

		//レンダーターゲットビューのセット
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		//レンダーターゲットのクリア
		float clearColor[] = { 1.0f, 0.0f, 1.0f, 1.0f }; //紫
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		
		
		_cmdList->RSSetViewports(1, &viewport); // ビューポートとシザー矩形
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);// プリミティブトポロジの設定
		_cmdList->IASetVertexBuffers(0, 1, &vbView); // 頂点バッファーの設定
		_cmdList->DrawInstanced(3, 1, 0, 0); // 描画命令の設定

		// レンダーターゲットからPresent状態に移行
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//命令のクローズ（絶対必要）
		_cmdList->Close();

		//コマンドリストの実行
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);
		//待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// イベントハンドルの取得
			auto event = CreateEvent(nullptr, false, false, nullptr);

			_fence->SetEventOnCompletion(_fenceVal, event);

			// イベントが発生するまで待ち続ける
			WaitForSingleObject(event, INFINITE);

			// イベントハンドルを閉じる
			CloseHandle(event);
		}

		//実行後はコマンドリストは不要なのでクリア
		_cmdAllocator->Reset(); //キューのクリア
		_cmdList->Reset(_cmdAllocator, _pipelinestate); //再びコマンドリストをためる準備

		//コマンドアロケーターのリセット
		//result = _cmdAllocator->Reset();
		
		//画面のスワップをする（フリップ）
		_swapchain->Present(1, 0);



	}

	//クラスはこれ以上使わないため登録解除（？）
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}


