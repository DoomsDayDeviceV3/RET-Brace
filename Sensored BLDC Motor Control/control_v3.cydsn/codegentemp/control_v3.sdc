# THIS FILE IS AUTOMATICALLY GENERATED
# Project: D:\Sensored BLDC Motor Control\control_v3.cydsn\control_v3.cyprj
# Date: Thu, 27 Mar 2025 09:28:32 GMT
#set_units -time ns
create_clock -name {ADC_SAR_Seq_1_intClock(FFB)} -period 62.5 -waveform {0 31.25} [list [get_pins {ClockBlock/ff_div_7}]]
create_clock -name {clock_1(FFB)} -period 208.33333333333331 -waveform {0 104.166666666667} [list [get_pins {ClockBlock/ff_div_8}]]
create_clock -name {UART_BCP_SCBCLK(FFB)} -period 729.16666666666663 -waveform {0 364.583333333333} [list [get_pins {ClockBlock/ff_div_2}]]
create_clock -name {clock_2(FFB)} -period 5000 -waveform {0 2500} [list [get_pins {ClockBlock/ff_div_9}]]
create_clock -name {CyRouted1} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/dsi_in_0}]]
create_clock -name {CyILO} -period 31250 -waveform {0 15625} [list [get_pins {ClockBlock/ilo}]]
create_clock -name {CyLFCLK} -period 31250 -waveform {0 15625} [list [get_pins {ClockBlock/lfclk}]]
create_clock -name {CyIMO} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/imo}]]
create_clock -name {CyHFCLK} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/hfclk}]]
create_clock -name {CySYSCLK} -period 20.833333333333332 -waveform {0 10.4166666666667} [list [get_pins {ClockBlock/sysclk}]]
create_generated_clock -name {ADC_SAR_Seq_1_intClock} -source [get_pins {ClockBlock/hfclk}] -edges {1 3 7} [list]
create_generated_clock -name {clock_1} -source [get_pins {ClockBlock/hfclk}] -edges {1 11 21} -nominal_period 208.33333333333331 [list]
create_generated_clock -name {UART_BCP_SCBCLK} -source [get_pins {ClockBlock/hfclk}] -edges {1 35 71} [list]
create_generated_clock -name {clock_2} -source [get_pins {ClockBlock/hfclk}] -edges {1 241 481} [list [get_pins {ClockBlock/udb_div_0}]]

set_false_path -from [get_pins {__ONE__/q}]

# Component constraints for D:\Sensored BLDC Motor Control\control_v3.cydsn\TopDesign\TopDesign.cysch
# Project: D:\Sensored BLDC Motor Control\control_v3.cydsn\control_v3.cyprj
# Date: Thu, 27 Mar 2025 09:28:25 GMT
