//***************************************************************************************
// Boxes
//
// Hold down '1' key to view scene in wireframe mode.
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();
	XMFLOAT4 color = XMFLOAT4(0.f, 0.f, 0.f, 0.f);

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

class ShapesApp : public D3DApp
{
public:
	ShapesApp(HINSTANCE hInstance);
	ShapesApp(const ShapesApp& rhs) = delete;
	ShapesApp& operator=(const ShapesApp& rhs) = delete;
	~ShapesApp();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

	void BuildDescriptorHeaps();
	void BuildConstantBufferViews();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildRenderItems();
	void BuildPrimitives();
	void BuildGround(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildHospital(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildFourBuildings(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildWaterBuilding(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildTwoBuildings(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildSquareBuilding(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void Tower(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildStrangeBuildings(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	UINT mPassCbvOffset = 0;

	bool mIsWireframe = false;

	XMFLOAT3 mEyePos = { 0.0f, 5.0f, -40.0f };
	XMFLOAT3 mFront = { 0.f, 0.f, 1.f };
	XMFLOAT3 mRight = { 1.f, 0.f, 0.f };
	XMFLOAT3 mUp = { 0.f ,1.f, 0.f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 0.f;// 1.5f * XM_PI;
	float mPhi = 0.f;// 0.2f * XM_PI;
	float mRadius = 15.0f;
	float mCameraSpeed = 10.f;
	

	POINT mLastMousePos;

	UINT mObjCBIndex = 0;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		ShapesApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

ShapesApp::ShapesApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

ShapesApp::~ShapesApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool ShapesApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSOs();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void ShapesApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void ShapesApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
}

void ShapesApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void ShapesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void ShapesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void ShapesApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi -= dy;

		// Restrict the angle mPhi.
		//mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
		mPhi = MathHelper::Clamp(mPhi, -MathHelper::Pi * 0.5f + 0.1f, MathHelper::Pi * 0.5f - 0.1f);
	}
	//else if ((btnState & MK_RBUTTON) != 0)
	//{
	//	// Make each pixel correspond to 0.2 unit in the scene.
	//	float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
	//	float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

	//	// Update the camera radius based on input.
	//	mRadius += dx - dy;

	//	// Restrict the radius.
	//	mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	//}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void ShapesApp::OnKeyboardInput(const GameTimer& gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;

	//w
	if (GetAsyncKeyState(0x57) & 0x8000)
	{
		mEyePos.x += mFront.x * mCameraSpeed * gt.DeltaTime();
		mEyePos.y += mFront.y * mCameraSpeed * gt.DeltaTime();
		mEyePos.z += mFront.z * mCameraSpeed * gt.DeltaTime();
	}
	//s
	if (GetAsyncKeyState(0x53) & 0x8000)
	{
		mEyePos.x -= mFront.x * mCameraSpeed * gt.DeltaTime();
		mEyePos.y -= mFront.y * mCameraSpeed * gt.DeltaTime();
		mEyePos.z -= mFront.z * mCameraSpeed * gt.DeltaTime();
	}
	//a
	if (GetAsyncKeyState(0x41) & 0x8000)
	{
		mEyePos.x -= mRight.x * mCameraSpeed * gt.DeltaTime();
		mEyePos.y -= mRight.y * mCameraSpeed * gt.DeltaTime();
		mEyePos.z -= mRight.z * mCameraSpeed * gt.DeltaTime();
	}
	//d
	if (GetAsyncKeyState(0x44) & 0x8000)
	{
		mEyePos.x += mRight.x * mCameraSpeed * gt.DeltaTime();
		mEyePos.y += mRight.y * mCameraSpeed * gt.DeltaTime();
		mEyePos.z += mRight.z * mCameraSpeed * gt.DeltaTime();
	}
}

void ShapesApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	//mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	//mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	//mEyePos.y = mRadius * cosf(mPhi);

	//mTheta == Yaw, mPhi == Pitch

	mFront.x = sin(mTheta) * cos(mPhi);
	mFront.y = sin(mPhi);
	mFront.z = cos(mTheta) * cos(mPhi);
	
	//calculate front vector
	XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&mFront));
	XMStoreFloat3(&mFront, front);

	//calculate right vector
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), front));
	XMStoreFloat3(&mRight, right);

	//calculate up vector
	XMVECTOR up = XMVector3Normalize(XMVector3Cross(front, right));
	XMStoreFloat3(&mUp, up);

	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	//XMVECTOR target = XMVectorZero();
	XMVECTOR target = pos + front;
	//XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void ShapesApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMVECTOR color = XMLoadFloat4(&e->color);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4(&objConstants.color, color);


			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void ShapesApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void ShapesApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)mOpaqueRitems.size();

	// Need a CBV descriptor for each object for each frame resource,
	// +1 for the perPass CBV for each frame resource.
	UINT numDescriptors = (objCount + 1) * gNumFrameResources;

	// Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
	mPassCbvOffset = objCount * gNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = numDescriptors;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
		IID_PPV_ARGS(&mCbvHeap)));
}

