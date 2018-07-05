<!-- the scenario configuration file -->
<?xml version="1.0" encoding="UTF-8"?>
<scenario>
    <!-- ============================================
          Machine configuration
         ============================================ -->
    <machine>
        <core num="autodetect" />
    </machine>
    <!-- ============================================
          Broker 
         ============================================ -->
    <broker type="backtesting">
        <cash>1000000</cash>
    </broker>
    <!-- ============================================
          Data feed, the first symbol is the default
          instrument use to feed strategy
         ============================================ -->
    <datafeed>
        <symbol name="RU888" resolution="minute" realtime="false">
            <source type="file" location="..\\RU888.ts" format="bin" />
            <contract>
                <multiple>300</multiple>
                <pricetick>0.2</pricetick>
                <commission type="percentage" value="0.000026" />
            </contract>
        </symbol>
    </datafeed>
    <!-- ============================================
          Strategies 
         ============================================ -->
    <strategy>
        <name>AMP</name>
        <description>AMP model</description>
        <author>zhuangshaobo</author>
        <entry>amp.dll</entry>
        <mode>standalone</mode>
        <parameters>
            <param name="upper" type="double" value="0.6">
<!--                <optimizing start="0.1" end="1.0" step="0.1" /> -->
            </param>
            <param name="lower" type="double" value="0.4">
<!--                <optimizing start="0.1" end="1.0" step="0.1" /> -->
            </param>
        </parameters>
    </strategy>
    <!-- ============================================
          Analysis report
         ============================================ --> 
    <report>
        <summary>
          <file location="summary-amp.txt" />
        </summary>
        <trade>
            <file location="trades-amp.csv" />
        </trade>
        <!--
        <return>
            <file location="returns.csv" />
        </return>
        -->
    </report>
</scenario>
