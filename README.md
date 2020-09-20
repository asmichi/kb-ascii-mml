KbAsciiMml.kpi - ASCII music.com MML Player version 0.8

本リポジトリは、記録のために作成したものです。

本ソフトウェアは、 Boost Spirit ライブラリが使いたかったというだけの理由で軽率に作成されたものです。
もはやメンテナンスされません。

### これは何？
PC-9801 シリーズ上の ASCII ツクールシリーズを中心に使用されていた ASCII music.com の MML ファイルを再生するための KbMedia Player プラグインです。

FM音源エミュレーションに、 cisc 氏の fmgen ライブラリを使用しており、高い再現性があります。

本家 music.com 並に文法チェックを緩くできなかったので、再生できない MML があります。

＃ あと本家 music.com は SR の扱いがおかしいような…… (SR として SL が使用されている？)

KbAsciiMml.ini 中の FMVolume、PSGVolumeの値を設定することで、
FM 音源と PSG 音源のバランスを調節できます。

### 永遠に未実装なもの
- LFO: による LFO 指定
- D: パート (実装の予定もなし)
- もう少しマシな seek
- ビブラート・トレモロの正しい幅

### ライセンス
本ソフトウェアのうち、 FM音源のエミュレーション部分、すなわち [external/fmgen](external/fmgen) ディレクトリ以下のファイルの著作権は cisc 氏に帰属します。
この部分のライセンスの詳細は [external/fmgen/readme.txt](external/fmgen/readme.txt) を参照してください。

それ以外の部分の著作権は、 asmichi に帰属します。この部分は、 MIT ライセンス (下記) でライセンスされます。

本ソフトウェア全体が MIT ライセンスでライセンスされているわけでは **ない** ことに改めて注意してください。
そのような誤解を避けるため、あえて LICENSE ファイルを置いていません。

#### MIT ライセンス

Copyright (c) @asmichi (https://github.com/asmichi)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

### ビルド

次の手順でビルドできます。

- 環境変数 BOOST_ROOT を Boost のルートディレクトリにセットする
- 環境変数 KPISDK_ROOT を KbMedia Player プラグイン SDK のルートディレクトリにセットする
- "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat" を実行する
- build.cmd を実行する

ビルドに使用した環境は次の通りです。

- Visual Studio 2019 Professional (「C++ ネイティブ開発」ワークロードあり)
- KbMedia Player 3.08
- Boost 1.74.0

### 更新履歴
- 2020/09/21 ver.0.8
    - kbmucom から KbAsciiMml に改名 (MUCOM88 との誤解を避けるため)
    - 環境を Visual Studio 2019 に変更し、雰囲気上はビルドできるように

- 2010/xx/xx ver.0.7
    - 環境を Visual Studio 2008 に変更

- 2007/10/09 ver.0.6
    - D: パートは単に無視するように (エラーにしない)
    - & の扱いを ASCII music.com に近づけた
        - ex.) 次のようにマクロ中の音符と外の音符をタイでつなぐことを可能に
        ```
        STR: MACRO=A4
        $MACRO$ & A8
        ```
    - FM音源とPSG音源のバランスを調節できるように (kbmucom.ini : FMVolume, PSGVolume)

- 2007/05/31 ver.0.5
    - 初版

### 謝辞

FM音源のエミュレーションには、cisc 氏の fmgen ライブラリを使用しており、
このプラグインで実現できた高い再現性のほとんどはこのライブラリによります。
また、この部分の著作権は cisc 氏に帰属します。
詳細は [external/fmgen/readme.txt](external/fmgen/readme.txt) を参照してください。

> ```
> ------------------------------------------------------------------------------
>         FM Sound Generator with OPN/OPM interface
>         Copyright (C) by cisc 1998, 2003.
> ------------------------------------------------------------------------------
project M88 http://www.retropc.net/cisc/m88/


