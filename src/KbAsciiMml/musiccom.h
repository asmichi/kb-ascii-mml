#pragma once

#include <fmgen/opna.h>

namespace MusicCom
{
    class Sequencer;
    class MusicData;

    class MusicCom
    {
    public:
        MusicCom();
        ~MusicCom();
        bool Load(const char* filename);
        bool PrepareMix(uint rate);
        void Mix(FM_SAMPLETYPE* dest, int nsamples);
        void SetFMVolume(int vol);
        void SetPSGVolume(int vol);

    protected:
        MusicCom(const MusicCom&); // non-copyable
    private:
        FM::OPN opn;
        Sequencer* pseq;
        MusicData* pmusicdata;
        int fmVolume;
        int psgVolume;
    };

} // namespace MusicCom