void ShapesApp::BuildConstantBufferViews()
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	UINT objCount = (UINT)mOpaqueRitems.size();

	// Need a CBV descriptor for each object for each frame resource.
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
		for (UINT i = 0; i < objCount; ++i)
		{
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

			// Offset to the ith object constant buffer in the buffer.
			cbAddress += i * objCBByteSize;

			// Offset to the object cbv in the descriptor heap.
			int heapIndex = frameIndex * objCount + i;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;

			md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Last three descriptors are the pass CBVs for each frame resource.
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Offset to the pass cbv in the descriptor heap.
		int heapIndex = mPassCbvOffset + frameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;

		md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void ShapesApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	// Create root CBVs.
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void ShapesApp::BuildShadersAndInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\VS.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\PS.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void ShapesApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.0f, 1.0f, 0);
	
	//Step1
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(10.0f, 10.0f, 10, 10);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.5f, 3.0f, 20, 20);
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1, 1, 1, 0);
	GeometryGenerator::MeshData cone = geoGen.CreateCone(1.f, 1.f, 40, 6);
	GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(1, 2, 1, 0);
	GeometryGenerator::MeshData wedge = geoGen.CreateWedge(1, 1, 1, 0);
	GeometryGenerator::MeshData halfPyramid = geoGen.CreateHalfPyramid(1, 1, 0.5, 0.5, 1, 0);
	GeometryGenerator::MeshData triSquare = geoGen.CreateTriSquare(1, 2, 0);


	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	//Step2
	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT pyramidVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
	UINT coneVertexOffset = pyramidVertexOffset + (UINT)pyramid.Vertices.size();
	UINT diamondVertexOffset = coneVertexOffset + (UINT)cone.Vertices.size();
	UINT wedgeVertexOffset = diamondVertexOffset + (UINT)diamond.Vertices.size();
	UINT halfPyramidVertexOffset = wedgeVertexOffset + (UINT)wedge.Vertices.size();
	UINT triSquareVertexOffset = halfPyramidVertexOffset + (UINT)halfPyramid.Vertices.size();


	//Step3
	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT pyramidIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
	UINT coneIndexOffset = pyramidIndexOffset + (UINT)pyramid.Indices32.size();
	UINT diamondIndexOffset = coneIndexOffset + (UINT)cone.Indices32.size();
	UINT wedgeIndexOffset = diamondIndexOffset + (UINT)diamond.Indices32.size();
	UINT halfPyramidIndexOffset = wedgeIndexOffset + (UINT)wedge.Indices32.size();
	UINT triSquareIndexOffset = halfPyramidIndexOffset + (UINT)halfPyramid.Indices32.size();




	//Step4
	// Define the SubmeshGeometry that cover different 
	// regions of the vertex/index buffers.

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;
	
	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	SubmeshGeometry pyramidSubmesh;
	pyramidSubmesh.IndexCount = (UINT)pyramid.Indices32.size();
	pyramidSubmesh.StartIndexLocation = pyramidIndexOffset;
	pyramidSubmesh.BaseVertexLocation = pyramidVertexOffset;

	SubmeshGeometry coneSubmesh;
	coneSubmesh.IndexCount = (UINT)cone.Indices32.size();
	coneSubmesh.StartIndexLocation = coneIndexOffset;
	coneSubmesh.BaseVertexLocation = coneVertexOffset;

	SubmeshGeometry diamondSubmesh;
	diamondSubmesh.IndexCount = (UINT)diamond.Indices32.size();
	diamondSubmesh.StartIndexLocation = diamondIndexOffset;
	diamondSubmesh.BaseVertexLocation = diamondVertexOffset;

	SubmeshGeometry wedgeSubmesh;
	wedgeSubmesh.IndexCount = (UINT)wedge.Indices32.size();
	wedgeSubmesh.StartIndexLocation = wedgeIndexOffset;
	wedgeSubmesh.BaseVertexLocation = wedgeVertexOffset;

	SubmeshGeometry halfPyramidSubmesh;
	halfPyramidSubmesh.IndexCount = (UINT)halfPyramid.Indices32.size();
	halfPyramidSubmesh.StartIndexLocation = halfPyramidIndexOffset;
	halfPyramidSubmesh.BaseVertexLocation = halfPyramidVertexOffset;

	SubmeshGeometry triSquareSubmesh;
	triSquareSubmesh.IndexCount = (UINT)triSquare.Indices32.size();
	triSquareSubmesh.StartIndexLocation = triSquareIndexOffset;
	triSquareSubmesh.BaseVertexLocation = triSquareVertexOffset;




	//step5
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.

	auto totalVertexCount = 
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size() +
		pyramid.Vertices.size() + 
		cone.Vertices.size() +
		diamond.Vertices.size() +
		wedge.Vertices.size() + 
		halfPyramid.Vertices.size() +
		triSquare.Vertices.size();



	//step6
	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkOrange);
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Aqua);
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
	}

	for (size_t i = 0; i < pyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = pyramid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Ivory);
	}

	for (size_t i = 0; i < cone.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cone.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Black);
	}

	for (size_t i = 0; i < diamond.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = diamond.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::LightPink);
	}

	for (size_t i = 0; i < wedge.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = wedge.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Magenta);
	}

	for (size_t i = 0; i < halfPyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = halfPyramid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}

	for (size_t i = 0; i < triSquare.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = triSquare.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Gold);
	}




	//step7
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(pyramid.GetIndices16()), std::end(pyramid.GetIndices16()));
	indices.insert(indices.end(), std::begin(cone.GetIndices16()), std::end(cone.GetIndices16()));
	indices.insert(indices.end(), std::begin(diamond.GetIndices16()), std::end(diamond.GetIndices16()));
	indices.insert(indices.end(), std::begin(wedge.GetIndices16()), std::end(wedge.GetIndices16()));
	indices.insert(indices.end(), std::begin(halfPyramid.GetIndices16()), std::end(halfPyramid.GetIndices16()));
	indices.insert(indices.end(), std::begin(triSquare.GetIndices16()), std::end(triSquare.GetIndices16()));



	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;


	//step8
	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["pyramid"] = pyramidSubmesh;
	geo->DrawArgs["cone"] = coneSubmesh;
	geo->DrawArgs["diamond"] = diamondSubmesh;
	geo->DrawArgs["wedge"] = wedgeSubmesh;
	geo->DrawArgs["halfPyramid"] = halfPyramidSubmesh;
	geo->DrawArgs["triSquare"] = triSquareSubmesh;




	mGeometries[geo->Name] = std::move(geo);
}

void ShapesApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	//
	// PSO for opaque wireframe objects.
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
}

void ShapesApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, (UINT)mAllRitems.size()));
	}
}

