#ifndef CHIP8_H_INCLUDED
#define CHIP8_H_INCLUDED

#define CPU_FLAG_DRAW                  0x1
#define CPU_FLAG_PAUSED                0x2
#define CPU_FLAG_DETECTCOLLISION       0x8
#define CPU_FLAG_HWRAP                 0x10
#define CPU_FLAG_VWRAP                 0x20
#define CPU_FLAG_DETECTGAMEOVER        0x40
#define CPU_FLAG_KEYDOWN               0x80
#define CPU_FLAG_SCHIP                 0x100

class Chip8
{
    typedef unsigned char   byte;
    typedef unsigned short  word;

    typedef struct {
        word pc;            // Program counter stores the address of next opcode
        byte sp;            // Stack pointer points to top of stack
        word opcode;        // Next opcode to execute, all opcodes are 2 bytes
        word I;             // Index register used by some instructions

        byte delayTimer;    // Delay timer decrements at 60Hz
        byte soundTimer;    // Sound timer decrements at 60Hz, beep while non-zero

        byte V[16];         // 16 8-bit registers
        byte R[8];          // 8 8-bit SCHIP flags
    } regs;

    typedef struct {
        regs _regs;
        byte _RAM[4096];
        byte _VRAM[64*32];
        word _stack[16];
    } saveState;

    public:
        Chip8();
        ~Chip8();

        byte* GetRAM            ();
        byte* GetVRAM           ();
        byte GetDelayTimer      ();
        byte GetSoundTimer      ();
        unsigned int GetFlags   ();
        void SetFlags           (const unsigned int flags);
        bool GetFlag            (const unsigned int flag);
        void SetFlag            (const unsigned int flag);
        void ResetFlag          (const unsigned int flag);
        void ToggleFlag         (const unsigned int flag);

        void Step               ();
        void DumpRegisters      ();
        void SetKeys            (const char keyState[16]);

        bool Initialize         ();
        bool Reset              ();
        bool LoadRom            (const char* filename);
        bool UnloadRom          ();
        void EmulateCycles      (const unsigned int nCycles);
        void ExecuteOpcode      (const word opcode);
        void SaveState          ();
        void LoadState          ();
        void TickDelayTimer     ();
        void TickSoundTimer     ();

    private:
        regs _regs;
        unsigned int _flags;
        saveState _saveState;

        //byte _RAM[4096];      // 4096 bytes memory
        //byte _VRAM[128*64];   // 2048 bytes video memory
        //word _stack[16];      // Stack supports 16 subroutine calls
        byte* _RAM = new byte[4096];
        byte* _VRAM = new byte[128 * 64];
        word* _stack = new word[16];

        byte _keyState[16];
};

#endif // CHIP8_H_INCLUDED
