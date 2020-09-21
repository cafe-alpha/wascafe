## Generated SDC file "wasca.out.sdc"

## Copyright (C) 1991-2016 Altera Corporation. All rights reserved.
## Your use of Altera Corporation's design tools, logic functions 
## and other software and tools, and its AMPP partner logic 
## functions, and any output files from any of the foregoing 
## (including device programming or simulation files), and any 
## associated documentation or information are expressly subject 
## to the terms and conditions of the Altera Program License 
## Subscription Agreement, the Altera Quartus Prime License Agreement,
## the Altera MegaCore Function License Agreement, or other 
## applicable license agreement, including, without limitation, 
## that your use is for the sole purpose of programming logic 
## devices manufactured by Altera and sold by Altera or its 
## authorized distributors.  Please refer to the applicable 
## agreement for further details.


## VENDOR  "Altera"
## PROGRAM "Quartus Prime"
## VERSION "Version 15.1.2 Build 193 02/01/2016 SJ Lite Edition"

## DATE    "Thu Mar 14 18:30:27 2019"

##
## DEVICE  "10M08SCE144C8G"
##

###########################################
#--- References used :                  ---
# - http://retroramblings.net/?p=515
# - https://fpgawiki.intel.com/wiki/Timing_Constraints#Constrain_Qsys_SPI_core
# - https://fpgawiki.intel.com/wiki/File:TimeQuest_User_Guide.pdf


#**************************************************************
# Time Information
#**************************************************************

set_time_format -unit ns -decimal_places 3



#**************************************************************
# Create Clock
#**************************************************************

create_clock -name {altera_reserved_tck} -period 100.000 -waveform { 0.000 50.000 } [get_ports {altera_reserved_tck}]

# SCSPCLK : 22.5792 MHz -> 44.288548752834 ns
#	my_little_wasca|altpll_0|sd1|pll7|inclk[0]
#create_clock -name {clk_clk} -period 44.288 -waveform { 0.000 22.144 } [get_ports {clk_clk~inputclkctrl\|inclk\[0\]}]
create_clock -name {clk_clk} -period 44.288 -waveform { 0.000 22.144 } clk_clk

derive_pll_clocks


# Clocks list :
# - altera_reserved_tck	                     -> for JTAG
# - my_little_wasca|onchip_flash_0|altera_onchip_flash_block|ufm_block|osc -> for UFM
# - clk_clk                                  -> 22.58 MHz clock from Saturn
# - my_little_wasca|altpll_0|sd1|pll7|clk[0] -> 116 MHz, for SPI, NIOS SDRAM controller and other QSys modules
# - my_little_wasca|altpll_0|sd1|pll7|clk[1] -> Same 116 MHz, but with appriopriate phase for feeding external SDRAM



#**************************************************************
# Create Generated Clock
#**************************************************************
# clk_clk~inputclkctrl\|outclk
#create_generated_clock -name sd1clk_pin -source my_little_wasca|altpll_0|sd1|pll7|clk[1] [get_ports {sdr_clk}]


#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************
derive_clock_uncertainty 



# Now we come to the trickier stuff. Here we need to refer to the
# datasheet of our SDRAM. We also need to make a rough estimate
# of our PCB routing delays. Assuming the SDRAM and FPGA are right
# next to each other and have only short traces between them, 1 ns
# wouldn’t be an unreasonable estimate.
#
# I’ll take the following parameters for an AS4C16M16SA-6TIN.
# The parameters we need to know are:
#
#    Min Output Time (Low impedance or output) – 0ns
#    Max Output Time (High impedance or input – i.e. bus turnaround time) – 5ns
#    Input Setup Time (often specified separately for address, data, etc. Take the largest.) – 1.5ns
#    Input Hold Time (again, take the largest.) – 0.8ns


#**************************************************************
# Set Input Delay
#**************************************************************
set_input_delay -clock my_little_wasca|altpll_0|sd1|pll7|clk[1] -max 5.0 [get_ports external_sdram_controller_wire_[*]]
set_input_delay -clock my_little_wasca|altpll_0|sd1|pll7|clk[1] -min 1.0 [get_ports external_sdram_controller_wire_[*]]


#**************************************************************
# Set Output Delay
#**************************************************************
set_output_delay -clock my_little_wasca|altpll_0|sd1|pll7|clk[1] -max 1.5 [get_ports external_sdram_controller_wire_[*]]
set_output_delay -clock my_little_wasca|altpll_0|sd1|pll7|clk[1] -min -0.8 [get_ports external_sdram_controller_wire_[*]]


#**************************************************************
# Set Clock Groups
#**************************************************************

