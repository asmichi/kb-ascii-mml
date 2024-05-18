#include "pch.h"
#include "musdata.h"

using namespace std;
using namespace boost::spirit::classic;
using boost::bad_lexical_cast;
using boost::lexical_cast;

namespace MusicCom
{
    int ParseLength(string s)
    {
        if (s.empty())
            return 0;
        bool dotted;
        if (*s.rbegin() == '.')
        {
            s.erase(--s.end());
            dotted = true;
        }
        else
        {
            dotted = false;
        }

        int len = lexical_cast<int>(s);
        if (len <= 0 || 64 < len)
        {
            //throw runtime_error("illegal note length");
            return 64;
        }
        len = 64 / len;
        if (dotted)
            len += len / 2;

        return len;
    }

    int StringToInt(string s)
    {
        try
        {
            return lexical_cast<int>(s);
        }
        catch (bad_lexical_cast)
        {
            return 0;
        }
    }

    struct MMLParser : public grammar<MMLParser>
    {
        enum MMLLineType
        {
            CH,
            SOUND,
            LFO,
            OP,
            SSGENV,
            STR
        };

        struct MMLParserState
        {
            MMLParserState()
            {
                LineNumber = 1;
            }

        public:
            vector<string> args;

        public:
            MusicData* pMusicData;

        public:
            int LineNumber;
            MMLLineType LineType;

        public:
            int ChNumber; // ch番号 (0-origin)
            char CommandType;
            string MacroName;

            // Sound
        public:
            FMSound Sound;
            int SoundNumber;
        };

        MMLParser(MMLParserState& s) : state(s)
        {
        }
        MMLParserState& state;

    private:
        static void AddCommand(MMLParserState& state, const Command& command)
        {
            switch (state.LineType)
            {
            case CH:
                state.pMusicData->AddCommandToChannel(state.ChNumber, command);
                break;
            case STR:
                state.pMusicData->AddCommandToMacro(state.MacroName, command);
                break;
            default:
                assert(0);
                break;
            }
        }

        template<typename T>
        struct Increment
        {
            Increment(T& t) : target(t) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                target++;
            }

            T& target;
        };

        template<enum MMLLineType t>
        struct BeginLine
        {
            BeginLine(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                state.args.clear();
                state.LineType = t;
            }

            MMLParserState& state;
        };

        struct SetChNumber
        {
            SetChNumber(MMLParserState& s) : state(s) {}

            void operator()(char c) const
            {
                state.ChNumber = c - '1';
            }

            MMLParserState& state;
        };

        struct SetMacroName
        {
            SetMacroName(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                state.MacroName.assign(first, last);
            }

            MMLParserState& state;
        };

        struct PushArg
        {
            PushArg(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                string s(first, last);
                state.args.push_back(s);
            }

            MMLParserState& state;
        };

        // Sound
        struct ProcessLFO
        {
            ProcessLFO(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                vector<int> a;
                transform(state.args.begin(), state.args.end(), back_inserter(a), StringToInt);
                a.resize(5, 0);

                // LFO:	WF,SPEED,DEPTH,ALG,FB
                FMSound& sound = state.Sound;
                sound.SetLFO(a[0], a[1], a[2]);
                sound.SetAlgFb(a[3], a[4]);
                state.pMusicData->SetFMSound(state.SoundNumber, sound);
            }

            MMLParserState& state;
        };

        struct ProcessOP
        {
            ProcessOP(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                int op = state.ChNumber;

                vector<int> a;
                transform(state.args.begin(), state.args.end(), back_inserter(a), StringToInt);
                a.resize(10, 0);

                // OP1:	AR,DR,SR,RR,SL,TL,KS,ML,DT,DT2
                FMSound& sound = state.Sound;
                sound.SetDtMl(op, a[8], a[7]);
                sound.SetTl(op, a[5]);
                sound.SetKsAr(op, a[6], a[0]);
                sound.SetDr(op, a[1]);
                //			sound.SetSr(op, a[2]);
                sound.SetSr(op, a[4]); // music.comでSrとしてSlが使われるバグ！？
                sound.SetSlRr(op, a[4], a[3]);
                sound.SetDt2(op, a[9]);
                state.pMusicData->SetFMSound(state.SoundNumber, state.Sound);
            }

            MMLParserState& state;
        };

        // SSGEnv
        struct ProcessSSGEnv
        {
            ProcessSSGEnv(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                SSGEnv env;

                if (state.args.size() < 3)
                {
                    //throw std::runtime_error("SSGEnvの引数が足りない");
                    return;
                }

                int no = StringToInt(state.args[0]);
                env.Unit = StringToInt(state.args[1]);
                env.Env.clear();
                transform(state.args.begin() + 2, state.args.end(), back_inserter(env.Env), StringToInt);
                state.pMusicData->SetSSGEnv(no, env);
            }

