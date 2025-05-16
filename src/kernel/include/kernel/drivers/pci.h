#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI configuration space access
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

struct pci_dev {
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
};

// Function prototypes
uint32_t pci_read_config(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset);
struct pci_dev *pci_find_device(uint8_t class_code, uint8_t subclass);

#endif