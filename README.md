# WebXpressionPlus.x ver0.46.2

- WebXpression.x programmed by Mitsuky
- UTF-8, TLS support by Yuichi Nakamura (@yunkya2)

## 概要

このプログラムは、X680x0 用 WWW ブラウザ [WebXpression.x](http://retropc.net/x68000/software/internet/wwwbrowser/webxpression/) をベースに以下の機能追加を行ったものです。

- UTF-8 対応
- TLS (HTTPS) 対応

四半世紀も前に開発されたブラウザを現代に蘇らせようという酔狂な試みです。
当時と比べると Web の仕様も劇的に変化しているため出来ることはかなり限られますが、サイトの閲覧くらいには使えるかもしれません。

## 動作環境

X680x0 で、計測技研製 TCP/IP ドライバやその互換ドライバを用いたネットワーク接続が可能な環境が必要です。

TLS プロトコルは X680x0 にとって非常に重い処理なので、正常な動作のためには各種設定のチューニングが不可欠です。具体的には以下の設定を行ってください。

* CONFIG.SYS
  * TCP/IPドライバを動かすために必要な `PROCESS=` 行を以下の設定にします
    ```
    PROCESS = 3 2 100
    ```
    * 最初の `3` は最大スレッド数の指定です。3 以上の値であればよいです
    * 2,3 番目の値 (メインスレッドの優先度とタイムスライス値) は計測技研製ドライバの設定例では `10 10` になっていますが、この値では TLS 処理のための CPU 時間が不足するため、変更の必要があります
* TCP/IP ドライバ
  * 計測技研製ドライバより高速な hinetd.x (K.Shirakata さん作) を使用してください
    * X68000 LIBRARY (http://retropc.net/x68000/software/internet/tcpip/hinetd/) から入手できます
  * hinetd.x 起動時のパラメータは以下の設定にします
    ```
    hinetd -w512 -n15
    ```
    高速化係数(`-n`)を最大に設定した上で、TCP のウィンドウサイズ(`-w`)を 512 バイトに設定します
  * 実行中にドライバのメモリ不足が発生する場合があるため、メモリに余裕のある環境では更に `-+-h:524288 -+-s:262144` のオプションを追加してヒープサイズとスタックサイズを増やしておくとよいです

## 使い方

実行ファイルとコンフィグファイルの名前がそれぞれ WebXpressionPlus.x, WebXpressionPlus.cnf に変わっていますが、基本的にはオリジナルの WebXpression.x と同じです。
doc ディレクトリにオリジナルのドキュメントがありますので、そちらを参照してください。

オリジナルが作られた頃に比べると現在の Web サイトはページのサイズも画像の数やサイズもかなり大きくなっているので、表示までにはかなり時間がかかります。気長に待ちましょう。

コンフィグファイルに新規に「IMGタグを無視する」設定を追加してあります。
WebXpressionPlus.cnf の ignore-image の値を 1 に設定すると、すべての IMG タグを無視して画像を表示しなくなります。特にアクセラレータ等のない素のマシンの場合は 1 を推奨します。

## ビルド方法

クロス開発環境でのビルドに移行させたので、ビルドには Linux 環境で以下のツール、ファイルが必要です。

- [elf2x68k](https://github.com/yunkya2/elf2x68k) (20250907 版以降)
- [run68](https://github.com/yunkya2/run68mac)
- [HAS060.X](http://retropc.net/x68000/software/develop/as/has060/)
- IOCSCALL.MAC, DOSCALL.MAC (XCのヘッダ)

run68 をパスの通った場所に置き、HAS060.X, IOCSCALL.MAC, DOSCALL.MAC は Source ディレクトリに置いてください。

```
git clone --recursive https://github.com/yunkya2/WebXpressionPlus.git
```
で、サブモジュール込みで clone した後、Source ディレクトリ内で以下のコマンドでビルドします。

```
make
```

## 謝辞

オリジナルのソースコードを公開してくださっていたおかげで、令和の世に X680x0 用ブラウザを動かそうという無謀な試みに挑戦することができました。
WebXpression.x の作者である Mitsuky さんに深く感謝いたします。

## 配布規定

オリジナルの WebXpression.x の以下の配布規定に従います。
```
  配布＆改造自由のフリーウェアです。ソースの引用＆組み込みも自由です。い
ずれの場合も連絡の必要はありません。

  また、大変申し訳ありませんが、本プログラムは無保証であり本プログラムに
起因するいかなる損失にも免責とさせて頂きます。
```

新規に追加したファイルについては以下の通りです。
- axTLS は BSD ライセンスです
- UTF-8 -> SJIS 変換ライブラリ iconv_mini は MIT ライセンスです