void ShapesApp::BuildRenderItems()
{
	BuildPrimitives();
	BuildGround(XMVectorSet(0.f, 0.f, 0.f, 0.f), XMVectorSet(3, 1.f, 3.f, 0.f));
	BuildHospital(XMVectorSet(-5.f, 1.f, 12.f, 0.f));
	BuildFourBuildings(XMVectorSet(-11.f, 5.f, 5.f, 0.f));
	BuildWaterBuilding(XMVectorSet(-10.f, 1.5f, -11.f, 0.f));
	BuildTwoBuildings(XMVectorSet(5.f, 5.f, 12.f, 0.f));
	BuildStrangeBuildings(XMVectorSet(12.f, 2.f, 2.f, 0.f));
	BuildSquareBuilding(XMVectorSet(12.f, 2.f, -11.f, 0.f));
	Tower(XMVectorSet(0.f, 0.f, 0.f, 0.f));


	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void ShapesApp::BuildPrimitives()
{
	//Build Box
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(-15.0f, 5.f, 0.0f));
	boxRitem->color = XMFLOAT4(DirectX::Colors::DarkOrange);
	boxRitem->ObjCBIndex = mObjCBIndex++;
	boxRitem->Geo = mGeometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));


	//Build Cylinder
	auto leftCylRitem = std::make_unique<RenderItem>();
	XMMATRIX leftCylWorld = XMMatrixTranslation(-20.0f, 5.f, 0.f);
	XMStoreFloat4x4(&leftCylRitem->World, leftCylWorld);
	leftCylRitem->color = XMFLOAT4(DirectX::Colors::SteelBlue);
	leftCylRitem->ObjCBIndex = mObjCBIndex++;
	leftCylRitem->Geo = mGeometries["shapeGeo"].get();
	leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
	leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
	leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
	mAllRitems.push_back(std::move(leftCylRitem));


	//Build SPhere
	auto leftSphereRitem = std::make_unique<RenderItem>();
	XMMATRIX leftSphereWorld = XMMatrixTranslation(-25.0f, 5.f, 0.f);
	XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
	leftSphereRitem->color = XMFLOAT4(DirectX::Colors::Crimson);
	leftSphereRitem->ObjCBIndex = mObjCBIndex++;
	leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
	leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
	leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
	mAllRitems.push_back(std::move(leftSphereRitem));
	

	//Build Pyramid
	auto pyramidRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&pyramidRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-15.0f, 5.f, 5.0f));
	pyramidRitem->color = XMFLOAT4(DirectX::Colors::Ivory);
	pyramidRitem->ObjCBIndex = mObjCBIndex++;
	pyramidRitem->Geo = mGeometries["shapeGeo"].get();
	pyramidRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	pyramidRitem->IndexCount = pyramidRitem->Geo->DrawArgs["pyramid"].IndexCount;
	pyramidRitem->StartIndexLocation = pyramidRitem->Geo->DrawArgs["pyramid"].StartIndexLocation;
	pyramidRitem->BaseVertexLocation = pyramidRitem->Geo->DrawArgs["pyramid"].BaseVertexLocation;
	mAllRitems.push_back(std::move(pyramidRitem));


	//Build COne
	auto coneRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&coneRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-20.0f, 5.f, 5.0f));
	coneRitem->color = XMFLOAT4(DirectX::Colors::Black);
	coneRitem->ObjCBIndex = mObjCBIndex++;
	coneRitem->Geo = mGeometries["shapeGeo"].get();
	coneRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	coneRitem->IndexCount = coneRitem->Geo->DrawArgs["cone"].IndexCount;
	coneRitem->StartIndexLocation = coneRitem->Geo->DrawArgs["cone"].StartIndexLocation;
	coneRitem->BaseVertexLocation = coneRitem->Geo->DrawArgs["cone"].BaseVertexLocation;
	mAllRitems.push_back(std::move(coneRitem));


	//Build Diamond
	auto diamondRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&diamondRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-25.0f, 5.f, 5.0f));
	diamondRitem->color = XMFLOAT4(DirectX::Colors::LightPink);
	diamondRitem->ObjCBIndex = mObjCBIndex++;
	diamondRitem->Geo = mGeometries["shapeGeo"].get();
	diamondRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	diamondRitem->IndexCount = diamondRitem->Geo->DrawArgs["diamond"].IndexCount;
	diamondRitem->StartIndexLocation = diamondRitem->Geo->DrawArgs["diamond"].StartIndexLocation;
	diamondRitem->BaseVertexLocation = diamondRitem->Geo->DrawArgs["diamond"].BaseVertexLocation;
	mAllRitems.push_back(std::move(diamondRitem));


	//BUild Wedge
	auto wedgeRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&wedgeRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)* XMMatrixTranslation(-15.0f, 5.f, 10.0f));
	wedgeRitem->color = XMFLOAT4(DirectX::Colors::Magenta);
	wedgeRitem->ObjCBIndex = mObjCBIndex++;
	wedgeRitem->Geo = mGeometries["shapeGeo"].get();
	wedgeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wedgeRitem->IndexCount = wedgeRitem->Geo->DrawArgs["wedge"].IndexCount;
	wedgeRitem->StartIndexLocation = wedgeRitem->Geo->DrawArgs["wedge"].StartIndexLocation;
	wedgeRitem->BaseVertexLocation = wedgeRitem->Geo->DrawArgs["wedge"].BaseVertexLocation;
	mAllRitems.push_back(std::move(wedgeRitem));


	//Build halfPyramid
	auto halfPyramidRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&halfPyramidRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)* XMMatrixTranslation(-20.0f, 5.f, 10.0f));
	halfPyramidRitem->color = XMFLOAT4(DirectX::Colors::Blue);
	halfPyramidRitem->ObjCBIndex = mObjCBIndex++;
	halfPyramidRitem->Geo = mGeometries["shapeGeo"].get();
	halfPyramidRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	halfPyramidRitem->IndexCount = halfPyramidRitem->Geo->DrawArgs["halfPyramid"].IndexCount;
	halfPyramidRitem->StartIndexLocation = halfPyramidRitem->Geo->DrawArgs["halfPyramid"].StartIndexLocation;
	halfPyramidRitem->BaseVertexLocation = halfPyramidRitem->Geo->DrawArgs["halfPyramid"].BaseVertexLocation;
	mAllRitems.push_back(std::move(halfPyramidRitem));


	//BUild triSqure
	auto triSquareRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&triSquareRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f)* XMMatrixTranslation(-25.0f, 5.f, 10.0f));
	triSquareRitem->color = XMFLOAT4(DirectX::Colors::Gold);
	triSquareRitem->ObjCBIndex = mObjCBIndex++;
	triSquareRitem->Geo = mGeometries["shapeGeo"].get();
	triSquareRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	triSquareRitem->IndexCount = triSquareRitem->Geo->DrawArgs["triSquare"].IndexCount;
	triSquareRitem->StartIndexLocation = triSquareRitem->Geo->DrawArgs["triSquare"].StartIndexLocation;
	triSquareRitem->BaseVertexLocation = triSquareRitem->Geo->DrawArgs["triSquare"].BaseVertexLocation;
	mAllRitems.push_back(std::move(triSquareRitem));
}

