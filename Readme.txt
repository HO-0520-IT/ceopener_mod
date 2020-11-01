ceOpener 第二世代・第三世代対応版 日本語版 VisualStudio2008用 楽々ビルドパッケージ for Ver0.14

１．このパッケージについて
本パッケージは、同時に公開された「ceOpener 第二世代・第三世代対応版 日本語版」のソースコードです。
VisualStudio2008用のプロジェクトを組んであるので、簡単にビルドすることができます。

２．各ディレクトリの説明
AppMain - ReleaseビルドのceOpenerが格納されます。ビルドしたのち、このフォルダをそのままBrainにコピーするだけで使用できます。
brain-essentials - knatech氏のアプリケーションで使われている共通ライブラリのソリューションが格納されています。
ceopener - ceopenerのソリューションが格納されています。
ceopener_plugins - ceopenerの各種プラグインをビルドできるソリューションが格納されています。
Debug - DebugビルドのceOpenerがここに格納されます。
include - brain-essentialsとlibjpegのヘッダファイルが格納されています。
lib - Releaseビルドの各種ライブラリが格納されます。
lib_d - Debugビルドの各種ライブラリが格納されます。
libjpeg - libjpegをビルドできるソリューションが格納されています。

３．ビルド方法
ビルドは以下の手順で行ってください。
１．libjpegのソリューションを開き、ビルドする。
２．brain-essentialsのソリューションを開き、ビルドする。
(このときbrain-essentialsを2回ビルドしてください。)
３．ceopenerのソリューションを開き、ビルドする。
４．ceopener_pluginsのソリューションを開き、ビルドする。

４．ライセンスについて
このソフトはGPLでライセンスされています。ライセンスの詳細については「COPYING」ファイルを参照してください。
GPLで規定された範囲を超えての使用を禁止します。

５．その他
このソフトの制作者はknatech氏ですが、現在Brainアプリの開発をしていないため、
質問やご要望などありましたらGrainに連絡してください。
メールアドレス:naonaokiryu2@gmail.com

(c) 2015 Grain