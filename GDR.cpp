#property copyright "Copyright 2021, MetaQuotes Software Corp."
#property link      "https://www.mql5.com"
#property version   "1.00"
#property strict

enum TradeDirection
{
    BUY  = 0,//BUY
    SELL = 1,//SELL
};

extern TradeDirection tradeDirection = SELL;   //做单方向

input double openLots = 0.1;               //首次开仓手数
input int addPositionInterval = 50;        //加仓间隔点数
input double positionMultiplier = 1.3;      //加仓的倍数
input double maxOrderLots = 2.0;            //单个订单最大手数
input int additionalSpreadAfterMaxOrder = 100;//单个订单达到最大手数后开仓间距增加点数
input double maxStopLoss = -2000;            //亏损多大时停止开仓（-2000）
input int averagePriceTakeProfitPoints = 1100;//均价止盈点数
input double overallProfitTarget = 1;   //整体盈利金额
input string orderComment = "第一组";  //注释1
input int magicNumber = 666;//模式一魔术码
input bool isOpenTrailingTakeProfit = false;//第一组是否开启移动止损
input string s1 = "单边盈利移动止损参数";
input int danbianCTrailingStop1 = 200;//盈利最小盈利点数
input int danbianCTrailingStep2 = 500;//盈利移动止损移动间隔点数
input string s2 = "解套移动止损参数";
input int jietaoCTrailingStop1 = 1500;//解套最小盈利点数
input int jietaoCTrailingStep2 = 1500;//解套移动止损移动间隔点数
input bool isOpenIEMA = false;//是否开启均线辅助开仓
input bool isOpenSAR  = true;//是否开启SAR开仓
input string s3 = "消单逻辑参数";
input bool isOpenDismiss  = true;//是否开启多空消单逻辑
input int dismissProfit = 2;//盈利多少开始消单
input int OverBuyValue = 85;//Stochastic超买值
input int OverSellValue = 15;//Stochastic超卖值


extern ENUM_TIMEFRAMES iMAPERIOD = PERIOD_M5;//均线周期
input int slow_k_num = 12;//快线k线历史个数

datetime firstOrderBuyTime  = 0;
datetime firstOrderSellTime = 0;

double firstBuyOrderLots;
double firstBuyOrderPrice;

double firstSellOrderLots;
double firstSellOrderPrice;

// 订单统计
struct Counter
{
    
    double            totalProfit;
    int               totalOrderCount;
    double            buyTotalProfit;
    double            sellTotalProfit;
    double            buyTotalLots;
    double            sellTotalLots;
    int               firstOrderType;
    int               lastHistoryOrderType;
    
    //=====多单参数=================
    double            lastBuyOrderPrice;
    double            hightBuyOrderPrice;//当前买单的最高订单的开仓价格
    double            lowBuyOrderPrice;//当前买单的最低订单的开仓价格
    double            secendLowBuyOrderPrice;//当前买单的倒数第二低订单的开仓价格
    double            lastBuyOrderLots;
    double            firstBuyOrderPrice;
    double            firstBuyOrderLots;
    double            firstBuyOrderProfit;
    double            firstBuyOrderStopless;
    double            firstBuyOrderTakeProfit;
    
    datetime          firstBuyOrderOpenTime;
    
    int               orderCountUpFirstBuyOrder;
    int               orderCountLowFirstBuyOrder;
    
    int               buyOrderCount;
    double            lastBuyStop;
    
    //=====空单参数=================
    int               orderCountUpFirstSellOrder;
    int               orderCountLowFirstSellOrder;
    
    double            lastSellOrderPrice;
    double            hightSellOrderPrice;//当前卖单的最高订单的开仓价格
    double            lowSellOrderPrice;//当前卖单的最低订单的开仓价格
    double            secendLowHightOrderPrice;//当前买单的倒数第二高订单的开仓价格
    double            lastSellOrderLots;
    double            firstSellOrderPrice;
    double            firstSellOrderLots;
    double            firstSellOrderProfit;
    double            firstSellOrderStopless;
    double            firstSellOrderTakeProfit;
    datetime          firstSellOrderOpenTime;
    int               sellOrderCount;
    double            lastSellStop;
    
    //=====消单参数=================
};

bool isFirstModeFirstOrderPlaced = false;//第一模式首单是否开过

datetime t1=0;
datetime t2=0;
datetime t3=0;
int StopLevel;
int spread;
int baseInterval;
double sar;

//趋势震荡策略变量
// 全局变量
double shortMA, longMA, rsi, adx, atr;
input double RsiHigh = 70;
input double RsiLow = 30;

// 2024-6-17 优化思路：1.单边行情开启整体平仓单边加仓；2.震荡行情新加消单逻辑；3.SAR单边做单逻辑不变

int OnInit()
{
    initSubViews();
    
    StopLevel = MarketInfo(Symbol(), MODE_STOPLEVEL);
    baseInterval = addPositionInterval;
    
    Print("StopLevel = ", StopLevel);
    return(INIT_SUCCEEDED);
}

