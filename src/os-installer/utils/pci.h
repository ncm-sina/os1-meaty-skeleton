#ifndef PCI_H
#define PCI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint32_t pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
int pci_find_device(uint8_t class, uint8_t subclass, uint8_t* bus, uint8_t* slot, uint8_t* func);
uint32_t pci_read_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar);

#endif // PCI_H