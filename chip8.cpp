#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "chip8.hpp"

#define fetch(addr) ((_RAM[addr] << 8) | (_RAM[addr + 1]))
#define X           ((_regs.opcode & 0x0F00) >> 8)
#define Y           ((_regs.opcode & 0x00F0) >> 4)
#define N           (_regs.opcode & 0x000F)
#define NN          (_regs.opcode & 0x00FF)
#define NNN         (_regs.opcode & 0x0FFF)
#define Vx          (_regs.V[(_regs.opcode & 0x0F00) >> 8])
#define Vy          (_regs.V[(_regs.opcode & 0x00F0) >> 4])
#define Vf          (_regs.V[0xF])

unsigned char fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static const unsigned char chip8_font8x10[160] =
{
    0x00, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00, // 0
    0x00, 0x08, 0x38, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00, // 1
    0x00, 0x38, 0x44, 0x04, 0x08, 0x10, 0x20, 0x44, 0x7C, 0x00, // 2
    0x00, 0x38, 0x44, 0x04, 0x18, 0x04, 0x04, 0x44, 0x38, 0x00, // 3
    0x00, 0x0C, 0x14, 0x24, 0x24, 0x7E, 0x04, 0x04, 0x0E, 0x00, // 4
    0x00, 0x3E, 0x20, 0x20, 0x3C, 0x02, 0x02, 0x42, 0x3C, 0x00, // 5
    0x00, 0x0E, 0x10, 0x20, 0x3C, 0x22, 0x22, 0x22, 0x1C, 0x00, // 6
    0x00, 0x7E, 0x42, 0x02, 0x04, 0x04, 0x08, 0x08, 0x08, 0x00, // 7
    0x00, 0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00, // 8
    0x00, 0x3C, 0x42, 0x42, 0x42, 0x3E, 0x02, 0x04, 0x78, 0x00, // 9
    0x00, 0x18, 0x08, 0x14, 0x14, 0x14, 0x1C, 0x22, 0x77, 0x00, // A
    0x00, 0x7C, 0x22, 0x22, 0x3C, 0x22, 0x22, 0x22, 0x7C, 0x00, // B
    0x00, 0x1E, 0x22, 0x40, 0x40, 0x40, 0x40, 0x22, 0x1C, 0x00, // C
    0x00, 0x78, 0x24, 0x22, 0x22, 0x22, 0x22, 0x24, 0x78, 0x00, // D 
    0x00, 0x7E, 0x22, 0x28, 0x38, 0x28, 0x20, 0x22, 0x7E, 0x00, // E
    0x00, 0x7E, 0x22, 0x28, 0x38, 0x28, 0x20, 0x20, 0x70, 0x00  // F
};

Chip8::Chip8()
{
}

Chip8::~Chip8()
{
    delete[] _RAM;
    delete[] _VRAM;
    delete[] _stack;
}

unsigned char* Chip8::GetRAM()
{
    return _RAM;
}

unsigned char* Chip8::GetVRAM()
{
    return _VRAM;
}

unsigned char Chip8::GetDelayTimer()
{
    return _regs.delayTimer;
}

unsigned char Chip8::GetSoundTimer()
{
    return _regs.soundTimer;
}

unsigned Chip8::GetFlags()
{
    return _flags;
}

void Chip8::SetFlags(const unsigned flags)
{
    _flags = flags;
}

bool Chip8::GetFlag(const unsigned flag)
{
    return _flags & flag;
}

void Chip8::SetFlag(const unsigned flag)
{
    _flags |= flag;
}

void Chip8::ResetFlag(const unsigned flag)
{
    _flags &= ~flag;
}

void Chip8::ToggleFlag(const unsigned flag)
{
    _flags = (_flags & flag) ? _flags & ~flag : _flags | flag;
}

void Chip8::SetKeys(const char keyState[16])
{
    memcpy(_keyState, keyState, 16);
}

void Chip8::TickDelayTimer()
{
    _regs.delayTimer -= (_regs.delayTimer > 0);
}

void Chip8::TickSoundTimer()
{
    if (_regs.soundTimer > 0) {
        if (_regs.soundTimer == 1) {
//            PlayBeep();
        }
        _regs.soundTimer--;
    }
}

//void Chip8::PlayBeep()
//{
//
//}

void Chip8::Step()
{
    _regs.opcode = fetch(_regs.pc);
    ExecuteOpcode(_regs.opcode);
    printf("Stepped to opcode %04X at address %04X\n", _regs.opcode, _regs.pc);
}

/*|=======================================================|
  |  Initialize()                                         |
  |                                                       |
  |  Initialize the state of the emulator. Points program |
  |  counter to rom entry point 0x200 and sets the        |
  |  initial value of the flags bitmask; all other        |
  |  registers, timers and arrays are cleared.            |
  |-------------------------------------------------------|*/
