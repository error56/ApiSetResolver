#ifndef APISETRESOLVER_HPP
#define APISETRESOLVER_HPP

#include <string>
#include <windows.h>

struct ProcessEnvironmentBlock {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    UCHAR BitField;
    UCHAR Padding0[4];
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PVOID Ldr;
    PVOID ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    PVOID FastPebLock;
    PVOID AtlThunkSListPtr;
    PVOID IFEOKey;
    ULONG CrossProcessFlags;
    UCHAR Padding1[4];
    PVOID KernelCallbackTable;
    ULONG SystemReserved;
    ULONG AtlThunkSListPtr32;
    PVOID ApiSetMap;
};

struct ThreadEnvironmentBlock {
    NT_TIB NtTib;
    PVOID EnvironmentPointer;
    PVOID Reserved[2];
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    ProcessEnvironmentBlock *ProcessEnvironmentBlock;
};

static_assert(offsetof(ThreadEnvironmentBlock, ProcessEnvironmentBlock) == 0x60);
static_assert(offsetof(ProcessEnvironmentBlock, ApiSetMap) == 0x68);

struct ApiSetValueEntry {
    DWORD Flags;
    DWORD NameOffset;
    DWORD NameLength;
    DWORD ValueOffset;
    DWORD ValueLength;
};

struct ApiSetNamespaceEntry {
    DWORD Flags;
    DWORD NameOffset;
    DWORD SizeOfApiSetName;
    DWORD SizeOfApiSetNameEx; // without hyphens
    DWORD ValueEntriesArrayOffset;
    DWORD HostsNumber;
};

struct ApiSetHeader {
    DWORD Version;
    DWORD Size;
    DWORD Flags;
    DWORD Count;
    DWORD NamespaceEntriesOffset;
    DWORD HashEntriesOffset;
    DWORD HashMultiplier;
};

struct ApiSetHashEntry {
    DWORD Hash;
    DWORD IndexInEntriesNamespace;
};

class ApiSetResolver {
    // The library name hash is computed using a fast and efficient algorithm:
    // For each letter in the string, multiply the hash value by a predefined multiplier (from the header)
    // and add the lowercase ASCII value of the letter.
    static DWORD CalculateHash(const PWSTR String, const DWORD Length, const DWORD HashMultiplier) {
        DWORD Hash = 0;

        for (auto i = 0; i < Length; i++) {
            Hash *= HashMultiplier;
            Hash += tolower(String[i]);
        }

        return Hash;
    }

    // The lookup name is not the full DLL name—we strip the extension (.dll)
    // and remove everything up to the last hyphen.
    //
    // The function returns the index of the last valid character.
    static DWORD CutDllSuffix(const PWSTR String, const DWORD Length) {
        for (auto i = Length; i > 0; i -= 2) {
            const auto Character = *(reinterpret_cast<WCHAR *>(String) + i);
            if (Character == L'-') {
                return i;
            }
        }

        return Length;
    }

public:
    static std::string Resolve(const std::string &LibraryName) {
        // Convert library name to wide characters (all calculations are based on wchars)
        const std::wstring WideLibraryName(LibraryName.begin(), LibraryName.end());
        auto *WideLibraryNamePointer = const_cast<PWSTR>(WideLibraryName.c_str());

        // Get pointer to the ApiSetHeader - the entry point of our calculations
        const auto *Teb = reinterpret_cast<ThreadEnvironmentBlock *>(NtCurrentTeb());
        const auto *Peb = Teb->ProcessEnvironmentBlock;
        const auto ApiSetMapAddress = reinterpret_cast<uint64_t>(Peb->ApiSetMap);
        const auto *Header = reinterpret_cast<const ApiSetHeader *>(ApiSetMapAddress);

        // Get length of library name without extension and last version digit
        const auto NewLength = CutDllSuffix(WideLibraryNamePointer, WideLibraryName.length());

        // Calculate hash of api-set library name
        const auto Hash = CalculateHash(WideLibraryNamePointer, NewLength, Header->HashMultiplier);

        // ApiSet hash table is sorted in ascending order by hash value, so instead of looking up every single entry and
        // comparing it with our hash, we can use the binary search algorithm.
        auto Low = 0;
        auto High = Header->Count - 1;

        while (Low <= High) {
            // Find the middle of our range
            const auto Mid = Low + (High - Low) / 2;

            // Get the middle entry and compare it with our hash
            const auto *HashEntry = reinterpret_cast<ApiSetHashEntry *>(ApiSetMapAddress + Header->HashEntriesOffset +
                                                                        Mid * sizeof(ApiSetHashEntry));
            if (HashEntry->Hash == Hash) {
                // If it's the current hash entry, then get namespace entry from it and resolve final DLL name
                const auto *NamespaceEntry = reinterpret_cast<ApiSetNamespaceEntry *>(
                        ApiSetMapAddress + Header->NamespaceEntriesOffset +
                        sizeof(struct ApiSetNamespaceEntry) * HashEntry->IndexInEntriesNamespace);

                const auto *ValueEntry = reinterpret_cast<ApiSetValueEntry *>(ApiSetMapAddress +
                                                                              NamespaceEntry->ValueEntriesArrayOffset);

                auto ResolvedLibraryWideName =
                        std::wstring(reinterpret_cast<PWCHAR>(ApiSetMapAddress + ValueEntry->ValueOffset),
                                     ValueEntry->ValueLength / sizeof(WCHAR));
                auto ResolvedLibraryName = std::string(ResolvedLibraryWideName.begin(), ResolvedLibraryWideName.end());

                return ResolvedLibraryName;
            }

            // If it's not current entry, then ignore left/right half and do the search again.
            if (HashEntry->Hash < Hash) {
                Low = Mid + 1;
            } else {
                High = Mid - 1;
            }
        }

        return "";
    }
};


#endif // APISETRESOLVER_HPP