void OnTick()
{
    
    if(!IsConnected() && t1 !=Time[0])
    {
        Print("No connection!");
        t1=Time[0];
        return;
    }
    
    if(!IsExpertEnabled() && t2 !=Time[0])
    {
        Alert("No ExpertEnabled!");
        t2=Time[0];
        return;
    }
    
    if(!IsTradeAllowed() && t3 !=Time[0])
    {
        Alert("No TradeAllowed!");
        t3=Time[0];
        return;
    }
    
    spread  = SymbolInfoInteger(Symbol(),SYMBOL_SPREAD);
    baseInterval = addPositionInterval;
    Counter magic1Counter = CalcTotal(magicNumber);
    updateAccountInfo(magic1Counter);
    
    if (magic1Counter.totalProfit < maxStopLoss) {
        return;
    }
    
    // 整体止盈
    if((magic1Counter.totalProfit) > overallProfitTarget)
    {
        closeAllOrdersByMagicNumber(magicNumber);
    }
    
    // 移动止盈
    if(isOpenTrailingTakeProfit)
    {
        TrailingPositions(magic1Counter, magicNumber);
    }
    
    // 消单逻辑
    if (isOpenDismiss) {
        dismissProfit(magic1Counter);
    }
    
    // 开始跑策略
    openStrategy0(magic1Counter);
    
    //openStrategyFollowByMarketCondition(magic1Counter);
    
}

/**
 策略：根据SAR指标做多空方向，每次开始都是从0.01手开始，每单设置1美金止盈，比如SAR指标显示多，从2000进场，到2001止盈1美金离场。然后SAR继续显示多，2001继续进场2002止盈1美金继续离场。如果SAR继续显示多2002进场继续设置1美金止盈，而此时如果一直没有触发止盈而且SAR又变成了空，那开始进空单0.01，如果继续下跌，继续1.5倍加仓0.02，整体所有订单1美金止盈。如果先进了0.01多，然后变成了空头，又进了0.01/0.02空单，又再次变成了多单，那再次1.5倍跟着趋势加仓0.02多单，整体所有订单永远1美金止盈。
 */
