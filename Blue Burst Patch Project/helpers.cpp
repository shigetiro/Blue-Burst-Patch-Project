#include "helpers.h"
#include "trinity_items.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <cwchar>
#include <windows.h>
#include <excpt.h>

int gcd(int a, int b)
{
    if (b == 0)
    {
        return a;
    }
    else
    {
        return gcd(b, a % b);
    }
}

void PatchNOP(int addrIn, int size)
{
    memset((void*)addrIn, 0x90, size);
}
void PatchCALL(int addrIn, int addrOut, int addrDest)
{
    int size = addrOut - addrIn;
    memset((void*)addrIn, 0x90, size);
    *(unsigned char*)addrIn = 0xE8;
    *(int*)(addrIn + 1) = addrDest - (addrIn + 5);
}
void PatchJMP(int addrIn, int addrOut, int addrDest)
{
 int size = addrOut - addrIn;
 memset((void*)addrIn, 0x90, size);
 *(unsigned char*)addrIn = 0xE9;
 *(int*)(addrIn + 1) = addrDest - (addrIn + 5);
}

// Process $item command
#include "trinity_items.h"

bool ProcessItemCommand(const wchar_t* input) {
 if (wcsstr(input, L"$item ") == input) {
 const wchar_t* itemSpec = input + 6;
 uint32_t itemId = 0;

 // Try hex ID first
 if (iswxdigit(itemSpec[0])) {
 itemId = wcstoul(itemSpec, nullptr, 16);
 }

 // If not valid hex or ID is 0/FFFF, try item name
 if (itemId == 0 || itemId > 0xFFFF) {
 // Map common custom items - use IDs from server's custom-items.json
 if (wcsstr(itemSpec, L"Divine Handgun") == itemSpec || wcsstr(itemSpec, L"Spirit Needle") == itemSpec) {
 itemId = 0x000F0500;
 }
 else if (wcsstr(itemSpec, L"Buff Stick") == itemSpec) {
 itemId = 0x000F0600;
 }
 else if (wcsstr(itemSpec, L"Divine Sword") == itemSpec) {
 itemId = 0x00020800;
 }
 else if (wcsstr(itemSpec, L"Cloak of Darkness") == itemSpec) {
 itemId = 0x0102A600;
 }
 else if (wcsstr(itemSpec, L"Divine Shine") == itemSpec) {
 itemId = 0x01036500;
 }
 else if (wcsstr(itemSpec, L"Randomized Box") == itemSpec) {
 itemId = 0x03030100;
 }
 else {
 Log(L"Unknown item '%s' or invalid ID", itemSpec);
 return true;
 }
 }

 // Get player position
 float* playerX = reinterpret_cast<float*>(0x00A9C4F4);
 float* playerY = reinterpret_cast<float*>(0x00A9C4F8);
 float* playerZ = reinterpret_cast<float*>(0x00A9C4FC);  // Fixed address

 // Validate pointers before dereferencing
 if (!playerX || !playerY || !playerZ) {
 Log(L"Player position pointers invalid");
 return true;
 }

 // Call game's item spawn function
 using SpawnItemFn = int(__cdecl*)(uint32_t, float, float, float, int);
 SpawnItemFn spawnItem = reinterpret_cast<SpawnItemFn>(0x00518E20);
 int result = spawnItem(itemId, *playerX, *playerY, *playerZ, 0);

  Log(L"Spawned item 0x%X, result: %d", itemId, result);

  // Debug: Read PMT entry for this item and log the id (unitxt index) field
  __try {
    // g_PmtDataPointer (at 0x00A8DC94) holds pointer P.
    // *P = weapon table base. *(P+8) = unit table. *(P+0xC) = tool table.
    uint32_t P = *(uint32_t*)0x00A8DC94;
    Log(L"PMT Debug: g_PmtDataPointer=0x%08X", P);

    if (P && P != 0xFFFFFFFF) {
      uint8_t cat = (itemId >> 24) & 0xFF;
      uint8_t grp = (itemId >> 16) & 0xFF;
      uint8_t idx = (itemId >> 8) & 0xFF;

      if (cat == 0x00) {
        // Weapon: iVar1 = *P (weapon table base)
        uint32_t wBase = *(uint32_t*)P;
        if (wBase) {
          uint32_t count = *(uint32_t*)(wBase + grp * 8);
          uint32_t dataPtr = *(uint32_t*)(wBase + 4 + grp * 8);
          Log(L"PMT Weapon: group=0x%X idx=0x%X count=%d dataPtr=0x%08X", grp, idx, count, dataPtr);
          if (dataPtr && idx < count) {
            uint32_t entryAddr = dataPtr + idx * 0x2C;
            uint32_t pmtId = *(uint32_t*)(entryAddr + 0);
            uint16_t type = *(uint16_t*)(entryAddr + 4);
            uint16_t skin = *(uint16_t*)(entryAddr + 6);
            uint8_t special = *(uint8_t*)(entryAddr + 0x1C);
            Log(L"PMT Entry@0x%08X: id=0x%X type=0x%X skin=0x%X special=0x%X", entryAddr, pmtId, type, skin, special);
          }
        }
      } else if (cat == 0x01) {
        if (grp == 1 || grp == 2) {
          // Armor/Shield: *(P+4) = armor table base
          uint32_t aBase = *(uint32_t*)(P + 4);
          if (aBase) {
            // armor_table has 2 sub-tables (frames, barriers)
            // Each sub-table: {count, data_ptr} at offset grp-1
            uint32_t subBase = *(uint32_t*)(aBase + (grp - 1) * 8);
            uint32_t subData = *(uint32_t*)(aBase + 4 + (grp - 1) * 8);
            Log(L"PMT Guard: group=%d idx=0x%X count=%d dataPtr=0x%08X", grp, idx, subBase, subData);
            if (subData && idx < subBase) {
              uint32_t entryAddr = subData + idx * 0x24;
              uint32_t pmtId = *(uint32_t*)(entryAddr + 0);
              Log(L"PMT Guard Entry@0x%08X: id=0x%X", entryAddr, pmtId);
            }
          }
        } else if (grp == 3) {
          // Unit: *(P+8) = unit table base
          uint32_t* uBase = *(uint32_t**)(P + 8);
          if (uBase) {
            uint32_t count = uBase[0];
            uint32_t dataPtr = uBase[1];
            Log(L"PMT Unit: idx=0x%X count=%d dataPtr=0x%08X", idx, count, dataPtr);
            if (dataPtr && idx < count) {
              uint32_t entryAddr = dataPtr + idx * 0x14;
              uint32_t pmtId = *(uint32_t*)(entryAddr + 0);
              Log(L"PMT Unit Entry@0x%08X: id=0x%X", entryAddr, pmtId);
            }
          }
        }
      } else if (cat == 0x03) {
        // Tool: *(P+0xC) = tool table base
        uint32_t tBase = *(uint32_t*)(P + 0xC);
        if (tBase) {
          uint32_t count = *(uint32_t*)(tBase + grp * 8);
          uint32_t dataPtr = *(uint32_t*)(tBase + 4 + grp * 8);
          Log(L"PMT Tool: group=0x%X idx=0x%X count=%d dataPtr=0x%08X", grp, idx, count, dataPtr);
          if (dataPtr && idx < count) {
            uint32_t entryAddr = dataPtr + idx * 0x18;
            uint32_t pmtId = *(uint32_t*)(entryAddr + 0);
            Log(L"PMT Tool Entry@0x%08X: id=0x%X", entryAddr, pmtId);
          }
        }
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log(L"PMT Debug: Exception reading PMT (code=0x%X)", GetExceptionCode());
  }

  return true;
 }
 return false;
}

void Log(const WCHAR* fmt, ...)
{
    va_list args;
    WCHAR text[4096];
    FILE* fp;
    SYSTEMTIME rawtime;

    _wfopen_s(&fp, L"log\\dll.log", L"a, ccs=UTF-16LE");
    if (fp != NULL)
    {
        GetLocalTime(&rawtime);
        va_start(args, fmt);
        vswprintf_s(text, sizeof(text) / sizeof(WCHAR), fmt, args);
        va_end(args);

        wcscat_s(text, sizeof(text) / sizeof(WCHAR), L"\n");

        fwprintf_s(fp,
            L"[%02u-%02u-%u, %02u:%02u:%02u] %s",
            rawtime.wMonth,
            rawtime.wDay,
            rawtime.wYear,
            rawtime.wHour,
            rawtime.wMinute,
            rawtime.wSecond,
            text);
        fclose(fp);
    }
}

void StubOutFunction(int addrIn, int addrOut)
{
    PatchNOP(addrIn, addrOut - addrIn);
    *(uint8_t*) addrIn = 0xc3; // ret
}

std::vector<std::vector<std::string>> ReadCsvFile(const std::string& path)
{
    std::ifstream file;
    file.open(path);

    if (file.fail())
        throw std::runtime_error("Failed to open '" + path + "'");

    std::vector<std::vector<std::string>> lines;

    // Read lines
    std::string line;
    while (std::getline(file, line))
    {
        // Split line
        std::istringstream lineStream(line);
        std::string splitPart;
        auto& splitParts = lines.emplace_back();
        
        while (std::getline(lineStream, splitPart, ','))
            splitParts.push_back(splitPart);
    }

    return lines;
}
