//***************************************************************************************
// TreeBillboardsApp.cpp 
//***************************************************************************************

#include "../../Common/d3dApp.h"
#include "../../Common/MathHelper.h"
#include "../../Common/UploadBuffer.h"
#include "../../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

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

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

    // Primitive topology.
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters.
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	AlphaTestedTreeSprites,
	Count
};

class TreeBillboardsApp : public D3DApp
{
public:
    TreeBillboardsApp(HINSTANCE hInstance);
    TreeBillboardsApp(const TreeBillboardsApp& rhs) = delete;
    TreeBillboardsApp& operator=(const TreeBillboardsApp& rhs) = delete;
    ~TreeBillboardsApp();

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
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt); 

	void LoadTextures();
	void CreateTexture(std::string name, std::wstring path, bool bTextureArray = false);
    void BuildRootSignature();
	void BuildDescriptorHeaps();
    void BuildShadersAndInputLayouts();
	void BuildShapeGeometry();
    void BuildWavesGeometry();
	void BuildTreeSpritesGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
	void CreateMaterials(std::string name, XMFLOAT4 diffuseAlbedo, XMFLOAT3 fresnelR0, float roughness);



    void BuildRenderItems();
	void BuildGround(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));
	void BuildWater(FXMVECTOR pos, FXMVECTOR scale = XMVectorSet(1.f, 1.f, 1.f, 0.f), FXMVECTOR rotation = XMVectorSet(0.f, 0.f, 0.f, 0.f));



    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    float GetHillsHeight(float x, float z)const;
    XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mStdInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

    RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

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
	UINT mNumOfTex = 0;
	int mMatCBIdx = 0;
	int mDiffuseSrvHeapIdx = 0;

	std::vector<std::string> mTextuersName;
	std::vector<std::string> mTextuerArraysName;
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
        TreeBillboardsApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

TreeBillboardsApp::TreeBillboardsApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

TreeBillboardsApp::~TreeBillboardsApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool TreeBillboardsApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, 
	// so we have to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mWaves = std::make_unique<Waves>(32, 32, 1.0f, 0.03f, 4.0f, 0.2f);
 
	LoadTextures();
    BuildRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayouts();
	BuildShapeGeometry();
    BuildWavesGeometry();
	//BuildTreeSpritesGeometry();
	BuildMaterials();
    BuildRenderItems();
    BuildFrameResources();
    BuildPSOs();

    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void TreeBillboardsApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
}

void TreeBillboardsApp::Update(const GameTimer& gt)
{
    OnKeyboardInput(gt);
	UpdateCamera(gt);

    // Cycle through the circular frame resource array.
    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // Has the GPU finished processing the commands of the current frame resource?
    // If not, wait until the GPU has completed commands up to this fence point.
    if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
    UpdateWaves(gt);
}

void TreeBillboardsApp::Draw(const GameTimer& gt)
{
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdListAlloc->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    // Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Clear the back buffer and depth buffer.
    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // Specify the buffers we are going to render to.
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	mCommandList->SetPipelineState(mPSOs["treeSprites"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites]);

	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);

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

void TreeBillboardsApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);
}

void TreeBillboardsApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TreeBillboardsApp::OnMouseMove(WPARAM btnState, int x, int y)
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
 
void TreeBillboardsApp::OnKeyboardInput(const GameTimer& gt)
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
 
void TreeBillboardsApp::UpdateCamera(const GameTimer& gt)
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

void TreeBillboardsApp::AnimateMaterials(const GameTimer& gt)
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water"].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * gt.DeltaTime();
	tv += 0.02f * gt.DeltaTime();

	if(tu >= 1.0f)
		tu -= 1.0f;

	if(tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = gNumFrameResources;
}

void TreeBillboardsApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for(auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if(e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void TreeBillboardsApp::UpdateMainPassCB(const GameTimer& gt)
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
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 1.f, -1.f, 1.f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void TreeBillboardsApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for(int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);
		
		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void TreeBillboardsApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

    // A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
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

void TreeBillboardsApp::LoadTextures()
{
	/////////////////////////////////////////////////////////////////////////////
	/* Order matters when Adding to mSrvDescriptorHeap
		When we build materials, DiffuseSrvHeapIndex should match the order of adding descriptor to mSrvDescriptorHeap
	*/
	/////////////////////////////////////////////////////////////////////////////
	//Texture
	CreateTexture("grassTex", L"../../Textures/grass.dds");				
	CreateTexture("waterTex", L"../../Textures/water1.dds");			
	CreateTexture("fenceTex", L"../../Textures/WireFence.dds");
	CreateTexture("brickTex", L"../../Textures/bricks.dds");
	CreateTexture("stoneTex", L"../../Textures/stone.dds");


	//Texture Array
	CreateTexture("treeArrayTex", L"../../Textures/treeArray.dds", true);
}

void TreeBillboardsApp::CreateTexture(std::string name, std::wstring path, bool bTextureArray)
{
	auto texture = std::make_unique<Texture>();
	texture->Name = name;
	texture->Filename = path;
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), texture->Filename.c_str(),
		texture->Resource, texture->UploadHeap));

	mNumOfTex++;

	if (bTextureArray == false)
		mTextuersName.push_back(name);
	else
		mTextuerArraysName.push_back(name);

	mTextures[texture->Name] = std::move(texture);
}

void TreeBillboardsApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = mNumOfTex;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	/////////////////////////////////////////////////////////////////////////////
	/* Order matters when Adding to mSrvDescriptorHeap
		When we build materials, DiffuseSrvHeapIndex should match the order of adding descriptor to mSrvDescriptorHeap
	*/
	/////////////////////////////////////////////////////////////////////////////

	//Texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	for (int i = 0; i < mTextuersName.size(); ++i)
	{
		auto texture = mTextures[mTextuersName[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		md3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	}



	//Texture Array
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;

	for (int i = 0; i < mTextuerArraysName.size(); ++i)
	{
		auto texture = mTextures[mTextuerArraysName[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2DArray.ArraySize = texture->GetDesc().DepthOrArraySize;
		md3dDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
	}
}

void TreeBillboardsApp::BuildMaterials()
{
	/////////////////////////////////////////////////////////////////////////////
/* Order matters when Adding to mSrvDescriptorHeap
	When we build materials, DiffuseSrvHeapIndex should match the order of adding descriptor to mSrvDescriptorHeap
*/
/////////////////////////////////////////////////////////////////////////////

	//Texture
	CreateMaterials("grass", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.125f);
	// This is not a good water material definition, but we do not have all the rendering
	// tools we need (transparency, environment reflection), so we fake it for now.
	CreateMaterials("water", XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f), XMFLOAT3(0.1f, 0.1f, 0.1f), 0.0f);
	CreateMaterials("wirefence", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.02f, 0.02f, 0.02f), 0.25f);
	CreateMaterials("brick", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.125f);
	CreateMaterials("stone", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.125f);



	//Textuer Array
	CreateMaterials("treeSprites", XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), XMFLOAT3(0.01f, 0.01f, 0.01f), 0.125f);
}

void TreeBillboardsApp::CreateMaterials(std::string name, XMFLOAT4 diffuseAlbedo, XMFLOAT3 fresnelR0, float roughness)
{
	auto material = std::make_unique<Material>();
	material->Name = name;
	material->MatCBIndex = mMatCBIdx++;
	material->DiffuseSrvHeapIndex = mDiffuseSrvHeapIdx++;
	material->DiffuseAlbedo = diffuseAlbedo;
	material->FresnelR0 = fresnelR0;
	material->Roughness = roughness;
	mMaterials[name] = std::move(material);
}

void TreeBillboardsApp::BuildShadersAndInputLayouts()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	
	mShaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	mShaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");

    mStdInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void TreeBillboardsApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;

	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.0f, 1.0f, 0);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(10.0f, 10.0f, 10, 10);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.5f, 3.0f, 20, 20);
	GeometryGenerator::MeshData pyramid = geoGen.CreatePyramid(1, 1, 1, 0);
	GeometryGenerator::MeshData cone = geoGen.CreateCone(1.f, 1.f, 40, 6);
	GeometryGenerator::MeshData diamond = geoGen.CreateDiamond(1, 2, 1, 0);
	GeometryGenerator::MeshData wedge = geoGen.CreateWedge(1, 1, 1, 0);
	GeometryGenerator::MeshData halfPyramid = geoGen.CreateHalfPyramid(1, 1, 0.5, 0.5, 1, 0);
	GeometryGenerator::MeshData triSquare = geoGen.CreateTriSquare(1, 2, 0);

	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.

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
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	for (size_t i = 0; i < pyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = pyramid.Vertices[i].Position;
		vertices[k].Normal = pyramid.Vertices[i].Normal;
		vertices[k].TexC = pyramid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cone.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cone.Vertices[i].Position;
		vertices[k].Normal = cone.Vertices[i].Normal;
		vertices[k].TexC = cone.Vertices[i].TexC;
	}

	for (size_t i = 0; i < diamond.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = diamond.Vertices[i].Position;
		vertices[k].Normal = diamond.Vertices[i].Normal;
		vertices[k].TexC = diamond.Vertices[i].TexC;
	}

	for (size_t i = 0; i < wedge.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = wedge.Vertices[i].Position;
		vertices[k].Normal = wedge.Vertices[i].Normal;
		vertices[k].TexC = wedge.Vertices[i].TexC;
	}

	for (size_t i = 0; i < halfPyramid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = halfPyramid.Vertices[i].Position;
		vertices[k].Normal = halfPyramid.Vertices[i].Normal;
		vertices[k].TexC = halfPyramid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < triSquare.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = triSquare.Vertices[i].Position;
		vertices[k].Normal = triSquare.Vertices[i].Normal;
		vertices[k].TexC = triSquare.Vertices[i].TexC;
	}



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