void ShapesApp::BuildGround(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//Ground
	auto ground = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&ground->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 0.f, 0.0f));
	
	//World
	XMStoreFloat4x4(&ground->World,
		XMLoadFloat4x4(&ground->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	ground->color = XMFLOAT4(DirectX::Colors::Gray);

	ground->ObjCBIndex = mObjCBIndex++;
	ground->Geo = mGeometries["shapeGeo"].get();
	ground->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ground->IndexCount = ground->Geo->DrawArgs["grid"].IndexCount;
	ground->StartIndexLocation = ground->Geo->DrawArgs["grid"].StartIndexLocation;
	ground->BaseVertexLocation = ground->Geo->DrawArgs["grid"].BaseVertexLocation;
	mAllRitems.push_back(std::move(ground));


	//Water
	auto water = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&water->World, XMMatrixScaling(1.0f, 1.0f, 0.3f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 0.f, -6.5f));

	//World
	XMStoreFloat4x4(&water->World,
		XMLoadFloat4x4(&water->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	water->color = XMFLOAT4(DirectX::Colors::Blue);

	water->ObjCBIndex = mObjCBIndex++;
	water->Geo = mGeometries["shapeGeo"].get();
	water->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	water->IndexCount = water->Geo->DrawArgs["grid"].IndexCount;
	water->StartIndexLocation = water->Geo->DrawArgs["grid"].StartIndexLocation;
	water->BaseVertexLocation = water->Geo->DrawArgs["grid"].BaseVertexLocation;
	mAllRitems.push_back(std::move(water));
}

void ShapesApp::BuildHospital(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//Main Box
	auto mainBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&mainBox->World, XMMatrixScaling(3.0f, 2.0f, 1.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&mainBox->World,
		XMLoadFloat4x4(&mainBox->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	mainBox->color = XMFLOAT4(DirectX::Colors::White);

	mainBox->ObjCBIndex = mObjCBIndex++;
	mainBox->Geo = mGeometries["shapeGeo"].get();
	mainBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mainBox->IndexCount = mainBox->Geo->DrawArgs["box"].IndexCount;
	mainBox->StartIndexLocation = mainBox->Geo->DrawArgs["box"].StartIndexLocation;
	mainBox->BaseVertexLocation = mainBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(mainBox));


	//Top Box
	auto topBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&topBox->World, XMMatrixScaling(1.0f, 1.0f, 0.6f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 1.5f, 0.2f));

	//World
	XMStoreFloat4x4(&topBox->World,
		XMLoadFloat4x4(&topBox->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	topBox->color = XMFLOAT4(DirectX::Colors::Red);

	topBox->ObjCBIndex = mObjCBIndex++;
	topBox->Geo = mGeometries["shapeGeo"].get();
	topBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	topBox->IndexCount = topBox->Geo->DrawArgs["box"].IndexCount;
	topBox->StartIndexLocation = topBox->Geo->DrawArgs["box"].StartIndexLocation;
	topBox->BaseVertexLocation = topBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(topBox));

	//Left BIg Box
	auto leftBigBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&leftBigBox->World, XMMatrixScaling(1.0f, 4.0f, 0.7f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 1.f, 0.3f));

	//World
	XMStoreFloat4x4(&leftBigBox->World,
		XMLoadFloat4x4(&leftBigBox->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	leftBigBox->color = XMFLOAT4(DirectX::Colors::Red);

	leftBigBox->ObjCBIndex = mObjCBIndex++;
	leftBigBox->Geo = mGeometries["shapeGeo"].get();
	leftBigBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftBigBox->IndexCount = leftBigBox->Geo->DrawArgs["box"].IndexCount;
	leftBigBox->StartIndexLocation = leftBigBox->Geo->DrawArgs["box"].StartIndexLocation;
	leftBigBox->BaseVertexLocation = leftBigBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(leftBigBox));


	//Right BIg Box
	auto rightBigBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&rightBigBox->World, XMMatrixScaling(1.0f, 4.0f, 0.7f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(2.0f, 1.f, 0.3f));

	//World
	XMStoreFloat4x4(&rightBigBox->World,
		XMLoadFloat4x4(&rightBigBox->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	rightBigBox->color = XMFLOAT4(DirectX::Colors::Red);

	rightBigBox->ObjCBIndex = mObjCBIndex++;
	rightBigBox->Geo = mGeometries["shapeGeo"].get();
	rightBigBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightBigBox->IndexCount = rightBigBox->Geo->DrawArgs["box"].IndexCount;
	rightBigBox->StartIndexLocation = rightBigBox->Geo->DrawArgs["box"].StartIndexLocation;
	rightBigBox->BaseVertexLocation = rightBigBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(rightBigBox));


	//left small Box
	auto leftSmallBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&leftSmallBox->World, XMMatrixScaling(1.0f, 1.0f, 1.f)* XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f)* XMMatrixTranslation(-2.0f, -0.5f, -0.6f));

	//World
	XMStoreFloat4x4(&leftSmallBox->World,
		XMLoadFloat4x4(&leftSmallBox->World)*
		XMMatrixScalingFromVector(scale)*
		XMMatrixRotationRollPitchYawFromVector(rotation)*
		XMMatrixTranslationFromVector(pos));

	//Color
	leftSmallBox->color = XMFLOAT4(DirectX::Colors::White);

	leftSmallBox->ObjCBIndex = mObjCBIndex++;
	leftSmallBox->Geo = mGeometries["shapeGeo"].get();
	leftSmallBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	leftSmallBox->IndexCount = leftSmallBox->Geo->DrawArgs["box"].IndexCount;
	leftSmallBox->StartIndexLocation = leftSmallBox->Geo->DrawArgs["box"].StartIndexLocation;
	leftSmallBox->BaseVertexLocation = leftSmallBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(leftSmallBox));


	//right small Box
	auto rightSmallBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&rightSmallBox->World, XMMatrixScaling(1.0f, 1.0f, 1.f)* XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f)* XMMatrixTranslation(2.0f, -0.5f, -0.6f));

	//World
	XMStoreFloat4x4(&rightSmallBox->World,
		XMLoadFloat4x4(&rightSmallBox->World)*
		XMMatrixScalingFromVector(scale)*
		XMMatrixRotationRollPitchYawFromVector(rotation)*
		XMMatrixTranslationFromVector(pos));

	//Color
	rightSmallBox->color = XMFLOAT4(DirectX::Colors::White);

	rightSmallBox->ObjCBIndex = mObjCBIndex++;
	rightSmallBox->Geo = mGeometries["shapeGeo"].get();
	rightSmallBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rightSmallBox->IndexCount = rightSmallBox->Geo->DrawArgs["box"].IndexCount;
	rightSmallBox->StartIndexLocation = rightSmallBox->Geo->DrawArgs["box"].StartIndexLocation;
	rightSmallBox->BaseVertexLocation = rightSmallBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(rightSmallBox));

	
	//Cross Vertical Box
	auto crossVerticalBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&crossVerticalBox->World, XMMatrixScaling(0.7f, 0.2f, 0.1f)* XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f)* XMMatrixTranslation(0.0f, 1.5f, -0.1f));

	//World
	XMStoreFloat4x4(&crossVerticalBox->World,
		XMLoadFloat4x4(&crossVerticalBox->World)*
		XMMatrixScalingFromVector(scale)*
		XMMatrixRotationRollPitchYawFromVector(rotation)*
		XMMatrixTranslationFromVector(pos));

	//Color
	crossVerticalBox->color = XMFLOAT4(DirectX::Colors::Green);

	crossVerticalBox->ObjCBIndex = mObjCBIndex++;
	crossVerticalBox->Geo = mGeometries["shapeGeo"].get();
	crossVerticalBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	crossVerticalBox->IndexCount = crossVerticalBox->Geo->DrawArgs["box"].IndexCount;
	crossVerticalBox->StartIndexLocation = crossVerticalBox->Geo->DrawArgs["box"].StartIndexLocation;
	crossVerticalBox->BaseVertexLocation = crossVerticalBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(crossVerticalBox));


	//Cross Horizontal Box
	auto crossHorizontalBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&crossHorizontalBox->World, XMMatrixScaling(0.2f, 0.7f, 0.1f)* XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f)* XMMatrixTranslation(0.0f, 1.5f, -0.1f));

	//World
	XMStoreFloat4x4(&crossHorizontalBox->World,
		XMLoadFloat4x4(&crossHorizontalBox->World)*
		XMMatrixScalingFromVector(scale)*
		XMMatrixRotationRollPitchYawFromVector(rotation)*
		XMMatrixTranslationFromVector(pos));

	//Color
	crossHorizontalBox->color = XMFLOAT4(DirectX::Colors::Green);

	crossHorizontalBox->ObjCBIndex = mObjCBIndex++;
	crossHorizontalBox->Geo = mGeometries["shapeGeo"].get();
	crossHorizontalBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	crossHorizontalBox->IndexCount = crossHorizontalBox->Geo->DrawArgs["box"].IndexCount;
	crossHorizontalBox->StartIndexLocation = crossHorizontalBox->Geo->DrawArgs["box"].StartIndexLocation;
	crossHorizontalBox->BaseVertexLocation = crossHorizontalBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(crossHorizontalBox));
}

