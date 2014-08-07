#CLeaR: OpenCL experimental a\*\*\* Raytracer

CLeaRはOpenCLでどのくらい実用的なレンダラーが書けるかを実験するために作られたレンダラーです。
BSDF周りの処理は[PBRT-v2](https://github.com/mmp/pbrt-v2)をかなり参考にしています。  
現状ではCPU用のロジックをそのまま移植したような形になっているため、レジスターの大量使用や、条件分岐によるフローの発散など、2014年現在のGPUアーキテクチャーにとって効率的な実装にはなっていません。(参考：["Megakernels Considered Harmful: Wavefront Path Tracing on GPUs"](https://research.nvidia.com/publication/megakernels-considered-harmful-wavefront-path-tracing-gpus))

特徴
* Unidirectional Path Tracing
* Multiple BSDF Layers
* Multiple Importance Sampling
* Image Based Environmental Light
* Depth of Field
* Bump Mapping (Normal Map)
* BVH Spatial Partitioning
* .obj Loader

現状以下の環境で動作を確認しています。

* OS X 10.9.4
* MacBook Pro Retina Late 2013
* Geforce GT 750M

動作させるにあたっては以下のライブラリが必要です。

* OpenEXR 2.10
* libpng 16.16

##注意
objファイルやテクスチャーを読み込むコードが書かれていますが、それらアセットはリポジトリには含まれていません。
objファイルはBlender 2.70で、デフォルト設定 + 「法線を出力」 + 「三角面化」 + 「マテリアルグループ」でエクスポートしたものを前提としています。

----
2014 [@Shocker_0x15](https://twitter.com/Shocker_0x15)
