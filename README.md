# UbiquantSystem
该项目是九坤投资举办的量化系统竞赛的一个系统实现 By 三卜白水(张辛宁、石蒙、张若琦)
具体订单撮合规则是基于2023年8月深交所的规则，题目要求在只考虑连续竞价下，采取TWAP策略进行策略单下单的模拟以及历史订单与策略订单的撮合。
最终要求输出仓位与当日利润率。
本项目的方案取得了本竞赛的**季军**，以2s的延迟落后于冠军，项目报告在report目录下。
(PS:决赛时运行的脚本是parallel.sh，cmake+make了7秒左右，如果去掉的话本方案就是冠军，痛失3w2，可以说是大家这辈子写过的最贵的代码了)