bool Chip8::Initialize()
{
    _regs.pc         = 0x0200;
    _regs.sp         = 0x0;
    _regs.opcode     = 0x0000;
    _regs.I          = 0x0000;
    _regs.delayTimer = 0;
    _regs.soundTimer = 0;

    memset(_RAM, 0, 4096);
    memset(_VRAM, 0, 2048);
    memset(_regs.V, 0, 16);
    memset(_stack, 0, 32);
    memset(_keyState, 0, 16);
    memcpy(_RAM, fontset, 80);

    _flags = CPU_FLAG_DETECTCOLLISION | CPU_FLAG_HWRAP | CPU_FLAG_VWRAP;

    srand(time(NULL));
    
    return true;
}

/*|=======================================================|
  |  Reset()                                              |
  |                                                       |
  |  Reset the state of the emulator. This method is the  |
  |  same as Init(), except a reset does not clear RAM,   |
  |  so a game can be restarted without loading the rom   |
  |  again.                                               |
  |-------------------------------------------------------|*/
bool Chip8::Reset()
{
    _regs.pc         = 0x0200;
    _regs.sp         = 0x0;
    _regs.opcode     = 0x0000;
    _regs.I          = 0x0000;
    _regs.delayTimer = 0;
    _regs.soundTimer = 0;

    memset(_VRAM, 0, 2048);
    memset(_regs.V, 0, 16);
    memset(_stack, 0, 32);
    memset(_keyState, 0, 16);

    _flags = CPU_FLAG_DETECTCOLLISION | CPU_FLAG_HWRAP | CPU_FLAG_VWRAP;

    return true;
}

/*|=======================================================|
  |  LoadRom(const char *filename)                        |
  |                                                       |
  |  Load a Chip8/SCHIP rom at 0x200. Chip8 supports      |
  |  4096 bytes of memory, So a rom can be no bigger      |
  |  than 3584 bytes (0xFFF - 0x200).                     |
  |-------------------------------------------------------|*/