void openStrategy0(Counter & magic1Counter)
{
    
    bool buySignal = false;
    bool sellSignal = false;
    
    //以SAR为多空开仓方向
    // 2024-6-17 优化为5分钟和15分钟的方向一致 才判定一个方向
    if (isOpenSAR) {
        sar = iSAR(NULL, 0, 0.02, 0.2, 0);
        double longSAR = iSAR(NULL,PERIOD_M15,0.02,0.2,0);
        
        if (Bid > sar && Bid > longSAR) {
            buySignal = true;
        }else if(Ask < sar && Ask < longSAR){
            sellSignal = true;
        }
    }
    
    if(IsSpreadWithinThreshold())
    {
        int nextBuyOrderIntervalPoint = baseInterval;
        int nextSellOrderIntervalPoint = baseInterval;
        
        if(magic1Counter.lastBuyOrderLots > maxOrderLots)
        {
            nextBuyOrderIntervalPoint += additionalSpreadAfterMaxOrder;
        }
        if(magic1Counter.lastSellOrderLots > maxOrderLots)
        {
            nextSellOrderIntervalPoint += additionalSpreadAfterMaxOrder;
        }
        
        //开仓逻辑
        /**
         A：如果是多头并且没有空单，就只开多，下跌进行马丁加仓；
         B:  如果是空头并且没有多单，就只开空，上涨进行马丁加仓;
         C：如果既有多单又有空单，按照之前的逻辑顺势加仓；
         */
        
        // C：如果既有多单又有空单，按照之前的逻辑顺势加仓
        if (magic1Counter.buyOrderCount != 0 && magic1Counter.sellOrderCount != 0) {
            
            // 既有多单又有空单，进行顺势加仓，解套
            if(magic1Counter.firstOrderType == OP_BUY)
            {
                if(magic1Counter.buyOrderCount == 0 && buySignal)
                {
                    openBuy(openLots, 0,0,orderComment,magicNumber);
                    firstBuyOrderLots = openLots;
                    return;
                }
                
                if(Ask >= magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
                {
                    double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                    if(magic1Counter.sellOrderCount == 0)
                    {
                        lots = openLots;
                    }
                    openBuy(lots,0,0,orderComment,magicNumber);
                    return;
                }
                
                if(Bid <= magic1Counter.firstBuyOrderPrice)
                {
                    if(magic1Counter.sellOrderCount == 0 && Bid<=magic1Counter.firstBuyOrderPrice- nextSellOrderIntervalPoint*Point && sellSignal)
                    {
                        openSell(openLots, 0,0,orderComment,magicNumber);
                        return;
                    }
                    else
                        if(Bid <= magic1Counter.lastSellOrderPrice - nextSellOrderIntervalPoint*Point)
                        {
                            double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
                            if(magic1Counter.buyOrderCount == 0)
                            {
                                lots = openLots;
                            }
                            openSell(lots,0,0,orderComment,magicNumber);
                            return;
                        }
                }
            }
            
            if(magic1Counter.firstOrderType==OP_SELL)
            {
                if(magic1Counter.sellOrderCount == 0 && sellSignal)
                {
                    openSell(openLots,0,0,orderComment,magicNumber);
                    firstSellOrderLots = openLots;
                    return;
                }
                if(Bid<=magic1Counter.lastSellOrderPrice- nextSellOrderIntervalPoint*Point)
                {
                    double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
                    if(magic1Counter.buyOrderCount == 0)
                    {
                        lots = openLots;
                    }
                    openSell(lots,0,0,orderComment,magicNumber);
                    return;
                }
                
                if(Ask >= magic1Counter.firstSellOrderPrice)
                {
                    if(magic1Counter.buyOrderCount == 0 && Ask >= magic1Counter.firstSellOrderPrice + nextBuyOrderIntervalPoint*Point && buySignal)
                    {
                        openBuy(openLots,0,0,orderComment,magicNumber);
                        return;
                    }
                    else
                        if(magic1Counter.buyOrderCount > 0 && Ask >= magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
                        {
                            double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                            if(magic1Counter.sellOrderCount == 0)
                            {
                                lots = openLots;
                            }
                            openBuy(lots,0,0,orderComment,magicNumber);
                            return;
                        }
                }
            }
        }else{
            // 只有逻辑 A 或者 B
            // 开多单
            if(buySignal)
            {
                //没有多单，直接开一单多
                if(magic1Counter.buyOrderCount == 0)
                {
                    openBuy(openLots, 0,0,orderComment,magicNumber);
                    firstBuyOrderLots = openLots;
                    return;
                }
                
                // 只有多单，下跌继续进行马丁加仓
                if (magic1Counter.sellOrderCount == 0) {
                    // 当前价格比最后一个多单的价格还要低满足下个开单要求，马丁加多单，尽快跑出来
                    if(Ask <= magic1Counter.lastBuyOrderPrice - nextSellOrderIntervalPoint * Point)
                    {
                        double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                        if(magic1Counter.sellOrderCount == 0)
                        {
                            lots = openLots;
                        }
                        openBuy(lots,0,0,orderComment,magicNumber);
                        return;
                    }
                }
                
            }else if (sellSignal){
                // 开空单
                if (magic1Counter.sellOrderCount == 0) {
                    openSell(openLots, 0,0,orderComment,magicNumber);
                    firstSellOrderLots = openLots;
                    return;
                }
                
                // 只有空单，上涨继续进行马丁加仓
                if (magic1Counter.buyOrderCount == 0) {
                    // 当前价格比最后一个多单的价格还要低满足下个开单要求，马丁加多单，尽快跑出来
                    if(Bid >= magic1Counter.lastSellOrderPrice + nextSellOrderIntervalPoint * Point)
                    {
                        double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                        if(magic1Counter.sellOrderCount == 0)
                        {
                            lots = openLots;
                        }
                        openSell(lots,0,0,orderComment,magicNumber);
                        return;
                    }
                }
            }
            
        }
    }
}

/**
 策略：根据均线 + RSI + ADX 指标来识别 趋势行情和震荡行情，然后根据不同的行情 执行 不同的策略
 
 */
void openStrategyFollowByMarketCondition(Counter & magic1Counter)
{
    // 识别市场条件
    string marketCondition = IdentifyMarketCondition();

    // 执行相应的交易策略
    if (marketCondition == "trending")
    {
        ExecuteTrendingStrategy();
    }
    else if (marketCondition == "range")
    {
        ExecuteRangeStrategy();
    }
}

// 识别市场条件函数
string IdentifyMarketCondition()
{
    // 获取指定时间周期的价格数据
    double prices[];
    ArraySetAsSeries(prices, true);
    CopyClose(NULL, iMAPERIOD, 0, 100, prices);
    
    // 计算技术指标
    shortMA = iMA(NULL, iMAPERIOD, 20, 0, MODE_SMA, PRICE_CLOSE, 0);
    longMA = iMA(NULL, iMAPERIOD, 50, 0, MODE_SMA, PRICE_CLOSE, 0);
    rsi = iRSI(NULL, iMAPERIOD, 14, PRICE_CLOSE, 0);
    adx = iADX(NULL, iMAPERIOD, 14, PRICE_CLOSE, MODE_MAIN,0);
    atr = iATR(NULL, iMAPERIOD, 14, 0);
    
    if (((shortMA > longMA && rsi > RsiHigh) || (shortMA < longMA && rsi < RsiLow)) && adx > 25)
    {
        //Print("当前趋势为：趋势行情");
        return "trending";
    }
    else if (adx < 20 && rsi > RsiLow && rsi < RsiHigh)
    {
        //Print("当前趋势为：震荡行情");
        return "range";
    }
    else
    {
        //Print("当前趋势为：不知道什么鬼行情，啥都不做");
        return "uncertain";
    }
}

// 执行趋势跟踪策略函数
void ExecuteTrendingStrategy()
{
    // 检查是否已有持仓
    if (OrdersTotal() == 0)
    {
     if (rsi > RsiHigh)
     {
        // 开多单
        Print("当前趋势为：趋势行情  开多单");
        //SendNotification("趋势行情  开多单");
     }
     else if (rsi < RsiLow)
     {
         // 开空单
        Print("当前趋势为：趋势行情  开空单");
        //SendNotification("趋势行情  开空单");
     }
    }
}


// 执行震荡交易策略函数
void ExecuteRangeStrategy()
{
        // 检查是否已有持仓
    if (OrdersTotal() == 0)
    {
         // RSI小于30时买入
        if (rsi < RsiLow)
        {
         Print("当前趋势为：震荡行情  开多单");
        }
        // RSI大于70时卖出
        else if (rsi > RsiHigh)
        {
         Print("当前趋势为：震荡行情  开空单");
        }
    }
}

void openStrategy1(Counter & magic1Counter)
{
    
    bool buySignal = true;
    bool sellSignal = true;
    
    if(isOpenIEMA)
    {
        double ma = iMA(_Symbol, iMAPERIOD, slow_k_num, 0,MODE_SMMA,PRICE_MEDIAN, 0);
        double ma_1 = iMA(_Symbol, iMAPERIOD, slow_k_num, 0,MODE_SMMA,PRICE_MEDIAN, 1);
        double ma_2 = iMA(_Symbol, iMAPERIOD, slow_k_num, 0,MODE_SMMA,PRICE_MEDIAN, 2);
        // KD指标
        double StocMain = iStochastic(_Symbol, iMAPERIOD, 5, 3, 3, MODE_SMA, 0, MODE_MAIN, 1);
        double StocSignal = iStochastic(_Symbol, iMAPERIOD, 5, 3, 3, MODE_SMA, 0, MODE_SIGNAL, 1);
        bool IsOverBuy = (StocMain >= OverBuyValue && StocSignal >= OverBuyValue);
        bool IsOverSell = (StocMain <= OverSellValue && StocSignal <= OverSellValue);
        buySignal =  ma > ma_1 && ma_1 > ma_2;
        sellSignal =  ma < ma_1  && ma_1 < ma_2;
    }
    
    //开仓逻辑
    if(IsSpreadWithinThreshold())
    {
        int nextBuyOrderIntervalPoint = baseInterval;
        int nextSellOrderIntervalPoint = baseInterval;
        
        if(magic1Counter.lastBuyOrderLots > maxOrderLots)
        {
            nextBuyOrderIntervalPoint += additionalSpreadAfterMaxOrder;
        }
        
        if(magic1Counter.lastSellOrderLots > maxOrderLots)
        {
            nextSellOrderIntervalPoint += additionalSpreadAfterMaxOrder;
        }
        
        if(magic1Counter.firstOrderType == OP_BUY)
        {
            if(magic1Counter.buyOrderCount == 0 && buySignal)
            {
                openBuy(openLots, 0,0,orderComment,magicNumber);
                firstBuyOrderLots = openLots;
                return;
            }
            
            if(Ask >= magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
            {
                double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                if(magic1Counter.sellOrderCount == 0)
                {
                    lots = openLots;
                }
                openBuy(lots,0,0,orderComment,magicNumber);
                return;
            }
            
            if(Bid <= magic1Counter.firstBuyOrderPrice)
            {
                if(magic1Counter.sellOrderCount == 0 && Bid <= magic1Counter.firstBuyOrderPrice - nextSellOrderIntervalPoint * Point && sellSignal)
                {
                    openSell(openLots, 0,0,orderComment,magicNumber);
                    return;
                }else if(Bid <= magic1Counter.lastSellOrderPrice - nextSellOrderIntervalPoint*Point)
                {
                    double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
                    if(magic1Counter.buyOrderCount == 0)
                    {
                        lots = openLots;
                    }
                    openSell(lots,0,0,orderComment,magicNumber);
                    return;
                }
            }
        }
        
        if(magic1Counter.firstOrderType == OP_SELL)
        {
            if(magic1Counter.sellOrderCount == 0 && sellSignal)
            {
                openSell(openLots,0,0,orderComment,magicNumber);
                firstSellOrderLots = openLots;
                return;
            }
            
            if(Bid <= magic1Counter.lastSellOrderPrice- nextSellOrderIntervalPoint * Point)
            {
                double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
                if(magic1Counter.buyOrderCount == 0)
                {
                    lots = openLots;
                }
                openSell(lots,0,0,orderComment,magicNumber);
                return;
            }
            
            if(Ask >= magic1Counter.firstSellOrderPrice)
            {
                if(magic1Counter.buyOrderCount == 0 && Ask >= magic1Counter.firstSellOrderPrice + nextBuyOrderIntervalPoint * Point && buySignal)
                {
                    openBuy(openLots,0,0,orderComment,magicNumber);
                    return;
                }else if(magic1Counter.buyOrderCount > 0 && Ask >= magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint * Point)
                {
                    double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                    if(magic1Counter.sellOrderCount == 0)
                    {
                        lots = openLots;
                    }
                    openBuy(lots,0,0,orderComment,magicNumber);
                    return;
                }
            }
        }
    }
}

// 根据止盈金额进行消单
// 消单逻辑分成两种：1.用最盈利的单子去平仓最亏损的单子。2.用最大手数的盈利单子去平仓另外一个方向最大手数的单子。
// 震荡行情的时候 采用 1 方案，趋势行情采用 2 方案；
void dismissProfit(Counter & counter)
{
    // 识别市场条件
    string marketCondition = IdentifyMarketCondition();

    // 执行相应的交易策略
    if (marketCondition == "trending")
    {
        // 方案2：用最大手数的单子对冲
        // 找到最大手数的多单和空单并对冲它们
        HedgeLargestLotOrders(counter);
    }
    else if (marketCondition == "range")
    {
        // 方案 1
        // 找到最盈利和最亏损的订单并对冲它们
        HedgeMostProfitableAndMostLosingOrders(counter);
    }
}

// 方案 1
// 找到最盈利和最亏损的订单并对冲它们
void HedgeMostProfitableAndMostLosingOrders(Counter & counter)
{
    int totalOrders = OrdersTotal();
    
    // 如果订单数量少于2，不执行对冲
    if (totalOrders < 2)
    {
        return;
    }
    
    // 如果只有一个方向的单子，不执行对冲
    if (counter.sellOrderCount == 0 || counter.buyOrderCount == 0)
    {
        return;
    }

    int mostProfitableOrderIndex = -1;
    int mostLosingOrderIndex = -1;
    double maxProfit = 0;
    double maxLoss = 0;

    // 找到最盈利和最亏损的订单
    for (int i = 0; i < totalOrders; i++)
    {
        if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES))
        {
            double profit = OrderProfit() + OrderSwap() + OrderCommission();

            if (profit > maxProfit)
            {
                maxProfit = profit;
                mostProfitableOrderIndex = i;
            }
            if (profit < maxLoss)
            {
                maxLoss = profit;
                mostLosingOrderIndex = i;
            }
        }
    }

    // 计算最盈利和最亏损订单的总利润
    double totalProfit = maxProfit + maxLoss;
    Print("计算最盈利",maxProfit);
    Print("计算最亏损",maxLoss);
    Print("计算最盈利和最亏损订单的总利润",totalProfit);
     
    if (mostProfitableOrderIndex != -1 && mostLosingOrderIndex != -1 && totalProfit >= dismissProfit)
    {
        OrderSelect(mostProfitableOrderIndex, SELECT_BY_POS, MODE_TRADES);
        int ticket1 = OrderTicket();
        double lots1 = OrderLots();
        int type1 = OrderType();

        OrderSelect(mostLosingOrderIndex, SELECT_BY_POS, MODE_TRADES);
        int ticket2 = OrderTicket();
        double lots2 = OrderLots();
        int type2 = OrderType();

        // 平仓最盈利和最亏损的订单
        if (OrderClose(ticket1, lots1, OrderClosePrice(), 300,Violet) && OrderClose(ticket2, lots2, OrderClosePrice(), 300, Violet))
        {
            Print("平仓最盈利的订单 (ticket ", ticket1, ") 和最亏损的订单 (ticket ", ticket2, ")");
        }
        else
        {
            Print("对冲失败");
        }
    }
    else
    {
        Print("暂时没有需要对冲的单子");
    }
}

// 方案2：用最大手数的单子对冲
// 找到最大手数的多单和空单并对冲它们
void HedgeLargestLotOrders(Counter & counter)
{
    int totalOrders = OrdersTotal();

    // 如果订单数量少于2，不执行对冲
    if (totalOrders < 2)
    {
        return;
    }
    
    // 如果只有一个方向的单子，不执行对冲
    if (counter.sellOrderCount == 0 || counter.buyOrderCount == 0)
    {
        return;
    }

    int largestLongOrderIndex = -1;
    int largestShortOrderIndex = -1;
    double largestLongLot = 0;
    double largestShortLot = 0;

    // 找到最大手数的多单和空单
    for (int i = 0; i < totalOrders; i++)
    {
        if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES))
        {
            double lots = OrderLots();
            int type = OrderType();

            if (type == OP_BUY && lots > largestLongLot)
            {
                largestLongLot = lots;
                largestLongOrderIndex = i;
            }
            if (type == OP_SELL && lots > largestShortLot)
            {
                largestShortLot = lots;
                largestShortOrderIndex = i;
            }
        }
    }

    // 检查是否找到有效的多单和空单
    if (largestLongOrderIndex != -1 && largestShortOrderIndex != -1)
    {
        OrderSelect(largestLongOrderIndex, SELECT_BY_POS, MODE_TRADES);
        int longTicket = OrderTicket();
        double longLots = OrderLots();
        double longProfit = OrderProfit();

        OrderSelect(largestShortOrderIndex, SELECT_BY_POS, MODE_TRADES);
        int shortTicket = OrderTicket();
        double shortLots = OrderLots();
        double shortProfit = OrderProfit();

        // 检查利润是否满足条件
        if (longProfit + shortProfit >= dismissProfit)
        {
            // 平仓最大手数的多单和空单
            if (OrderClose(longTicket, longLots, OrderClosePrice(), 300, Violet) && OrderClose(shortTicket, shortLots, OrderClosePrice(), 300, Violet))
            {
                Print("平仓最大手数的多单 (ticket ", longTicket, ") 和空单(ticket ", shortTicket, ")");
            }
            else
            {
                Print("最大手数平仓失败");
            }
        }
        else
        {
            Print("利润不满足条件: longProfit + shortProfit = ", longProfit + shortProfit);
        }
    }
    else
    {
        Print("没有需要平仓的单子");
    }
}

