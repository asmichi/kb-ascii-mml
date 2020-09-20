#include "pch.h"
#include "sequencer.h"
#include "musdata.h"
#include <fmgen/opna.h>

// KeyOff したあと、音量レベルが低下するまでMixしてからOnしないとタイになってしまう
// fmgen の問題？

using namespace std;
using namespace boost;

namespace MusicCom
{
    const int* const Sequencer::FNumber = &FNumberBase[1];

    // clang-format off
    const int Sequencer::FNumberBase[14] = {
	    //C-,  C,  C#,   D,  D#,   E,   F,  F#,   G,  G#,   A,  A#,   B,   B#
	    584, 618, 655, 694, 735, 779, 825, 874, 926, 981,1040,1101,1167, 1236
    };
    // clang-format on

    // clang-format off
    const int Sequencer::SSGToneNum[10][12] = {
        // 124800 / Hz
        //0  C,1 C#,2  D,3 D#,4  E,5  F,6 F#,7  G,8 G#,9  A,10A#,11 B
	    { 3816,3602,3400,3209,3029,2859,2698,2547,2404,2269,2142,4043},	// O0
	    { 3816,3602,3400,3209,3029,2859,2698,2547,2404,2269,2142,2022}, // O1
	    { 1908,1801,1700,1604,1514,1429,1349,1273,1202,1135,1071,1011},
	    {  954, 900, 850, 802, 757, 715, 675, 637, 601, 567, 535, 505},
	    {  477, 450, 425, 401, 379, 357, 337, 318, 301, 284, 268, 253},	// O4
	    {  239, 225, 212, 201, 189, 179, 169, 159, 150, 142, 134, 126},
	    {  119, 113, 106, 100,  95,  89,  84,  80,  75,  71,  67,  63},
	    {   60,  56,  53,  50,  47,  45,  42,  40,  38,  35,  33,  32},
	    {   30,  28,  27,  25,  24,  22,  21,  20,  19,  18,  17,  16},	// O8
	    {   15,  14,  13,  13,  12,  11,  11,  10,   9,   9,   8,   8},
    };
    // clang-format on

    Sequencer::Sequencer(FM::OPN& o, MusicData* pmd) : opn(o), fmwrap(o), ssgwrap(o), musicdata(*pmd)
    {
    }

    bool Sequencer::Init(int r)
    {
        const unsigned int OPN_CLOCKFREQ = 3993600; // OPNのクロック周波数

        rate = r;
        samplesPerFrame = (int)(rate * (60.0 / (musicdata.GetTempo() * 16.0)) / 1.1 + 0.5); // 1.1: music.comの演奏は速いので補正
        samplesLeft = 0;
        currentFrame = 0;

        if (!opn.Init(OPN_CLOCKFREQ, rate))
            return false;

        for (int ch = 0; ch < 6; ch++)
        {
            partData[ch].Playing = musicdata.IsChannelPresent(ch);
            if (partData[ch].Playing)
                partData[ch].CommandPtr = musicdata.GetChannelHead(ch);
        }

        // 効果音モード on
        opn.SetReg(0x27, 0x40);

        return true;
    }

    void Sequencer::Mix(__int16* dest, int nsamples)
    {
        memset(dest, 0, nsamples * sizeof(__int16) * 2);
        while (nsamples > 0)
        {
            if (samplesLeft < nsamples)
            {
                opn.Mix(dest, samplesLeft);
                dest += samplesLeft * 2;
                nsamples -= samplesLeft;
                NextFrame();
                samplesLeft = samplesPerFrame;
            }
            else
            {
                opn.Mix(dest, nsamples);
                samplesLeft -= nsamples;
                nsamples = 0;
            }
        }
    }

    void Sequencer::NextFrame()
    {
        for (int ch = 0; ch < 6; ch++)
        {
            PartData& part = partData[ch];
            if (part.Playing)
                NextFramePart(ch);
        }

        currentFrame++;
    }

    boost::optional<CommandList::const_iterator> Sequencer::ProcessLoop(PartData& part, CommandList::const_iterator ptr)
    {
        while (1)
        {
            const Command& command = *ptr;

            switch (command.GetType())
            {
            case '$':
                if (!musicdata.IsMacroPresent(command.GetStrArg()))
                {
                    ptr++;
                    continue;
                }
                part.CallStack.push(++CommandList::const_iterator(ptr));
                ptr = musicdata.GetMacroHead(command.GetStrArg());
                continue;
            case Command::TYPE_RET:
                if (part.CallStack.empty())
                {
                    part.Playing = false;
                    return optional<CommandList::const_iterator>();
                }
                ptr = part.CallStack.top();
                part.CallStack.pop();
                continue;
            case '{':
                part.LoopStack.push(pair<CommandList::const_iterator, int>(++CommandList::const_iterator(ptr), command.GetArg(0)));
                ptr++;
                continue;
            case '}':
            {
                if (part.LoopStack.empty())
                {
                    part.Playing = false;
                    return optional<CommandList::const_iterator>();
                }
                pair<CommandList::const_iterator, int>& p = part.LoopStack.top();
                if (p.second != 0)
                {
                    p.second--;
                    // ループ脱出
                    if (p.second == 0)
                    {
                        part.LoopStack.pop();
                        ptr++;
                    }
                    else
                    {
                        ptr = p.first;
                    }
                }
                // 無限ループ
                else
                {
                    // 音符の入っていない無限ループ検出
                    if (part.InfiniteLooping)
                    {
                        part.Playing = false;
                        return optional<CommandList::const_iterator>();
                    }
                    part.InfiniteLooping = true;
                    ptr = p.first;
                }
            }
                continue;
            default:
                return ptr;
            }
        }
    }