void TreeBillboardsApp::BuildWavesGeometry()
{
    std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

    // Iterate over each quad.
    int m = mWaves->RowCount();
    int n = mWaves->ColumnCount();
    int k = 0;
    for(int i = 0; i < m - 1; ++i)
    {
        for(int j = 0; j < n - 1; ++j)
        {
            indices[k] = i*n + j;
            indices[k + 1] = i*n + j + 1;
            indices[k + 2] = (i + 1)*n + j;

            indices[k + 3] = (i + 1)*n + j;
            indices[k + 4] = i*n + j + 1;
            indices[k + 5] = (i + 1)*n + j + 1;

            k += 6; // next quad
        }
    }

	UINT vbByteSize = mWaves->VertexCount()*sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["water"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildTreeSpritesGeometry()
{
	//step5
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, 16> vertices;
	for(UINT i = 0; i < treeCount; ++i)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		// Move tree slightly above land height.
		y += 8.0f;

		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.0f, 20.0f);
	}

	std::array<std::uint16_t, 16> indices =
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	mGeometries["treeSpritesGeo"] = std::move(geo);
}

void TreeBillboardsApp::BuildPSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
    ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mStdInputLayout.data(), (UINT)mStdInputLayout.size() };
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
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;

	//there is abug with F2 key that is supposed to turn on the multisampling!
//Set4xMsaaState(true);
	//m4xMsaaState = true;

	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//transparentPsoDesc.BlendState.AlphaToCoverageEnable = true;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = 
	{ 
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	//
	// PSO for tree sprites
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteVS"]->GetBufferPointer()),
		mShaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePsoDesc.GS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpriteGS"]->GetBufferPointer()),
		mShaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["treeSpritePS"]->GetBufferPointer()),
		mShaders["treeSpritePS"]->GetBufferSize()
	};
	//step1
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs["treeSprites"])));
}

void TreeBillboardsApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void TreeBillboardsApp::BuildRenderItems()
{
	BuildGround(XMVectorSet(0.f, 0.f, 0.f, 0.f), XMVectorSet(3.f, 1.f, 3.f, 0.f));
	BuildWater(XMVectorSet(0.f, 0.f, -22.5f, 0.f), XMVectorSet(0.98f, 1.f, 0.5f, 0.f));

 

  /*  auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	gridRitem->ObjCBIndex = 0;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
    gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
    gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());*/

	//auto treeSpritesRitem = std::make_unique<RenderItem>();
	//treeSpritesRitem->World = MathHelper::Identity4x4();
	//treeSpritesRitem->ObjCBIndex = 3;
	//treeSpritesRitem->Mat = mMaterials["treeSprites"].get();
	//treeSpritesRitem->Geo = mGeometries["treeSpritesGeo"].get();
	////step2
	//treeSpritesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	//treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs["points"].IndexCount;
	//treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs["points"].StartIndexLocation;
	//treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs["points"].BaseVertexLocation;

	//mRitemLayer[(int)RenderLayer::AlphaTestedTreeSprites].push_back(treeSpritesRitem.get());

   // mAllRitems.push_back(std::move(wavesRitem));
   // mAllRitems.push_back(std::move(gridRitem));
	//mAllRitems.push_back(std::move(boxRitem));
	//mAllRitems.push_back(std::move(treeSpritesRitem));
}

