#include <kernel/drivers/pci.h>
#include <io.h>

// Read PCI configuration space
uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31) | (bus << 16) | (dev << 11) | (func << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Find PCI device by class and subclass
struct pci_dev *pci_find_device(uint8_t class_code, uint8_t subclass) {
    static struct pci_dev dev;
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t vendor = pci_read_config(bus, slot, func, 0);
                if ((vendor & 0xFFFF) == 0xFFFF) continue; // No device

                uint32_t class_info = pci_read_config(bus, slot, func, 0x08);
                uint8_t dev_class = (class_info >> 24) & 0xFF;
                uint8_t dev_subclass = (class_info >> 16) & 0xFF;

                if (dev_class == class_code && dev_subclass == subclass) {
                    dev.bus = bus;
                    dev.dev = slot;
                    dev.func = func;
                    dev.vendor_id = vendor & 0xFFFF;
                    dev.device_id = (vendor >> 16) & 0xFFFF;
                    dev.class_code = dev_class;
                    dev.subclass = dev_subclass;
                    return &dev;
                }
            }
        }
    }
    return NULL;
}