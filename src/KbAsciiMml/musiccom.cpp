#include "pch.h"

#include "mmlparser.h"
#include "musdata.h"
#include "musiccom.h"
#include "sequencer.h"

namespace MusicCom
{
    MusicCom::MusicCom()
    {
        pmusicdata = NULL;
        pseq = NULL;
    }

    MusicCom::~MusicCom()
    {
        delete pmusicdata;
        delete pseq;
    }

    bool MusicCom::Load(const char* filename)
    {
        delete pmusicdata;

        pmusicdata = ParseMML(filename);
        if (pmusicdata == NULL)
            return false;

        return true;
    }

    bool MusicCom::PrepareMix(uint rate)
    {
        const uint OPN_CLOCKFREQ = 3993600; // OPNのクロック周波数

        delete pseq;
        pseq = new Sequencer(opn, pmusicdata);
        if (!pseq->Init(rate))
        {
            return false;
        }
        opn.SetVolumeFM(fmVolume);
        opn.SetVolumePSG(psgVolume);

        return true;
    }

    void MusicCom::Mix(FM_SAMPLETYPE* dest, int nsamples)
    {
        pseq->Mix(dest, nsamples);
    }

    void MusicCom::SetFMVolume(int vol)
    {
        fmVolume = vol;
    }

    void MusicCom::SetPSGVolume(int vol)
    {
        psgVolume = vol;
    }

} // namespace MusicCom
