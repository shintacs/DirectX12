//�|���S���\��
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

#pragma pack(1) // 1�o�C�g�p�b�L���O�ƂȂ�C�A���C�����g�͔������Ȃ��i�H�j

// PMD�}�e���A���ǂݍ��ݗp�̍\����
struct PMDMaterial
{
	XMFLOAT3 diffuse; // �f�B�t���[�Y�F
	float alpha; // �f�B�t���[�Y��
	float specularity; // �X�y�L�����̋���
	XMFLOAT3 specular; // �X�y�L�����̐F
	XMFLOAT3 ambient; // �A���r�G���g�̐F
	unsigned char toonIdx; // �g�D�[���ԍ�
	unsigned char edgeFlg; // �}�e���A�����Ƃ̗֊s���t���O
	unsigned int indicesNum; // ���̃}�e���A�������蓖�Ă���C���f�b�N�X��
	char texFilePath[20]; // �e�N�X�`���t�@�C���p�X+��
}; // �{����70�o�C�g�����C�p�f�B���O�ɂ����72�o�C�g�ɂȂ�炵��

#pragma pack() // ���ɖ߂�

// PMD���_�\����
struct PMDVertex
{
	XMFLOAT3 pos; // ���_���W�i12�o�C�g�j
	XMFLOAT3 normal; // �@���x�N�g���i12�o�C�g�j
	XMFLOAT2 uv; // uv���W�i8�o�C�g�j
	unsigned short boneno[2]; // �{�[���ԍ��i4�o�C�g�j
	unsigned char weight; // �{�[���e���x(1�o�C�g�j
	unsigned char edgeFlg; // �֊s���t���O�i1�o�C�g�j
};

// �V�F�[�_�[���ɓn�����߂̍s��f�[�^
struct MatricesData
{
	// �@���x�N�g�������f���̓����ɍ��킹�邽�߂ɒ�`
	XMMATRIX world; // ���f���{�̂���]��������ړ��������肷�邽�߂̍s��
	XMMATRIX viewproj; // �r���[�ƃv���W�F�N�V���������s��
};

// �V�F�[�_�[���ɓ�������}�e���A���f�[�^
struct MaterialForHlsl
{
	XMFLOAT3 diffuse; // �f�B�t�[�Y�F
	float alpha; // �f�B�t���[�Y��
	XMFLOAT3 specular; // �X�y�L�����F
	float specularity; // �X�y�L�����̋����i��Z�l�j
	XMFLOAT3 ambient; // �A���r�G���g�F
};

// ����ȊO�̃}�e���A���f�[�^
struct AdditionalMaterial
{
	std::string texPath; // �e�N�X�`���t�@�C���p�X
	int toonIdx; // �g�D�[���ԍ�
	bool edgeFlg; // �}�e���A�����Ƃ̗֊s���t���O
};

// �S�̂��܂Ƃ߂�f�[�^
struct Material
{
	unsigned int indicesNum; // �C���f�b�N�X��
	MaterialForHlsl material;
	AdditionalMaterial additional;
};


///@brief �R���\�[����ʂɃt�H�[�}�b�g�t���������\��
///@param format �t�H�[�}�b�g(%d�Ƃ�%f�Ƃ���)
///@param �ϒ�����
///@remarks���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif
}

