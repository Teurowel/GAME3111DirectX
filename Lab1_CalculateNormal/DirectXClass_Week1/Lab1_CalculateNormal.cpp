#include <windows.h> // for XMVerifyCPUSupport
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <iostream>
using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ostream& XM_CALLCONV operator << (ostream& os, FXMVECTOR v)
{
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z << ", " << dest.w << ")";
	return os;

}
XMVECTOR ComputeNormal(FXMVECTOR p0, FXMVECTOR p1, FXMVECTOR p2)
{
	XMVECTOR U = p1 - p0;
	XMVECTOR V = p2 - p0;

	XMVECTOR Normal = XMVector3Normalize(XMVector3Cross(U, V));

	cout << U << endl;
	cout << V << endl;
	cout << Normal << endl;

	return Normal;
}



int main()
{
	// Check support for SSE2 (Pentium4, AMD K8, and above).
	if (!XMVerifyCPUSupport())
	{
		cout << "directx math not supported" << endl;
		return 0;
	}

	//XMVECTOR color = XMVectorSet(0.9f, 0.8f, 0.7f, 1.0f);
	XMCOLOR color32 = XMCOLOR(4288269490);
	//XMStoreColor(&color32, color);

	XMVECTOR color = XMLoadColor(&color32);
	int a = 10;


	//XMVECTOR p0 = XMVectorSet(1.0f, 0.0f, 0.0f, 1.f);
	//XMVECTOR p1 = XMVectorSet(1.0f, 2.0f, 3.0f, 1.f);
	//XMVECTOR p2 = XMVectorSet(-2.0f, 1.0f, -3.0f, 1.f);

	//ComputeNormal(p0, p1, p2);






	//XMVECTOR p = XMVectorSet(2.0f, 2.0f, 1.0f, 0.0f);
	//XMVECTOR q = XMVectorSet(2.0f, -0.5f, 0.5f, 0.1f);
	//XMVECTOR u = XMVectorSet(1.0f, 2.0f, 4.0f, 8.0f);
	//XMVECTOR v = XMVectorSet(-2.0f, 1.0f, -3.0f, 2.5f);
	//XMVECTOR w = XMVectorSet(0.0f, XM_PIDIV4, XM_PIDIV2, XM_PI);



	//cout << endl;;
	//cout << XMVectorAbs(v) << endl;
	//cout << XMVectorCos(w) << endl;
	//cout << XMVectorLog(u) << endl;


	//cout << endl;
	//cout << XMVectorLog(u) << endl;
	//cout << XMVectorExp(p) << endl;
	//cout << XMVectorPow(u, p) << endl;

	//cout << endl;
	//cout << XMVectorSqrt(u) << endl;
	//cout << XMVectorSwizzle(u, 2, 2, 1, 3) << endl;
	//cout << XMVectorSwizzle(u, 2, 1, 0, 3) << endl;
	//cout << XMVectorMultiply(u, v) << endl;

	//cout << endl;
	//cout << XMVectorSaturate(q) << endl;
	//cout << XMVectorMin(p, v) << endl;
	//cout << XMVectorMax(p, v) << endl;

	return 0;
}