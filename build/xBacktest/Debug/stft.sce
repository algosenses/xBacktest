<!-- the scenario configuration file -->
<?xml version="1.0" encoding="UTF-8"?>
<scenario>
    <!-- ============================================
          Environment configuration
         ============================================ -->
    <environment>
        <core num="autodetect" />
        <optimizing mode="genetic" />
    </environment>
    <!-- ============================================
          Broker 
         ============================================ -->
    <broker type="backtesting">
        <cash>1000000</cash>
        <iop>false</iop>
    </broker>
    <!-- ============================================
          Data Feed 
         ============================================ -->
    <datafeed>
        <symbol name="RU1501" resolution="minute" realtime="false">
            <source type="file" location="..\\ru1501.csv" format="csv" /> 
        </symbol>
    </datafeed>
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>STFT</name>
        <description>STFT model</description>
        <author>zhuangshaobo</author>
        <entry>example.dll</entry>
        <mode>standalone</mode>
        <maxbarsback>300</maxbarsback>
        <parameters>
            <param name="LongMaLength" type="int" value="16">
            </param>
            <param name="ShortMaLength" type="int" value="15">
            </param>
            <param name="SlidingWndLength" type="int" value="225">
            </param>
            <param name="upperBandRatio" type="double" value="0.0025">
            </param>
            <param name="LowerBandRatio" type="double" value="0.0025">
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
