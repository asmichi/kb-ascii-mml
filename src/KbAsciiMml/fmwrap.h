#pragma once

#include "musdata.h"

namespace FM
{
    class OPN;
}

namespace MusicCom
{
    // ch は0-origin
    class FMWrap
    {
    public:
        FMWrap(FM::OPN& o);
        void SetSound(int ch, const FMSound& sound);
        void SetTone(int ch, int block, int fnumber);
        void SetVolume(int ch, int vol);

        void KeyOnOff(int ch, bool on);

    private:
        void SetToneReg(int highaddr, int lowaddr, int block, int fnumber);

        FM::OPN& opn;
        FMSound sound[3];
        int vol[3];
        static const double detune2_table[4];
        static const bool vol_tl_flag[8][4];
        static const int op_table[4];
    };

    class SSGWrap
    {
    public:
        SSGWrap(FM::OPN& o);
        void SetEnv(int ch, bool on);
        void SetEnvForm(int form);
        void SetEnvPeriod(int period);
        void SetNoisePeriod(int period);
        void SetTone(int ch, int tone);
        void SetNoise(int ch, bool on);
        void SetVolume(int ch, int vol);

        void KeyOnOff(int ch, bool on);

    private:
        void SetNoiseToneEnable();

        FM::OPN& opn;
        bool noise[3];
        bool keyon[3];
        bool env[3];
        int vol[3];
    };

} // namespace MusicCom
