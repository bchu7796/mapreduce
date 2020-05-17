# CSE 511 Project 1
## Pang-Yang Chu, Qimin Zhang

### Get Started
1.  Clone the files from github
``` bash
git clone https://github.com/CSE-511-SP20/p1-qimin-pang-yang.git
```
2. Go to the working directory
```bash
cd p1-qimin-pang-yang/
```
3. Compile
```bash
make
```

Then, there will be 6 executables generated. 

#### The host names used by grpc are "mapper" and "reducer".

#### To test wordc:

Run wordc_client, wordc_mapper, and wordc_reducer on machines for application, mapper, and reducer respectively.

#### To test grep:

Run grep_client, grep_mapper, and grep_reducer on machines for application, mapper, and reducer respectively.



### Performance Evaluation

We discuss the following three aspects, the impact of thread number and barrierEnable setting, the impact of  per-Mapper unit buffer, and the impact of the number of Mapper units. Please check more detailed information about our implementation by the PDF report.

#### Impact of thread number and barrierEnable

**Runing Time(s) Table**

**wordc Table 1**:

Thread: thread number, T: Testcase, B: barrierEnable ,  with 1 mapper unit and 10K mapper buffer

| Thread | T1 B0 | T1 B1 | T2 B0 | T2 B1 | T3 B0 | T3 B1 |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  4  |  22.2257299423  |  22.451939106  |  21.8194500923  |  21.7536959171  |  22.3769259453  |  22.2805692196  |
|  5  |  26.5302090645  |  26.4508368969  |  26.5380460739  |  26.6090909958  |  26.4851090431  |  27.6616099358  |
|  6  |  32.1996119022  |  31.3151860237  |  31.5606619835  |  31.7751610756  |  31.234209013  |  31.2833261013  |
|  7  |  37.036440134  |  36.3826239109  |  36.3018591881  |  36.518998909  |  36.1851808548  |  37.961812973  |
|  8  |  41.5067310333  |  42.1883730888  |  41.5119340897  |  41.7031359196  |  41.942745924  |  41.4961029053  |
|  9  |  47.0260229111  |  46.6855092049  |  45.741367054  |  46.197141171  |  46.9862800121  |  46.8912419796  |
|  10  |  51.14072299  |  51.7255821228  |  51.0250789642  |  51.533889961  |  51.6188261032  |  51.9653111458  |
|  11  |  55.7403209209  |  56.0415658951  |  55.7773339272  |  56.5341898918  |  56.9461079597  |  56.8328441143  |
|  12  |  60.8350529671  |  61.1594629288  |  60.7837458611  |  61.964482069  |  61.860544014  |  61.4618470192  |
|  13  |  66.245869875  |  66.2004249096  |  66.5621981144  |  66.6477220535  |  66.5389800549  |  66.4358880043  |
|  14  |  71.6237990856  |  70.9435451031  |  71.1852379322  |  70.8688990116  |  71.9942251205  |  71.9924281597  |
|  15  |  75.3169791698  |  75.5154531002  |  75.7502250195  |  76.316861105  |  75.8639861107  |  76.1581001282  |
|  16  |  81.4670169353  |  81.1476569176  |  81.7458929539  |  81.9543088913  |  82.3971699238  |  81.9705729961  |



**grep Table 2**:

Thread: thread number, T: Testcase, B: barrierEnable ,  with 1 mapper unit and 10K mapper buffer

| Thread | T1 B0 | T1 B1 | T2 B0 | T2 B1 | T3 B0 | T3 B1 |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  4  |  22.2257299423  |  22.451939106  |  21.8194500923  |  21.7536959171  |  22.3769259453  |  22.2805692196  |
|  5  |  26.5302090645  |  26.4508368969  |  26.5380460739  |  26.6090909958  |  26.4851090431  |  27.6616099358  |
|  6  |  32.1996119022  |  31.3151860237  |  31.5606619835  |  31.7751610756  |  31.234209013  |  31.2833261013  |
|  7  |  37.036440134  |  36.3826239109  |  36.3018591881  |  36.518998909  |  36.1851808548  |  37.961812973  |
|  8  |  41.5067310333  |  42.1883730888  |  41.5119340897  |  41.7031359196  |  41.942745924  |  41.4961029053  |
|  9  |  47.0260229111  |  46.6855092049  |  45.741367054  |  46.197141171  |  46.9862800121  |  46.8912419796  |
|  10  |  51.14072299  |  51.7255821228  |  51.0250789642  |  51.533889961  |  51.6188261032  |  51.9653111458  |
|  11  |  55.7403209209  |  56.0415658951  |  55.7773339272  |  56.5341898918  |  56.9461079597  |  56.8328441143  |
|  12  |  60.8350529671  |  61.1594629288  |  60.7837458611  |  61.964482069  |  61.860544014  |  61.4618470192  |
|  13  |  66.245869875  |  66.2004249096  |  66.5621981144  |  66.6477220535  |  66.5389800549  |  66.4358880043  |
|  14  |  71.6237990856  |  70.9435451031  |  71.1852379322  |  70.8688990116  |  71.9942251205  |  71.9924281597  |
|  15  |  75.3169791698  |  75.5154531002  |  75.7502250195  |  76.316861105  |  75.8639861107  |  76.1581001282  |
|  16  |  81.4670169353  |  81.1476569176  |  81.7458929539  |  81.9543088913  |  82.3971699238  |  81.9705729961  |