    // もう適当
    void Sequencer::NextFramePart(int ch)
    {
        bool fm = ch < 3;
        int fmch = ch;
        int ssgch = ch - 3;

        PartData& part = partData[ch];
        if (!part.Playing)
            return;

        if (part.NoteEndFrame <= currentFrame && !part.Tied)
        {
            part.LastOctave = part.Octave;
            part.LastTone = part.Tone;
            KeyOnOff(ch, false);
        }

        ProcessCommand(ch);
        ProcessEffect(ch);
    }

    void Sequencer::ProcessCommand(int ch)
    {
        bool fm = ch < 3;
        int fmch = ch;
        int ssgch = ch - 3;

        PartData& part = partData[ch];
        part.InfiniteLooping = false;
        // もうかなり適当
        CommandList::const_iterator initial_ptr = part.CommandPtr;
        CommandList::const_iterator ptr = initial_ptr;

        while (part.NoteEndFrame <= currentFrame)
        {
            // ptrを特別に更新するひとたち
            {
                optional<CommandList::const_iterator> ret = ProcessLoop(part, ptr);
                if (!ret)
                    return;
                ptr = *ret;
            }

            // もうとても適当
            const Command& command = *ptr++;

            // 音符はタイの処理を行うため特別処理
            switch (command.GetType())
            {
            case Command::TYPE_NOTE:
            {
                if (!part.Tied)
                {
                    part.KeyOnFrame = currentFrame;
                }

                // Tone
                if (fm)
                {
                    int tone = FNumber[command.GetArg(0)];
                    if (part.Detune != 0)
                    {
                        tone = (int)(tone * pow(2.0, part.Detune / (255.0 * 12.0)) + 0.5);
                    }
                    fmwrap.SetTone(fmch, part.Octave, tone);
                    part.Tone = tone;
                }
                else
                {
                    int tone = SSGToneNum[part.Octave][command.GetArg(0)];
                    if (part.Detune != 0)
                    {
                        int tone2 = (int)(tone * pow(0.5, part.Detune / (255.0 * 12.0)) + 0.5);
                        if (tone == tone2)
                        {
                            if (part.Detune < 0)
                                tone = tone + 1;
                            else
                                tone = tone - 1;
                        }
                        else
                            tone = tone2;
                    }
                    ssgwrap.SetTone(ssgch, tone);
                    part.Tone = tone;
                }
                KeyOnOff(ch, true);

                if (part.LastTone == 0)
                {
                    part.LastOctave = part.Octave;
                    part.LastTone = part.Tone;
                }

                int length = command.GetArg(1);
                if (length == 0)
                    length = part.DefaultNoteLength;
                part.NoteEndFrame = currentFrame + length;

                // '&' がついているか
                {
                    optional<CommandList::const_iterator> ret = ProcessLoop(part, ptr);
                    if (!ret)
                        return;
                    ptr = *ret;
                }

                if (ptr->GetType() == '&')
                {
                    part.Tied = true;
                    part.KeyOffFrame = part.NoteEndFrame;
                }
                else
                {
                    part.Tied = false;
                    part.KeyOffFrame = currentFrame + max(length * part.GateTime / 8, 1);
                }
                break;
            }
            case 'R':
                KeyOnOff(ch, false);
            case 'W':
            {
                int length = command.GetArg(0);
                if (length == 0)
                    length = part.DefaultNoteLength;
                part.NoteEndFrame = currentFrame + length;
                part.Tied = false;
            }
            break;
            case 'L':
                part.DefaultNoteLength = command.GetArg(0);
                break;
            case 'O':
                part.Octave = min(max(command.GetArg(0), 1), 8);
                break;
            case '<':
                part.Octave = max(part.Octave - 1, 0);
                break;
            case '>':
                part.Octave = min(part.Octave + 1, 8);
                break;
            case 'V':
                part.Volume = min(max(command.GetArg(0), 0), 15);
                if (fm)
                {
                    fmwrap.SetVolume(fmch, part.Volume);
                }
                else
                {
                    ssgwrap.SetVolume(ssgch, part.Volume);
                }
                break;
            case '@':
                part.SoundNo = command.GetArg(0);
                if (fm)
                {
                    fmwrap.SetSound(fmch, musicdata.GetFMSound(part.SoundNo));
                }
                else
                {
                    part.SSGEnvOn = true;
                }
                break;
            case 'Q':
                part.GateTime = command.GetArg(0);
                break;
            case 'N':
                part.Detune = command.GetArg(0);
                break;
            case 'P':
                part.LastOctave = part.Octave;
                part.LastTone = part.Tone;
                part.PLength = max(command.GetArg(0), 0);

                part.ILength = part.ULength = 0;
                break;
            case 'U':
                part.UDepth = command.GetArg(0);
                part.ULength = max(command.GetArg(1), 0);
                part.UDelay = max(command.GetArg(2), 0);

                part.ILength = part.PLength = 0;
                break;
            case 'I':
                part.IDepth = command.GetArg(0);
                part.ILength = max(command.GetArg(1), 0);
                part.IDelay = max(command.GetArg(2), 0);

                part.PLength = part.ULength = 0;
                break;
            case 'S':
                if (!fm)
                {
                    part.SSGEnvOn = false;
                    ssgwrap.SetEnv(ssgch, true);
                    ssgwrap.SetEnvForm(command.GetArg(0));
                }
                break;
            case 'M':
                if (!fm)
                {
                    part.SSGEnvOn = false;
                    ssgwrap.SetEnv(ssgch, true);
                    ssgwrap.SetEnvPeriod(command.GetArg(0));
                }
                break;
            case 'Y':
                opn.SetReg(command.GetArg(0), command.GetArg(1));
                break;
            }
        }

        part.CommandPtr = ptr;
    }

