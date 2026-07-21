#include <cstring>
#include <stdint.h>
#include <windows.h>
#include "globals.h"

#ifdef PATCH_FRAMERATE
#include "d3d8_hook.h"
#endif

#ifdef PATCH_ITEMS
#include "trinity_items.h"
#endif

#include "asset_manager.h"

// These should be specified in the project's preprocessor macros to enable.
// In Visual Studio, right click the project in the Solution Explorer and select Properties.
// Prepend or append them to "Preprocessor Definitions" which can be found under
// Configuration Properties > C/C++ > Preprocessor.
//
// Alternatively in the future, maybe they could go into pch.h or a file included there.
// Leaving here for documentation.
#if 0
#define PATCH_IME
#define PATCH_UNSELLABLE_RARES
#define PATCH_EARLY_WALK_FIX
#define PATCH_KEYBOARD_ALTERNATE_PALETTE
#define PATCH_SLOW_GIBBLES_FIX
#define PATCH_CUSTOMIZE_MENU
#define PATCH_FASTWARP
#define PATCH_OMNISPAWN
#define PATCH_NEWENEMY // This is missing a header?
#define PATCH_LARGE_ASSETS
#define PATCH_OMNISPAWN
#define PATCH_ENEMY_CONSTRUCTOR_LISTS
#define PATCH_EDITORS
#define PATCH_INITLISTS
#define PATCH_MAP_OBJECT_CONSTRUCTOR_LISTS
#define PATCH_HOOKS
#define PATCH_FRAMERATE
#define PATCH_EXTREME_DIFFICULTY
// #define PATCH_BOX  // Deactivated as requested
#endif

#ifdef PATCH_IME
#include "ime.h"
#endif

#ifdef PATCH_UNSELLABLE_RARES
#include "shop.h"
#endif

#ifdef PATCH_EARLY_WALK_FIX
#include "earlywalk.h"
#endif

#ifdef PATCH_EXTREME_DIFFICULTY
#include "extreme_difficulty.h"
#endif

#ifdef PATCH_KEYBOARD_ALTERNATE_PALETTE
#include "palette.h"
#endif

#ifdef PATCH_SLOW_GIBBLES_FIX
#include "slow_gibbles.h"
#endif

#ifdef PATCH_CUSTOMIZE_MENU
#include "customize_menu.h"
#endif

#ifdef PATCH_FASTWARP
#include "fastwarp.h"
#endif

#ifdef PATCH_OMNISPAWN
#include "omnispawn.h"
#endif

#ifdef PATCH_NEWENEMY
#include "newenemy.h"
#endif

#ifdef PATCH_LARGE_ASSETS
#include "large_assets.h"
#endif

#ifdef PATCH_ENEMY_CONSTRUCTOR_LISTS
#include "enemy.h"
#endif

#ifdef PATCH_MAP_OBJECT_CONSTRUCTOR_LISTS
#include "map_object.h"
#endif

#ifdef PATCH_EDITORS
#include "editors.h"
#endif

#ifdef PATCH_HOOKS
#include "hooking.h"
#endif

#ifdef PATCH_INITLISTS
#include "initlist.h"
#endif

#ifdef PATCH_NEWMAP
#include "newmap/newmap.h"
#endif

#ifdef PATCH_NOFALL
#include "nofall.h"
#endif

#ifdef PATCH_BOX
#include "trinity_box.h"
#endif

#ifdef PATCH_ITEMS
#include "trinity_items.h"
#endif

#ifdef PATCH_WEAPON_VTABLES
#include "weapon_specials.h"
#endif

#include "trinity_psobb.h"

void PSOBB()
{
    // By default, keep the game guard patch enabled
#ifndef DO_NOT_PATCH_DISABLE_GAMEGUARD
    memset((void*)addrMainGameGuardCall, 0x90, 0x05);
#endif

#ifdef PATCH_ASSET_MANAGER
    AssetManager::GetInstance().Initialize();
#endif

#ifdef PATCH_EARLY_WALK_FIX
    ApplyEarlyWalkFix();
#endif

#ifdef PATCH_EXTREME_DIFFICULTY
    ApplyExtremeDifficulty();
#endif

#ifdef PATCH_KEYBOARD_ALTERNATE_PALETTE
    PatchPalette();
#endif

#ifdef PATCH_SLOW_GIBBLES_FIX
    ApplySlowGibblesFix();
#endif

#ifdef PATCH_CUSTOMIZE_MENU
    CustomizeMenu::ApplyActionListPatch();
#endif

#ifdef PATCH_UNSELLABLE_RARES
    PatchShop();
#endif

#ifdef PATCH_IME
    PatchIme();
#endif

#ifdef PATCH_FASTWARP
    ApplyFastWarpPatch();
#endif

#ifdef PATCH_NOFALL
    ApplyNoFallPatch();
#endif

#ifdef PATCH_BOX
    ApplyBoxPatches();
#endif

#ifdef PATCH_SKIP_INTRO_CREDITS
    *(uint8_t*)0x007a645e = 2;
#endif

#ifdef PATCH_FRAMERATE
    SetupD3D8Hook();
#endif

#ifdef PATCH_OMNISPAWN
    Omnispawn::ApplyOmnispawnPatch();
#endif

#ifdef PATCH_NEWENEMY
    ApplyNewEnemyPatch();
#endif

#ifdef PATCH_LARGE_ASSETS
    ApplyLargeAssetsPatch();
#endif

#ifdef PATCH_EDITORS
    ApplyEditorPatch();
#endif

#ifdef PATCH_NEWMAP
    ApplyNewMapPatch();
#endif

#ifdef PATCH_ENEMY_CONSTRUCTOR_LISTS
    Enemy::PatchEnemyConstructorLists();
#endif

#ifdef PATCH_MAP_OBJECT_CONSTRUCTOR_LISTS
    MapObject::PatchMapObjectConstructorLists();
#endif

#ifdef PATCH_HOOKS
    // Should be last so that other patches can create their hooks first
    Hooking::InstallAllHooks();
#endif

#ifdef PATCH_ITEMS
    // Defer injection — PMT and unitxt aren't ready during DllMain.
    // Retry loop handles any startup delay.
    HANDLE hItemThread = CreateThread(NULL, 0, [](LPVOID) -> DWORD {
        for (int i = 0; i < 30; i++) {
            SafeInjectPMTData();
            Sleep(500);
        }
        return 0;
    }, NULL, 0, NULL);
    if (hItemThread) CloseHandle(hItemThread);
#endif

#ifdef PATCH_WEAPON_VTABLES
    // Initialize weapon specials and vtable fixes
    InitializeWeaponSpecials();
    InitializeEphineaVtableFixes();
#endif

#ifdef PATCH_INITLISTS
    // Should be last so that other patches can apply their changes first
    InitList::PatchAllInitLists();
#endif

    // Increase ax size of decompress buffer for ItemMagEdit
    *(uint32_t *)0x5dba7c = 0x2000;

    // Apply TrinityDLL patches (resolution, PMT, AFS counts, stats, etc.)
    TrinityStart();
}