void ShapesApp::BuildFourBuildings(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//1st Building
	auto firstBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&firstBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&firstBuilding->World,
		XMLoadFloat4x4(&firstBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	firstBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	firstBuilding->ObjCBIndex = mObjCBIndex++;
	firstBuilding->Geo = mGeometries["shapeGeo"].get();
	firstBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	firstBuilding->IndexCount = firstBuilding->Geo->DrawArgs["box"].IndexCount;
	firstBuilding->StartIndexLocation = firstBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	firstBuilding->BaseVertexLocation = firstBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(firstBuilding));

	//2nd Building
	auto secondBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&secondBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 0.f, -5.0f));

	//World
	XMStoreFloat4x4(&secondBuilding->World,
		XMLoadFloat4x4(&secondBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	secondBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	secondBuilding->ObjCBIndex = mObjCBIndex++;
	secondBuilding->Geo = mGeometries["shapeGeo"].get();
	secondBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	secondBuilding->IndexCount = secondBuilding->Geo->DrawArgs["box"].IndexCount;
	secondBuilding->StartIndexLocation = secondBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	secondBuilding->BaseVertexLocation = secondBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(secondBuilding));

	//3rd Building
	auto thirdBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&thirdBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(2.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&thirdBuilding->World,
		XMLoadFloat4x4(&thirdBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	thirdBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	thirdBuilding->ObjCBIndex = mObjCBIndex++;
	thirdBuilding->Geo = mGeometries["shapeGeo"].get();
	thirdBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	thirdBuilding->IndexCount = thirdBuilding->Geo->DrawArgs["box"].IndexCount;
	thirdBuilding->StartIndexLocation = thirdBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	thirdBuilding->BaseVertexLocation = thirdBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(thirdBuilding));

	//4th Building
	auto fourthBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&fourthBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(2.0f, 0.f, -5.0f));

	//World
	XMStoreFloat4x4(&fourthBuilding->World,
		XMLoadFloat4x4(&fourthBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	fourthBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	fourthBuilding->ObjCBIndex = mObjCBIndex++;
	fourthBuilding->Geo = mGeometries["shapeGeo"].get();
	fourthBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	fourthBuilding->IndexCount = fourthBuilding->Geo->DrawArgs["box"].IndexCount;
	fourthBuilding->StartIndexLocation = fourthBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	fourthBuilding->BaseVertexLocation = fourthBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(fourthBuilding));

	

}

void ShapesApp::BuildWaterBuilding(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//Main Box
	auto mainBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&mainBox->World, XMMatrixScaling(4.0f, 3.0f, 4.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&mainBox->World,
		XMLoadFloat4x4(&mainBox->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	mainBox->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	mainBox->ObjCBIndex = mObjCBIndex++;
	mainBox->Geo = mGeometries["shapeGeo"].get();
	mainBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mainBox->IndexCount = mainBox->Geo->DrawArgs["box"].IndexCount;
	mainBox->StartIndexLocation = mainBox->Geo->DrawArgs["box"].StartIndexLocation;
	mainBox->BaseVertexLocation = mainBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(mainBox));

	//Upper Box
	auto upperBox = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&upperBox->World, XMMatrixScaling(3.5f, 2.0f, 3.5f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 2.f, 0.0f));

	//World
	XMStoreFloat4x4(&upperBox->World,
		XMLoadFloat4x4(&upperBox->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	upperBox->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	upperBox->ObjCBIndex = mObjCBIndex++;
	upperBox->Geo = mGeometries["shapeGeo"].get();
	upperBox->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	upperBox->IndexCount = upperBox->Geo->DrawArgs["box"].IndexCount;
	upperBox->StartIndexLocation = upperBox->Geo->DrawArgs["box"].StartIndexLocation;
	upperBox->BaseVertexLocation = upperBox->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(upperBox));

	//Door
	auto door = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&door->World, XMMatrixScaling(1.f, 1.f, 0.1f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-1.0f, -0.5f, -2.f));

	//World
	XMStoreFloat4x4(&door->World,
		XMLoadFloat4x4(&door->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	door->color = XMFLOAT4(DirectX::Colors::DarkGray);

	door->ObjCBIndex = mObjCBIndex++;
	door->Geo = mGeometries["shapeGeo"].get();
	door->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	door->IndexCount = door->Geo->DrawArgs["box"].IndexCount;
	door->StartIndexLocation = door->Geo->DrawArgs["box"].StartIndexLocation;
	door->BaseVertexLocation = door->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(door));

	//Bridge
	auto bridge = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&bridge->World, XMMatrixScaling(1.f, 1.f, 0.1f) * XMMatrixRotationRollPitchYaw(1.6f, 0.f, 0.f) * XMMatrixTranslation(-1.0f, -1.0f, -2.5f));

	//World
	XMStoreFloat4x4(&bridge->World,
		XMLoadFloat4x4(&bridge->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	bridge->color = XMFLOAT4(DirectX::Colors::Orange);

	bridge->ObjCBIndex = mObjCBIndex++;
	bridge->Geo = mGeometries["shapeGeo"].get();
	bridge->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bridge->IndexCount = bridge->Geo->DrawArgs["box"].IndexCount;
	bridge->StartIndexLocation = bridge->Geo->DrawArgs["box"].StartIndexLocation;
	bridge->BaseVertexLocation = bridge->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(bridge));

	//Wood Ground
	auto woodGround = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&woodGround->World, XMMatrixScaling(3.f, 3.f, 0.2f) * XMMatrixRotationRollPitchYaw(1.6f, 0.f, 0.f) * XMMatrixTranslation(0.0f, -1.0f, -4.5f));

	//World
	XMStoreFloat4x4(&woodGround->World,
		XMLoadFloat4x4(&woodGround->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	woodGround->color = XMFLOAT4(DirectX::Colors::SandyBrown);

	woodGround->ObjCBIndex = mObjCBIndex++;
	woodGround->Geo = mGeometries["shapeGeo"].get();
	woodGround->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	woodGround->IndexCount = woodGround->Geo->DrawArgs["box"].IndexCount;
	woodGround->StartIndexLocation = woodGround->Geo->DrawArgs["box"].StartIndexLocation;
	woodGround->BaseVertexLocation = woodGround->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(woodGround));

}

void ShapesApp::BuildTwoBuildings(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//1st Building
	auto firstBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&firstBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&firstBuilding->World,
		XMLoadFloat4x4(&firstBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	firstBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	firstBuilding->ObjCBIndex = mObjCBIndex++;
	firstBuilding->Geo = mGeometries["shapeGeo"].get();
	firstBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	firstBuilding->IndexCount = firstBuilding->Geo->DrawArgs["box"].IndexCount;
	firstBuilding->StartIndexLocation = firstBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	firstBuilding->BaseVertexLocation = firstBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(firstBuilding));

	//2nd Building
	auto secondBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&secondBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(2.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&secondBuilding->World,
		XMLoadFloat4x4(&secondBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	secondBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	secondBuilding->ObjCBIndex = mObjCBIndex++;
	secondBuilding->Geo = mGeometries["shapeGeo"].get();
	secondBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	secondBuilding->IndexCount = secondBuilding->Geo->DrawArgs["box"].IndexCount;
	secondBuilding->StartIndexLocation = secondBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	secondBuilding->BaseVertexLocation = secondBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(secondBuilding));


	//bridge
	auto bridge = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&bridge->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 1.f, 0.0f));

	//World
	XMStoreFloat4x4(&bridge->World,
		XMLoadFloat4x4(&bridge->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	bridge->color = XMFLOAT4(DirectX::Colors::LightSkyBlue);

	bridge->ObjCBIndex = mObjCBIndex++;
	bridge->Geo = mGeometries["shapeGeo"].get();
	bridge->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	bridge->IndexCount = bridge->Geo->DrawArgs["box"].IndexCount;
	bridge->StartIndexLocation = bridge->Geo->DrawArgs["box"].StartIndexLocation;
	bridge->BaseVertexLocation = bridge->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(bridge));
}

void ShapesApp::BuildStrangeBuildings(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//1st Floor
	auto firstBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&firstBuilding->World, XMMatrixScaling(8.0f, 4.0f, 4.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&firstBuilding->World,
		XMLoadFloat4x4(&firstBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	firstBuilding->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	firstBuilding->ObjCBIndex = mObjCBIndex++;
	firstBuilding->Geo = mGeometries["shapeGeo"].get();
	firstBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	firstBuilding->IndexCount = firstBuilding->Geo->DrawArgs["box"].IndexCount;
	firstBuilding->StartIndexLocation = firstBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	firstBuilding->BaseVertexLocation = firstBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(firstBuilding));

	//2nd Floor
	auto secondFloor = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&secondFloor->World, XMMatrixScaling(7.0f, 3.0f, 4.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-1.5f, 3.f, 0.0f));

	//World
	XMStoreFloat4x4(&secondFloor->World,
		XMLoadFloat4x4(&secondFloor->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	secondFloor->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	secondFloor->ObjCBIndex = mObjCBIndex++;
	secondFloor->Geo = mGeometries["shapeGeo"].get();
	secondFloor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	secondFloor->IndexCount = secondFloor->Geo->DrawArgs["box"].IndexCount;
	secondFloor->StartIndexLocation = secondFloor->Geo->DrawArgs["box"].StartIndexLocation;
	secondFloor->BaseVertexLocation = secondFloor->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(secondFloor));

	//3rd Floor
	auto thirdFloor = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&thirdFloor->World, XMMatrixScaling(5.0f, 3.0f, 4.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-0.5f, 6.f, 0.0f));

	//World
	XMStoreFloat4x4(&thirdFloor->World,
		XMLoadFloat4x4(&thirdFloor->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	thirdFloor->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	thirdFloor->ObjCBIndex = mObjCBIndex++;
	thirdFloor->Geo = mGeometries["shapeGeo"].get();
	thirdFloor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	thirdFloor->IndexCount = thirdFloor->Geo->DrawArgs["box"].IndexCount;
	thirdFloor->StartIndexLocation = thirdFloor->Geo->DrawArgs["box"].StartIndexLocation;
	thirdFloor->BaseVertexLocation = thirdFloor->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(thirdFloor));

	//4th Floor
	auto fourthFloor = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&fourthFloor->World, XMMatrixScaling(5.0f, 3.0f, 4.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 9.f, 0.0f));

	//World
	XMStoreFloat4x4(&fourthFloor->World,
		XMLoadFloat4x4(&fourthFloor->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	fourthFloor->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	fourthFloor->ObjCBIndex = mObjCBIndex++;
	fourthFloor->Geo = mGeometries["shapeGeo"].get();
	fourthFloor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	fourthFloor->IndexCount = fourthFloor->Geo->DrawArgs["box"].IndexCount;
	fourthFloor->StartIndexLocation = fourthFloor->Geo->DrawArgs["box"].StartIndexLocation;
	fourthFloor->BaseVertexLocation = fourthFloor->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(fourthFloor));

	//long Floor
	auto longFloor = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&longFloor->World, XMMatrixScaling(2.5f, 8.0f, 4.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-0.7f, 14.f, 0.0f));

	//World
	XMStoreFloat4x4(&longFloor->World,
		XMLoadFloat4x4(&longFloor->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	longFloor->color = XMFLOAT4(DirectX::Colors::SkyBlue);

	longFloor->ObjCBIndex = mObjCBIndex++;
	longFloor->Geo = mGeometries["shapeGeo"].get();
	longFloor->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	longFloor->IndexCount = longFloor->Geo->DrawArgs["box"].IndexCount;
	longFloor->StartIndexLocation = longFloor->Geo->DrawArgs["box"].StartIndexLocation;
	longFloor->BaseVertexLocation = longFloor->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(longFloor));

}

void ShapesApp::BuildSquareBuilding(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//1st Building
	auto firstBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&firstBuilding->World, XMMatrixScaling(5.0f, 5.0f, 5.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.0f, 0.5f, 0.0f));

	//World
	XMStoreFloat4x4(&firstBuilding->World,
		XMLoadFloat4x4(&firstBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	firstBuilding->color = XMFLOAT4(DirectX::Colors::DarkGray);

	firstBuilding->ObjCBIndex = mObjCBIndex++;
	firstBuilding->Geo = mGeometries["shapeGeo"].get();
	firstBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	firstBuilding->IndexCount = firstBuilding->Geo->DrawArgs["box"].IndexCount;
	firstBuilding->StartIndexLocation = firstBuilding->Geo->DrawArgs["box"].StartIndexLocation;
	firstBuilding->BaseVertexLocation = firstBuilding->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(firstBuilding));

	//Deco 1
	auto deco1 = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&deco1->World, XMMatrixScaling(3.0f, 0.5f, 0.5f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-3.0f, -0.5f, -2.5f));

	//World
	XMStoreFloat4x4(&deco1->World,
		XMLoadFloat4x4(&deco1->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	deco1->color = XMFLOAT4(DirectX::Colors::DarkSlateGray);

	deco1->ObjCBIndex = mObjCBIndex++;
	deco1->Geo = mGeometries["shapeGeo"].get();
	deco1->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	deco1->IndexCount = deco1->Geo->DrawArgs["box"].IndexCount;
	deco1->StartIndexLocation = deco1->Geo->DrawArgs["box"].StartIndexLocation;
	deco1->BaseVertexLocation = deco1->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(deco1));


	//Deco 2
	auto deco2 = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&deco2->World, XMMatrixScaling(0.5f, 1.5f, 0.5f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-1.725f, 0.5f, -2.5f));

	//World
	XMStoreFloat4x4(&deco2->World,
		XMLoadFloat4x4(&deco2->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	deco2->color = XMFLOAT4(DirectX::Colors::DarkSlateGray);

	deco2->ObjCBIndex = mObjCBIndex++;
	deco2->Geo = mGeometries["shapeGeo"].get();
	deco2->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	deco2->IndexCount = deco2->Geo->DrawArgs["box"].IndexCount;
	deco2->StartIndexLocation = deco2->Geo->DrawArgs["box"].StartIndexLocation;
	deco2->BaseVertexLocation = deco2->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(deco2));

	//Deco 3
	auto deco3 = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&deco3->World, XMMatrixScaling(2.0f, 0.5f, 0.5f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(-2.5f, 1.5f, -2.5f));

	//World
	XMStoreFloat4x4(&deco3->World,
		XMLoadFloat4x4(&deco3->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	deco3->color = XMFLOAT4(DirectX::Colors::DarkSlateGray);

	deco3->ObjCBIndex = mObjCBIndex++;
	deco3->Geo = mGeometries["shapeGeo"].get();
	deco3->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	deco3->IndexCount = deco3->Geo->DrawArgs["box"].IndexCount;
	deco3->StartIndexLocation = deco3->Geo->DrawArgs["box"].StartIndexLocation;
	deco3->BaseVertexLocation = deco3->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(deco3));

	//Deco 4
	auto deco4 = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&deco4->World, XMMatrixScaling(0.5f, 0.5f, 3.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation (- 4.5f, -0.5f, -1.f));

	//World
	XMStoreFloat4x4(&deco4->World,
		XMLoadFloat4x4(&deco4->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	deco4->color = XMFLOAT4(DirectX::Colors::DarkSlateGray);

	deco4->ObjCBIndex = mObjCBIndex++;
	deco4->Geo = mGeometries["shapeGeo"].get();
	deco4->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	deco4->IndexCount = deco4->Geo->DrawArgs["box"].IndexCount;
	deco4->StartIndexLocation = deco4->Geo->DrawArgs["box"].StartIndexLocation;
	deco4->BaseVertexLocation = deco4->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(deco4));

	//Deco 5
	auto deco5 = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&deco5->World, XMMatrixScaling(0.5f, 1.5f, 0.5f)* XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f)* XMMatrixTranslation(-4.5f, 0.5f, 0.25f));

	//World
	XMStoreFloat4x4(&deco5->World,
		XMLoadFloat4x4(&deco5->World)*
		XMMatrixScalingFromVector(scale)*
		XMMatrixRotationRollPitchYawFromVector(rotation)*
		XMMatrixTranslationFromVector(pos));

	//Color
	deco5->color = XMFLOAT4(DirectX::Colors::DarkSlateGray);

	deco5->ObjCBIndex = mObjCBIndex++;
	deco5->Geo = mGeometries["shapeGeo"].get();
	deco5->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	deco5->IndexCount = deco5->Geo->DrawArgs["box"].IndexCount;
	deco5->StartIndexLocation = deco5->Geo->DrawArgs["box"].StartIndexLocation;
	deco5->BaseVertexLocation = deco5->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(deco5));

	//Deco 6
	auto deco6 = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&deco6->World, XMMatrixScaling(0.5f, 0.5f, 2.0f)* XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f)* XMMatrixTranslation(-4.5f, 1.5f, -0.5f));

	//World
	XMStoreFloat4x4(&deco6->World,
		XMLoadFloat4x4(&deco6->World)*
		XMMatrixScalingFromVector(scale)*
		XMMatrixRotationRollPitchYawFromVector(rotation)*
		XMMatrixTranslationFromVector(pos));

	//Color
	deco6->color = XMFLOAT4(DirectX::Colors::DarkSlateGray);

	deco6->ObjCBIndex = mObjCBIndex++;
	deco6->Geo = mGeometries["shapeGeo"].get();
	deco6->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	deco6->IndexCount = deco6->Geo->DrawArgs["box"].IndexCount;
	deco6->StartIndexLocation = deco6->Geo->DrawArgs["box"].StartIndexLocation;
	deco6->BaseVertexLocation = deco6->Geo->DrawArgs["box"].BaseVertexLocation;
	mAllRitems.push_back(std::move(deco6));

}

void ShapesApp::Tower(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	//1st Building
	auto firstBuilding = std::make_unique<RenderItem>();

	//Local
	XMStoreFloat4x4(&firstBuilding->World, XMMatrixScaling(2.0f, 10.0f, 2.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&firstBuilding->World,
		XMLoadFloat4x4(&firstBuilding->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Color
	firstBuilding->color = XMFLOAT4(DirectX::Colors::DarkBlue);

	firstBuilding->ObjCBIndex = mObjCBIndex++;
	firstBuilding->Geo = mGeometries["shapeGeo"].get();
	firstBuilding->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	firstBuilding->IndexCount = firstBuilding->Geo->DrawArgs["cylinder"].IndexCount;
	firstBuilding->StartIndexLocation = firstBuilding->Geo->DrawArgs["cylinder"].StartIndexLocation;
	firstBuilding->BaseVertexLocation = firstBuilding->Geo->DrawArgs["cylinder"].BaseVertexLocation;
	mAllRitems.push_back(std::move(firstBuilding));


}



void ShapesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		// Offset to the CBV in the descriptor heap for this object and for this frame resource.
		UINT cbvIndex = mCurrFrameResourceIndex * (UINT)mOpaqueRitems.size() + ri->ObjCBIndex;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}