    void Sequencer::ProcessEffect(int ch)
    {
        PartData& part = partData[ch];

        bool fm = ch < 3;
        int fmch = ch;
        int ssgch = ch - 3;

        // エフェクト等の処理
        if (part.KeyOffFrame <= currentFrame)
        {
            KeyOnOff(ch, false);
        }

        // Volume
        int keyon_length = currentFrame - part.KeyOnFrame;
        int final_volume = part.Volume;

        if (part.UDepth != 0 && keyon_length >= part.UDelay)
        {
            if (((keyon_length - part.UDelay) / part.ULength) & 1)
                final_volume -= part.UDepth;
        }

        if (part.SSGEnvOn)
        {
            SSGEnv env = musicdata.GetSSGEnv(part.SoundNo);
            size_t pos = (currentFrame - part.KeyOnFrame) / env.Unit;
            if (pos >= env.Env.size())
            {
                pos = env.Env.size() - 1;
            }
            final_volume = final_volume + (env.Env[pos] - 15);
        }

        final_volume = min(max(final_volume, 0), 15);
        if (fm)
        {
            fmwrap.SetVolume(fmch, final_volume);
        }
        else
        {
            ssgwrap.SetVolume(ssgch, final_volume);
        }

        // Tone
        if (part.PLength != 0)
        {
            int d = currentFrame - part.KeyOnFrame;
            if (d <= part.PLength)
            {
                if (fm)
                {
                    double tone = part.LastTone * (1 << part.LastOctave)
                        + (double)(part.Tone * (1 << part.Octave) - part.LastTone * (1 << part.LastOctave)) * d / part.PLength;
                    int octave = 0;
                    while (tone >= 2048.0)
                    {
                        octave++;
                        tone /= 2;
                    }

                    fmwrap.SetTone(fmch, octave, (int)(tone + 0.5));
                }
                else
                {
                    double tone = part.LastTone + (double)(part.Tone - part.LastTone) * d / part.PLength;
                    ssgwrap.SetTone(ssgch, (int)(tone + 0.5));
                }
            }
        }
        else if (part.ILength != 0 && keyon_length >= part.IDelay)
        {
            int depth;
            if (((keyon_length - part.IDelay) / part.ILength) & 1)
                depth = -part.IDepth;
            else
                depth = part.IDepth;

            if (fm)
            {
                int tone = (int)(part.Tone * pow(2.0, depth / (255.0 * 12.0)) + 0.5);
                fmwrap.SetTone(fmch, part.Octave, tone);
            }
            else
            {
                int tone = (int)(part.Tone * pow(0.5, depth / (255.0 * 12.0)) + 0.5);
                ssgwrap.SetTone(ssgch, tone);
            }
        }
    }

    void Sequencer::KeyOnOff(int ch, bool on)
    {
        if (ch < 3)
        {
            fmwrap.KeyOnOff(ch, on);
        }
        else
        {
            ssgwrap.KeyOnOff(ch - 3, on);
        }
    }

    Sequencer::PartData::PartData()
    {
        NoteEndFrame = 0;
        KeyOnFrame = 0;
        KeyOffFrame = 0;

        LastOctave = 4;
        Octave = 4;
        Volume = 15;
        Tone = 0;
        LastTone = 0;

        SSGEnvOn = false;
        SoundNo = 0;
        DefaultNoteLength = 64;
        Detune = 0;
        GateTime = 8;

        PLength = 0;

        ILength = 0;
        IDepth = 0;
        IDelay = 0;

        ULength = 0;
        UDepth = 0;
        UDelay = 0;

        Tied = false;
        Playing = false;
        InfiniteLooping = false;
    }

} // namespace MusicCom