void TrailingPositions(Counter & magic1Counter, int magicma)
{
    if(!isOpenTrailingTakeProfit)
    {
        return;
    }
    int ordersTotal = OrdersTotal();
    double minstoplevel=MarketInfo(Symbol(),MODE_STOPLEVEL);
    int CTrailingStop1 = danbianCTrailingStop1;
    int CTrailingStep2 = danbianCTrailingStep2;
    if(magic1Counter.buyOrderCount > 0 && magic1Counter.sellOrderCount > 0)
    {
        CTrailingStop1 = jietaoCTrailingStop1;
        CTrailingStep2 = jietaoCTrailingStep2;
    }
    while(ordersTotal > 0)
    {
        ordersTotal--;
        //如果 仓单编号不符合，或者 选中仓单失败，跳过
        if(!OrderSelect(ordersTotal, SELECT_BY_POS, MODE_TRADES) || OrderMagicNumber() != magicma)
            continue;
        if(OrderType() == OP_BUY)
        {
            if(NormalizeDouble(Bid - OrderOpenPrice(), Digits) > NormalizeDouble(Point * (CTrailingStop1 + CTrailingStep2), Digits))
                if(NormalizeDouble(OrderStopLoss(), Digits) < NormalizeDouble(Bid - Point * CTrailingStep2, Digits) || OrderStopLoss() == 0.0)
                {
                    double StopLoss = NormalizeDouble(Bid-Point*CTrailingStep2,Digits);
                    if(Bid - StopLoss < StopLevel * Point)
                        StopLoss = Bid - StopLevel * Point;
                    bool res=OrderModify(OrderTicket(),OrderOpenPrice(),StopLoss,OrderTakeProfit(),0,Blue);
                    if(!res)
                        Alert("OrderModify buy error:",GetLastError(),"bid:",Bid);
                    else
                        Print("Order modified successfully.");
                }
        }
        
        if(OrderType() == OP_SELL)
        {
            if(NormalizeDouble(OrderOpenPrice() - Ask, Digits) > NormalizeDouble(Point * (CTrailingStop1 + CTrailingStep2), Digits))
                if(NormalizeDouble(OrderStopLoss(), Digits) > NormalizeDouble(Ask + Point * CTrailingStep2, Digits) || OrderStopLoss() == 0.0)
                {
                    double StopLoss =  NormalizeDouble(Ask + Point * CTrailingStep2, Digits);
                    
                    if(StopLoss - Ask < StopLevel * Point)
                    {
                        StopLoss = Ask + StopLevel * Point;
                    }
                    
                    bool res= OrderModify(OrderTicket(), OrderOpenPrice(), StopLoss, OrderTakeProfit(), 0, Orange);
                    if(!res)
                        Alert("OrderModify sell error:",GetLastError(),"ask:",Ask);
                    else
                        Print("Order modified successfully.");
                }
        }
    }
}