            MMLParserState& state;
        };

        // MML コマンド
        struct BeginCommand
        {
            BeginCommand(MMLParserState& s) : state(s) {}

            void operator()(char c) const
            {
                state.args.clear();
                state.CommandType = toupper(c);
            }

            MMLParserState& state;
        };

        struct ProcessNote
        {
            ProcessNote(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                static const int note_numbers[] = {
                    // A,  B, C, D, E, F, G
                    9,
                    11,
                    0,
                    2,
                    4,
                    5,
                    7,
                };

                vector<string>& a = state.args;

                int note = note_numbers[state.CommandType - 'A'];
                if (!a[0].empty())
                {
                    switch (a[0][0])
                    {
                    case '+':
                        note++;
                        break;
                    case '-':
                        note--;
                        break;
                    }
                }
                int len = ParseLength(state.args[1]);

                AddCommand(state, Command(Command::TYPE_NOTE, note, len));
            }

            MMLParserState& state;
        };

        struct ProcessCtrl
        {
            ProcessCtrl(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                const static struct CtrlDef
                {
                    char Type;
                    bool IsNoteLength;
                    int MinArgs;
                    int MaxArgs;
                    int Defaults[3];
                } ctrl_defs[] = {
                    {'&', false, 0, 0, {}},
                    {'T', false, 1, 1, {}},
                    {'R', true, 0, 1, {0}},
                    {'W', true, 0, 1, {0}},
                    {'L', true, 1, 1, {}},
                    {'O', false, 1, 1, {}},
                    {'<', false, 0, 0, {}},
                    {'>', false, 0, 0, {}},
                    {'V', false, 1, 1, {}},
                    {'@', false, 1, 1, {}},
                    {'Q', false, 1, 1, {}},
                    {'N', false, 1, 1, {}},
                    {'P', false, 1, 1, {}},
                    {'U', false, 1, 3, {0, 0, 0}},
                    {'I', false, 1, 3, {0, 0, 0}},
                    {'S', false, 1, 1, {}},
                    {'M', false, 1, 1, {}},
                    {'Y', false, 2, 2, {}},
                    {'{', false, 0, 1, {0}},
                    {'}', false, 0, 0, {}},
                };

                const CtrlDef* pctrldef = NULL;
                for (int i = 0; i < sizeof(ctrl_defs) / sizeof(ctrl_defs[0]); i++)
                {
                    if (ctrl_defs[i].Type == state.CommandType)
                    {
                        pctrldef = &ctrl_defs[i];
                        break;
                    }
                }

                if (pctrldef == NULL)
                {
                    //ostringstream ss;
                    //ss << "unknown command '" << state.CommandType << "'";
                    //throw runtime_error(ss.str());
                    return;
                }

                int args_supplied = (int)state.args.size();
                if (args_supplied < pctrldef->MinArgs || pctrldef->MaxArgs < args_supplied)
                {
                    //ostringstream ss;
                    //ss << "illegal number of argument (" << args_supplied << ") for command '" << state.CommandType << "'";
                    //throw runtime_error(ss.str());
                    return;
                }

                // 引数を取得
                vector<int> a;
                // とりあえずintに変換
                transform(state.args.begin(), state.args.end(), back_inserter(a), StringToInt);
                // 引数が足りなければデフォルト引数を補充
                for (int i = args_supplied; i < pctrldef->MaxArgs; i++)
                {
                    a.push_back(pctrldef->Defaults[i]);
                }
                // 音長指定なら音長で変換し直す
                if (pctrldef->IsNoteLength && state.args.size() > 0)
                {
                    a[0] = ParseLength(state.args[0]);
                }

                // Tのみ特別処理
                if (state.CommandType == 'T')
                {
                    state.pMusicData->SetTempo(a[0]);
                }
                else
                {
                    AddCommand(state, Command(state.CommandType, a.begin(), a.end()));
                }
                //cout << state.CommandType << "(";
                //copy(state.args.begin(), state.args.end(), ostream_iterator<string>(cout, ","));
                //cout << ");";
            }

            MMLParserState& state;
        };

        struct ProcessCall
        {
            ProcessCall(MMLParserState& s) : state(s) {}

            template<typename IteratorT>
            void operator()(IteratorT first, IteratorT last) const
            {
                AddCommand(state, Command('$', state.args[0]));
            }

            MMLParserState& state;
        };