bool Chip8::LoadRom(const char *filename)
{
    const int MAX_ROM_SIZE    = 3584;
    const int ROM_LOAD_OFFSET = 0x200;

    if (!filename) {
        return false;
    }

    FILE* pFile = fopen(filename, "rb");
    if (!pFile) {
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    int romSize = ftell(pFile);
    rewind(pFile);

    if (romSize > MAX_ROM_SIZE) {
        fclose(pFile);
        return false;
    }

    int bytesRead = fread(_RAM + ROM_LOAD_OFFSET, 1, romSize, pFile);
    fclose(pFile);
    if (bytesRead != romSize) {
        return false;
    }

    std::cout << "Loaded " << romSize << "KB ROM '" << filename << "'\n";
    return true;

    // std::ifstream rom;
    // rom.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    // int length = 0;

    // try {
    //     // Open rom
    //     rom.open(filename, std::ifstream::binary);

    //     rom.seekg(0, rom.end);
    //     length = rom.tellg();
    //     rom.seekg(0, rom.beg);

    //     // Read rom into cartridge memory
    //     rom.read(reinterpret_cast<char*>(_RAM), length);

    //    // _RAM((std::istreambuf_iterator<byte>(rom)),
    //    //              std::istreambuf_iterator<byte>());

    // } catch (std::ifstream::failure e) {
    //     std::cerr << "Exception opening/reading rom "
    //               << filename << std::endl;
    //     return false;
    // }

    //std::cout << "Loaded " << length << "KB ROM '" << filename << "'\n";

    // rom.close();
    // return true;
}

bool Chip8::UnloadRom()
{
    return true;
}

/*|=======================================================|
  |  SaveState()                                          |
  |                                                       |
  |  Save the current state of the emulator, storing all  |
  |  registers and memory into a save state.              |                                           
  |-------------------------------------------------------|*/
void Chip8::SaveState()
{
    _saveState._regs = _regs;
    memcpy(_saveState._RAM, _RAM, 4096);
    memcpy(_saveState._VRAM, _VRAM, 2048);
    memcpy(_saveState._stack, _stack, 32);
}

/*|=======================================================|
  |  LoadState()                                          |
  |                                                       |
  |  Load the save state of the emulator, storing all     |
  |  registers and memory into the current state.         |
  |-------------------------------------------------------|*/
void Chip8::LoadState()
{
    _regs = _saveState._regs;
    memcpy(_RAM, _saveState._RAM, 4096);
    memcpy(_VRAM, _saveState._VRAM, 2048);
    memcpy(_stack, _saveState._stack, 32);
}

void Chip8::DumpRegisters()
{
    std::cout << "Dump:" << std::endl;
    printf("PC = %04X    ", _regs.pc);
    printf("SP = %04X    ", _regs.sp);
    printf("OP = %04X    ", _regs.opcode);
    printf("I  = %04X    ", _regs.I);
    printf("DT = %04X    ", _regs.delayTimer);
    printf("ST = %04X\n",   _regs.soundTimer);
    printf("V0 = %04X    ", _regs.V[0x0]);
    printf("V1 = %04X    ", _regs.V[0x1]);
    printf("V2 = %04X    ", _regs.V[0x2]);
    printf("V3 = %04X\n",   _regs.V[0x3]);
    printf("V4 = %04X    ", _regs.V[0x4]);
    printf("V5 = %04X    ", _regs.V[0x5]);
    printf("V6 = %04X    ", _regs.V[0x6]);
    printf("V7 = %04X\n",   _regs.V[0x7]);
    printf("V8 = %04X    ", _regs.V[0x8]);
    printf("V9 = %04X    ", _regs.V[0x9]);
    printf("VA = %04X    ", _regs.V[0xA]);
    printf("VB = %04X\n",   _regs.V[0xB]);
    printf("VC = %04X    ", _regs.V[0xC]);
    printf("VD = %04X    ", _regs.V[0xD]);
    printf("VE = %04X    ", _regs.V[0xE]);
    printf("VF = %04X\n",   _regs.V[0xF]);
    printf("\n");

    // std::cout << std::hex;

    // std::cout << "PC = " << _regs.pc         << "    ";
    // std::cout << "SP = " << _regs.sp         << "    ";
    // std::cout << "OP = " << _regs.opcode     << "    ";
    // std::cout << "I  = " << _regs.I          << "    ";
    // std::cout << "DT = " << _regs.delayTimer << "    ";
    // std::cout << "ST = " << _regs.soundTimer << std::endl;

    // std::cout << "V0 = " << _regs.V[0x0] << "    ";
    // std::cout << "V1 = " << _regs.V[0x1] << "    ";
    // std::cout << "V2 = " << _regs.V[0x2] << "    ";
    // std::cout << "V3 = " << _regs.V[0x3] << std::endl;

    // std::cout << "V4 = " << _regs.V[0x4] << "    ";
    // std::cout << "V5 = " << _regs.V[0x5] << "    ";
    // std::cout << "V6 = " << _regs.V[0x6] << "    ";
    // std::cout << "V7 = " << _regs.V[0x7] << std::endl;

    // std::cout << "V8 = " << _regs.V[0x8] << "    ";
    // std::cout << "V9 = " << _regs.V[0x9] << "    ";
    // std::cout << "VA = " << _regs.V[0xA] << "    ";
    // std::cout << "VB = " << _regs.V[0xB] << std::endl;

    // std::cout << "VC = " << _regs.V[0xC] << "    ";
    // std::cout << "VD = " << _regs.V[0xD] << "    ";
    // std::cout << "VE = " << _regs.V[0xE] << "    ";
    // std::cout << "VF = " << _regs.V[0xF] << std::endl;

    // std::cout << std::dec;
}

void Chip8::ExecuteOpcode(const unsigned short opcode)
{
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (_regs.opcode & 0x00F0) {
                /*|=======================================================|
                  |  00CN: Scroll screen N lines down.              SCHIP |
                  |                                                       |
                  |  Scroll the display N lines down. Some SCHIP games    |
                  |  use this instruction for vertical scrolling.         |
                  |-------------------------------------------------------|*/
                case 0x00C0:
                {
                    for (int y = 0; y < (64 - N); y++) {
                        for (int x = 0; x < 128; x++) {
                            _VRAM[((y + N) * 64) + x] = _RAM[(y * 64) + x];
                            _VRAM[(y * 64) + x] = 0;
                        }
                    }

                    _regs.pc += 2;
                 }
                 break;
            }

            switch (_regs.opcode & 0x00FF) {
                case 0x00EE:
                {
                    if (!_regs.sp) {
                        #ifdef DEBUG
                        std::cerr << std::hex << pc
                                  << ": Stack empty, cannot return from "
                                  << "subroutine!\n" << std::dec;
                        #endif // DEBUG
                        Reset();
                     } else {
                         _regs.pc = _stack[--_regs.sp];
                         _stack[_regs.sp] = 0;
                     }
                 }
                 break;

                /*|=======================================================|
                  |  00E0: Clear screen.                                  |
                  |                                                       |
                  |  Reset 2048 bytes of video memory, clears the screen. |
                  |-------------------------------------------------------|*/
                 case 0x00E0:
                 {
                     memset(_VRAM, 0, 8196);
                     _regs.pc += 2;
                 }
                 break;

                /*|=======================================================|
                  |  00FB: Scroll screen 4 pixels right.            SCHIP |
                  |                                                       |
                  |  Scroll the display 4 pixels to the right.            |
                  |  This instruction performs horizontal scrolling.      |
                  |-------------------------------------------------------|*/
                 case 0x00FB:
                 {
                     for (int y = 0; y < 64; y++) {
                         for (int x = 0; x < (128 - 4); x++) {
                             _VRAM[(y * 64) + x + 4] = _RAM[(y * 64) + x];
                             _VRAM[(y * 64) + x] = 0;
                         }
                     }

                    _regs.pc += 2;
                 }
                 break;

                /*|=======================================================|
                  |  00FC: Scroll screen 4 pixels left.             SCHIP |
                  |                                                       |
                  |  Scroll the display 4 pixels to the left.             |
                  | This instruction performs horizontal scrolling.       |
                  |-------------------------------------------------------|*/
                 case 0x00FC:
                 {
                     for (int y = 0; y < 64; y++) {
                         for (int x = 4; x < 128; x++) {
                             _VRAM[(y * 64) + x - 4] = _RAM[(y * 64) + x];
                             _VRAM[(y * 64) + x] = 0;
                         }
                     }

                    _regs.pc += 2;
                 }
                 break;

                /*|=======================================================|
                  |  00FD: Exit the interpreter.                    SCHIP |
                  |                                                       |
                  |  Exit the SCHIP interpreter, stopping execution.      |
                  |-------------------------------------------------------|*/
                 case 0x00FD:
                 {
                     Initialize();
                 }
                 break;

                /*|=======================================================|
                  |  00FE: Disable extended screen mode.            SCHIP |
                  |                                                       |
                  |  Disable extended screen mode used by SCHIP games     |
                  |  (128x64) and revert to original Chip8 resolution     | 
                  |  (64x32).                                             |
                  |-------------------------------------------------------|*/
                 case 0x00FE:
                 {
                     _flags &= ~CPU_FLAG_SCHIP;
                     _regs.pc += 2;
                 }
                 break;

                /*|=======================================================|
                  |  00FF: Enable extended screen mode              SCHIP |
                  |        for fullscreen.                                |
                  |                                                       |
                  |  Enable extended screen mode (128x64) for             |
                  |  fullscreen.                                          |
                  |-------------------------------------------------------|*/
                 case 0x00FF:
                 {
                     _flags |= CPU_FLAG_SCHIP;

                     //memcpy(_RAM, chip8_font8x10, sizeof(chip8_font8x10));

                     _regs.pc += 2;
                 }
                 break;
             }
        break;

        /*|=======================================================|
          |  1NNN: Jump to address NNN                            |
          |                                                       |
          |  Program counter is set to address NNN.               |
          |-------------------------------------------------------|*/
        case 0x1000:
            if (NNN < 0x0200 || NNN > 0x0FFD) {
                #ifdef DEBUG
                printf("0x%04X: Illegal jump, resetting rom...\n", pc);
                #endif // DEBUG
                Reset();
            } else if (_flags & CPU_FLAG_DETECTGAMEOVER && _regs.pc == NNN) {
                #ifdef DEBUG
                printf("0x%04X: Game over detected, resetting rom...\n", pc);
                #endif // DEBUG
                Reset();
            } else if (NNN < 0x0200 || NNN > 0x0FFD) {
                #ifdef DEBUG
                printf("0x%04X: Illegal jump, resetting rom...\n", pc);
                #endif // DEBUG
                Reset();
            } else {
                _regs.pc = NNN;
            }
        break;

        /*|=======================================================|
          |  2NNN: Call subroutine.                               |
          |                                                       |
          |  The current value of the program counter + 2 is      |
          |  pushed on the stack, storing the address of the      |
          |  next instruction which will be executed when the     | 
          |  subroutine returns. The stack pointer is             |
          |  incremented, creating stack space for another call.  |
          |  The program counter is then set to NNN, which is     |
          |  the address of the subroutine.                       |
          |-------------------------------------------------------|*/
        case 0x2000:
        {
            if (NNN < 0x0200 || NNN > 0x0FFD) {
                #ifdef DEBUG
                std::cerr << std::hex << pc
                          << ": Illegal call, address out of memory range! "
                          << "Resetting emulator...\n" << std::dec;
                #endif // DEBUG
                Reset();
            }

            if (_regs.sp == 32) {
                #ifdef DEBUG
                std::cerr << std::hex << pc
                          << ": Stack full, cannot push memory address! "
                          << "Resetting emulator...\n" << std::dec;
                #endif // DEBUG
                Reset();
            }

            _stack[_regs.sp++] = _regs.pc + 2;
            _regs.pc = NNN;
        }
        break;

        /*|=======================================================|
          |  3XNN: Skip the next opcode if Vx equals NN.          |
          |                                                       |
          |  The next instruction is skipped if Vx and NN         |
          |  contain the same values. Otherwise, execute the      |
          |  instruction stored at the next 2 bytes.              |
          |-------------------------------------------------------|*/
        case 0x3000:
        {
            _regs.pc += (Vx == NN) ? 4 : 2;
        }
        break;

        /*|=======================================================|
          |  4XNN: Skip the next opcode if Vx does NOT equal NN.  |
          |                                                       |
          |  The next instruction is skipped if Vx and NN         |
          |  contain different values. Otherwise, execute the     |
          |  instruction stored at the next 2 bytes.              |
          |-------------------------------------------------------|*/
        case 0x4000:
        {
            _regs.pc += (Vx != NN) ? 4 : 2;
        }
        break;

        /*|=======================================================|
          |  5XNN: Skip the next opcode if Vx equsls Vy.          |
          |                                                       |
          |  The next instruction is skipped if Vx and Vy         |
          |  contain the same values. Otherwise, execute the      |
          |  instruction stored at the next 2 bytes.              |
          |-------------------------------------------------------|*/
        case 0x5000:
        {
            _regs.pc += (Vx == Vy) ? 4 : 2;
        }
        break;

        /*|=======================================================|
          |  6XNN: Assign NN to Vx.                               |
          |                                                       |
          |  Store the value NN in Vx.                            |
          |-------------------------------------------------------|*/
        case 0x6000:
        {
            Vx = NN;
            _regs.pc += 2;
        }
        break;

        /*|=======================================================|
          |  7XNN: Assign NN to Vx.                               |
          |                                                       |
          |  Add the value NN to Vx.                              |
          |-------------------------------------------------------|*/
        case 0x7000:
        {
            Vx += NN;
            _regs.pc += 2;
        }
        break;

        case 0x8000:
            switch (_regs.opcode & 0x000F)
            {
                /*|=======================================================|
                  |  8XY0: Assign Vy to Vx.                               |
                  |                                                       |
                  |  Store the value contained in Vy in Vx.               |
                  |-------------------------------------------------------|*/
                case 0x0000:
                {
                    Vx = Vy;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  8XY1: Vx logical OR Vy.                              |
                  |                                                       |
                  |  Logical OR Vx by Vy, storing the new value in Vx.    |
                  |-------------------------------------------------------|*/
                case 0x0001:
                {
                    Vx |= Vy;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  8XY2: Vx logical AND Vy.                             |
                  |                                                       |
                  |  Logical AND Vx by Vy, storing the new value in Vx.   |
                  |-------------------------------------------------------|*/
                case 0x0002:
                {
                    Vx &= Vy;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  8XY3: Vx logical XOR Vy.                             |
                  |                                                       |
                  |  Logical XOR Vx by Vy, storing the new value in Vx.   |
                  |-------------------------------------------------------|*/
                case 0x0003:
                {
                    Vx ^= Vy;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  8XY4: Add Vy to Vx.                                  |
                  |                                                       |
                  |  Incremented Vx by Vy. Set Vf if the operation        |
                  |  results in a carry i.e. the result is greater than   |
                  |  0xFF. Otherwise reset Vf.                            |
                  |-------------------------------------------------------|*/
                case 0x0004:
                {
                    Vf = (Vx + Vy) > 0xFF;
                    Vx += Vy;
                    //Vf = Vx > 0xFF;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  8XY5: Subtract Vy from Vx.                           |
                  |                                                       |
                  |  Subtract Vy from Vx. Set Vf if the operation         |
                  |  results in a borrow, i.e. the result is negative.    |
                  |  Otherwise reset Vf.                                  |
                  |-------------------------------------------------------|*/
                case 0x0005:
                {
                    Vf = Vx > Vy;
                    Vx -= Vy;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |   8XY6: Set Vf to LSB of Vx, then right shift Vx.     |
                  |                                                       |
                  |   Set Vf to the least significant bit of Vx, then     |
                  |   right shift Vx one digit.                           |
                  |-------------------------------------------------------|*/
                case 0x0006:
                {
                    Vf = Vx & 0x1;
                    Vx >>= 1;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |   8XY7: Set Vx to Vy minus Vx.                        |
                  |                                                       |
                  |   Subtract Vx from Vy, and store the result in Vx.    |
                  |   Set Vf if the operation results in a borrow, i.e.   |
                  |   the result is negative. Otherwise reset Vf.         |
                  |-------------------------------------------------------|*/
                case 0x0007:
                {
                    Vf = Vy > Vx;
                    Vx = Vy - Vx;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  8XYE: Set Vf to MSB of Vx, then left shift Vx.       |
                  |                                                       |
                  |  Set Vf to the most significant bit of Vx, then left  |
                  |  shift Vx one digit.                                  |
                  |-------------------------------------------------------|*/
                case 0x000E:
                {
                    Vf = Vx >> 7;
                    Vx <<= 1;
                    _regs.pc += 2;
                }
                break;
            }
        break;

        /*|=======================================================|
          |  9XY0: Skip the next opcode if Vx does NOT equal Vy   |
          |                                                       |
          |  The next instruction is skipped if Vx and Vy do NOT  |
          |  contain the same values. Otherwise, execute the      |
          |  instruction stored at the next 2 bytes.              |
          |-------------------------------------------------------|*/
        case 0x9000:
        {
            _regs.pc += (Vx != Vy) ? 4 : 2;
        }
        break;

        /*|=======================================================|
          |  ANNN: Load I with the value of NNN.                  |
          |                                                       |
          |  Load index register I with address NNN. The value    |
          |  of I is used by a few instructions, such as DXYN     |
          |  which draws sprites to video memory.                 |
          |-------------------------------------------------------|*/
        case 0xA000:
        {
            _regs.I = NNN;
            _regs.pc += 2;
        }
        break;

        /*|=======================================================|
          |  BNNN: Jump to address NNN + V0.                      |
          |                                                       |
          |  Load the program counter with the address formed     |
          |  by adding the value of V0 to NNN. NNN is a base      |
          |  address and V0 is added as an offset.                |
          |-------------------------------------------------------|*/
        case 0xB000:
        {
            if (NNN + _regs.V[0x0] < 0x0200 || NNN + _regs.V[0x0] > 0x0FFD) {
                #ifdef DEBUG
                std::cerr << std::hex << pc
                          << ": Illegal jump, address out of memory range! "
                          << "Resetting emulator...\n" << std::dec;
                #endif // DEBUG
                Reset();
            } else {
                _regs.pc = NNN + _regs.V[0x0];
            }
        }
        break;

        /*|=======================================================|
          |  CXNN: Set Vx to a random int (0-255) AND NN.         |
          |                                                       |
          |  Generate a random int between 0 and 255, logical     |
          |  AND this value with NN and store the result in Vx.   |
          |  Using rand() for RNG, seeded with value of           |
          |  RTC (time(NULL)).                                    |
          |-------------------------------------------------------|*/
        case 0xC000:
        {
            const int RAND_UPPER_BOUND = 255;
            Vx = (rand() % RAND_UPPER_BOUND) & NN;
            _regs.pc += 2;
        }
        break;

        /*|=======================================================|
          |  DXYN: Draw a sprite at coordinates (Vx,Vy); height N.|
          |                                                       |
          |  Draw a sprite to video memory. X and Y are the       |
          |  indexes of the registers containing the coordinates  |
          |  of the sprite, and N specifies the height of the     |
          |  sprite which is 16 pixels max.                       |
          |                                                       |
          |  Pixels are plotted by XOR'ing each byte of video     |
          |  memory. If the XOR operation results in a pixel      |
          |  being reset, that means the pixel was already set,   |
          |  which is detected as a collision. Collisions are     |
          |  signalled by setting Vf.                             |
          |                                                       |
          |  If horizontal/vertical sprite wrapping are enabled,  |
          |  a modulo operation is used to wrap pixel coordinates.|
          |  Otherwise, pixels drawn outside the dimensions of    |
          |  the video memory (64x32) will not be visible on      |
          |  the screen.                                          |
          |-------------------------------------------------------|*/
        case 0xD000:
        {
            // const int PIXELS_X = 64;
            // const int PIXELS_Y = 32;
            const int DEFAULT_HEIGHT = 16;
            
            byte x      = Vx;
            byte y      = Vy;
            byte height = N;
            byte row    = 0;

            if (height == 0) {
                height = DEFAULT_HEIGHT;
            }

            // Set carry flag to 0
            Vf = 0;

            if (!(_flags & CPU_FLAG_SCHIP)) {
                for (int yline = 0; yline < height; yline++) {
                    // Sprite stored at address I
                    row = _RAM[_regs.I + yline];
                    for (int xline = 0; xline < 8; xline++) {
                        // Shift pixels to draw row
                        if ((row & (0x80 >> xline)) != 0) {
                            // If collision detection flag is set, 
                            // check for collision
                            if (_flags & CPU_FLAG_DETECTCOLLISION) {
                                if (_VRAM[((yline + y) * 64) + xline + x] == 1) {
                                    Vf = 1;
                                }
                            }

                            // XOR this byte to set/reset pixel
                            _VRAM[((yline + y) * 64) + (xline + x)] ^= 1;
                        }
                    }
                }
            }

            else {
                for (int yline = 0; yline < height; yline++) {
                    // Sprite stored at address I
                    row = _RAM[_regs.I + yline];
                    for (int xline = 0; xline < 8; xline++) {
                        // Shift pixels to draw row
                        if ((row & (0x80 >> xline)) != 0) {
                            // If collision detection flag is set, 
                            // check for collision
                            if (_flags & CPU_FLAG_DETECTCOLLISION) {
                                if (_VRAM[((yline + y) * 64) + xline + x] == 1) {
                                    Vf = 1;
                                }
                            }

                            // XOR this byte to set/reset pixel
                            _VRAM[((yline + y) * 128) + (xline + x)] ^= 1;
                        }
                    }
                }
            }

            // All pixels plotted, now update screen
            _flags |= CPU_FLAG_DRAW;
            _regs.pc += 2;
        }
        break;

        case 0xE000:
            switch (_regs.opcode & 0x00FF)
            {
                /*|=======================================================|
                  |  EX9E: Skip the next instruction if key in Vx is      |
                  |        pressed.                                       |
                  |                                                       |
                  |  The next instruction is skipped if the key specified |
                  |  by Vx is pressed. Otherwise, execute the instruction |
                  |  stored at the next 2 bytes.                          |
                  |-------------------------------------------------------|*/
                case 0x009E:
                {
                    _regs.pc += (_keyState[Vx]) ? 4 : 2;
                }
                break;

                /*|=======================================================|
                  |  EX9E: Skip the next instruction if key in Vx is NOT  |
                  |        pressed.                                       |
                  |                                                       |
                  |  The next instruction is skipped if the key specified |
                  |  y Vx is NOT pressed. Otherwise, execute the          |
                  |  instruction stored at the next 2 bytes.              |
                  |-------------------------------------------------------|*/
                case 0x00A1:
                {
                    _regs.pc += !(_keyState[Vx]) ? 4 : 2;
                }
                break;
            }
        break;

        case 0xF000:
            switch (_regs.opcode & 0x00FF)
            {
                /*|=======================================================|
                  |  FX07: Set Vx to the value of the delay timer.        |
                  |                                                       |
                  |  Set Vx to the value of the delay timer.              |
                  |-------------------------------------------------------|*/
                case 0x0007:
                {
                    Vx = _regs.delayTimer;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX0A: Pause execution until key is pressed, store    |
                  |        in Vx.                                         |
                  |                                                       |
                  |  Execution is paused at this instruction until a key  |
                  |  is pressed. Wait until the key is pressed AND        |
                  |  released; otherwise Connect4 and Guess are nearly    |
                  |  unplayable because key input will be way too fast.   |
                  |  Store pressed key in Vx.                             |
                  |-------------------------------------------------------|*/
                case 0x000A:
                {
                    bool keyPress = false;

                    for (int i = 0; i < 16; i++) {
                        if (_keyState[i] == 1) {
                            Vx = i;
                            keyPress = true;
                            break;
                        }
                    }

                    if (!keyPress) {
                        return;
                    }

                    _regs.pc += 2;

//                        // Check if key is down
//                        if (_flags & ~CPU_FLAG_KEYDOWN)
//                        {
//                            for (int i = 0; i < 16; i++)
//                            {
//                                if (_keyState[i] == 1)
//                                {
//                                    Vx = i;
//                                    _flags |= CPU_FLAG_KEYDOWN;
//                                    return;
//                                }
//                            }
//                            return;
//                        }
//
//                        // Check if key is up
//                        else if (_flags & CPU_FLAG_KEYDOWN)
//                        {
//                            if (_keyState[Vx] == 0)
//                            {
//                                _flags & ~CPU_FLAG_KEYDOWN;
//                                _regs.pc += 2;
//                            }
//                        }
                }
                break;

                /*|=======================================================|
                  |  FX15: Set the delay timer to the value of Vx.        |
                  |                                                       |
                  |  Set the delay timer to the value of Vx.              |
                  |-------------------------------------------------------|*/
                case 0x0015:
                {
                    _regs.delayTimer = Vx;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX18: Set the sound timer to the value of Vx.        |
                  |                                                       |
                  |  Set the sound timer to the value of Vx.              |
                  |-------------------------------------------------------|*/
                case 0x0018:
                {
                    _regs.soundTimer = Vx;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX1E: Add Vx to I.                                   |
                  |                                                       |
                  |  Add the value of Vx to index register I. Vx is an    |
                  |  offset added to the I, which is a base address.      |
                  |  Set Vf if the operation results in a carry, i.e. the |
                  |  result is greater than 0xFF. Otherwise reset Vf.     |
                  |-------------------------------------------------------|*/
                case 0x001E:
                {
                    Vf = (_regs.I + Vx) > 0x0FFF;
                    _regs.I += Vx;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX29: Load I with Vx, which contains address of      |
                  |        font sprite.                                   |
                  |                                                       |
                  |  Load I with the value of Vx * 5, which is the        |
                  |  address of a character from the font set. Multiply   |
                  |  the value of Vx by 5, because each character is 5    |
                  |  bytes (where each byte specifies one row of          |
                  |  the sprite).                                         |
                  |-------------------------------------------------------|*/
                case 0x0029:
                {
                    if ((Vx * 0x5) < 0x0200 || (Vx * 0x5) > 0x0FFB) {
                        #ifdef DEBUG
                        std::cerr << std::hex << pc
                                  << ": Illegal opcode, " << Vx * 0x5 << 
                                  << " outside of memory range! "
                                  << "Resetting emulator...\n" << std::dec;
                        #endif // DEBUG
                        //_reset();
                    }

//                        else
//                        {
                        _regs.I = Vx * 5;
                        _regs.pc += 2;
//                        }
                }
                break;

                /*|=======================================================|
                  |  FX30: Load I with address of 10-byte font sprite to  |
                  |        draw digit in Vx.  SCHIP                       |
                  |                                                       |
                  |  Load I with the address of the 10-byte sprite from   |
                  |  the SCHIP font set to draw the character sprite      |
                  |  indexed by Vx.                                       |
                  |-------------------------------------------------------|*/
                case 0x0030:
                {
                    if ((Vx * 0x5) < 0x0200 || (Vx * 0x5) > 0x0FF6) {
                        #ifdef DEBUG
                            std::cerr << std::hex << pc
                                  << ": Illegal opcode, " << Vx * 0x5 << 
                                  << " outside of memory range! "
                                  << "Resetting emulator...\n" << std::dec;
                        #endif // DEBUG
                        //_reset();
                    }

//                        else
//                        {
                        _regs.I = Vx * 10;
                        _regs.pc += 2;
//                        }
                }
                break;

                /*|=======================================================|
                  |  FX33: Load I with Vx (BCD).                          |
                  |                                                       |
                  |  Load I with the value of Vx in binary-coded decimal  |
                  |  representation.                                      |
                  |-------------------------------------------------------|*/
                case 0x0033:
                {
                    _RAM[_regs.I] = Vx / 100;
                    _RAM[_regs.I + 1] = (Vx / 10) % 10;
                    _RAM[_regs.I + 2] = (Vx % 100) % 10;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX55: Store V0 to Vx in memory from address I.       |
                  |                                                       |
                  |  Iterate over a number of primary registers, storing  |
                  |  each value in a corresponding byte of memory from    |
                  |  base address I.                                      | 
                  |                                                       |
                  |  There is an undocumented operation of this           |
                  |  instruction which loads I with X + 1. However, this  |
                  |  breaks Connect4 as the discs appear to be drawn at   |
                  |  random locations instead of in straight lines.       |
                  |  This can be toggled on/off in case a game relies     |
                  |  on it to run properly.                               |
                  |-------------------------------------------------------|*/
                case 0x0055:
                {
                    if (_regs.I < 0x0200 || _regs.I > (0x0FFF - X)) {
                        #ifdef DEBUG
                        std::cerr << std::hex << pc
                                  << ": Illegal opcode, " << Vx * 0x5 << 
                                  << " outside of memory range! "
                                  << "Resetting emulator...\n" << std::dec;
                        #endif // DEBUG
                        Reset();
                    } else {
                        for (int i = 0; i <= X; i++) {
                            _RAM[_regs.I + i] = _regs.V[i];
                        }
                    }

                    //I += X + 1;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX65: Store values from memory from address I into   |
                  |        V0 to Vx.                                      |
                  |                                                       |
                  |  Iterate over a number of contiguous bytes from       |
                  |  memory, storing each byte in a corresponding primary |
                  |  register.                                            |
                  |                                                       |
                  |  There is an undocumented operation of this           |
                  |  instruction which loads I with X + 1. However, this  |
                  |  breaks Connect4 as the discs appear to be drawn at   |
                  |  random locations instead of in straight lines.       |
                  |  This can be toggled on/off in case a game relies     |
                  |  on it to run properly.                               |
                  |-------------------------------------------------------|*/
                case 0x0065:
                {
                    if (_regs.I > (0x0FFF - X)) {
                        #ifdef DEBUG
                        std::cerr << std::hex << pc
                                  << ": Illegal ld, address " << _regs.pc << 
                                  << " exceeds memory range! "
                                  << "Resetting emulator...\n" << std::dec;
                        #endif // DEBUG
                        Reset();
                    } else {
                        for (int i = 0; i <= X; i++) {
                            _regs.V[i] = _RAM[_regs.I + i];
                        }
                    }

                    //I += X + 1;
                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX75: Store V0-Vx in R0-Rx.                    SCHIP |
                  |                                                       |
                  |  Store the values contained in registers V0 to Vx     |
                  |  in the RPL user flags.                               |
                  |  Flags R0 to R7 are only used by SCHIP games.         |
                  |-------------------------------------------------------|*/
                case 0x0075:
                {
                    #ifdef DEBUG
                    if (X > 7) {
                        std::cerr << std::hex << pc
                                  << ": Illegal store, cannot" << Vx * 0x5 << 
                                  << " access more than 8 RPL flags! "
                                  << "Resetting emulator...\n" << std::dec;
                        resetCPU();
                    }
                    #endif

                    for (int i = 0; i <= X; i++) {
                        _regs.R[i] = _regs.V[i];
                    }

                    _regs.pc += 2;
                }
                break;

                /*|=======================================================|
                  |  FX85: Store R0-Rx in V0-Vx                     SCHIP |
                  |                                                       |
                  |  Store the values contained in RPL user flags R0-Rx   |
                  |  in registers V0-Vx.                                  |
                  |  Flags R0 to R7 are only used by SCHIP games.         |
                  |-------------------------------------------------------|*/
                case 0x0085:
                {
                    #ifdef DEBUG
                    if (X > 7) {
                        std::cerr << std::hex << pc
                                  << ": Illegal store, cannot" << Vx * 0x5 << 
                                  << " access more than 8 RPL flags! "
                                  << "Resetting emulator...\n" << std::dec;
                        resetCPU();
                    }
                    #endif

                    for (int i = 0; i <= X; i++) {
                        _regs.V[i] = _regs.R[i];
                    }

                    _regs.pc += 2;
                }
                break;
            }
        break;

        default:
            #ifdef DEBUG
            std::cerr << std::hex << pc
                      << ": Unknown opcode" << _regs.opcode << "\n";
                      << std::dec;
            #endif // DEBUG
            getchar();
        break;
    }
}

/*|=======================================================|
  |  EmulateCycles(const unsigned nCycles)            |
  |                                                       |
  |  Execute the number of cycles given by the argument   |
  |  nCycles. Because all CHIP8 instructions take one     |
  |  cycle to execute, effectively this method executes   |
  |  nCycles instructions.                                |
  |-------------------------------------------------------|*/
void Chip8::EmulateCycles(const unsigned nCycles)
{
    if (nCycles == 0 || (_flags & CPU_FLAG_PAUSED)) {
        return;
    }

    for (int i = nCycles; i > 0; i--) {
        _regs.opcode = fetch(_regs.pc);
        ExecuteOpcode(_regs.opcode);
    }
}