void checkPrice(double &Entry, double &StopLoss, double &TakeProfit)
{
    if(Entry - Ask < StopLevel * _Point && Entry > 0)
        Entry = Ask + StopLevel * _Point;
    if(Entry - StopLoss < StopLevel * _Point && StopLoss > 0)
        StopLoss = Entry - StopLevel * _Point;
    if(TakeProfit - Entry < StopLevel * _Point && TakeProfit > 0)
        TakeProfit = Entry + StopLevel * _Point;
}

bool closeAllOrdersByMagicNumber(int magicNumber)
{
    closeBuyOrder(magicNumber);
    closeSellOrder(magicNumber);
    return true;
}

bool closeBuyOrder(int magic1)
{
    bool closebuy=0;
    {
        int t=OrdersTotal();
        for(int i=t-1; i>=0; i--)
        {
            if(OrderSelect(i,SELECT_BY_POS,MODE_TRADES)==true)
            {
                if(OrderSymbol()==Symbol() && OrderType()==OP_BUY && OrderMagicNumber()==magic1)
                {
                    if(OrderClose(OrderTicket(),OrderLots(),OrderClosePrice(),300,Green)==true)
                    {
                        Print(magic1);
                    }
                }
            }
        }
    }
    return(closebuy);
}

bool closeSellOrder(int magic1)
{
    bool closesell=0;
    int t=OrdersTotal();
    for(int i=t-1; i>=0; i--)
    {
        if(OrderSelect(i,SELECT_BY_POS,MODE_TRADES)==true)
        {
            if(OrderSymbol()==Symbol() && OrderType()==OP_SELL && OrderMagicNumber()==magic1)
            {
                if(OrderClose(OrderTicket(),OrderLots(),OrderClosePrice(),300,Green)==true)
                {
                    Print(magic1);
                }
            }
        }
    }
    return(closesell);
}

