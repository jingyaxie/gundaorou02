//+------------------------------------------------------------------+
//|                                                       EA.mq4 |
//|                        Copyright 2021, MetaQuotes Software Corp. |
//|                                             https://www.mql5.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2021, MetaQuotes Software Corp."
#property link      "https://www.mql5.com"
#property version   "1.00"
#property strict

enum dn
  {
   b = 0,//BUY
   s = 1,//SELL
  };
extern dn o_d = 1;//做单方向
input double o_l=0.05;//开仓手数
input int a_n=500;//加仓间隔点数
input double multiple = 1.3;//加仓的倍数
input double maxOrderLots = 2.0;//单个订单最大手数
input int maxOrderLotsPlus = 100;//单个订单达到最大手数后开仓间距增加点数
input double maxStopLoss = 2000;//亏损多大时停止开仓

input int takeprofit_n=1100;//均价止盈点数

input double maxProfit = 5;//整体盈利金额

input string cm1="第一组";//注释1
input int magic_1=666;//模式一魔术码

input bool isOpenTrailingTakeProfit = true;//第一组是否开启移动止损
input string s1 = "单边盈利移动止损参数";
input int danbianCTrailingStop1 = 200;//盈利最小盈利点数
input int danbianCTrailingStep2 = 500;//盈利移动止损移动间隔点数
input string s2 = "解套移动止损参数";
input int jietaoCTrailingStop1 = 1500;//解套最小盈利点数
input int jietaoCTrailingStep2 = 1500;//解套移动止损移动间隔点数
input bool isOpenIEMA = false;//是否开启均线辅助开仓
input int OverBuyValue = 85;//Stochastic超买值
input int OverSellValue = 15;//Stochastic超卖值
enum iMAEnum
  {
   M1 = PERIOD_M1,//一分钟
   M5 = PERIOD_M5,//五分钟
   M15 = PERIOD_M15,//十五分钟
   M30 = PERIOD_M30,//十五分钟
   H1 = PERIOD_H1,//一小时
   H4 = PERIOD_H4,//四小时
  };

extern iMAEnum iMAPERIOD = PERIOD_M5;//均线周期
input int slow_k_num = 12;//快线k线历史个数

datetime bt=0;
datetime st=0;
datetime bt2=0;
datetime st2=0;

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



  };

bool f1=false;//第一模式首单是否开过

datetime t1=0;
datetime t2=0;
datetime t3=0;
int StopLevel;
int spread;
int baseInterval;


//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit()
  {
   createButtons();

   StopLevel = MarketInfo(Symbol(), MODE_STOPLEVEL);
   Print("StopLevel = ", StopLevel);
   baseInterval = a_n;
   return(INIT_SUCCEEDED);
  }


