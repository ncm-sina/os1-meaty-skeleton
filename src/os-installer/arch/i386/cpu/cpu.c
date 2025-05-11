#include <kernel/arch/i386/cpu.h>
#include <io.h>

// Global CPU features
static struct cpu_features cpu_features = {0};

// Check CPUID support
static int cpu_has_cpuid(void) {
    uint32_t flags, flags_after;
    asm volatile(
        "pushfl\n"
        "popl %0\n"
        "movl %0, %1\n"
        "xorl $0x200000, %0\n" // Flip ID bit
        "pushl %0\n"
        "popfl\n"
        "pushfl\n"
        "popl %0\n"
        : "=r"(flags), "=r"(flags_after));
    return flags != flags_after;
}

// Query CPU features using CPUID
static void cpu_detect_features(void) {
    if (!cpu_has_cpuid()) {
        return; // No CPUID, assume minimal features
    }

    uint32_t eax, ebx, ecx, edx;
    // Get feature flags (CPUID function 1)
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));

    cpu_features.fpu = (edx >> 0) & 1;
    cpu_features.sse = (edx >> 25) & 1;
    cpu_features.sse2 = (edx >> 26) & 1;
    cpu_features.tsc = (edx >> 4) & 1;
    cpu_features.apic = (edx >> 9) & 1;
    cpu_features.pae = (edx >> 6) & 1;
}

static inline void fpu_init(){
    // Initialize FPU
    asm volatile(
        "fninit\n"           // Reset FPU
        "movl %cr0, %eax\n"
        "andl $~0x4, %eax\n" // Clear EM (enable FPU)
        "orl $0x2, %eax\n"   // Set MP (monitor coprocessor)
        "movl %eax, %cr0\n"
    );
}

void cpu_init(void) {

    fpu_init();

    // Detect CPU features
    cpu_detect_features();

    // Enable SSE if supported
    if (cpu_features.sse) {
        asm volatile(
            "movl %cr0, %eax\n"
            "andl $~0x4, %eax\n" // Clear EM
            "movl %eax, %cr0\n"
            "movl %cr4, %eax\n"
            "orl $0x200, %eax\n" // Set OSFXSR (SSE support)
            "movl %eax, %cr4\n"
        );
    }
}

const struct cpu_features *cpu_get_features(void) {
    return &cpu_features;
}

void cpu_halt(void) {
    asm volatile("cli; hlt");
}

void cpu_enable_interrupts(void) {
    asm volatile("sti");
}

void cpu_disable_interrupts(void) {
    asm volatile("cli");
}

uint64_t read_tsc(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}