//�����Ȃ�����������ł��Ȃ��̂ł���������u

#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h> //DXGI�̐���
#include <vector>
#include <DirectXMath.h>
#include <d3dcompiler.h> //�V�F�[�_�[�̃R���p�C���p
#include <string>


#ifdef _DEBUG
#include <iostream>
#endif

//���C�u�����̃����N
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib") //�V�F�[�_�[�̃R���p�C���p

using namespace std;
using namespace DirectX;

// @brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
// @param format�t�H�[�}�b�g
// @param �ϒ�����
// @remarks �f�o�b�O�p�̕ϐ�


void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//�E�B���h�E�v���V�[�W��
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//�E�B���h�E���j�����ꂽ���ɌĂ΂��
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0); //OS�ɃA�v���̏I����`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam); //����̏������s���i�H�j�i����Ƃ́H�j
}

const unsigned int window_width = 1280;
const unsigned int window_height = 720;

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;

//�R�}���h�A���P�[�^�̐錾
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
//�R�}���h�L���[�̐ݒ�
ID3D12CommandQueue* _cmdQueue = nullptr;

// �f�o�b�O���C���[�̗L����
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(
		IID_PPV_ARGS(&debugLayer)
	);
	if (SUCCEEDED(result)) {
		debugLayer->EnableDebugLayer(); //�f�o�b�O�v���C���[��L��������
		debugLayer->Release(); //�L����������C���^�[�t�F�C�X�����U����
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
	//�E�B���h�E�N���X�̐����Ɠo�^
	WNDCLASSEX w = {};

	//�m�F�p
	//cout << _dev << " : " << typeid(_dev).name() << endl;
	//cout << _dxgiFactory << " : " << typeid(_dxgiFactory).name() << endl;

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; //�R�[���o�b�N�֐��i�H�j�̎w��
	w.lpszClassName = _T("Dx12Sample"); //�A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(nullptr); //�n���h���̎擾�i�H�j

	RegisterClassEx(&w); //�A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j

	RECT wrc = { 0, 0, window_width, window_height }; // �E�B���h�E�T�C�Y�����߂�
	//�֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//�E�B���h�E�I�u�W�F�N�g�i�H�j�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("Dx12�̃e�X�g"),		//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	//�^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,			//�\��x���W��OS�ɔC����
		CW_USEDEFAULT,			//�\��y���W��OS�ɔC����
		wrc.right - wrc.left,	//�E�B���h�E��
		wrc.bottom - wrc.top,	//�E�B���h�E��
		nullptr,				//�e�E�B���h�E�n���h���i�H�j
		nullptr,				//���j���[�n���h��
		w.hInstance,			//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);				//�ǉ��p�����[�^�[

#ifdef _DEBUG
	EnableDebugLayer();
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
	// DirectX12�܂��̏�����
	//�t�B�[�`���[���x���̔z��
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	auto result = CreateDXGIFactory(IID_PPV_ARGS(&_dxgiFactory));

	//�A�_�v�^�[��񋓂��邽�߂̕ϐ� 
	vector <IDXGIAdapter*> adapters;

	//����̖��O�����A�_�v�^�[�I�u�W�F�N�g������
	IDXGIAdapter* tmpAdapter = nullptr;

	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	//�A�_�v�^�[�����ʂ��邽�߂̏��(DXGI_ADAPTER_DESC�\���́j���擾
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc); //�A�_�v�^�[�̐����I�u�W�F�N�g�擾

		wstring strDesc = adesc.Description;

		//�T�������A�_�v�^�[�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != string::npos)
		{
			tmpAdapter = adpt; //NVIDIA�Ƃ������O���܂܂��A�_�v�^�[�I�u�W�F�N�g���i�[
			break;
		}
	}

	//Direct3D�f�o�C�X�̏�����
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


	//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	//�A�_�v�^�[��1�����g��Ȃ�����0�ł悢
	cmdQueueDesc.NodeMask = 0;

	//�v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	//�R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	//�L���[����
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
	cout << endl;
	cout << "cmdQueue result: " << result << endl;


	//�X���b�v�`�F�[�������v���O����
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width; //�摜�̕�
	swapchainDesc.Height = window_height; //�摜�̍���
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //�s�b�N�Z���t�H�[�}�b�g
	swapchainDesc.Stereo = false; //�X�e���I�\���t���O
	swapchainDesc.SampleDesc.Count = 1; //�}���`�T���v���̎w��
	swapchainDesc.SampleDesc.Quality = 0; //����
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2; //�o�b�t�@�[�̐��i�_�u���o�b�t�@�[�Ȃ�2�j

	//�X�P�[�����O�\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	//�t���b�v��͔j��
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	//�E�B���h�E�ƃt���X�N���[���̐؂�ւ����\�ɂ���
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

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[�iRTV�j
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //�\�Ɨ���2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* rtvHeaps = nullptr;

	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	cout << "descriptor heap result: " << result << endl;

	DXGI_SWAP_CHAIN_DESC swcDesc = {};

	result = _swapchain->GetDesc(&swcDesc);

	vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx])); //�X���b�v�`�F�[����̃o�b�N�o�b�t�@�[���擾
		D3D12_CPU_DESCRIPTOR_HANDLE handle
			= rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		//�|�C���^�[�����炷
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		//�����_�[�^�[�Q�b�g�r���[�̐���
		_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}

	// �t�F���X�̍쐬
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;

	result = _dev->CreateFence(
		_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)
	);
	cout << "CreateFence: " << result << endl;

	//���_�̐ݒ�
	XMFLOAT3 vertices[] =
	{
		{-1.0f, -1.0f, 0.0f}, //����
		{-1.0f,  1.0f, 0.0f}, //����
		{ 1.0f, -1.0f, 0.0f}, //�E��
	};

	//���_�o�b�t�@�[�̐���
	D3D12_HEAP_PROPERTIES heapprop = {};
	
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices); //���_��񂪓���T�C�Y
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

	//���_���̃R�s�[
	XMFLOAT3* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	
	copy(begin(vertices), end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr); //�}�b�v����������i�A���}�b�v�j

	//���_�o�b�t�@�[�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress(); //�o�b�t�@�[�̉��z�A�h���X
	vbView.SizeInBytes = sizeof(vertices); //�S�o�C�g��
	vbView.StrideInBytes = sizeof(vertices[0]); // 1���_�ӂ�̃o�C�g��

	//�V�F�[�_�[�I�u�W�F�N�g��ێ����邽�߂̕ϐ�
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	//���_�V�F�[�_�[�̓ǂݍ���
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl", //�V�F�[�_�[��
		nullptr, //define�͂Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE, //�f�t�H���g�ɂ���
		"BasicVS", "vs_5_0", //�֐���BasicVS, �ΏۃV�F�[�_�[��vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p�y�эœK���Ȃ�
		0,
		&_vsBlob, &errorBlob //�G���[����errorBlob�Ƀ��b�Z�[�W������
	);

	//�G���[���N�������ꍇ�̏���
	if (FAILED(result))
	{
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
		{
			::OutputDebugStringA("�t�@�C������������܂���");
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

	//�s�N�Z���V�F�[�_�[�̓ǂݍ���
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl", //�V�F�[�_�[��
		nullptr, //define�͂Ȃ�
		D3D_COMPILE_STANDARD_FILE_INCLUDE, //�f�t�H���g�ɂ���
		"BasicPS", "ps_5_0", //�֐���BasicVS, �ΏۃV�F�[�_�[��vs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, //�f�o�b�O�p�y�эœK���Ȃ�
		0,
		&_psBlob, &errorBlob //�G���[����errorBlob�Ƀ��b�Z�[�W������
	);

	cout << "Compile Pixel Shader: " << result << endl;

	//���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
	};

	// �V�F�[�_�[�̃Z�b�g
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;

	//���_�V�F�[�_�[�ƃs�N�Z���V�F�[�_�[����������
	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	// �u�����h�X�e�[�g�̏�����
	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	// �ǉ���
	gpipeline.RasterizerState.MultisampleEnable = false;//�܂��A���`�F���͎g��Ȃ�
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;//�[�x�����̃N���b�s���O�͗L����
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	gpipeline.DepthStencilState.DepthEnable = false;
	gpipeline.DepthStencilState.StencilEnable = false;

	// ���̓��C�A�E�g�̏�����
	gpipeline.InputLayout.pInputElementDescs = inputLayout;	// ���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout); // ���C�A�E�g�z��̗v�f��
	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // �g���C�A���O���X�g���b�v���g��Ȃ�

	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // �O�p�`�ō\������

	gpipeline.NumRenderTargets = 1; // ���͈�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // 0����1�ɐ��K�����ꂽRGBA

	// �A���`�G�C���A�V���O�̐ݒ�
	gpipeline.SampleDesc.Count = 1; // �T���v�����O��1�s�N�Z���ɂ�1
	gpipeline.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�

	// ���[�g�V�O�l�`���̐ݒ�
	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // ���_��񂪑��݂��邱�Ƃ�����

	// ���[�g�V�O�l�`���̃o�C�i���R�[�h�̍쐬
	ID3DBlob* rootSigBlob = nullptr;

	result = D3D12SerializeRootSignature(
		&rootSignatureDesc, // ���[�g�V�O�l�`���ݒ�
		D3D_ROOT_SIGNATURE_VERSION_1_0, // ���[�g�V�O�l�`���̃o�[�W����
		&rootSigBlob,
		&errorBlob
	);

	// ���[�g�V�O�l�`���I�u�W�F�N�g�̍쐬
	result = _dev->CreateRootSignature(
		0, // nodemask
		rootSigBlob->GetBufferPointer(), // �V�F�[�_�[�̎��Ɠ��l
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootsignature)
	);
	rootSigBlob->Release();

	// ���[�g�V�O�l�`���̐ݒ�
	gpipeline.pRootSignature = rootsignature;

	// �O���t�B�b�N�X�p�C�v���C���X�e�[�g
	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(&gpipeline, IID_PPV_ARGS(&_pipelinestate));
	cout << "CreateGraphicsPipelineState: " << result << endl; //���[�g�V�O�l�`����ݒ肵�Ă��Ȃ��ꍇ�C�G���[���͂�

	// �r���[�|�[�g�̍쐬
	D3D12_VIEWPORT viewport = {};
	viewport.Width = window_width; // �o�͐�̕�
	viewport.Height = window_height; // �o�͐�̍���
	viewport.TopLeftX = 0; // �o�͐�̍�����WX
	viewport.TopLeftY = 0; // �o�͐�̍�����WY
	viewport.MaxDepth = 1.0f; // �[�x�ő�l
	viewport.MinDepth = 0.0f; // �[�x�ŏ��l

	// �V�U�[��`
	D3D12_RECT scissorrect = {};
	scissorrect.top = 0; // �؂蔲������W
	scissorrect.left = 0; // �؂蔲�������W
	scissorrect.right = scissorrect.left + window_width; // �؂蔲���E���W
	scissorrect.bottom = scissorrect.top + window_height; // �؂蔲�������W

	// �E�B���h�E�̕\��
	ShowWindow(hwnd, SW_SHOW);

	// �E�B���h�E�̕\�����p��������
	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) //Q:�i�����͉������Ă���H�j
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//�A�v���P�[�V�������I���Ƃ��Cmessage��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}

		// DirectX����

		//���݂̃o�b�N�o�b�t�@�[���w���C���f�b�N�X�̕\��
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
		cout << "bbIdx: " << bbIdx << endl;

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; //�J��
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE; //���Ɏw��Ȃ�
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx]; //�o�b�N�o�b�t�@�[���\�[�X
		BarrierDesc.Transition.Subresource = 0;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; //���O��present���
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; //���ォ�烌���_�[�^�[�Q�b�g���

		_cmdList->ResourceBarrier(1, &BarrierDesc); //�o���A�w����s

		cout << "pipelinestate: " << _pipelinestate << endl;
		_cmdList->SetPipelineState(_pipelinestate); // �p�C�v���C���X�e�[�g�̐ݒ�
		_cmdList->SetGraphicsRootSignature(rootsignature); // ���[�g�V�O�l�`���̐ݒ�

		//�����_�[�^�[�Q�b�g�r���[�̃Z�b�g
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		//�����_�[�^�[�Q�b�g�̃N���A
		float clearColor[] = { 1.0f, 0.0f, 1.0f, 1.0f }; //��
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		
		
		_cmdList->RSSetViewports(1, &viewport); // �r���[�|�[�g�ƃV�U�[��`
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);// �v���~�e�B�u�g�|���W�̐ݒ�
		_cmdList->IASetVertexBuffers(0, 1, &vbView); // ���_�o�b�t�@�[�̐ݒ�
		_cmdList->DrawInstanced(3, 1, 0, 0); // �`�施�߂̐ݒ�

		// �����_�[�^�[�Q�b�g����Present��ԂɈڍs
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//���߂̃N���[�Y�i��ΕK�v�j
		_cmdList->Close();

		//�R�}���h���X�g�̎��s
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);
		//�҂�
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal)
		{
			// �C�x���g�n���h���̎擾
			auto event = CreateEvent(nullptr, false, false, nullptr);

			_fence->SetEventOnCompletion(_fenceVal, event);

			// �C�x���g����������܂ő҂�������
			WaitForSingleObject(event, INFINITE);

			// �C�x���g�n���h�������
			CloseHandle(event);
		}

		//���s��̓R�}���h���X�g�͕s�v�Ȃ̂ŃN���A
		_cmdAllocator->Reset(); //�L���[�̃N���A
		_cmdList->Reset(_cmdAllocator, _pipelinestate); //�ĂуR�}���h���X�g�����߂鏀��

		//�R�}���h�A���P�[�^�[�̃��Z�b�g
		//result = _cmdAllocator->Reset();
		
		//��ʂ̃X���b�v������i�t���b�v�j
		_swapchain->Present(1, 0);



	}

	//�N���X�͂���ȏ�g��Ȃ����ߓo�^�����i�H�j
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}


