# Computer-Photography_Grab_Cut
###### tags: `zju` `cg`
 [計算攝影學] 期末專題 Computer-Photography_Grab_Cut
![](https://i.imgur.com/YjwO7eT.png)

## 基本原理
### Kmeans
### GMM
### Minimum-Cut

## Graph定義
### 像素與像素之間的edge
![](https://i.imgur.com/no0KWbj.png)
### 像素與Source/Destination之間的edge
![](https://i.imgur.com/2abmNJU.png)

## visual c++ 動態連結 Opencv
![](https://i.imgur.com/utPknzs.png)
![](https://i.imgur.com/RGNRRrW.png)
![](https://i.imgur.com/jr9jFk7.png)
```
opencv_world345d.lib debug模式
opencv_world345.lib release模式
```
![](https://i.imgur.com/W0qHEOL.png)
```
opencv/build/vc14_or_15/bin 中的dll文件(Debug对应opencv_world345d.dll，Release对应opencv_world345.dll)，放到exe檔案的目錄下
```

## 執行檔案
```command=
在 ExecuteFile 目錄裡頭
平台: Windows
附加: opencv_world345d.dll (需要這個dll在同個目錄下才能運作)
```

## 運行說明
![](https://i.imgur.com/GheXw4r.png)
```command=
r : 還原圖片
f : 顯示前景
b : 顯示背景
n : 進行一次GrabCut迭代
s : 儲存去背的圖片
```
##### 選擇要去背的圖片
![](https://i.imgur.com/lbSTv6C.png)
##### 標註完前景的框框後，按n執行第一次GrabCut迭代
![](https://i.imgur.com/8hmwaJR.png)
##### 第一次迭帶的結果出來後，輔助標示一些沒有被認出來的前景，或是被誤認的背景，[ctrl]+左鍵 標示背景、[shift]+右鍵 標示前景。接這在按n執行一次GrabCut迭代。
![](https://i.imgur.com/bqxSCIm.png)
![](https://i.imgur.com/X3vRdSg.png)
##### 按s儲存前景
![](https://i.imgur.com/JE85mxw.png)
```
此檔案一定要是 png 格式，因為只有 png 格式支援第四個通道 alpha
通道，也就是透明度通道，才能真正保存去背的效果，而 jpg 格式沒
有這個通道，所以存起來的圖片背景會是白色的。
```
##### 結果
![](https://i.imgur.com/J6abGSf.png)

