#include "pch.h"
#include "musdata.h"

using namespace std;
using namespace boost;

namespace MusicCom
{
    MusicData::MusicData()
    {
        fill_n(channel_present, channel_count, false);
        tempo = 120;
    }

    CommandList::const_iterator MusicData::GetChannelHead(int index) const
    {
        assert(IsChannelPresent(index));

        return channels[index].begin();
    }

    CommandList::const_iterator MusicData::GetMacroHead(string name) const
    {
        assert(IsMacroPresent(name));

        return macros.find(name)->second.begin();
    }

    void MusicData::AddCommandToChannel(int index, const Command& command)
    {
        CommandList& cl = channels[index];
        if (!IsChannelPresent(index))
        {
            // 暗黙の無限ループを追加
            cl.push_back(Command('{', 0));
            cl.push_back(Command('}'));
            channel_present[index] = true;
        }

        cl.insert(--cl.end(), command);
    }

    void MusicData::AddCommandToMacro(std::string name, const Command& command)
    {
        if (!IsMacroPresent(name))
        {
            CommandList& cl = macros[name];
            // マクロからのreturnを追加
            cl.push_back(Command(Command::TYPE_RET));
        }

        CommandList& cl = macros[name];
        cl.insert(--cl.end(), command);
    }

} // namespace MusicCom
