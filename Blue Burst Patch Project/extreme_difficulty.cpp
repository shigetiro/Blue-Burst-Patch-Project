#ifdef PATCH_EXTREME_DIFFICULTY

#include "extreme_difficulty.h"
#include "helpers.h"
#include <windows.h>

static wchar_t g_extremeName[] = L"EXTREME";

static unsigned char origUnitxtGetStringBytes[5];
static unsigned char origUnitxtGroup11Bytes[5];

// ============================================================================
// Unitxt Hooks (to safely return "EXTREME" without modifying client files)
// ============================================================================

void* __cdecl HookUnitxtGetString(int group, int index)
{
    if (group == 0xE && index == 4) {
        return g_extremeName;
    }

    // Unpatch
    DWORD oldProt;
    VirtualProtect((void*)0x00792fe8, 5, PAGE_EXECUTE_READWRITE, &oldProt);
    memcpy((void*)0x00792fe8, origUnitxtGetStringBytes, 5);

    // Call original
    typedef void* (__cdecl* UnitxtGetStringFn)(int, int);
    void* result = ((UnitxtGetStringFn)0x00792fe8)(group, index);

    // Repatch
    unsigned char patch[5];
    patch[0] = 0xE9;
    *(int*)(patch + 1) = (int)&HookUnitxtGetString - (0x00792fe8 + 5);
    memcpy((void*)0x00792fe8, patch, 5);
    VirtualProtect((void*)0x00792fe8, 5, oldProt, &oldProt);

    return result;
}

void* __cdecl HookUnitxtGroup11(int index)
{
    if (index == 0x141) {
        return g_extremeName;
    }

    // Unpatch
    DWORD oldProt;
    VirtualProtect((void*)0x0079317c, 5, PAGE_EXECUTE_READWRITE, &oldProt);
    memcpy((void*)0x0079317c, origUnitxtGroup11Bytes, 5);

    // Call original
    typedef void* (__cdecl* UnitxtGroup11Fn)(int);
    void* result = ((UnitxtGroup11Fn)0x0079317c)(index);

    // Repatch
    unsigned char patch[5];
    patch[0] = 0xE9;
    *(int*)(patch + 1) = (int)&HookUnitxtGroup11 - (0x0079317c + 5);
    memcpy((void*)0x0079317c, patch, 5);
    VirtualProtect((void*)0x0079317c, 5, oldProt, &oldProt);

    return result;
}

// ============================================================================
// Tier A: Extend Boundary Clamps
// ============================================================================

static void PatchValidateFunction()
{
    Log(L"[ExtremeDiff] Tier A: patching validate function to allow index 4");

    struct { int addr; unsigned char orig; unsigned char patch; const wchar_t* desc; } patches[] = {
        { 0x0077b2b8, 0x04, 0x05, L"CMP EAX,4 -> CMP EAX,5 (dragon ON)" },
        { 0x0077b2c4, 0x03, 0x04, L"CMP EAX,3 -> CMP EAX,4 (dragon OFF)" },
    };
    struct { int addr; unsigned char orig[4]; unsigned char patch[4]; const wchar_t* desc; } movPatches[] = {
        { 0x0077b2bc, {0x03,0x00,0x00,0x00}, {0x04,0x00,0x00,0x00}, L"MOV EAX,3 -> MOV EAX,4 (dragon ON clamp)" },
        { 0x0077b2c8, {0x02,0x00,0x00,0x00}, {0x03,0x00,0x00,0x00}, L"MOV EAX,2 -> MOV EAX,3 (dragon OFF clamp)" },
    };

    DWORD oldProt = 0;
    for (auto& p : patches) {
        if (VirtualProtect((void*)p.addr, 1, PAGE_EXECUTE_READWRITE, &oldProt)) {
            unsigned char orig = *(unsigned char*)p.addr;
            *(unsigned char*)p.addr = p.patch;
            VirtualProtect((void*)p.addr, 1, oldProt, &oldProt);
            Log(L"[ExtremeDiff]   %s: 0x%02X->0x%02X (%s)",
                p.desc, orig, p.patch,
                *(unsigned char*)p.addr == p.patch ? L"OK" : L"FAILED");
        }
    }
    for (auto& p : movPatches) {
        if (VirtualProtect((void*)p.addr, 4, PAGE_EXECUTE_READWRITE, &oldProt)) {
            memcpy((void*)p.addr, p.patch, 4);
            VirtualProtect((void*)p.addr, 4, oldProt, &oldProt);
            Log(L"[ExtremeDiff]   %s (%s)",
                p.desc,
                memcmp((void*)p.addr, p.patch, 4) == 0 ? L"OK" : L"FAILED");
        }
    }
    Log(L"[ExtremeDiff] Tier A: done");
}

// ============================================================================
// Tier B: Replacement scale-factor function
// ============================================================================