void TreeBillboardsApp::BuildGround(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
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

	//Material
	ground->Mat = mMaterials["stone"].get();

	//Texture Scaling
	XMStoreFloat4x4(&ground->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));

	ground->ObjCBIndex = mObjCBIndex++;
	ground->Geo = mGeometries["shapeGeo"].get();
	ground->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	ground->IndexCount = ground->Geo->DrawArgs["grid"].IndexCount;
	ground->StartIndexLocation = ground->Geo->DrawArgs["grid"].StartIndexLocation;
	ground->BaseVertexLocation = ground->Geo->DrawArgs["grid"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(ground.get());
	mAllRitems.push_back(std::move(ground));
}

void TreeBillboardsApp::BuildWater(FXMVECTOR pos, FXMVECTOR scale, FXMVECTOR rotation)
{
	auto water = std::make_unique<RenderItem>();
	
	//Local
	XMStoreFloat4x4(&water->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixRotationRollPitchYaw(0.f, 0.f, 0.f) * XMMatrixTranslation(0.0f, 0.f, 0.0f));

	//World
	XMStoreFloat4x4(&water->World,
		XMLoadFloat4x4(&water->World) *
		XMMatrixScalingFromVector(scale) *
		XMMatrixRotationRollPitchYawFromVector(rotation) *
		XMMatrixTranslationFromVector(pos));

	//Material
	water->Mat = mMaterials["water"].get();

	//Texture Scaling
	XMStoreFloat4x4(&water->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));

	water->ObjCBIndex = mObjCBIndex++;
	water->Geo = mGeometries["waterGeo"].get();
	water->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	water->IndexCount = water->Geo->DrawArgs["water"].IndexCount;
	water->StartIndexLocation = water->Geo->DrawArgs["water"].StartIndexLocation;
	water->BaseVertexLocation = water->Geo->DrawArgs["water"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Transparent].push_back(water.get());
	mWavesRitem = water.get();

	mAllRitems.push_back(std::move(water));
}

