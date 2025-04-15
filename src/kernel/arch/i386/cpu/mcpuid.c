#include <cpuid.h>
#include "kernel/mcpuid.h"

// Vendor strings from CPUs.
#define CPUID_VENDOR_AMD           "AuthenticAMD"
#define CPUID_VENDOR_AMD_OLD       "AMDisbetter!" // Early engineering samples of AMD K5 processor
#define CPUID_VENDOR_INTEL         "GenuineIntel"
#define CPUID_VENDOR_VIA           "VIA VIA VIA "
#define CPUID_VENDOR_TRANSMETA     "GenuineTMx86"
#define CPUID_VENDOR_TRANSMETA_OLD "TransmetaCPU"
#define CPUID_VENDOR_CYRIX         "CyrixInstead"
#define CPUID_VENDOR_CENTAUR       "CentaurHauls"
#define CPUID_VENDOR_NEXGEN        "NexGenDriven"
#define CPUID_VENDOR_UMC           "UMC UMC UMC "
#define CPUID_VENDOR_SIS           "SiS SiS SiS "
#define CPUID_VENDOR_NSC           "Geode by NSC"
#define CPUID_VENDOR_RISE          "RiseRiseRise"
#define CPUID_VENDOR_VORTEX        "Vortex86 SoC"
#define CPUID_VENDOR_AO486         "MiSTer AO486"
#define CPUID_VENDOR_AO486_OLD     "GenuineAO486"
#define CPUID_VENDOR_ZHAOXIN       "  Shanghai  "
#define CPUID_VENDOR_HYGON         "HygonGenuine"
#define CPUID_VENDOR_ELBRUS        "E2K MACHINE "
 
// Vendor strings from hypervisors.
#define CPUID_VENDOR_QEMU          "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM           " KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE        "VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX    "VBoxVBoxVBox"
#define CPUID_VENDOR_XEN           "XenVMMXenVMM"
#define CPUID_VENDOR_HYPERV        "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS     " prl hyperv "
#define CPUID_VENDOR_PARALLELS_ALT " lrpepyh vr " // Sometimes Parallels incorrectly encodes "prl hyperv" as "lrpepyh vr" due to an endianness mismatch.
#define CPUID_VENDOR_BHYVE         "bhyve bhyve "
#define CPUID_VENDOR_QNX           " QNXQVMBSQG "

/* Example: Get CPU's model number */
int get_model(void)
{
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

/* Example: Check for builtin local APIC. */
int check_apic(void)
{
    unsigned int eax, unused, edx;
    __get_cpuid(1, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_EDX_APIC;
}

// Core CPUID function: Executes CPUID with a given leaf
void cpuid(uint32_t leaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    // Inline assembly to execute CPUID instruction
    // Inputs: leaf in EAX
    // Outputs: EAX, EBX, ECX, EDX stored in provided pointers
    __asm__ __volatile__ (
        "cpuid"              // Execute CPUID instruction
        : "=a" (*eax),       // Output: EAX register
          "=b" (*ebx),       // Output: EBX register
          "=c" (*ecx),       // Output: ECX register
          "=d" (*edx)        // Output: EDX register
        : "a" (leaf)         // Input: leaf value in EAX
        : "cc"               // Clobbers: condition codes (flags)
    );
}

// Extended CPUID function: Executes CPUID with leaf and subleaf
void cpuid_extended(uint32_t leaf, uint32_t subleaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    // Similar to cpuid(), but also sets ECX to subleaf
    __asm__ __volatile__ (
        "cpuid"
        : "=a" (*eax),
          "=b" (*ebx),
          "=c" (*ecx),
          "=d" (*edx)
        : "a" (leaf),        // Input: leaf in EAX
          "c" (subleaf)      // Input: subleaf in ECX
        : "cc"
    );
}

// Get CPU vendor string (from leaf 0x0)
void get_cpu_vendor(char* vendor_string) {
    uint32_t eax, ebx, ecx, edx;

    // Call CPUID with leaf 0x0
    cpuid(0, &eax, &ebx, &ecx, &edx);

    // Vendor string is returned in EBX, EDX, ECX (12 bytes total)
    // Copy registers into the string buffer in the correct order
    *(uint32_t*)(vendor_string)     = ebx;
    *(uint32_t*)(vendor_string + 4) = edx;
    *(uint32_t*)(vendor_string + 8) = ecx;

    // Ensure null termination (13th byte)
    vendor_string[12] = '\0';
}

// Check if a specific CPU feature is supported (leaf 0x1)
int has_cpu_feature(uint32_t feature_flag) {
    uint32_t eax, ebx, ecx, edx;

    // Call CPUID with leaf 0x1 for feature flags
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // Feature flags are in EDX or ECX; check if the requested bit is set
    if (feature_flag & 0xFC000000) {  // Bits 26-31 are in EDX
        return (edx & feature_flag) != 0;
    } else {                          // Bits 0-31 in ECX (e.g., AVX)
        return (ecx & feature_flag) != 0;
    }
}

// Get CPU info: family, model, stepping (from leaf 0x1)
void get_cpu_info(uint32_t* family, uint32_t* model, uint32_t* stepping) {
    uint32_t eax, ebx, ecx, edx;

    // Call CPUID with leaf 0x1
    cpuid(1, &eax, &ebx, &ecx, &edx);

    // Extract fields from EAX:
    // Bits 3-0: Stepping
    // Bits 7-4: Model
    // Bits 11-8: Family
    *stepping = eax & 0xF;              // Lower 4 bits
    *model    = (eax >> 4) & 0xF;       // Next 4 bits
    *family   = (eax >> 8) & 0xF;       // Next 4 bits

    // Extended model/family (for newer CPUs)
    uint32_t ext_model  = (eax >> 16) & 0xF;
    uint32_t ext_family = (eax >> 20) & 0xFF;
    if (*family == 0xF) {
        *family += ext_family;          // Add extended family
        *model   = (*model) + (ext_model << 4);  // Combine base and extended model
    }
}

// Check if CPUID is supported (rarely needed for i686, but included for completeness)
int is_cpuid_supported(void) {
    uint32_t flags_before, flags_after;

    // Get current EFLAGS value
    __asm__ __volatile__ (
        "pushfl\n\t"          // Push EFLAGS onto stack
        "popl %0\n\t"         // Pop into variable
        : "=r" (flags_before)
    );

    // Toggle the ID bit (bit 21) and write back
    flags_after = flags_before ^ (1U << 21);
    __asm__ __volatile__ (
        "pushl %0\n\t"        // Push modified flags
        "popfl\n\t"           // Pop into EFLAGS
        "pushfl\n\t"          // Push EFLAGS again
        "popl %0\n\t"         // Pop into variable
        : "=r" (flags_after)
        : "r" (flags_after)
    );

    // If the ID bit can be toggled, CPUID is supported
    return ((flags_before ^ flags_after) & (1U << 21)) != 0;
}