    public:
        template<typename ScannerT>
        struct definition
        {
            definition(MMLParser const& self)
            {
                MMLParserState& s = self.state;

                // 0x1a = [EOF]
                line =
                    (ch_line | drum_line | sound_line | lfo_line | op_line | ssgenv_line | str_line | blank_line)
                    >> !comment
                    >> (eol_p[Increment<int>(self.state.LineNumber)] | end_p | ch_p(0x1a));

                args =
                    arg % !ch_p(',') // !: スペースで区切るMML対策
                    >> !ch_p(',');
                arg =
                    (int_p || ch_p('.'))[PushArg(s)];

                macro_name =
                    +chset<>("a-zA-Z0-9_");
                mml_Command =
                    mml_note[ProcessNote(s)] | mml_ctrl[ProcessCtrl(s)] | mml_call[ProcessCall(s)];
                mml_note =
                    as_lower_d[range_p('a', 'g')][BeginCommand(s)]
                    >> (ch_p('+') | ch_p('-') | eps_p)[PushArg(s)]
                    >> (arg | eps_p[PushArg(s)]);
                // '&' はコマンドとして扱う
                //				>> (ch_p('&') | eps_p)[PushArg(s)];
                mml_ctrl =
                    as_lower_d[chset<>("h-z@{}<>&")][BeginCommand(s)]
                    >> !args;
                //				>> !ch_p('&');	// 変なところに&を置くMML対策
                mml_call =
                    ch_p('$')[BeginCommand(s)]
                    >> macro_name[PushArg(s)]
                    >> ch_p('$')
                    >> !ch_p(','); // なぜかマクロ呼び出しを','で区切るMML対策

                // 空行
                blank_line = eps_p;

                // コメント
                comment =
                    ch_p(';')
                    >> *(anychar_p - eol_p);

                // チャンネル定義 1:, ...
                ch_line =
                    (range_p('1', '6')[SetChNumber(s)] >> ch_p(':'))[BeginLine<CH>(s)]
                    >> *mml_Command;

                // D: は無視する
                drum_line =
                    as_lower_d[str_p("d:")]
                    >> *(anychar_p - eol_p);
                sound_line =
                    (as_lower_d[str_p("sound")] >> ch_p(':'))[BeginLine<SOUND>(s)]
                    >> !ch_p('@')
                    >> int_p[assign(s.SoundNumber)];
                lfo_line =
                    (as_lower_d[str_p("lfo")] >> ch_p(':'))[BeginLine<LFO>(s)]
                    >> args[ProcessLFO(s)];
                op_line =
                    (as_lower_d[str_p("op")]
                     >> range_p('1', '4')[SetChNumber(s)] >> ch_p(':'))[BeginLine<OP>(s)]
                    >> args[ProcessOP(s)];
                ssgenv_line =
                    (as_lower_d[str_p("ssgenv")] >> ch_p(':'))[BeginLine<SSGENV>(s)]
                    >> !ch_p('@')
                    >> ((int_p | eps_p)[PushArg(s)] % ((eol_p[Increment<int>(s.LineNumber)] >> str_p("->") | ch_p(','))))[ProcessSSGEnv(s)];
                str_line =
                    (as_lower_d[str_p("str")] >> ch_p(':'))[BeginLine<STR>(s)]
                    >> macro_name[SetMacroName(s)]
                    >> !ch_p('$') >> ch_p('=')
                    >> *mml_Command;
            }

            rule<ScannerT> line;
            rule<ScannerT> blank_line, ch_line, drum_line, sound_line, lfo_line, op_line, ssgenv_line, str_line;
            rule<ScannerT> mml_Command, mml_note, mml_ctrl, mml_call;
            rule<ScannerT> args, arg, macro_name, comment;

            rule<ScannerT> const&
            start() const { return line; }
        };
    };

    MusicData* ParseMML(const char* filename)
    {
        MusicData* pMusicData = new MusicData();
        MMLParser::MMLParserState state;
        state.pMusicData = pMusicData;
        MMLParser mmlparser(state);

        typedef file_iterator<char> iterator_t;

        // ファイルを開いて、その先頭を指すイテレータを生成
        iterator_t begin(filename);
        if (!begin)
        {
            return nullptr;
        }

        iterator_t first = begin;
        iterator_t last = first.make_end();

        while (first != last)
        {
            parse_info<iterator_t> info;
            try
            {
                info = parse(first, last, mmlparser, blank_p);
                if (!info.hit)
                {
                    ostringstream ss;
                    ss << filename << "(" << state.LineNumber << ") : parse error at \"";
                    iterator_t i = find_if(
                        info.stop,
                        last,
                        [](char c)
                        {
                            return c == '\r' || c == '\n';
                        });
                    copy(info.stop, i, ostream_iterator<char>(ss));
                    ss << "\"" << endl;
                    MessageBoxA(NULL, ss.str().c_str(), "エラー", MB_OK);
                    delete pMusicData;
                    return 0;
                }
            }
            catch (exception& e)
            {
                ostringstream ss;
                ss << filename << "(" << state.LineNumber << ") : " << e.what() << endl;
                MessageBoxA(NULL, ss.str().c_str(), "エラー", MB_OK);
                delete pMusicData;
                return 0;
            }

            first = info.stop;
        }

        return pMusicData;
    }

} // namespace MusicCom