static float __cdecl ExtremeDifficultyScale(float base)
{
    static const float factors[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    int floor = *(int*)0x00a9cd68;

    if (floor < 0) floor = 0;
    if (floor > 4) floor = 4;

    return base * factors[floor];
}

// ============================================================================
// Tier C: Trampoline to add EXTREME to CreateParty popup
// ============================================================================

void __declspec(naked) AddExtremeDifficultyToCreatePartyPopup()
{
    __asm
    {
        // 1. Call original addItem (adds Ultimate)
        mov eax, 0x00735c90
        call eax

        // 2. Call addItem again to add EXTREME
        push 4        // value = 4
        push 6        // flags = 6
        push offset g_extremeName
        mov ecx, esi  // list pointer (this)
        mov eax, 0x00735c90
        call eax

        // 3. Jump to 0x00734693
        mov eax, 0x00734693
        jmp eax
    }
}

// ============================================================================
// ApplyExtremeDifficulty
// ============================================================================

void ApplyExtremeDifficulty()
{
    Log(L"[ExtremeDiff] ApplyExtremeDifficulty: starting");

    DWORD oldProt = 0;

    // Install unitxt_get_string hook
    if (VirtualProtect((void*)0x00792fe8, 5, PAGE_EXECUTE_READWRITE, &oldProt)) {
        memcpy(origUnitxtGetStringBytes, (void*)0x00792fe8, 5);
        unsigned char patch[5];
        patch[0] = 0xE9;
        *(int*)(patch + 1) = (int)&HookUnitxtGetString - (0x00792fe8 + 5);
        memcpy((void*)0x00792fe8, patch, 5);
        VirtualProtect((void*)0x00792fe8, 5, oldProt, &oldProt);
        Log(L"[ExtremeDiff] Installed unitxt_get_string hook");
    }

    // Install unitxt_get_group_11_String hook
    if (VirtualProtect((void*)0x0079317c, 5, PAGE_EXECUTE_READWRITE, &oldProt)) {
        memcpy(origUnitxtGroup11Bytes, (void*)0x0079317c, 5);
        unsigned char patch[5];
        patch[0] = 0xE9;
        *(int*)(patch + 1) = (int)&HookUnitxtGroup11 - (0x0079317c + 5);
        memcpy((void*)0x0079317c, patch, 5);
        VirtualProtect((void*)0x0079317c, 5, oldProt, &oldProt);
        Log(L"[ExtremeDiff] Installed unitxt_get_group_11_String hook");
    }

    // ----------------------------------------------------------------
    // Tier A: Allow difficulty index 4 in validation
    // ----------------------------------------------------------------
    PatchValidateFunction();

    // ----------------------------------------------------------------
    // Tier B: 5-element scale factor arrays & BP loading caps
    // ----------------------------------------------------------------
    Log(L"[ExtremeDiff] Tier B: redirecting scale functions via PatchJMP");
    PatchJMP(0x00678d2c, 0x00678d31, (int)&ExtremeDifficultyScale);
    PatchJMP(0x0063da1c, 0x0063da21, (int)&ExtremeDifficultyScale);
    
    // LoadBattleParamsForEpisode (0x006631f0): Expand array loops from 4 to 5
    struct { int addr; unsigned char orig; unsigned char patch; const wchar_t* desc; } bpPatches[] = {
        { 0x006631c2, 0x04, 0x05, L"LoadBattleParams loop 1 cap -> 5" },
        { 0x00663220, 0x04, 0x05, L"LoadBattleParams loop 2 cap -> 5" },
    };
    for (auto& p : bpPatches) {
        if (VirtualProtect((void*)p.addr, 1, PAGE_EXECUTE_READWRITE, &oldProt)) {
            unsigned char orig = *(unsigned char*)p.addr;
            *(unsigned char*)p.addr = p.patch;
            VirtualProtect((void*)p.addr, 1, oldProt, &oldProt);
            Log(L"[ExtremeDiff]   %s: 0x%02X->0x%02X (%s)",
                p.desc, orig, p.patch,
                *(unsigned char*)p.addr == p.patch ? L"OK" : L"FAILED");
        }
    }
    Log(L"[ExtremeDiff] Tier B: done");

    // ----------------------------------------------------------------
    // Tier C: Add EXTREME to the Create Party difficulty selection popup
    // ----------------------------------------------------------------
    Log(L"[ExtremeDiff] Tier C: patching CreateParty popup at 0x00734628");

    // Patch cap 4->5 at 0x0073462d (PUSH 4 -> PUSH 5)
    if (VirtualProtect((void*)0x0073462d, 1, PAGE_EXECUTE_READWRITE, &oldProt)) {
        unsigned char orig = *(unsigned char*)0x0073462d;
        *(unsigned char*)0x0073462d = 0x05;
        VirtualProtect((void*)0x0073462d, 1, oldProt, &oldProt);
        Log(L"[ExtremeDiff]   cap 4->5 at 0x0073462D: 0x%02X->0x%02X (%s)",
            orig, *(unsigned char*)0x0073462d,
            *(unsigned char*)0x0073462d == 0x05 ? L"OK" : L"FAILED");
    }

    // Hook at 0x007346c4 to add EXTREME
    // Length: 8 bytes (CALL 0x00735c90 (5) + JMP 0x00734693 (2) + NOP (1))
    if (VirtualProtect((void*)0x007346c4, 8, PAGE_EXECUTE_READWRITE, &oldProt)) {
        PatchJMP(0x007346c4, 0x007346c9, (int)&AddExtremeDifficultyToCreatePartyPopup);
        memset((void*)0x007346c9, 0x90, 3); // NOP remaining 3 bytes
        VirtualProtect((void*)0x007346c4, 8, oldProt, &oldProt);
        Log(L"[ExtremeDiff]   hooked at 0x007346C4 to add EXTREME");
    }

    Log(L"[ExtremeDiff] ApplyExtremeDifficulty: complete");
}

#endif // PATCH_EXTREME_DIFFICULTY