int openBuy(double lots,double sl,double tp,string com,int buymagic)
{
    int a=0;
    a=OrderSend(Symbol(),OP_BUY,lots,Ask,20,sl,tp,com,buymagic,0,White);
    return(a);
}

int openBuyStop(double lots,double buyStop, double sl,double tp,string com,int buymagic)
{
    int a=0;
    checkPrice(buyStop, sl, tp);
    a=OrderSend(Symbol(),OP_BUYSTOP,lots,buyStop,20,sl,tp,com,buymagic,0,White);
    Print(GetLastError());
    return(a);
}

int openSell(double lots,double sl,double tp,string com,int sellmagic)
{
    int a=0;
    a=OrderSend(Symbol(),OP_SELL,lots,Bid,20,sl,tp,com,sellmagic,0,Red);
    return(a);
}

int openSellStop(double lots,double sellStop, double sl,double tp,string com,int sellmagic)
{
    int a=0;
    checkPrice(sellStop, sl, tp);
    a=OrderSend(Symbol(),OP_SELLSTOP,lots,sellStop,20,sl,tp,com,sellmagic,0,Red);
    Print(GetLastError());
    return(a);
}

bool IsSpreadWithinThreshold()
{
    bool isWithinThreshold = false;
    if(Ask - Bid < baseInterval * Point)
    {
        isWithinThreshold = true;
    }
    return(isWithinThreshold);
}

