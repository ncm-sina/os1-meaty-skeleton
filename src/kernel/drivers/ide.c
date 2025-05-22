#include <kernel/drivers/ide.h>
#include <io.h>
#include <string.h>

// Static variables
static uint16_t ide_base = IDE_PRIMARY_BASE;
static uint16_t ide_control = IDE_PRIMARY_CONTROL;
static volatile int ide_irq_fired = 0;
static block_dev_t ide_dev;

// Wait for IDE to be ready (BSY=0)
static void ide_wait_ready(void) {
    uint32_t timeout = 1000000;
    while (timeout-- && (inb(ide_base + IDE_REG_STATUS) & IDE_STATUS_BSY))
        ;
}

// Wait for data request (DRQ=1)
static int ide_wait_drq(void) {
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint8_t status = inb(ide_base + IDE_REG_STATUS);
        if (status & IDE_STATUS_ERR) return -1;
        if (status & IDE_STATUS_DRQ) return 0;
    }
    return -1;
}

// Select drive (master)
void ide_select_drive(void) {
    outb(ide_base + IDE_REG_DRIVE_HEAD, IDE_DRIVE_MASTER);
    ide_wait_ready();
}

// Identify drive
int ide_identify(uint8_t *buffer) {
    ide_select_drive();
    outb(ide_base + IDE_REG_SECTOR_COUNT, 0);
    outb(ide_base + IDE_REG_LBA_LOW, 0);
    outb(ide_base + IDE_REG_LBA_MID, 0);
    outb(ide_base + IDE_REG_LBA_HIGH, 0);
    outb(ide_base + IDE_REG_COMMAND, ATA_CMD_IDENTIFY);

    if (ide_wait_drq()) return -1;

    insw(ide_base + IDE_REG_DATA, buffer, 256);
    return 0;
}

// Read sectors (PIO, interrupt-driven)
int ide_read_sectors(uint32_t lba, uint8_t *buffer, uint32_t count) {
    if (!count) return -1;

    ide_select_drive();

    outb(ide_control + IDE_REG_CONTROL, 0x00); // Enable interrupts

    for (uint32_t i = 0; i < count; i++) {
        outb(ide_base + IDE_REG_SECTOR_COUNT, 1);
        outb(ide_base + IDE_REG_LBA_LOW, lba & 0xFF);
        outb(ide_base + IDE_REG_LBA_MID, (lba >> 8) & 0xFF);
        outb(ide_base + IDE_REG_LBA_HIGH, (lba >> 16) & 0xFF);
        outb(ide_base + IDE_REG_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
        outb(ide_base + IDE_REG_COMMAND, ATA_CMD_READ_SECTORS);

        ide_irq_fired = 0;
        while (!ide_irq_fired){
            // serial_printf("%d",ide_irq_fired);
            asm volatile("pause");            
        }

        if (ide_wait_drq()) return -1;
        insw(ide_base + IDE_REG_DATA, buffer + i * 512, 256);
        lba++;
    }

    return 0;
}

// Write sectors (PIO, interrupt-driven)
int ide_write_sectors(uint32_t lba, const uint8_t *buffer, uint32_t count) {
    if (!count) return -1;

    ide_select_drive();
    outb(ide_control + IDE_REG_CONTROL, 0x00); // Enable interrupts

    for (uint32_t i = 0; i < count; i++) {
        outb(ide_base + IDE_REG_SECTOR_COUNT, 1);
        outb(ide_base + IDE_REG_LBA_LOW, lba & 0xFF);
        outb(ide_base + IDE_REG_LBA_MID, (lba >> 8) & 0xFF);
        outb(ide_base + IDE_REG_LBA_HIGH, (lba >> 16) & 0xFF);
        outb(ide_base + IDE_REG_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
        outb(ide_base + IDE_REG_COMMAND, ATA_CMD_WRITE_SECTORS);

        if (ide_wait_drq()) return -1;
        outsw(ide_base + IDE_REG_DATA, buffer + i * 512, 256);

        ide_irq_fired = 0;
        while (!ide_irq_fired){
            asm volatile("pause");            
        }

        if (inb(ide_base + IDE_REG_STATUS) & IDE_STATUS_ERR) return -1;
        lba++;
    }

    return 0;
}

// IRQ handler for IDE
void ide_irq_handler(void) {
    ide_irq_fired = 1;
    inb(ide_base + IDE_REG_STATUS); // Clear interrupt
}

// Block device interface
int block_read(uint32_t lba, uint8_t *buffer, uint32_t count) {
    return ide_read_sectors(lba, buffer, count);
}

int block_write(uint32_t lba, const uint8_t *buffer, uint32_t count) {
    return ide_write_sectors(lba, buffer, count);
}

// Initialize IDE driver
void ide_init(void) {
    // Check PCI for IDE controller
    struct pci_dev *ide_pci = pci_find_device(0x01, 0x01);
    if (ide_pci) {
        uint32_t bar0 = pci_read_config(ide_pci->bus, ide_pci->dev, ide_pci->func, 0x10);
        uint32_t bar1 = pci_read_config(ide_pci->bus, ide_pci->dev, ide_pci->func, 0x14);
        ide_base = bar0 & ~0x3;
        ide_control = bar1 & ~0x3;
        uint8_t prog_if = pci_read_config(ide_pci->bus, ide_pci->dev, ide_pci->func, 0x09) & 0xFF;
        if (!(prog_if & 0x01)) { // Not native mode
            ide_base = IDE_PRIMARY_BASE;
            ide_control = IDE_PRIMARY_CONTROL;
        }
    }

    // Reset controller
    outb(ide_control + IDE_REG_CONTROL, 0x04);
    ide_wait_ready();
    outb(ide_control + IDE_REG_CONTROL, 0x00);

    // Identify drive
    uint8_t identify_data[512];
    if (ide_identify(identify_data)) {
        return;
    }

    // IDE IRQ14 is handled by vector 46 in isr_handler
    // Unmask IRQ14
    uint8_t mask = inb(0xA1);
    outb(0xA1, mask & ~(1 << (14 - 8))); // Unmask IRQ14

    // Initialize block device
    ide_dev.read_sectors = block_read;
    ide_dev.write_sectors = block_write;
}

block_dev_t *ide_get_block_dev(void) {
    return &ide_dev;
}