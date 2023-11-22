typedef struct memory
{
    // 00000000-00003FFF BIOS - System ROM (16 KBytes)
    char bios[16384];

    // 02000000-0203FFFF WRAM - On-board Work RAM (256 KBytes) 2 Wait
    char wram[262144];

    // 03000000-03007FFF WRAM - On-chip Work RAM (32 KBytes)
    char wram_chip[32768];

    // 04000000-040003FE I/O Registers
    char io[1022];

    // 05000000-050003FF Palette RAM (1 Kbyte)
    char palette[1024];

    // 06000000-06017FFF VRAM (96 KBytes)
    char vram[98304];

    // 07000000-070003FF OAM (1 Kbyte)
    char oam[1024];

    // 08000000-09FFFFFF Game Pak ROM/FlashROM (max 32MB) - Wait State 0
    char rom[33554432];

    // 0A000000-0BFFFFFF Game Pak ROM/FlashROM (max 32MB) - Wait State 1
    char rom2[33554432];

    // 0C000000-0DFFFFFF Game Pak ROM/FlashROM (max 32MB) - Wait State 2
    char rom3[33554432];

    // 0E000000-0E00FFFF Game Pak SRAM (max 64 KBytes) - 8bit Bus width
    char sram[65536];

} memory_t;