double CalcNextOrderLots(const Counter & counter, int openOrderType)
{
    double nextOrderLots = openLots;
    if(openOrderType == OP_BUY && counter.buyOrderCount > 0)
    {
        nextOrderLots = counter.firstBuyOrderLots;
        for(int i = 1; i < counter.buyOrderCount; i++)
        {
            nextOrderLots = nextOrderLots * positionMultiplier;
        }
    }
    if(openOrderType == OP_SELL && counter.sellOrderCount > 0)
    {
        nextOrderLots = counter.firstSellOrderLots;
        for(int i = 1; i < counter.sellOrderCount; i++)
        {
            nextOrderLots = nextOrderLots * positionMultiplier;
        }
    }
    nextOrderLots = NormalizeDouble(nextOrderLots, 2);
    if(nextOrderLots < 0.01)
    {
        nextOrderLots = 0.01;
    }
    if(nextOrderLots > maxOrderLots)
    {
        nextOrderLots = maxOrderLots;
    }
    return (nextOrderLots);
}

//+------------------------------------------------------------------+
//+------------------------------------------------------------------+
//| 统计订单参数                                                   |
//+------------------------------------------------------------------+
Counter CalcTotal(int magicma)
{
    
    Counter counter = {};
    counter.firstOrderType = -1;
    counter.lastHistoryOrderType = -1;
    
    int ordersTotal = OrdersTotal();
    for(int i = 0; i < ordersTotal; i++)
    {
        //如果 仓单编号不符合，或者 选中仓单失败，跳过
        if(!OrderSelect(i, SELECT_BY_POS, MODE_TRADES) || OrderMagicNumber() != magicma)
            continue;
        
        //多单
        if(OrderType() == OP_BUY)
        {
            counter.totalOrderCount++;
            if(counter.totalOrderCount == 1)
            {
                counter.firstOrderType = OrderType();
            }
            //统计利润
            counter.totalProfit += (OrderProfit() + OrderCommission() + OrderSwap());//这里减去手续费
            // 计数
            counter.buyOrderCount++;
            //统计利润
            counter.buyTotalProfit += (OrderProfit() + OrderCommission() + OrderSwap());//这里减去手续费
            counter.buyTotalLots  += OrderLots();
            if(counter.buyOrderCount == 1)
            {
                counter.firstBuyOrderPrice = OrderOpenPrice();
                counter.firstBuyOrderLots = OrderLots();
                counter.firstBuyOrderProfit = OrderProfit() + OrderCommission() + OrderSwap();
                counter.firstBuyOrderStopless = OrderStopLoss();
                counter.firstBuyOrderTakeProfit = OrderTakeProfit();
            }
            counter.lastBuyOrderPrice = OrderOpenPrice();
            counter.lastBuyOrderLots = OrderLots();
            if(counter.lastBuyOrderPrice > counter.firstBuyOrderPrice)
            {
                counter.orderCountUpFirstBuyOrder++;
                counter.hightBuyOrderPrice = counter.lastBuyOrderPrice;
            }else
            {
                counter.lowBuyOrderPrice = counter.lastBuyOrderPrice;
                counter.orderCountLowFirstBuyOrder++;
            }
        }else
            if(OrderType() == OP_BUYSTOP)
            {
                counter.lastBuyStop = OrderOpenPrice();
            }
        //空单
            else if(OrderType() == OP_SELL)
            {
                counter.totalOrderCount++;
                if(counter.totalOrderCount == 1)
                {
                    counter.firstOrderType = OrderType();
                }
                //统计利润
                counter.totalProfit += (OrderProfit() + OrderCommission() + OrderSwap());//这里减去手续费
                // 计数
                counter.sellOrderCount++;
                //统计利润
                counter.sellTotalProfit += (OrderProfit() + OrderCommission() + OrderSwap());//这里减去手续费
                counter.sellTotalLots  +=OrderLots();
                if(counter.sellOrderCount == 1)
                {
                    counter.firstSellOrderPrice = OrderOpenPrice();
                    counter.firstSellOrderLots = OrderLots();
                    counter.firstSellOrderProfit = OrderProfit() + OrderCommission() + OrderSwap();
                    counter.firstSellOrderStopless = OrderStopLoss();
                    counter.firstSellOrderTakeProfit = OrderTakeProfit();
                }
                counter.lastSellOrderPrice = OrderOpenPrice();
                counter.lastSellOrderLots = OrderLots();
                if(counter.lastSellOrderPrice > counter.firstSellOrderPrice)
                {
                    counter.orderCountUpFirstSellOrder++;
                    counter.hightSellOrderPrice = counter.lastSellOrderPrice;
                }
                else
                {
                    counter.orderCountLowFirstSellOrder++;
                    counter.lowSellOrderPrice = counter.lastSellOrderPrice;
                }
            }
            else
                if(OrderType() == OP_SELLSTOP)
                {
                    counter.lastSellStop = OrderOpenPrice();
                }
    }
    
    int ordersHistoryIndex = OrdersHistoryTotal();
    
    if(ordersHistoryIndex > 0)
    {
        while(ordersHistoryIndex > 0)
        {
            ordersHistoryIndex--;
            if(!OrderSelect(ordersHistoryIndex, SELECT_BY_POS,MODE_HISTORY) || OrderMagicNumber() != magicma)
            {
                continue;
            }
            if(OrderType() == OP_SELL)
            {
                counter.lastHistoryOrderType = OP_SELL;
                break;
            }
            if(OrderType() == OP_BUY)
            {
                counter.lastHistoryOrderType = OP_BUY;
                break;
            }
        }
    }
    if(counter.lastHistoryOrderType != -1)
    {
        if(counter.totalOrderCount == 0)
        {
            counter.firstOrderType = counter.lastHistoryOrderType;
        }
        
        if(counter.buyOrderCount == 0
           && counter.sellOrderCount > 0
           && Ask > counter.firstSellOrderPrice
           && counter.lastHistoryOrderType == OP_BUY)
        {
            counter.firstOrderType = OP_BUY;
        }
        
        if(counter.sellOrderCount == 0
           && counter.buyOrderCount > 0
           && Bid < counter.firstBuyOrderPrice
           && counter.lastHistoryOrderType == OP_SELL)
        {
            counter.firstOrderType = OP_SELL;
        }
    }
    return counter;
}