//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
//---

  }
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick()
  {

   if(!IsConnected() && t1 !=Time[0])
     {
      Print("No connection!");
      Alert("No connection!");
      t1=Time[0];
      return;
     }
   if(!IsExpertEnabled() && t2 !=Time[0])
     {
      Print("No ExpertEnabled!");
      Alert("No ExpertEnabled!");
      t2=Time[0];
      return;
     }
   if(!IsTradeAllowed() && t3 !=Time[0])
     {
      Print("No TradeAllowed!");
      Alert("No TradeAllowed!");
      t3=Time[0];
      return;
     }

   spread  = SymbolInfoInteger(Symbol(),SYMBOL_SPREAD);
   baseInterval = a_n;
   Counter magic1Counter = CalcTotal(magic_1);
   updateAccountInfo(magic1Counter);

//检查模式1是否需要开首单buy
   if(o_d==0 && f1==false  && magic1Counter.totalOrderCount==0)
     {
      if(bt!=Time[0])
        {
         f1=true;
         openBuy(o_l,0,0,cm1,magic_1);
         bt=Time[0];
         return;
        }
     }
//检查模式1是否需要开首单sell
   if(o_d==1 && f1==false && magic1Counter.totalOrderCount==0)
     {
      if(st!=Time[0])
        {
         f1=true;
         openSell(o_l,0,0,cm1,magic_1);
         st=Time[0];
         return;
        }
     }

   if((magic1Counter.totalProfit) > maxProfit)
     {
      closeBuyOrder(magic_1);
      closeSellOrder(magic_1);
      if(magic1Counter.buyTotalProfit > 0)
        {
         openBuy(o_l,0,0,cm1,magic_1);
         return;
        }
      else
         if(magic1Counter.sellTotalProfit > 0)
           {
            openSell(o_l,0,0,cm1,magic_1);
            return;
           }
     }
   if((magic1Counter.totalProfit + magic1Counter.totalProfit) > -maxStopLoss)
     {
      openStrategy1(magic1Counter);
     }

   if(isOpenTrailingTakeProfit)
     {
      TrailingPositions(magic1Counter, magic_1);
     }
  }

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void openStrategy0(Counter & magic1Counter)
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
   if(o_q()==true)
     {
      int nextBuyOrderIntervalPoint = baseInterval;
      int nextSellOrderIntervalPoint = baseInterval;
      if(magic1Counter.lastBuyOrderLots > maxOrderLots)
        {
         nextBuyOrderIntervalPoint += maxOrderLotsPlus;
        }
      if(magic1Counter.lastSellOrderLots > maxOrderLots)
        {
         nextSellOrderIntervalPoint += maxOrderLotsPlus;
        }

      if(buySignal)
        {

         if(magic1Counter.buyOrderCount == 0)
           {
            openBuy(o_l, 0,0,cm1,magic_1);
            firstBuyOrderLots = o_l;
            return;
           }

         if(Ask>=magic1Counter.hightBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
           {
            double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
            if(magic1Counter.sellOrderCount == 0)
              {
               lots = o_l;
              }
            openBuy(lots,0,0,cm1,magic_1);
            return;
           }


        }

      if(magic1Counter.firstOrderType == OP_BUY)
        {
         if(magic1Counter.buyOrderCount == 0 && buySignal)
           {
            openBuy(o_l, 0,0,cm1,magic_1);
            firstBuyOrderLots = o_l;
            return;
           }
         if(Ask>=magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
           {
            double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
            if(magic1Counter.sellOrderCount == 0)
              {
               lots = o_l;
              }
            openBuy(lots,0,0,cm1,magic_1);
            return;
           }
         if(Bid<=magic1Counter.firstBuyOrderPrice)
           {
            if(magic1Counter.sellOrderCount == 0 && Bid<=magic1Counter.firstBuyOrderPrice- nextSellOrderIntervalPoint*Point && sellSignal)
              {
               openSell(o_l, 0,0,cm1,magic_1);
               return;
              }
            else
               if(Bid <= magic1Counter.lastSellOrderPrice - nextSellOrderIntervalPoint*Point)
                 {
                  double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
                  if(magic1Counter.buyOrderCount == 0)
                    {
                     lots = o_l;
                    }
                  openSell(lots,0,0,cm1,magic_1);
                  return;
                 }
           }
        }

      if(magic1Counter.firstOrderType==OP_SELL)
        {
         if(magic1Counter.sellOrderCount == 0 && sellSignal)
           {
            openSell(o_l,0,0,cm1,magic_1);
            firstSellOrderLots = o_l;
            return;
           }
         if(Bid<=magic1Counter.lastSellOrderPrice- nextSellOrderIntervalPoint*Point)
           {
            double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
            if(magic1Counter.buyOrderCount == 0)
              {
               lots = o_l;
              }
            openSell(lots,0,0,cm1,magic_1);
            return;
           }

         if(Ask >= magic1Counter.firstSellOrderPrice)
           {
            if(magic1Counter.buyOrderCount == 0 && Ask >= magic1Counter.firstSellOrderPrice + nextBuyOrderIntervalPoint*Point && buySignal)
              {
               openBuy(o_l,0,0,cm1,magic_1);
               return;
              }
            else
               if(magic1Counter.buyOrderCount > 0 && Ask >= magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
                 {
                  double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                  if(magic1Counter.sellOrderCount == 0)
                    {
                     lots = o_l;
                    }
                  openBuy(lots,0,0,cm1,magic_1);
                  return;
                 }
           }
        }

     }
  }


//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
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
   if(o_q()==true)
     {
      int nextBuyOrderIntervalPoint = baseInterval;
      int nextSellOrderIntervalPoint = baseInterval;
      if(magic1Counter.lastBuyOrderLots > maxOrderLots)
        {
         nextBuyOrderIntervalPoint += maxOrderLotsPlus;
        }
      if(magic1Counter.lastSellOrderLots > maxOrderLots)
        {
         nextSellOrderIntervalPoint += maxOrderLotsPlus;
        }
      if(magic1Counter.firstOrderType == OP_BUY)
        {
         if(magic1Counter.buyOrderCount == 0 && buySignal)
           {
            openBuy(o_l, 0,0,cm1,magic_1);
            firstBuyOrderLots = o_l;
            return;
           }
         if(Ask>=magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
           {
            double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
            if(magic1Counter.sellOrderCount == 0)
              {
               lots = o_l;
              }
            openBuy(lots,0,0,cm1,magic_1);
            return;
           }
         if(Bid<=magic1Counter.firstBuyOrderPrice)
           {
            if(magic1Counter.sellOrderCount == 0 && Bid<=magic1Counter.firstBuyOrderPrice- nextSellOrderIntervalPoint*Point && sellSignal)
              {
               openSell(o_l, 0,0,cm1,magic_1);
               return;
              }
            else
               if(Bid <= magic1Counter.lastSellOrderPrice - nextSellOrderIntervalPoint*Point)
                 {
                  double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
                  if(magic1Counter.buyOrderCount == 0)
                    {
                     lots = o_l;
                    }
                  openSell(lots,0,0,cm1,magic_1);
                  return;
                 }
           }
        }

      if(magic1Counter.firstOrderType==OP_SELL)
        {
         if(magic1Counter.sellOrderCount == 0 && sellSignal)
           {
            openSell(o_l,0,0,cm1,magic_1);
            firstSellOrderLots = o_l;
            return;
           }
         if(Bid<=magic1Counter.lastSellOrderPrice- nextSellOrderIntervalPoint*Point)
           {
            double lots = CalcNextOrderLots(magic1Counter, OP_SELL);
            if(magic1Counter.buyOrderCount == 0)
              {
               lots = o_l;
              }
            openSell(lots,0,0,cm1,magic_1);
            return;
           }

         if(Ask >= magic1Counter.firstSellOrderPrice)
           {
            if(magic1Counter.buyOrderCount == 0 && Ask >= magic1Counter.firstSellOrderPrice + nextBuyOrderIntervalPoint*Point && buySignal)
              {
               openBuy(o_l,0,0,cm1,magic_1);
               return;
              }
            else
               if(magic1Counter.buyOrderCount > 0 && Ask >= magic1Counter.lastBuyOrderPrice + nextBuyOrderIntervalPoint*Point)
                 {
                  double lots = CalcNextOrderLots(magic1Counter, OP_BUY);
                  if(magic1Counter.sellOrderCount == 0)
                    {
                     lots = o_l;
                    }
                  openBuy(lots,0,0,cm1,magic_1);
                  return;
                 }
           }
        }

     }
  }
  
  


//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
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


//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void checkPrice(double &Entry, double &StopLoss, double &TakeProfit)
  {
   if(Entry - Ask < StopLevel * _Point && Entry > 0)
      Entry = Ask + StopLevel * _Point;
   if(Entry - StopLoss < StopLevel * _Point && StopLoss > 0)
      StopLoss = Entry - StopLevel * _Point;
   if(TakeProfit - Entry < StopLevel * _Point && TakeProfit > 0)
      TakeProfit = Entry + StopLevel * _Point;
  }

//+------------------------------------------------------------------+
//| ChartEvent function                                              |
//+------------------------------------------------------------------+
void OnChartEvent(const int id,
                  const long &lparam,
                  const double &dparam,
                  const string &sparam)
  {
//---
   if(sparam == "ping_duo")
     {
      closeBuyOrder(magic_1);
     }
   if(sparam == "ping_kong")
     {
      closeSellOrder(magic_1);
     }

   if(sparam == "quan_ping")
     {
      closeBuyOrder(magic_1);
      closeSellOrder(magic_1);
     }
  }
//+------------------------------------------------------------------+
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
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
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

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int openBuy(double lots,double sl,double tp,string com,int buymagic)
  {
   int a=0;
   a=OrderSend(Symbol(),OP_BUY,lots,Ask,20,sl,tp,com,buymagic,0,White);
   return(a);
  }

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int openBuyStop(double lots,double buyStop, double sl,double tp,string com,int buymagic)
  {
   int a=0;
   checkPrice(buyStop, sl, tp);
   a=OrderSend(Symbol(),OP_BUYSTOP,lots,buyStop,20,sl,tp,com,buymagic,0,White);
   Print(GetLastError());
   return(a);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int openSell(double lots,double sl,double tp,string com,int sellmagic)
  {
   int a=0;
   a=OrderSend(Symbol(),OP_SELL,lots,Bid,20,sl,tp,com,sellmagic,0,Red);
   return(a);
  }

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
int openSellStop(double lots,double sellStop, double sl,double tp,string com,int sellmagic)
  {
   int a=0;
   checkPrice(sellStop, sl, tp);
   a=OrderSend(Symbol(),OP_SELLSTOP,lots,sellStop,20,sl,tp,com,sellmagic,0,Red);
   Print(GetLastError());
   return(a);
  }
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
bool o_q()
  {
   bool o_q=false;
   if(Ask-Bid<baseInterval*Point)
     {
      o_q=true;
     }
   return(o_q);
  }
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
double CalcNextOrderLots(const Counter & counter, int openOrderType)
  {
   double nextOrderLots = o_l;
   if(openOrderType == OP_BUY && counter.buyOrderCount > 0)
     {
      nextOrderLots = counter.firstBuyOrderLots;
      for(int i = 1; i < counter.buyOrderCount; i++)
        {
         nextOrderLots = nextOrderLots * multiple;
        }
     }
   if(openOrderType == OP_SELL && counter.sellOrderCount > 0)
     {
      nextOrderLots = counter.firstSellOrderLots;
      for(int i = 1; i < counter.sellOrderCount; i++)
        {
         nextOrderLots = nextOrderLots * multiple;
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
         counter.totalProfit += (OrderProfit()+OrderCommission()+OrderSwap());//这里减去手续费
         // 计数
         counter.buyOrderCount++;
         //统计利润
         counter.buyTotalProfit += (OrderProfit()+OrderCommission()+OrderSwap());//这里减去手续费
         counter.buyTotalLots  += OrderLots();
         if(counter.buyOrderCount == 1)
           {
            counter.firstBuyOrderPrice = OrderOpenPrice();
            counter.firstBuyOrderLots = OrderLots();
            counter.firstBuyOrderProfit = OrderProfit()+OrderCommission()+OrderSwap();
            counter.firstBuyOrderStopless = OrderStopLoss();
            counter.firstBuyOrderTakeProfit = OrderTakeProfit();
           }
         counter.lastBuyOrderPrice = OrderOpenPrice();
         counter.lastBuyOrderLots = OrderLots();
         if(counter.lastBuyOrderPrice > counter.firstBuyOrderPrice)
           {
            counter.orderCountUpFirstBuyOrder++;
            counter.hightBuyOrderPrice = counter.lastBuyOrderPrice;
           }
         else
           {
            counter.lowBuyOrderPrice = counter.lastBuyOrderPrice;
            counter.orderCountLowFirstBuyOrder++;
           }
        }

      else
         if(OrderType() == OP_BUYSTOP)
           {
            counter.lastBuyStop = OrderOpenPrice();
           }


         //空单
         else
            if(OrderType() == OP_SELL)
              {
               counter.totalOrderCount++;
               if(counter.totalOrderCount == 1)
                 {
                  counter.firstOrderType = OrderType();
                 }
               //统计利润
               counter.totalProfit += (OrderProfit()+OrderCommission()+OrderSwap());//这里减去手续费
               // 计数
               counter.sellOrderCount++;
               //统计利润
               counter.sellTotalProfit += (OrderProfit()+OrderCommission()+OrderSwap());//这里减去手续费
               counter.sellTotalLots  +=OrderLots();
               if(counter.sellOrderCount == 1)
                 {
                  counter.firstSellOrderPrice = OrderOpenPrice();
                  counter.firstSellOrderLots = OrderLots();
                  counter.firstSellOrderProfit = OrderProfit()+OrderCommission()+OrderSwap();
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
         if(!OrderSelect(ordersHistoryIndex, SELECT_BY_POS,MODE_HISTORY)
            || OrderMagicNumber() != magicma)
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
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void createButtons()
  {
//--- chart window size
   long x_distance;
   long y_distance;
//--- set window size
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
//--- define the step for changing the button size
   int x_step=(int)x_distance/32;
   int y_step=(int)y_distance/32;
//--- set the button coordinates and its size
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



//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void createButton(const long    chart_ID=0,               // chart's ID
                  const string  name="Button",
                  const int     sub_window=0,
                  const int     x=0,                      // X coordinate
                  const int     y=0,                      // Y coordinate
                  const int     width=50,                 // button width
                  const int     height=18,                // button height
                  const string            text="Button"            // text
                 )
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
//+------------------------------------------------------------------+
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
//|                                                                  |
//+------------------------------------------------------------------+
void updateAccountInfo(const Counter & counter
                      )
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
   string msg =  "                                        账户余额:"+DoubleToStr(balance,2)+"\n"+
                 "                                        账户信用额度:"+DoubleToStr(credit, 2)+"\n"+
                 "                                        账户净值:"+DoubleToStr(equity, 2)+"\n"+
                 "                                        账户自由保证金:"+DoubleToStr(freeMargin, 2)+"\n"+
                 "                                        账户已用保证金:"+DoubleToStr(margin, 2)+"\n"+
                 "                                        账户盈利金额:"+DoubleToStr(profit, 2)+"\n"+
                 "                                        sell:"+DoubleToStr(counter.sellOrderCount,0)+",  "+
                 DoubleToStr(counter.sellTotalLots,2)+"手,  "+
                 "盈亏:"+DoubleToStr(counter.sellTotalProfit,2)+"\n"+
                 "                                        buy:"+DoubleToStr(counter.buyOrderCount,2)+",  "+
                 DoubleToStr(counter.buyTotalLots,2)+"手,  "+
                 "盈亏:"+DoubleToStr(counter.buyTotalProfit,2)+"\n";

   Comment(msg);

  }
//+------------------------------------------------------------------+

//+------------------------------------------------------------------+
