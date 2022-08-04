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

bool godMode = false;
bool autoGuard = false;
bool infiniteHp = false;
bool infiniteFever = false;
bool infiniteThurst = false;
bool infiniteJump = false;
bool infiniteKey = false;
bool infiniteGold = false;

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

		ImGui::Checkbox("God Mode", &godMode);
		ImGui::Checkbox("Auto Guard", &autoGuard);
		ImGui::Checkbox("Infinite HP", &infiniteHp);
		ImGui::Checkbox("Infinite Key", &infiniteKey);
		ImGui::Checkbox("Infinite Gold", &infiniteGold);
		ImGui::Checkbox("Infinite Fever", &infiniteFever);
		ImGui::Checkbox("Infinite Thurst", &infiniteThurst);
		ImGui::Checkbox("Infinite Jump", &infiniteJump);

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

DWORD WINAPI MainCheatThread(LPVOID lpReserved)
{
	while (isRunning)
	{
		Sleep(100);

		if (gameMain == nullptr) continue;

		uintptr_t* rootPtr = (uintptr_t*)&gameMain;

		uintptr_t* gameManager = MemFindDMAAddy(rootPtr, { 0x18 });
		if (gameManager == nullptr) continue;

		uintptr_t* playerController = MemFindDMAAddy(gameManager, { 0x40 });
		if (playerController == nullptr) continue;

		uintptr_t* battleCharaData = MemFindDMAAddy(playerController, { 0x18 });
		if (battleCharaData == nullptr) continue;

		uintptr_t* battleCharaParameter = MemFindDMAAddy(battleCharaData, { 0x28 });
		if (battleCharaParameter == nullptr) continue;

		uintptr_t* playerCharaData = MemFindDMAAddy(gameManager, { 0x48 });
		if (playerCharaData == nullptr) continue;

		uintptr_t* characterBehaviour = MemFindDMAAddy(battleCharaData, { 0x20 });
		if (characterBehaviour == nullptr) continue;

		uintptr_t* gameStatus = MemFindDMAAddy(rootPtr, { 0x38 });
		if (gameStatus == nullptr) continue;

		// godMode
		bool* battleCharaData_bNoHit = (bool*)MemFindDMAAddy(battleCharaData, { 0x1D9 });
		if (battleCharaData_bNoHit != nullptr) {
			*battleCharaData_bNoHit = godMode;
		}

		// autoGuard
		bool* battleCharaData_autoGuard = (bool*)MemFindDMAAddy(battleCharaData, { 0x1D8 });
		if (battleCharaData_autoGuard != nullptr) {
			*battleCharaData_autoGuard = autoGuard;
		}

		// infiniteHp
		if (infiniteHp) {
			int* battleCharaParameter_mHp = (int*)MemFindDMAAddy(battleCharaParameter, { 0x464 });
			int* battleCharaParameter_CharaStatus_Hp = (int*)MemFindDMAAddy(battleCharaParameter, { 0x238 + 0x0 });
			if (battleCharaParameter_mHp != nullptr && battleCharaParameter_CharaStatus_Hp != nullptr) {
				*battleCharaParameter_mHp = *battleCharaParameter_CharaStatus_Hp;
			}
		}

		// infiniteFever
		if (infiniteFever) {
			//float* playerCharaData_FeverGauge = (float*)MemFindDMAAddy(playerCharaData, { 0x54 });
			//float* playerCharaData_FeverGaugeMax = (float*)MemFindDMAAddy(playerCharaData, { 0x58 });
			int* playerCharaData_FeverGaugeStockNum = (int*)MemFindDMAAddy(playerCharaData, { 0x5C });
			int* playerCharaData_FeverGaugeStockNumMax = (int*)MemFindDMAAddy(playerCharaData, { 0x60 });
			if (playerCharaData_FeverGaugeStockNum != nullptr && playerCharaData_FeverGaugeStockNumMax != nullptr) {
				*playerCharaData_FeverGaugeStockNum = *playerCharaData_FeverGaugeStockNumMax;
			}
		}

		// infiniteThurst
		if (infiniteThurst) {
			int* playerCharaData_TelepNum = (int*)MemFindDMAAddy(playerCharaData, { 0x64 });
			int* playerCharaData_TelepNumMax = (int*)MemFindDMAAddy(playerCharaData, { 0x68 });
			if (playerCharaData_TelepNum != nullptr && playerCharaData_TelepNumMax != nullptr) {
				*playerCharaData_TelepNum = *playerCharaData_TelepNumMax;
			}
		}

		// infiniteJump
		if (infiniteJump) {
			int* characterBehaviour_mAirJumpCnt = (int*)MemFindDMAAddy(characterBehaviour, { 0x334 });
			if (characterBehaviour_mAirJumpCnt != nullptr) {
				*characterBehaviour_mAirJumpCnt = 0;
			}
		}

		// infiniteKey
		if (infiniteKey) {
			int* gameStatus_BattleDataStatus_KeyNum = (int*)MemFindDMAAddy(gameStatus, { 0x1F8 + 0x1C });
			if (gameStatus_BattleDataStatus_KeyNum != nullptr) {
				*gameStatus_BattleDataStatus_KeyNum = 99;
			}
		}

		// infiniteGold
		if (infiniteGold) {
			int* gameStatus_BattleDataStatus_Gold = (int*)MemFindDMAAddy(gameStatus, { 0x1F8 + 0x18 });
			if (gameStatus_BattleDataStatus_Gold != nullptr) {
				*gameStatus_BattleDataStatus_Gold = 9999;
			}
		}
	}

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
		CreateThread(nullptr, 0, MainCheatThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		isRunning = false;
		MH_Uninitialize();
		kiero::shutdown();
		break;
	}
	return TRUE;
}