//void TreeBillboardsApp::BuildPrimitives()
//{
	////Build Box
	//auto boxRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&boxRitem->World, XMMatrixTranslation(-15.0f, 5.f, 0.0f));
	//boxRitem->ObjCBIndex = mObjCBIndex++;
	//boxRitem->Mat = mMaterials["grass"].get();
	//boxRitem->Geo = mGeometries["shapeGeo"].get();
	//boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	//boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	//boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	//mRitemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	//mAllRitems.push_back(std::move(boxRitem));


	////Build Cylinder
	//auto cylRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&cylRitem->World, XMMatrixTranslation(-20.0f, 5.f, 0.f));
	//cylRitem->ObjCBIndex = mObjCBIndex++;
	//cylRitem->Mat = mMaterials["brick"].get();
	//cylRitem->Geo = mGeometries["shapeGeo"].get();
	//cylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//cylRitem->IndexCount = cylRitem->Geo->DrawArgs["cylinder"].IndexCount;
	//cylRitem->StartIndexLocation = cylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
	//cylRitem->BaseVertexLocation = cylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;
	//mRitemLayer[(int)RenderLayer::Opaque].push_back(cylRitem.get());
	//mAllRitems.push_back(std::move(cylRitem));


	////Build SPhere
	//auto leftSphereRitem = std::make_unique<RenderItem>();
	//XMMATRIX leftSphereWorld = XMMatrixTranslation(-25.0f, 5.f, 0.f);
	//XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
	//leftSphereRitem->color = XMFLOAT4(DirectX::Colors::Crimson);
	//leftSphereRitem->ObjCBIndex = mObjCBIndex++;
	//leftSphereRitem->Geo = mGeometries["shapeGeo"].get();
	//leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
	//leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	//leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(leftSphereRitem));


	////Build Pyramid
	//auto pyramidRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&pyramidRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-15.0f, 5.f, 5.0f));
	//pyramidRitem->color = XMFLOAT4(DirectX::Colors::Ivory);
	//pyramidRitem->ObjCBIndex = mObjCBIndex++;
	//pyramidRitem->Geo = mGeometries["shapeGeo"].get();
	//pyramidRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//pyramidRitem->IndexCount = pyramidRitem->Geo->DrawArgs["pyramid"].IndexCount;
	//pyramidRitem->StartIndexLocation = pyramidRitem->Geo->DrawArgs["pyramid"].StartIndexLocation;
	//pyramidRitem->BaseVertexLocation = pyramidRitem->Geo->DrawArgs["pyramid"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(pyramidRitem));


	////Build COne
	//auto coneRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&coneRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-20.0f, 5.f, 5.0f));
	//coneRitem->color = XMFLOAT4(DirectX::Colors::Black);
	//coneRitem->ObjCBIndex = mObjCBIndex++;
	//coneRitem->Geo = mGeometries["shapeGeo"].get();
	//coneRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//coneRitem->IndexCount = coneRitem->Geo->DrawArgs["cone"].IndexCount;
	//coneRitem->StartIndexLocation = coneRitem->Geo->DrawArgs["cone"].StartIndexLocation;
	//coneRitem->BaseVertexLocation = coneRitem->Geo->DrawArgs["cone"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(coneRitem));


	////Build Diamond
	//auto diamondRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&diamondRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-25.0f, 5.f, 5.0f));
	//diamondRitem->color = XMFLOAT4(DirectX::Colors::LightPink);
	//diamondRitem->ObjCBIndex = mObjCBIndex++;
	//diamondRitem->Geo = mGeometries["shapeGeo"].get();
	//diamondRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//diamondRitem->IndexCount = diamondRitem->Geo->DrawArgs["diamond"].IndexCount;
	//diamondRitem->StartIndexLocation = diamondRitem->Geo->DrawArgs["diamond"].StartIndexLocation;
	//diamondRitem->BaseVertexLocation = diamondRitem->Geo->DrawArgs["diamond"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(diamondRitem));


	////BUild Wedge
	//auto wedgeRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&wedgeRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-15.0f, 5.f, 10.0f));
	//wedgeRitem->color = XMFLOAT4(DirectX::Colors::Magenta);
	//wedgeRitem->ObjCBIndex = mObjCBIndex++;
	//wedgeRitem->Geo = mGeometries["shapeGeo"].get();
	//wedgeRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//wedgeRitem->IndexCount = wedgeRitem->Geo->DrawArgs["wedge"].IndexCount;
	//wedgeRitem->StartIndexLocation = wedgeRitem->Geo->DrawArgs["wedge"].StartIndexLocation;
	//wedgeRitem->BaseVertexLocation = wedgeRitem->Geo->DrawArgs["wedge"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(wedgeRitem));


	////Build halfPyramid
	//auto halfPyramidRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&halfPyramidRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-20.0f, 5.f, 10.0f));
	//halfPyramidRitem->color = XMFLOAT4(DirectX::Colors::Blue);
	//halfPyramidRitem->ObjCBIndex = mObjCBIndex++;
	//halfPyramidRitem->Geo = mGeometries["shapeGeo"].get();
	//halfPyramidRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//halfPyramidRitem->IndexCount = halfPyramidRitem->Geo->DrawArgs["halfPyramid"].IndexCount;
	//halfPyramidRitem->StartIndexLocation = halfPyramidRitem->Geo->DrawArgs["halfPyramid"].StartIndexLocation;
	//halfPyramidRitem->BaseVertexLocation = halfPyramidRitem->Geo->DrawArgs["halfPyramid"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(halfPyramidRitem));


	////BUild triSqure
	//auto triSquareRitem = std::make_unique<RenderItem>();
	//XMStoreFloat4x4(&triSquareRitem->World, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(-25.0f, 5.f, 10.0f));
	//triSquareRitem->color = XMFLOAT4(DirectX::Colors::Gold);
	//triSquareRitem->ObjCBIndex = mObjCBIndex++;
	//triSquareRitem->Geo = mGeometries["shapeGeo"].get();
	//triSquareRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//triSquareRitem->IndexCount = triSquareRitem->Geo->DrawArgs["triSquare"].IndexCount;
	//triSquareRitem->StartIndexLocation = triSquareRitem->Geo->DrawArgs["triSquare"].StartIndexLocation;
	//triSquareRitem->BaseVertexLocation = triSquareRitem->Geo->DrawArgs["triSquare"].BaseVertexLocation;
	//mAllRitems.push_back(std::move(triSquareRitem));
//}

void TreeBillboardsApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
    UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

    // For each render item...
    for(size_t i = 0; i < ritems.size(); ++i)
    {
        auto ri = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
		//step3
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex*objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex*matCBByteSize;

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
        cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> TreeBillboardsApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return { 
		pointWrap, pointClamp,
		linearWrap, linearClamp, 
		anisotropicWrap, anisotropicClamp };
}

float TreeBillboardsApp::GetHillsHeight(float x, float z)const
{
    return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 TreeBillboardsApp::GetHillsNormal(float x, float z)const
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
        1.0f,
        -0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}
