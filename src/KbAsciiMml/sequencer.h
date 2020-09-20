#pragma once

#include "fmwrap.h"
#include "musdata.h"
#include <stack>

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    // 適当すぎ
    class Sequencer
    {
    public:
        Sequencer(FM::OPN& o, MusicData* pmd);
        bool Init(int rate);
        void Mix(__int16* dest, int nsamples);
        int GetSamplesPerFrame()
        {
            return samplesPerFrame;
        }

    private:
        struct PartData
        {
            PartData();

            std::stack<CommandList::const_iterator> CallStack;
            std::stack<std::pair<CommandList::const_iterator, int>> LoopStack;
            CommandList::const_iterator CommandPtr;

            int NoteEndFrame;
            int KeyOnFrame;
            int KeyOffFrame;

            int Octave;
            int LastOctave;
            int Volume;
            int Tone;
            int LastTone;

            bool SSGEnvOn;
            int SoundNo;
            int DefaultNoteLength;
            int Detune;
            int GateTime;

            // いい加減適当
            int PLength;

            int ILength;
            int IDepth;
            int IDelay;

            int ULength;
            int UDepth;
            int UDelay;

            bool Tied;
            bool Playing;
            // 無限ループ検出
            bool InfiniteLooping;
        };

    private:
        void NextFrame();
        void NextFramePart(int ch);
        void KeyOnOff(int ch, bool on);
        boost::optional<CommandList::const_iterator> ProcessLoop(PartData& part, CommandList::const_iterator ptr);
        void ProcessCommand(int ch);
        void ProcessEffect(int ch);

        static const int* const FNumber;
        static const int FNumberBase[14];
        static const int SSGToneNum[10][12];

        PartData partData[6];

        FM::OPN& opn;
        FMWrap fmwrap;
        SSGWrap ssgwrap;
        MusicData& musicdata;

        int rate;
        int samplesPerFrame;
        int samplesLeft; // このフレーム(64分音符)でmixすべき残りのサンプル数
        int currentFrame;
        bool partPlaying[6];
    };

} // namespace MusicCom