void initSubViews()
{
    createButtons();
    createInfoBoard();
}

void OnChartEvent(const int id,
                  const long &lparam,
                  const double &dparam,
                  const string &sparam)
{
    if(sparam == "ping_duo")
    {
        closeBuyOrder(magicNumber);
    }
    if(sparam == "ping_kong")
    {
        closeSellOrder(magicNumber);
    }
    
    if(sparam == "quan_ping")
    {
        closeBuyOrder(magicNumber);
        closeSellOrder(magicNumber);
    }
}

/**
 信息面板
 */
void createInfoBoard()
{
    
}

void createButtons()
{
    long x_distance;
    long y_distance;
    if(!ChartGetInteger(0,CHART_WIDTH_IN_PIXELS,0,x_distance))
    {
        Print("Failed to get the chart width! Error code = ",GetLastError());
        return;
    }
    if(!ChartGetInteger(0,CHART_HEIGHT_IN_PIXELS,0,y_distance))
    {
        Print("Failed to get the chart height! Error code = ",GetLastError());
        return;
    }
    int x_step=(int)x_distance/32;
    int y_step=(int)y_distance/32;
    int x=(int)x_distance/32;
    int y=(int)y_distance/32;
    int x_size=80;
    int y_size=30;
    
    //平多
    createButton(0,"ping_duo", 0,x,y,x_size,y_size,"平多");
    //平空
    createButton(0,"ping_kong", 0,x,y+y_size+10,x_size,y_size,"平空");
    //全平
    createButton(0,"quan_ping", 0,x,y+y_size+y_size+20,x_size,y_size,"全平");
}

void createButton(const long    chart_ID=0,               // chart's ID
                  const string  name="Button",
                  const int     sub_window=0,
                  const int     x=0,                      // X coordinate
                  const int     y=0,                      // Y coordinate
                  const int     width=50,                 // button width
                  const int     height=18,                // button height
                  const string  text="Button" )
{
    ObjectDelete(chart_ID,name);
    if(!ObjectCreate(chart_ID,name,OBJ_BUTTON,sub_window,0,0))
    {
        Alert(": failed to create the button! Error code = ",GetLastError());
        return;
    }
    
    //--- set button coordinates
    ObjectSetInteger(chart_ID,name,OBJPROP_XDISTANCE,x);
    ObjectSetInteger(chart_ID,name,OBJPROP_YDISTANCE,y);
    //--- set button size
    ObjectSetInteger(chart_ID,name,OBJPROP_XSIZE,width);
    ObjectSetInteger(chart_ID,name,OBJPROP_YSIZE,height);
    ObjectSetString(chart_ID,name,OBJPROP_TEXT,text);
    ObjectSetInteger(chart_ID,name,OBJPROP_COLOR,clrBlack);
    ObjectSetInteger(chart_ID,name,OBJPROP_FONTSIZE,14);
}

void updateAccountInfo(const Counter & counter)
{
    if(TimeSeconds(TimeCurrent())%5 != 0)
    {
        return;
    }
    double balance = AccountBalance();//账户余额
    double credit = AccountCredit();//账户信用额度
    double equity = AccountEquity();//账户净值
    double freeMargin = AccountFreeMargin();//账户自由保证金
    double margin = AccountMargin();//账户已用保证金
    double profit = AccountProfit();//账户盈利金额
    string msg =  "                                                              账户余额:"+DoubleToStr(balance,2)+"\n"+
    "                                                               账户信用额度:"+DoubleToStr(credit, 2)+"\n"+
    "                                                               账户净值:"+DoubleToStr(equity, 2)+"\n"+
    "                                                               账户自由保证金:"+DoubleToStr(freeMargin, 2)+"\n"+
    "                                                               账户已用保证金:"+DoubleToStr(margin, 2)+"\n"+
    "                                                               账户盈利金额:"+DoubleToStr(profit, 2)+"\n"+
    "                                                               空单个数和手数:"+DoubleToStr(counter.sellOrderCount,0)+",  "+
    DoubleToStr(counter.sellTotalLots,2)+"手,  "+
    "盈亏:"+DoubleToStr(counter.sellTotalProfit,2)+"\n"+
    "                                                               多单个数和手数:"+DoubleToStr(counter.buyOrderCount,2)+",  "+
    DoubleToStr(counter.buyTotalLots,2)+"手,  "+
    "盈亏:"+DoubleToStr(counter.buyTotalProfit,2)+"\n" +
    "                                                                Version: 2024-6-18 09:55";
    
    Comment(msg);
    
}

void OnDeinit(const int reason)
{}


