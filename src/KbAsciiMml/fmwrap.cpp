#include "pch.h"
#include "fmwrap.h"
#include "musdata.h"
#include <fmgen/opna.h>

using namespace std;

namespace MusicCom
{
    // detune2による周波数の倍率
    const double FMWrap::detune2_table[4] = {
        1.0, 1.41421356, 1.581, 1.7320508};

    // TLをvolume変化に使用するかどうか
    // [alg][op]
    const bool FMWrap::vol_tl_flag[8][4] = {
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 0, 1, 1},
        {0, 1, 1, 1},
        {0, 1, 1, 1},
        {1, 1, 1, 1}};

    const int FMWrap::op_table[4] = {0, 2, 1, 3};

    FMWrap::FMWrap(FM::OPN& o) : opn(o)
    {
        fill_n(vol, 3, 15);
    }

    void FMWrap::SetSound(int ch, const FMSound& s)
    {
        assert(0 <= ch && ch < 3);

        sound[ch] = s;
        opn.SetReg(0xb0 + ch, s.AlgFb);
        for (int op = 0; op < 4; op++)
        {
            int d = op_table[op] * 4 + ch;
            opn.SetReg(0x30 + d, s.Op[op].DtMl);
            opn.SetReg(0x40 + d, s.Op[op].Tl);
            opn.SetReg(0x50 + d, s.Op[op].KsAr);
            opn.SetReg(0x60 + d, s.Op[op].Dr);
            opn.SetReg(0x70 + d, s.Op[op].Sr);
            opn.SetReg(0x80 + d, s.Op[op].SlRr);
        }
        // TL 設定
        SetVolume(ch, vol[ch]);
    }

    void FMWrap::SetVolume(int ch, int v)
    {
        assert(0 <= ch && ch < 3);

        vol[ch] = v;
        const FMSound& s = sound[ch];
        for (int op = 0; op < 4; op++)
        {
            int d = op_table[op] * 4 + ch;
            if (vol_tl_flag[s.GetAlg()][op_table[op]])
                opn.SetReg(0x40 + d, s.Op[op].Tl + 4 * (15 - v));
        }
    }

    void FMWrap::KeyOnOff(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);

        int val;
        if (on)
            val = 0xf0 | ch;
        else
            val = 0x00 | ch;
        opn.SetReg(0x28, val);
    }

    void FMWrap::SetTone(int ch, int block, int fnumber)
    {
        assert(0 <= ch && ch < 3);

        const FMSound& s = sound[ch];
        if (ch == 2)
        {
            static const int addrs[4][2] = {
                {0xad, 0xa9},
                {0xae, 0xaa},
                {0xac, 0xa8},
                {0xa6, 0xa2},
            };

            for (int op = 0; op < 4; op++)
            {
                int i = op_table[op];
                SetToneReg(addrs[i][0], addrs[i][1], block, (int)(fnumber * detune2_table[s.Op[op].Dt2]));
            }
        }
        else
        {
            SetToneReg(0xa4 + ch, 0xa0 + ch, block, fnumber);
        }
    }

    void FMWrap::SetToneReg(int highaddr, int lowaddr, int block, int fnumber)
    {
        while (fnumber >= 2048)
        {
            fnumber /= 2;
            block++;
        }

        if (block >= 8)
        {
            block = 7;
        }

        opn.SetReg(highaddr, ((block & 0x7) << 3) | ((fnumber >> 8) & 0x7));
        opn.SetReg(lowaddr, fnumber & 0xff);
    }

    SSGWrap::SSGWrap(FM::OPN& o) : opn(o)
    {
        fill_n(noise, 3, false);
        fill_n(keyon, 3, false);
        fill_n(env, 3, false);
        fill_n(vol, 3, 15);
    }

    void SSGWrap::SetEnvForm(int form)
    {
        opn.SetReg(0x0d, form);
    }

    void SSGWrap::SetEnvPeriod(int period)
    {
        opn.SetReg(0x0b, period & 0xff);
        opn.SetReg(0x0c, (period >> 8) & 0xff);
    }

    void SSGWrap::SetNoisePeriod(int period)
    {
        opn.SetReg(0x06, period & 0xff);
    }

    void SSGWrap::SetTone(int ch, int tone)
    {
        assert(0 <= ch && ch < 3);

        int d = ch * 2;
        opn.SetReg(0x00 + d, tone & 0xff);
        opn.SetReg(0x01 + d, (tone >> 8) & 0x0f);
    }

    void SSGWrap::SetVolume(int ch, int v)
    {
        assert(0 <= ch && ch < 3);
        vol[ch] = v;
        int d = ch;

        int val = (env[ch] ? 0x10 : 0x00) | (v & 0x0f);
        opn.SetReg(0x08 + d, val);
    }

    void SSGWrap::SetEnv(int ch, bool on)
    {
        env[ch] = on;
        SetVolume(ch, vol[ch]);
    }
    void SSGWrap::SetNoise(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);
        noise[ch] = on;
        KeyOnOff(ch, keyon[ch]);
    }
    void SSGWrap::KeyOnOff(int ch, bool on)
    {
        assert(0 <= ch && ch < 3);

        keyon[ch] = on;
        SetNoiseToneEnable();
    }

    void SSGWrap::SetNoiseToneEnable()
    {
        int val = 0;
        for (int ch = 0; ch < 3; ch++)
        {
            int n = (int)!(noise[ch] && keyon[ch]) << 3;
            int t = (int)!keyon[ch];
            val |= (n | t) << ch;
        }
        val |= 0x80;

        opn.SetReg(0x07, val);
    }

} // namespace MusicCom
