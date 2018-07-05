<!-- the scenario configuration file -->
<?xml version="1.0" encoding="UTF-8"?>
<scenario>
    <!-- ============================================
          Environment configuration
         ============================================ -->
    <environment>
        <core num="autodetect" />
        <optimizing mode="exhaustive" />
        <!-- optimizing mode="genetic" -->
    </environment>
    <!-- ============================================
          Broker 
         ============================================ -->
    <broker type="backtesting">
        <cash>10000000</cash>
    </broker>
    <!-- ============================================
          Data Stream 
         ============================================ -->
    <datastream>
        <stream name="rb000" resolution="minute">
            <source="e:/Workshop/YuanLang/Data/HistoricalData/Wind/WindSync/rb000.bin" format="csv" />
            <contract>
                <multiplier>10</multiplier>
                <priceunit>1</priceunit>
                <marginratio>0.07</marginratio>
                <commissiontype>TRADE_PERCENTAGE</commissiontype>
                <commission>0.000027</commission>
                <slippagetype>TRADE_PERCENTAGE</slippagetype>
                <slippage>0.000173</slippage>
            </contract>
        </stream>
    </datastream>
    
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>smacross</name>
        <description>SMA cross model</description>
        <author>zhuangshaobo</author>
        <entry>Strategies.dll</entry>
        <mode>standalone</mode>
        <parameters>
            <param name="period" type="int" value="20">
<!--                <optimizing start="1" end="150" step="1" /> -->
            </param>
            <param name="StopLossPct" type="double" value="0.01">
<!--                <optimizing start="0.01" end="0.99" step="0.01" /> -->
            </param>
            <param name="StopProfitPct" type="double" value="0.17">
<!--                <optimizing start="0.01" end="0.99" step="0.01" /> -->
            </param>
            <param name="DrawdownPct" type="double" value="0.01">
<!--                <optimizing start="0.01" end="0.99" step="0.01" /> -->
            </param>
        </parameters>
    </strategy>
    
    <!-- ============================================
          Analysis report
         ============================================ -->
    <report>
        <summary>
          <file location="summary.txt" />
        </summary>
        <trade>
            <file location="trades.csv" />
        </trade>
        <return>
            <file location="returns.csv" />
        </return>
    </report>
</scenario>