set_clock_groups -asynchronous \
                 -group { altera_reserved_tck } \
                 -group { my_little_wasca|onchip_flash_0|altera_onchip_flash_block|ufm_block|osc } \
                 -group { clk_clk } \
                 -group { my_little_wasca|altpll_0|sd1|pll7|clk[0]} \
                 -group { my_little_wasca|altpll_0|sd1|pll7|clk[1]}


#**************************************************************
# Set False Path
#**************************************************************

set_false_path -to [get_keepers {*altera_std_synchronizer:*|din_s1}]
set_false_path -to [get_registers {*|flash_busy_reg}]
set_false_path -to [get_registers {*|flash_busy_clear_reg}]
set_false_path -to [get_pins -nocase -compatibility_mode {*|alt_rst_sync_uq1|altera_reset_synchronizer_int_chain*|clrn}]
set_false_path -from [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_nios2_oci_break:the_wasca_nios2_gen2_0_cpu_nios2_oci_break|break_readreg*}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_tck:the_wasca_nios2_gen2_0_cpu_debug_slave_tck|*sr*}]
set_false_path -from [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_nios2_oci_debug:the_wasca_nios2_gen2_0_cpu_nios2_oci_debug|*resetlatch}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_tck:the_wasca_nios2_gen2_0_cpu_debug_slave_tck|*sr[33]}]
set_false_path -from [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_nios2_oci_debug:the_wasca_nios2_gen2_0_cpu_nios2_oci_debug|monitor_ready}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_tck:the_wasca_nios2_gen2_0_cpu_debug_slave_tck|*sr[0]}]
set_false_path -from [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_nios2_oci_debug:the_wasca_nios2_gen2_0_cpu_nios2_oci_debug|monitor_error}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_tck:the_wasca_nios2_gen2_0_cpu_debug_slave_tck|*sr[34]}]
set_false_path -from [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_nios2_ocimem:the_wasca_nios2_gen2_0_cpu_nios2_ocimem|*MonDReg*}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_tck:the_wasca_nios2_gen2_0_cpu_debug_slave_tck|*sr*}]
set_false_path -from [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_tck:the_wasca_nios2_gen2_0_cpu_debug_slave_tck|*sr*}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_sysclk:the_wasca_nios2_gen2_0_cpu_debug_slave_sysclk|*jdo*}]
set_false_path -from [get_keepers {sld_hub:*|irf_reg*}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_debug_slave_wrapper:the_wasca_nios2_gen2_0_cpu_debug_slave_wrapper|wasca_nios2_gen2_0_cpu_debug_slave_sysclk:the_wasca_nios2_gen2_0_cpu_debug_slave_sysclk|ir*}]
set_false_path -from [get_keepers {sld_hub:*|sld_shadow_jsm:shadow_jsm|state[1]}] -to [get_keepers {*wasca_nios2_gen2_0_cpu:*|wasca_nios2_gen2_0_cpu_nios2_oci:the_wasca_nios2_gen2_0_cpu_nios2_oci|wasca_nios2_gen2_0_cpu_nios2_oci_debug:the_wasca_nios2_gen2_0_cpu_nios2_oci_debug|monitor_go}]


#**************************************************************
# Set Multicycle Path
#**************************************************************
# Last but not least, we need to set a “multicycle path” to deal with
# the fact that we’re skipping a clock edge when reading data back from the SDRAM.
set_multicycle_path -from [get_clocks {get_ports external_sdram_controller_wire_dq[*]}] -to my_little_wasca|altpll_0|sd1|pll7|clk[1] -setup -end 2


#**************************************************************
# Set Maximum Delay
#**************************************************************

set_max_delay -from [get_registers {*altera_avalon_st_clock_crosser:*|in_data_buffer*}] -to [get_registers {*altera_avalon_st_clock_crosser:*|out_data_buffer*}] 100.000
set_max_delay -from [get_registers *] -to [get_registers {*altera_avalon_st_clock_crosser:*|altera_std_synchronizer_nocut:*|din_s1}] 100.000


#**************************************************************
# Set Minimum Delay
#**************************************************************

set_min_delay -from [get_registers {*altera_avalon_st_clock_crosser:*|in_data_buffer*}] -to [get_registers {*altera_avalon_st_clock_crosser:*|out_data_buffer*}] -100.000
set_min_delay -from [get_registers *] -to [get_registers {*altera_avalon_st_clock_crosser:*|altera_std_synchronizer_nocut:*|din_s1}] -100.000


#**************************************************************
# Set Input Transition
#**************************************************************



#**************************************************************
# Set Net Delay
#**************************************************************

set_net_delay -max 2.000 -from [get_registers *] -to [get_registers {*altera_avalon_st_clock_crosser:*|altera_std_synchronizer_nocut:*|din_s1}]
set_net_delay -max 2.000 -from [get_registers {*altera_avalon_st_clock_crosser:*|in_data_buffer*}] -to [get_registers {*altera_avalon_st_clock_crosser:*|out_data_buffer*}]
