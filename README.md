#CLeaR: OpenCL experimental a\*\*\* Raytracer

CLeaRはOpenCLでどの程度実用的なレンダラーが書けるかを実験する目的で作ったレンダラーです。
BSDF周りの処理は[PBRT-v2](https://github.com/mmp/pbrt-v2)をかなり参考にしています。  
現状ではCPU用のロジックをそのまま移植したような形になっているため、レジスターの大量使用や、条件分岐によるフローの発散など、2014年現在のGPUアーキテクチャーにとって効率的な実装にはなっていません[1]。
空間分割構造にはBVHを使用、CPUによる単純な中点スプリットに加えて、GPUによるLinear BVH [2, 3]、~~Treelet Restructuring BVH~~(実装中) [4]の並列構築も実装しています。

CLeaR has been developed with a purpose to experiment how practical renderer can be written using OpenCL.
In writing procedures regarding BSDF, I refered to [PBRT-v2](https://github.com/mmp/pbrt-v2) considerably.  
For now, it looks like the same as logic used in CPU implementation, so it is not efficient implementation for current GPU architectures as of 2014 due to the massive usage of registers and control-flow divergence by conditional branches [1].
It uses BVH for the spatial partitioning structure. Parallel construction of Linear BVH [2, 3] and ~~Treelet Restructuring BVH~~ (under development) [4] on GPU are implemented in addition to simple midpoint splitting on CPU.

[1] "Megakernels Considered Harmful: Wavefront Path Tracing on GPUs"  
[2] "Fast BVH Construction on GPUs"  
[3] "Maximizing Parallelism in the Construction of BVHs, Octrees, and k-d Trees"  
[4] "Fast Parallel Construction of High-Quality Bounding Volume Hierarchies"

##特徴 / Features
* Unidirectional Path Tracing
* Multiple BSDF Layers
* Multiple Importance Sampling
* Image Based Environmental Light
* Depth of Field
* Bump Mapping (Normal Map)
* Simple Midpoint Splitting BVH (constructed on CPU)
* Linear BVH
* ~~Treelet Restructuring BVH~~ (under development)
* BVH Visualizer
* .obj Loader

##動作環境 / Confirmed Environment
現状以下の環境で動作を確認しています。  
I've confirmed that the program runs correctly on the following environment.

* OS X 10.9.5
* MacBook Pro Retina Late 2013
* Geforce GT 750M

動作させるにあたっては以下のライブラリが必要です。  
It requires the following libraries.

* OpenEXR 2.10
* libpng 1.6.14

##注意 / Note
objファイルやテクスチャーを読み込むコードが書かれていますが、それらアセットはリポジトリには含まれていません。
objファイルはBlender 2.70で、デフォルト設定 + 「法線を出力」 + 「三角面化」 + 「マテリアルグループ」でエクスポートしたものを前提としています。

There are code for loading a obj file and textures, but those assets are NOT included in this repository.
It assumes for a obj file to be created using Blender 2.70 with export settings: default settings + "Write Normals" + "Triangulate Faces" + "Material Groups".

----
2014 [@Shocker_0x15](https://twitter.com/Shocker_0x15)
