#include "includes.h"
#include <iostream>
#include "kiero/minhook/include/MinHook.h"
#include <fmt/core.h>
#include <fmt/format.h>
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

bool isRunning = true;
bool isShowMenu = true;

uintptr_t* gameMain = nullptr;

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	io.MouseDrawCursor = true;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (isShowMenu) {
		ImGui::Begin("ImGui Window");

		if (gameMain != nullptr) {
			ImGui::Text(fmt::format("GameMain: {}", fmt::ptr(gameMain)).c_str());
		}
		else {
			ImGui::Text("GameMain: Not Found");
		}

		ImGui::End();
	}

	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI HotKeyThread(LPVOID lpReserved)
{
	while (isRunning)
	{
		Sleep(10);

		if (GetAsyncKeyState(VK_END) & 1) {
			isRunning = false;
		}

		if (GetAsyncKeyState(VK_HOME) & 1) {
			isShowMenu = !isShowMenu;
		}
	}

	MH_Uninitialize();
	kiero::shutdown();

	return TRUE;
}

// GameMain_Update hook
typedef void(__stdcall* tGameMain_Update)(uintptr_t*, uintptr_t*);
static tGameMain_Update oGameMain_Update = NULL;
void hkGameMain_Update(uintptr_t* __this, uintptr_t* method) {
	gameMain = __this;
	return oGameMain_Update(__this, method);
}

DWORD WINAPI InitCheatThread(LPVOID lpReserved)
{
	HMODULE hModule = GetModuleHandle("GameAssembly.dll");
	uintptr_t address = (uintptr_t)ScanPattern(hModule, "\x40\x53\x48\x83\xEC\x20\x80\x3D\xD1\xAB\x62\x01\x00", "xxxxxxxxxxxxx");
	LPVOID pTarget = reinterpret_cast<void*>(address);

	MH_CreateHook(pTarget, &hkGameMain_Update, (void**)&oGameMain_Update);
	MH_EnableHook(pTarget);

	while (isRunning && gameMain == nullptr) Sleep(1000);

	MH_DisableHook(pTarget);

	return TRUE;
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		MH_Initialize();
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, HotKeyThread, hMod, 0, nullptr);
		CreateThread(nullptr, 0, InitCheatThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		isRunning = false;
		MH_Uninitialize();
		kiero::shutdown();
		break;
	}
	return TRUE;
}