//�ʓ|�����Ǐ����Ȃ�������
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//�E�B���h�E���j�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);//OS�ɑ΂��āu�������̃A�v���͏I�����v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);//�K��̏������s��
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
	//�E�B���h�E�N���X�������o�^
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	w.lpszClassName = _T("DirectXTest");//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	w.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&w);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0,0, window_width, window_height };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	//�E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(w.lpszClassName,//�N���X���w��
		_T("DX12 �P���|���S���e�X�g"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		w.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^

#ifdef _DEBUG
	//�f�o�b�O���C���[���I����
	EnableDebugLayer();
#endif
	//DirectX12�܂�菉����
	//�t�B�[�`�����x����
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

	//Direct3D�f�o�C�X�̏�����
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
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�v���C�I���e�B���Ɏw��Ȃ�
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;//�����̓R�}���h���X�g�ƍ��킹�Ă�������
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));//�R�}���h�L���[����

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
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//�����_�[�^�[�Q�b�g�r���[�Ȃ̂œ��RRTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//�\���̂Q��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//���Ɏw��Ȃ�
	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);
	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};

	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // �K���}�␳����(sRGB)�ɐݒ肷��
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (size_t i = 0; i < swcDesc.BufferCount; ++i) {
		//ID3D12Resource* backBuffer = nullptr;
		result = _swapchain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&_backBuffers[i]));
		_dev->CreateRenderTargetView(_backBuffers[i], &rtvDesc, handle); //��Q������rtvDesc�̃A�h���X������
		handle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;
	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	ShowWindow(hwnd, SW_SHOW);//�E�B���h�E�\��

	// PMD�w�b�_�[�\��
	struct PMDHeader
	{
		float version;
		char model_name[20];
		char comment[256];
	};

	// �擪3�o�C�g�̉��
	char signature[3] = {}; // �V�O�l�`��
	PMDHeader pmdheader = {};
	FILE* fp;
	auto err = fopen_s(&fp, "Model/�����~�N.pmd", "rb");

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


	// ���[�h���e�̊m�F
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

	// ���_���̎擾
	unsigned int vertNum; // ���_��
	fread(&vertNum, sizeof(vertNum), 1, fp);
	std::cout << "vertNum: " << vertNum << std::endl;

	constexpr size_t pmdvertex_size = 38; // ���_�������̃T�C�Y�i���_�������ʁj

	// ���_�f�[�^�̓ǂݍ���
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size); // �o�b�t�@�[�̊m�ہivertNum�ɒ��_�T�C�Y38�o�C�g���|����j
	fread(vertices.data(), vertices.size(), 1, fp); // �ǂݍ���


	// UPLOAD
	ID3D12Resource* vertBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), //UPLOAD�q�[�v�Ƃ��ăT�C�Y�ɉ����ēK�؂Ȑݒ�����Ă����
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertices.size()), // PMD�f�[�^�̂��ׂĂ̒��_�̍��v�T�C�Y�ɕϊ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff)
	);

	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);

	vertBuff->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//�o�b�t�@�̉��z�A�h���X
	vbView.SizeInBytes = vertices.size();//�S�o�C�g��
	vbView.StrideInBytes = pmdvertex_size;//1���_������̃o�C�g��

	//unsigned short indices[] = { 0,1,2, 2,1,3 };	
	std::vector<unsigned short> indices;

	// �C���f�b�N�X�f�[�^�ǂݍ��݂̏���
	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);
	std::cout << "indicesNum: " << indicesNum << std::endl;

	// �C���f�b�N�X�f�[�^�̓ǂݍ���
	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

	// PMD���f���̃}�e���A���̓ǂݍ���
	unsigned int materialNum; // �}�e���A����
	fread(&materialNum, sizeof(materialNum), 1, fp); // �}�e���A������ǂݍ���
	std::cout << "�}�e���A���̐�: " << materialNum << std::endl;

	std::vector<PMDMaterial> pmdMaterials(materialNum);

	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(PMDMaterial), 1, fp); // ��C�ɓǂݍ���

	fclose(fp); // �t�@�C���̃N���[�Y

	// �}�e���A���̐ݒ�
	std::vector<Material> materials(pmdMaterials.size());

	// �R�s�[
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
	//�ݒ�́A�o�b�t�@�̃T�C�Y�ȊO���_�o�b�t�@�̐ݒ���g���܂킵��
	//OK���Ǝv���܂��B

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff));

	//������o�b�t�@�ɃC���f�b�N�X�f�[�^���R�s�[
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	idxBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�r���[���쐬
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
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//�s�V�������ȁc
	}
	result = D3DCompileFromFile(L"BasicPixelShader.hlsl",
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &_psBlob, &errorBlob);
	if (FAILED(result)) {
		if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			::OutputDebugStringA("�t�@�C������������܂���");
		}
		else {
			std::string errstr;
			errstr.resize(errorBlob->GetBufferSize());
			std::copy_n((char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize(), errstr.begin());
			errstr += "\n";
			OutputDebugStringA(errstr.c_str());
		}
		exit(1);//�s�V�������ȁc
	}

	// ���_���C�A�E�g�̕ύX�iPMD�̒��_�f�[�^�ɍ��킹��j
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BONE_NO", 0, DXGI_FORMAT_R16G16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"EDGE_FLG", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// �[�x�o�b�t�@�[�̍쐬
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2�����̃e�N�X�`���f�[�^
	depthResDesc.Width = window_width; // ���̓����_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.Height = window_height; // �����̓����_�[�^�[�Q�b�g�Ɠ���
	depthResDesc.DepthOrArraySize = 1; // �e�N�X�`���z��ł��C3D�e�N�X�`���ł��Ȃ�
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l�������ݗp�t�H�[�}�b�g
	depthResDesc.SampleDesc.Count = 1; // �T���v����1�s�N�Z��������1��
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // �f�v�X�X�e���V���Ƃ��Ďg�p�i8�o�C�g���X�e���V���Ɋ��蓖�Ă���H�j

	// �[�x�l�p�q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT; // DEFAULT�Ȃ̂ł��Ƃ�UNKNOWN
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// ���̃N���A�o�����[���d�v�ȈӖ������i�炵���D�܂��ǂ��������ĂȂ��j
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f; // �[��1.0f�i�ő�l�j�ŃN���A
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // 32�r�b�gfloat�l�Ƃ��ăN���A

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // �[�x�l�������݂Ɏg�p
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer)
	);
	std::cout << "�[�x�o�b�t�@�[�̍쐬�m�F: " << result << std::endl;

	// �[�x�o�b�t�@�[�r���[�̍쐬
	// �f�B�X�N���v�^�q�[�v�����i�[�x�o�b�t�@�[�r���[�͂���܂łƈقȂ�Ɨ������J�e�S���Ȃ̂ŁC�V�����f�B�X�N���v�^�q�[�v�����
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {}; // �[�x�Ɏg�����Ƃ�������悤�ɂ���
	dsvHeapDesc.NumDescriptors = 1; // �[�x�r���[��1��
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // �f�v�X�X�e���V���r���[�Ƃ��Ďg��
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	std::cout << "�[�x�o�b�t�@�[�r���[�̍쐬�m�F: " << result << std::endl;

	// �f�v�X�X�e���V���r���[�̍쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // �[�x�l��32�r�b�g�g�p
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE; // �t���O�͂Ȃ��i�����������Ɏg���H�j

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

	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//���g��0xffffffff

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//�ЂƂ܂����Z���Z�⃿�u�����f�B���O�͎g�p���Ȃ�
	renderTargetBlendDesc.BlendEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//�ЂƂ܂��_�����Z�͎g�p���Ȃ�
	renderTargetBlendDesc.LogicOpEnable = false;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;


	gpipeline.RasterizerState.MultisampleEnable = false;//�܂��A���`�F���͎g��Ȃ�
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//���g��h��Ԃ�
	gpipeline.RasterizerState.DepthClipEnable = true;//�[�x�����̃N���b�s���O�͗L����

	//�c��
	gpipeline.RasterizerState.FrontCounterClockwise = false;
	gpipeline.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	gpipeline.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	gpipeline.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	gpipeline.RasterizerState.AntialiasedLineEnable = false;
	gpipeline.RasterizerState.ForcedSampleCount = 0;
	gpipeline.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// �[�x�o�b�t�@�[�̐ݒ�
	gpipeline.DepthStencilState.DepthEnable = true; // �[�x�o�b�t�@�[���g���̂�true�ɂ���
	gpipeline.DepthStencilState.StencilEnable = false;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // �}�X�N�̏������݁i����͉��̂��ƁH���s�N�Z���̕`�掞�ɐ[�x�o�b�t�@�[�ɐ[�x�l���������ނ��Ƃ�\���Ă���j
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // �����������̗p
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT; // 32�r�b�gfloat��[�x�l�Ƃ��Ďg�p����

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	gpipeline.NumRenderTargets = 1;//���͂P�̂�
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	gpipeline.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	gpipeline.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�


	D3D12_DESCRIPTOR_RANGE descTblRange[3] = {};

	/*
	// �萔�p���W�X�^�[0��
	descTblRange[0].NumDescriptors = 1; // �萔��1��
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // ��ʂ͒萔
	descTblRange[0].BaseShaderRegister = 0; //0�ԃX���b�g����
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// �}�e���A���p���W�X�^�[1��
	descTblRange[1].NumDescriptors = 1; // �f�B�X�N���v�^�q�[�v�͕���������x�Ɏg���͈̂��
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // ��ʂ͒萔
	descTblRange[1].BaseShaderRegister = 1; // 1�ԃX���b�g����i�n�߂�H�j
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// �e�N�X�`���p���W�X�^�[0��
	descTblRange[2].NumDescriptors = 1; // �e�N�X�`����1��
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // ��ʂ̓e�N�X�`��
	descTblRange[2].BaseShaderRegister = 0; //0�ԃX���b�g����
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	*/


	// �e�N�X�`���p���W�X�^�[0��
	descTblRange[0].NumDescriptors = 1; // �e�N�X�`����1��
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // ��ʂ̓e�N�X�`��
	descTblRange[0].BaseShaderRegister = 0; //0�ԃX���b�g����
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �萔�p���W�X�^�[0��
	descTblRange[1].NumDescriptors = 1; // �萔��1��
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // ��ʂ͒萔
	descTblRange[1].BaseShaderRegister = 0; //0�ԃX���b�g����
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �}�e���A���p���W�X�^�[1��
	descTblRange[2].NumDescriptors = 1; // �f�B�X�N���v�^�q�[�v�͕���������x�Ɏg���͈̂��
	descTblRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; // ��ʂ͒萔
	descTblRange[2].BaseShaderRegister = 1; // 1�ԃX���b�g����i�n�߂�H�j
	descTblRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�̐ݒ�i�萔�͒��_�̍��W�ϊ��Ȃ̂ŁC���_�V�F�[�_�[���猩����悤�ɂ������j
	D3D12_ROOT_PARAMETER rootparam[2] = {};
	//D3D12_ROOT_PARAMETER rootparam[1] = {};

	/*
	// �萔��ڂ̃��[�g�p�����^
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0]; // �z��擪�A�h���X
	rootparam[0].DescriptorTable.NumDescriptorRanges = 1; // �f�B�X�N���v�^�����W��
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ���ׂẴV�F�[�_�[���猩����
	// �萔��ڂ̃��[�g�p�����^
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1]; // �z��擪�A�h���X
	rootparam[1].DescriptorTable.NumDescriptorRanges =; // �f�B�X�N���v�^�����W��
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ���ׂẴV�F�[�_�[���猩����
	*/

	// �萔��ڂ̃��[�g�p�����^
	rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0]; // �z��擪�A�h���X
	rootparam[0].DescriptorTable.NumDescriptorRanges = 2; // �f�B�X�N���v�^�����W��
	rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ���ׂẴV�F�[�_�[���猩����

	// �萔��ڂ̃��[�g�p�����^
	rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[2]; // �z��擪�A�h���X
	rootparam[1].DescriptorTable.NumDescriptorRanges = 1; // �f�B�X�N���v�^�����W��
	rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL; // ���ׂẴV�F�[�_�[���猩����

	ID3D12RootSignature* rootsignature = nullptr;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootparam; // ���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = 2; // ���[�g�p�����[�^��

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //�������̌J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //�c�����̌J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; //���s���̌J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; //�{�[�_�[�͍�
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //���`�⊮
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; //�~�b�v�}�b�v�ő�l
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // �s�N�Z���V�F�[�_�[���猩����
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // ���T���v�����O���Ȃ�

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
	viewport.Width = window_width;//�o�͐�̕�(�s�N�Z����)
	viewport.Height = window_height;//�o�͐�̍���(�s�N�Z����)
	viewport.TopLeftX = 0;//�o�͐�̍�����WX
	viewport.TopLeftY = 0;//�o�͐�̍�����WY
	viewport.MaxDepth = 1.0f;//�[�x�ő�l
	viewport.MinDepth = 0.0f;//�[�x�ŏ��l


	D3D12_RECT scissorrect = {};
	scissorrect.top = 0;//�؂蔲������W
	scissorrect.left = 0;//�؂蔲�������W
	scissorrect.right = scissorrect.left + window_width;//�؂蔲���E���W
	scissorrect.bottom = scissorrect.top + window_height;//�؂蔲�������W

	// WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(
		L"C:/Users/belie/Documents/DirectX12/textest.png", WIC_FLAGS_NONE,
		&metadata, scratchImg
	);
	auto img = scratchImg.GetImage(0, 0, 0); // ���f�[�^���o


	//WriteToSubresource�œ]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;//����Ȑݒ�Ȃ̂�default�ł�upload�ł��Ȃ�
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//���C�g�o�b�N��
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//�]����L0�܂�CPU�����璼��
	texHeapProp.CreationNodeMask = 0;//�P��A�_�v�^�̂���0
	texHeapProp.VisibleNodeMask = 0;//�P��A�_�v�^�̂���0

	//���\�[�X�̐ݒ�
	D3D12_RESOURCE_DESC resDesc = {};

	// �摜�ɍ��킹�Đݒ������
	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width; // ��
	resDesc.Height = metadata.height; // ����
	resDesc.DepthOrArraySize = metadata.arraySize;
	resDesc.MipLevels = metadata.mipLevels;
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	resDesc.SampleDesc.Count = 1; // �ʏ�e�N�X�`���Ȃ̂ŃA���`�G�C���A�V���O���Ȃ�
	resDesc.SampleDesc.Quality = 0; // �N�I���e�B�͍Œ�
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // ���C�A�E�g�͌��肵�Ȃ�
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // ���Ƀt���O�Ȃ�

	// ���\�[�X�̐���
	ID3D12Resource* texbuff = nullptr;

	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE, // ���ɂȂ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // �e�N�X�`���p�w��
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	// �摜�f�[�^��GPU�ɓ]������
	result = texbuff->WriteToSubresource(
		0,
		nullptr, // �S�̈�փR�s�[
		img->pixels, // ���f�[�^�A�h���X
		img->rowPitch, // 1���C���T�C�Y
		img->slicePitch // 1���T�C�Y 
	);

	// �f�B�X�N���v�^�q�[�v�����
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};

	// �V�F�[�_�[���猩����悤�ɂ���
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// �}�X�N��0
	descHeapDesc.NodeMask = 0;

	// �e�N�X�`���o�b�t�@�r���[�iSRV�j�ƒ萔�o�b�t�@�[�r���[�iCBV�j�̓��
	descHeapDesc.NumDescriptors = 2;

	// �V�F�[�_�[���\�[�X�r���[�p
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	// ����
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));
	std::cout << "\nCreateDescriptorHeap: " << result << std::endl;

	// �V�F�[�_�[���\�[�X�r���[�����
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA(0.0f-1.0f�ɐ��K��)
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // �f�[�^��RGBA���ǂ̂悤�Ƀ}�b�s���O���邩
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2D�e�N�X�`��
	srvDesc.Texture2D.MipLevels = 1; // �~�b�v�}�b�v�͎g�p���Ȃ�����1

	// �n���h���̎擾�i�f�B�X�N���v�^�q�[�v�̐擪�i�܂�C���̏ꍇ�V�F�[�_�[���\�[�X�r���[�̈ʒu�j�������j
	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �V�F�[�_�[���\�[�X�r���[���q�[�v��ɍ쐬
	_dev->CreateShaderResourceView(
		texbuff, // �r���[�Ɗ֘A�t����o�b�t�@�[
		&srvDesc, // ��Őݒ肵���e�N�X�`���ݒ���
		basicHeapHandle // �q�[�v�̂ǂ��Ɋ��蓖�Ă邩
	);

	// ��]�A�j���[�V�����p
	XMMATRIX worldMat = XMMatrixIdentity();

	// �r���[�s��̒�`
	XMFLOAT3 eye(0, 10, -15);
	XMFLOAT3 target(0, 10, 0);
	XMFLOAT3 up(0, 1, 0);

	// XMFLOAT3����XMVECTOR�ւ̕ϊ�
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// �v���W�F�N�V�����s��̒�`
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2, // ��p��90�x
		static_cast<float>(window_width) / static_cast<float>(window_height), // �A�X�y�N�g��
		1.0f, // �߂����inear�j
		100.0f // �������ifar�j
	);

	// �萔�o�b�t�@�[�̍쐬
	ID3D12Resource* constBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff), // �o�b�t�@�[�̃T�C�Y�ɍ��킹�Ċm�ۂ���̈��ω�������
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);
	std::cout << "�萔�o�b�t�@�[�̍쐬�m�F: " << result << std::endl;

	MatricesData* mapMatrix; // �}�b�v��������|�C���^�[
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix); // �}�b�v
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;
	//*mapMatrix = worldMat * viewMat * projMat; //�s��̓��e���R�s�[

						 // ���̏ꏊ�Ɉړ�
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	// �萔�o�b�t�@�[�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);

	// �}�e���A���o�b�t�@�[�̍쐬
	auto materialBuffSize = sizeof(MaterialForHlsl);
	materialBuffSize = (materialBuffSize + 0xff) & ~0xff; // 256�A���C�����g�ɂ��邽�߂̏���

	ID3D12Resource* materialBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(materialBuffSize * materialNum),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&materialBuff)
	);

	// �}�e���A���o�b�t�@�[�Ƀf�[�^���R�s�[
	char* mapMaterial = nullptr;

	result = materialBuff->Map(0, nullptr, (void**)&mapMaterial);


	for (auto& m : materials) {
		*((MaterialForHlsl*)mapMaterial) = m.material; // �f�[�^�̃R�s�[
		mapMaterial += materialBuffSize; // ���̃A���C�����g�̈ʒu�܂Ői�߂�
	}
	materialBuff->Unmap(0, nullptr);

	// �}�e���A���p�̃f�B�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* materialDescHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC matDescHeapDesc = {};
	matDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeapDesc.NodeMask = 0;
	matDescHeapDesc.NumDescriptors = materialNum; // �r���[�̓}�e���A���̐��������
	matDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&matDescHeapDesc, IID_PPV_ARGS(&materialDescHeap)); // ����
	std::cout << "�}�e���A���p�̃f�B�X�N���v�^�q�[�v�̍쐬�m�F: " << result << std::endl;

	// �}�e���A���p�̃r���[�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matCBVDesc = {};

	matCBVDesc.BufferLocation = materialBuff->GetGPUVirtualAddress(); // �o�b�t�@�[�A�h���X
	matCBVDesc.SizeInBytes = materialBuffSize; // �}�e���A��256�A���C�������g�T�C�Y

	auto matDescHeapH = materialDescHeap->GetCPUDescriptorHandleForHeapStart(); // �f�B�X�N���v�^�q�[�v�̐擪���L�^

	// ���[�v�Ŋe�}�e���A���̃r���[���쐬����
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
		//�����A�v���P�[�V�������I�����Ď���message��WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT) {
			break;
		}

		angle += 0.02f;
		mapMatrix->world = XMMatrixRotationY(angle);
		mapMatrix->viewproj = viewMat * projMat;

		//DirectX����
		//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		_cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				_backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT,
				D3D12_RESOURCE_STATE_RENDER_TARGET)
		);
		// �R�����g�A�E�g�̂Ƃ���̃R�[�h����̐��s�ōςށI�i�s���I�ɂ͂��܂�ς��Ȃ����Ǖ�����₷���j


		_cmdList->SetPipelineState(_pipelinestate);

		//�����_�[�^�[�Q�b�g���w��
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// �����_�[�^�[�Q�b�g�Ɛ[�x�o�b�t�@�[�r���[�̊֘A�t��
		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH); //�����_�[�^�[�Q�b�g�ւ̏������݂Ɛ[�x�o�b�t�@�[�ւ̏������݂𓯎��ɍs��
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // ���F
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		// ���[�g�V�O�l�`���̐ݒ�
		_cmdList->SetGraphicsRootSignature(rootsignature);

		// �f�B�X�N���v�^�q�[�v�̎w��
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);

		_cmdList->SetGraphicsRootDescriptorTable(
			0,
			basicDescHeap->GetGPUDescriptorHandleForHeapStart()
		);

		// �}�e���A���p�r���[�̃Z�b�g
		
		_cmdList->SetDescriptorHeaps(1, &materialDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(
			1,
			materialDescHeap->GetGPUDescriptorHandleForHeapStart()
		);
		

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);

		//���߂̃N���[�Y
		_cmdList->Close();


		//�R�}���h���X�g�̎��s
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		////�҂�
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		_cmdAllocator->Reset();//�L���[���N���A
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//�ĂуR�}���h���X�g�����߂鏀��

		//�t���b�v
		_swapchain->Present(1, 0);

	}
	//�����N���X�g��񂩂�o�^�������Ă�
	UnregisterClass(w.lpszClassName, w.hInstance);
	return 0;
}