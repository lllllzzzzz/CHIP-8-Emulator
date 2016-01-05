#ifndef CHIP8_H_INCLUDED
#define CHIP8_H_INCLUDED

#define CHIP8_MEM_SIZE                  4096
#define CHIP8_VIDMEM_WIDTH              64
#define CHIP8_VIDMEM_HEIGHT             32
#define CHIP8_VIDMEM_SIZE               2048
#define SCHIP_VIDMEM_WIDTH              128
#define SCHIP_VIDMEM_HEIGHT             64
#define SCHIP_VIDMEM_SIZE               8192
#define CHIP8_STACK_SIZE                16
#define CHIP8_NUM_REGISTERS             16
#define SCHIP_NUM_RPLFLAGS              8
#define CHIP8_NUM_KEYS                  16
#define CHIP8_FONTSET_SIZE              80
#define CHIP8_ROM_ADDRESS               0x200
#define CHIP8_MAX_ROM_SIZE              3584

#define CPU_FLAG_DRAW                   0x1
#define CPU_FLAG_PAUSED                 0x2
#define CPU_FLAG_DETECTCOLLISION        0x8
#define CPU_FLAG_HWRAP                  0x10
#define CPU_FLAG_VWRAP                  0x20
#define CPU_FLAG_DETECTGAMEOVER         0x40
#define CPU_FLAG_KEYDOWN                0x80
#define CPU_FLAG_SCHIP                  0x100

class Chip8
{
    typedef unsigned char   byte;
    typedef unsigned short  word;

    typedef struct {
        word pc;                        // Program counter stores the address of next opcode
        byte sp;                        // Stack pointer points to top of stack
        word opcode;                    // Next opcode to execute, all opcodes are 2 bytes
        word I;                         // Index register used by some instructions

        byte delayTimer;                // Delay timer decrements at 60Hz
        byte soundTimer;                // Sound timer decrements at 60Hz, beep while non-zero

        byte V[CHIP8_NUM_REGISTERS];    // 16 8-bit registers
        byte R[SCHIP_NUM_RPLFLAGS];     // 8 8-bit SCHIP flags
    } regs;

    typedef struct {
        regs _regs;
        byte _RAM[CHIP8_MEM_SIZE];
        byte _VRAM[CHIP8_VIDMEM_SIZE];
        word _stack[CHIP8_STACK_SIZE];
    } saveState;

    public:
        Chip8();
        ~Chip8();

        byte* GetRAM            ();
        byte* GetVRAM           ();
        byte GetDelayTimer      ();
        byte GetSoundTimer      ();
        unsigned GetFlags       ();
        void SetFlags           (const unsigned flags);
        bool GetFlag            (const unsigned flag);
        void SetFlag            (const unsigned flag);
        void ResetFlag          (const unsigned flag);
        void ToggleFlag         (const unsigned flag);

        void Step               ();
        void DumpRegisters      ();
        void SetKeys            (const char keyState[16]);

        bool Initialize         ();
        bool Reset              ();
        bool LoadRom            (const char* filename);
        bool UnloadRom          ();
        void EmulateCycles      (const unsigned nCycles);
        void ExecuteOpcode      (const word opcode);
        void SaveState          ();
        void LoadState          ();
        void TickDelayTimer     ();
        void TickSoundTimer     ();

    private:
        regs _regs;
        unsigned _flags;
        saveState _saveState;

        //byte _RAM[4096];      // 4096 bytes memory
        //byte _VRAM[128*64];   // 2048 bytes video memory
        //word _stack[16];      // Stack supports 16 subroutine calls
        byte* _RAM = new byte[CHIP8_MEM_SIZE];
        byte* _VRAM = new byte[SCHIP_VIDMEM_SIZE];
        word* _stack = new word[CHIP8_STACK_SIZE];
        byte _keyState[CHIP8_NUM_KEYS];
};

#endif // CHIP8_H_INCLUDED
