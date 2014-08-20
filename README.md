#CLeaR: OpenCL experimental a\*\*\* Raytracer

CLeaRはOpenCLでどの程度実用的なレンダラーが書けるかを実験する目的で作ったレンダラーです。
BSDF周りの処理は[PBRT-v2](https://github.com/mmp/pbrt-v2)をかなり参考にしています。  
現状ではCPU用のロジックをそのまま移植したような形になっているため、レジスターの大量使用や、条件分岐によるフローの発散など、2014年現在のGPUアーキテクチャーにとって効率的な実装にはなっていません。
(参考：["Megakernels Considered Harmful: Wavefront Path Tracing on GPUs"](https://research.nvidia.com/publication/megakernels-considered-harmful-wavefront-path-tracing-gpus))

CLeaR has been developed with a purpose to experiment how practical renderer can be written using OpenCL.
In writing procedures regarding BSDF, I refered to [PBRT-v2](https://github.com/mmp/pbrt-v2) considerably.  
For now, it looks like the same as logic used in CPU implementation, so it is not efficient implementation for current GPU architectures as of 2014 due to the massive usage of registers and control-flow divergence by conditional branches.
(refer to ["Megakernels Considered Harmful: Wavefront Path Tracing on GPUs"](https://research.nvidia.com/publication/megakernels-considered-harmful-wavefront-path-tracing-gpus))

##特徴 / Features
* Unidirectional Path Tracing
* Multiple BSDF Layers
* Multiple Importance Sampling
* Image Based Environmental Light
* Depth of Field
* Bump Mapping (Normal Map)
* BVH Spatial Partitioning
* .obj Loader

##動作環境 / Confirmed Environment
現状以下の環境で動作を確認しています。  
I've confirmed that the program runs correctly on the following environment.

* OS X 10.9.4
* MacBook Pro Retina Late 2013
* Geforce GT 750M

動作させるにあたっては以下のライブラリが必要です。  
It requires the following libraries.

* OpenEXR 2.10
* libpng 16.16

##注意 / Note
objファイルやテクスチャーを読み込むコードが書かれていますが、それらアセットはリポジトリには含まれていません。
objファイルはBlender 2.70で、デフォルト設定 + 「法線を出力」 + 「三角面化」 + 「マテリアルグループ」でエクスポートしたものを前提としています。

There are code for loading a obj file and textures, but those assets are NOT included in this repository.
It assumes for a obj file to be created using Blender 2.70 with export settings: default settings + "Write Normals" + "Triangulate Faces" + "Material Groups".

----
2014 [@Shocker_0x15](https://twitter.com/Shocker_0x15)