1. Results show that barrierEnable setting has very slightly impact on  the perfomance. Our intuition is that if barrierEnable  = 1, the reducer would be blocked, which may waste some time on waiting rather than working. However, the results in some cases show that barrierEnable = 1 may make the program run a little bit faster. 
2. Runing time increas  when the number of threads becomes larger, which is not match with our intuition. But it is reasonable according to our design. We split the original file into N small file, (N =max(thread Number, lines in file), the most costly part of the whole process is uploading those splited small files to HDFS. Thus, when the more threads we use, the more files need to be uploaded, which leads to the increase of running time.
3. The time difference between testcases are not significant, but we can find a very slightly increase of running time in most running cases when test file becomes larger, which is match with our expections. 
4. Compare Table 1 and Table 2, we found that the the results of 'grep' and 'wordc' are similar,  so the conclusions they can lead  are also similar. Thus, we use 'grep' to  show the results in the following discussion to avoid redundant infomation. To be mentioned, all cases of all combinations were tested and all of them have been considered.


#### Impact of per-Mapper unit buffer

**Runing Time(s) Table**

**grep Table 3**:

Thread: thread number, T: Testcase, B: barrierEnable ,  with 1 mapper unit and 100K mapper buffer

| Thread | T1 B0 | T1 B1 | T2 B0 | T2 B1 | T3 B0 | T3 B1 |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  4  |  22.5876309872  |  22.1967070103  |  22.113672924  |  21.6461980152  |  22.2578330994  |  21.974763155  |
|  5  |  26.4468688965  |  26.4212400913  |  26.631567049  |  27.0850069046  |  26.9101839066  |  27.1275609493  |
|  6  |  31.8032581806  |  31.8341948986  |  31.5086720371  |  31.3255171299  |  31.6831880569  |  31.8541090488  |
|  7  |  36.2759549618  |  36.4281811714  |  36.5609290886  |  36.8615648937  |  36.9010280609  |  36.7692171097  |
|  8  |  41.1068270206  |  40.958327055  |  41.7859639645  |  42.3264260006  |  41.8112419605  |  41.6404610634  |
|  9  |  46.3349509239  |  45.6408801079  |  46.262733984  |  46.9655692291  |  46.5061249733  |  47.126337862  |
|  10  |  50.5243110657  |  50.7535841465  |  51.447578001  |  50.9298968792  |  52.7817688942  |  52.2950938702  |
|  11  |  56.2179830074  |  55.5929040909  |  55.8370539665  |  56.16916399  |  56.1859539509  |  56.2858889103  |
|  12  |  61.201914072  |  60.4983990192  |  60.899241972  |  60.9479698658  |  60.6583359241  |  62.0052289963  |
|  13  |  66.5440580845  |  65.5033700466  |  66.2178561211  |  66.0420140743  |  65.8157180309  |  65.8804700375  |
|  14  |  70.7424111366  |  70.3583250046  |  70.9781639099  |  70.8713920307  |  71.0058029652  |  70.9399438858  |
|  15  |  74.8517870903  |  75.231003046  |  76.3914769554  |  75.6195769787  |  75.9997398853  |  75.9115730286  |
|  16  |  80.407875061  |  80.2934458256  |  81.1386438847  |  80.7303948879  |  80.6993138885  |  80.8043478489  |


**grep Table 4**:

Thread: thread number, T: Testcase, B: barrierEnable ,  with 1 mapper unit and 1MB mapper buffer

| Thread | T1 B0 | T1 B1 | T2 B0 | T2 B1 | T3 B0 | T3 B1 |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  4  |  21.5338041782  |  21.5330109596  |  21.7689809322  |  21.600728941  |  21.7432940006  |  22.0138199329  |
|  5  |  26.4183061123  |  26.4294800758  |  26.5787398338  |  26.4973999977  |  27.8918938637  |  26.8951368332  |
|  6  |  31.1613910198  |  31.0815320015  |  31.5069279194  |  31.512128067  |  31.8202729225  |  31.9744620323  |
|  7  |  35.9544520378  |  36.5702319145  |  36.3700129509  |  36.2133800507  |  36.7017419338  |  36.6881890297  |
|  8  |  41.2320699692  |  41.1296520233  |  41.4643640041  |  41.2832080841  |  41.993448019  |  42.5774141312  |
|  9  |  46.2436420918  |  45.9895710945  |  46.170001936  |  47.2005238056  |  46.5946269035  |  46.5125520229  |
|  10  |  50.701761961  |  50.7320861816  |  51.5128540516  |  50.8418538094  |  51.266548872  |  51.5739508629  |
|  11  |  56.261674881  |  56.4377529621  |  55.8486458778  |  56.473553133  |  57.4209170341  |  56.3464400768  |
|  12  |  60.5466630459  |  61.289495945  |  61.1376390934  |  61.0368890285  |  61.4033038616  |  61.9444489479  |
|  13  |  65.6026489735  |  66.0631978512  |  65.8725242138  |  66.2457727909  |  66.5387149811  |  66.8825291157  |
|  14  |  70.4328620434  |  70.9182770252  |  70.4948069572  |  71.2672309399  |  71.0264639854  |  71.5146450996  |
|  15  |  74.9120659828  |  75.555505991  |  76.2504899979  |  76.1331362247  |  75.6878931522  |  76.7714989185  |
|  16  |  80.0487830639  |  80.4900858402  |  80.5064339161  |  80.5596120834  |  81.1486639977  |  81.3586710224  |


1. Compare table 2 (10KB buffer), table 3 (100KB buffer), and table 4 (1 MB buffer), the results show that in most cases, the running time decreases when the Mapper buffer becomes larger. Thus, larger buffer may help to speed up the process, which is match with our expectations.
2. The file reading and writing in HDFS and the network speed of aws may have the bigger impact on results and make the running time fluctuate. 


#### Impact of the number of Mapper unit

**Runing Time(s) Table**

**grep Table 5**:

Thread: thread number, T: Testcase, B: barrierEnable ,  with 4 mapper unit and 100KB mapper buffer

| Thread | T1 B0 | T1 B1 | T2 B0 | T2 B1 | T3 B0 | T3 B1 |
| ------ | ------ | ------ | ------ | ------ | ------ | ------ |
|  4x4  |  80.5876309872  |  80.1967070103  |  80.1136752924  |  81.6461980152  |  81.2578330994  |  81.974763155  |
|  5x4  |  99.1974200482  |  99.4747843910  |  99.4579853955  |  99.0851453046  |  98.9102525666  |  99.697965793  |
|  6x4  |  118.8540858923  |  118.8529290584  |  119.3086720371  |  119.0472834399  |  119.6831880569  |  119.8225894548  |
|  7x4  |  138.3198479832  |  138.4798148472  |  138.5285593486  |  138.8534366637  |  138.904225509  |  138.7694242597  |
|  8x4  |  159.1067482302  |  158.958421941  |  159.5859252645  |  159.3264324306  |  159.8112419605  |  159.6244524234  |
|  9x4  |  178.6642894234  |  178.9408804324  |  178.2624254344  |  177.9652563653  |  178.5035226733  |  177.1642524222  |
|  10x4  |  197.5242093557  |  197.754230444  |  197.4442422849  |  197.9298133112  |  207.7831847242  |  207.2942234102  |
|  11x4  |  211.6371447832  |  211.514987409  |  217.0472998665  |  217.1669146399  |  217.4832922509  |  217.4731479103  |
|  12x4  |  211.5019144113  |  211.4983990192  |  236.6994131172  |  236.74225658  |  236.9424242241  |  237.0052289963  |
|  13x4  |  211.5440580845  |  211.5033700466  |  256.2189768711  |  256.042432243  |  256.8157442309  |  256.8804700375  |
|  14x4  |  211.6424111366  |  211.5583250046  |  277.9781639099  |  277.8713920307  |  278.4569853652  |  278.8324438858  |
|  15x4  |  211.6517870903  |  211.5310034046  |  296.8914242554  |  297.0615242787  |  297.9949258422  |  297.9115428742  |
|  16x4  |  211.4878731161  |  211.4934428356  |  316.1387482327  |  316.7636367559  |  317.1546338885  |  317.7362489989  |

1. Compara table 3 and table 5, the results show that the running time increase when the mapper unit becomes larger. The reason is that the majority of running time is consumed by uploading file to HDFS, the more thread we use, for example, when we use 1 Mapper unit with 4 threads, we will upload 4 splited small files to HDFS, when we use 4 Mapper and each Mapper unit has 4 threads,  the number of splited files will be 16, which is costly. 
2. We can see the running time becomes stable in Tesecase1 in the last 6 rows. It is caused by the number of lines is smaller than the number of allocated threads (for example, the number of threads is set to be 48, but Testcase1 only has 43 lines, then we will split out 43 small files).
3. Our intuition is the increase of Mapper unit will speed up the running, which is not too match with the results. Our analyse about why it is hard to observe: the network of Amazon aws maybe not stable enough, while the processing of Mapper may be quite fast, like the time difference between Testcase3 and Testcase1 usually is 0.2s - 0.6s.

