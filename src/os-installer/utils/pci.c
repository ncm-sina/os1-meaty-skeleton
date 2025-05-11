#include "pci.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t inl(uint16_t port) {
    uint32_t value;
    asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                       ((uint32_t)func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t value = inl(PCI_CONFIG_DATA);
    // printf("PCI read: bus %d, slot %d, func %d, offset 0x%02x, value 0x%08x\n", bus, slot, func, offset, value);
    return value;
}

void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (1U << 31) | ((uint32_t)bus << 16) | ((uint32_t)slot << 11) |
                       ((uint32_t)func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

int pci_find_device(uint8_t class, uint8_t subclass, uint8_t* bus, uint8_t* slot, uint8_t* func) {
    for (*bus = 0; *bus < 256; (*bus)++) {
        for (*slot = 0; *slot < 32; (*slot)++) {
            for (*func = 0; *func < 8; (*func)++) {
                uint32_t config = pci_read_config(*bus, *slot, *func, 0x08);
                uint8_t class_code = (config >> 24) & 0xFF;
                uint8_t subclass_code = (config >> 16) & 0xFF;
                if (class_code == class && subclass_code == subclass) {
                    printf("AHCI found at bus %d, slot %d, func %d\n", *bus, *slot, *func);                    
                    return 1;
                }
                if (*func == 0) {
                    uint32_t header = pci_read_config(*bus, *slot, 0, 0x0C);
                    if (!(header & 0x00800000)) break; // Not multifunction
                }
            }
        }
        if(*bus == 255){
            printf(" no AHCI found!! ");
            break;
        }
    }
    // printf("device: c:%02x s:%02x b:%02x l:%02x f:%02x", class, subclass, bus, slot, func);
    return 0;
}

uint32_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar) {
    return pci_read_config(bus, slot, func, 0x10 + (bar * 4)) & 0xFFFFFFF0;
}