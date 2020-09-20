#pragma once

#include <cassert>
#include <cstring>
#include <list>
#include <map>

namespace MusicCom
{
    struct FMSound
    {
        int LFOForm;
        int LFOSpeed;
        int LFODepth;

        unsigned char AlgFb;
        struct
        {
            unsigned char DtMl;
            unsigned char Tl;
            unsigned char KsAr;
            unsigned char Dr;
            unsigned char Sr;
            unsigned char SlRr;
            unsigned char Dt2;
        } Op[4];

        struct FMSound()
        {
            Clear();
        }
        void Clear()
        {
            std::memset(this, 0, sizeof(*this));
        }
        void SetLFO(int form, int speed, int depth)
        {
            LFOForm = form;
            LFOSpeed = speed;
            LFODepth = depth;
        }
        void SetAlgFb(int alg, int fb)
        {
            AlgFb = ((fb & 0x7) << 3) | (alg & 0x7);
        }
        void SetDtMl(int op, int dt, int ml)
        {
            assert(0 <= op && op < 4);
            Op[op].DtMl = ((dt & 0x7) << 4) | (ml & 0xf);
        }
        void SetTl(int op, int tl)
        {
            assert(0 <= op && op < 4);
            Op[op].Tl = tl & 0x7f;
        }
        void SetKsAr(int op, int ks, int ar)
        {
            assert(0 <= op && op < 4);
            Op[op].KsAr = ((ks & 0x3) << 6) | (ar & 0x1f);
        }
        void SetDr(int op, int dr)
        {
            assert(0 <= op && op < 4);
            Op[op].Dr = dr & 0x1f;
        }
        void SetSr(int op, int sr)
        {
            assert(0 <= op && op < 4);
            Op[op].Sr = sr & 0x1f;
        }
        void SetSlRr(int op, int sl, int rr)
        {
            assert(0 <= op && op < 4);
            Op[op].SlRr = ((sl & 0xf) << 4) | (rr & 0xf);
        }
        void SetDt2(int op, int dt2)
        {
            assert(0 <= op && op < 4);
            Op[op].Dt2 = dt2 & 0x3;
        }

        int GetAlg() const
        {
            return AlgFb & 0x7;
        }
    };

    struct SSGEnv
    {
        SSGEnv()
        {
            Unit = 64;
            Env.push_back(15);
        }
        int Unit;
        std::vector<unsigned char> Env;
    };

    class Command
    {
    public:
        static const char TYPE_NOTE = '#'; // args = {Note, Length, Tied};
        static const char TYPE_RET = '%'; // macro return

        Command()
        {
            type = 0;
            std::memset(args, 0, sizeof(args));
        }
        Command(char t, int arg0 = 0, int arg1 = 0, int arg2 = 0)
        {
            SetType(t);
            SetArg(0, arg0);
            SetArg(1, arg1);
            SetArg(2, arg2);
        }
        Command(char t, std::string arg)
        {
            SetType(t);
            std::memset(args, 0, sizeof(args));
            SetStrArg(arg);
        }
        template<class InIt>
        Command(char t, InIt first, InIt last)
        {
            SetType(t);
            std::memset(args, 0, sizeof(args));
            for (int i = 0; i < sizeof(args) / sizeof(args[0]) && first != last; i++, first++)
            {
                SetArg(i, *first);
            }
        }
        void SetType(char t)
        {
            type = t;
        }
        char GetType() const
        {
            return type;
        }
        void SetArg(int index, int val)
        {
            assert(0 <= index && index < 3);
            args[index] = val;
        }
        int GetArg(int index) const
        {
            assert(0 <= index && index < 3);
            return args[index];
        }

        // （; ＾ω＾）
        void SetStrArg(std::string val)
        {
            strarg = val;
        }
        const std::string& GetStrArg() const
        {
            return strarg;
        }

    private:
        char type;
        int args[3];
        std::string strarg;
    };

    typedef std::list<Command> CommandList;

    class MusicData
    {
    public:
        MusicData();
        void SetFMSound(int no, const FMSound& sound)
        {
            fmsounds[no] = sound;
        }
        void SetSSGEnv(int no, const SSGEnv& env)
        {
            ssgenvs[no] = env;
        }
        const FMSound& GetFMSound(int no)
        {
            return fmsounds[no];
        }
        const SSGEnv& GetSSGEnv(int no)
        {
            return ssgenvs[no];
        }
        void SetTempo(int t) { tempo = t; }
        int GetTempo() const { return tempo; }
        bool IsChannelPresent(int index) const
        {
            assert(0 <= index && index < channel_count);
            return channel_present[index];
        }
        bool IsMacroPresent(std::string name) const
        {
            return macros.find(name) != macros.end();
        }

        void AddCommandToChannel(int index, const Command& command);
        void AddCommandToMacro(std::string name, const Command& command);

        CommandList::const_iterator GetChannelHead(int channel) const;
        CommandList::const_iterator GetMacroHead(std::string name) const;

    private:
        static const int channel_count = 6;

        std::map<int, FMSound> fmsounds;
        std::map<int, SSGEnv> ssgenvs;
        std::map<std::string, std::list<Command>> macros;
        std::list<Command> channels[channel_count];
        bool channel_present[6];
        int tempo;
    };

} // namespace MusicCom
