#CLeaR: OpenCL experimental a\*\*\* Raytracer

CLeaRはOpenCLでどのくらい実用的なレンダラーが書けるかを実験するために作られたレンダラーです。

現状以下の環境で動作を確認しています。

* OS X 10.9.4
* MacBook Pro Retina Late2013
* Geforce GT 750M

動作させるにあたっては以下のライブラリが必要です。

* OpenEXR 2.10
* libpng 16.16

##注意
objファイルやテクスチャーを読み込むコードが書かれていますが、それらアセットはリポジトリには含まれていません。
objファイルはBlender 2.70で、デフォルト設定 + 「法線を出力」 + 「三角面化」 + 「マテリアルグループ」でエクスポートしたものを前提としています。

----
2014 @Shocker_0x15
