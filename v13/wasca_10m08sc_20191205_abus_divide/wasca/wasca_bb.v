
module wasca (
	abus_slave_0_abus_address,
	abus_slave_0_abus_chipselect,
	abus_slave_0_abus_read,
	abus_slave_0_abus_write,
	abus_slave_0_abus_waitrequest,
	abus_slave_0_abus_interrupt,
	abus_slave_0_abus_addressdata,
	abus_slave_0_abus_direction,
	abus_slave_0_abus_muxing,
	abus_slave_0_abus_disableout,
	abus_slave_0_conduit_saturn_reset_saturn_reset,
	altpll_0_areset_conduit_export,
	altpll_0_locked_conduit_export,
	altpll_0_phasedone_conduit_export,
	clk_clk,
	external_sdram_controller_wire_addr,
	external_sdram_controller_wire_ba,
	external_sdram_controller_wire_cas_n,
	external_sdram_controller_wire_cke,
	external_sdram_controller_wire_cs_n,
	external_sdram_controller_wire_dq,
	external_sdram_controller_wire_dqm,
	external_sdram_controller_wire_ras_n,
	external_sdram_controller_wire_we_n,
	leds_conn_export,
	sdram_clkout_clk,
	switches_conn_export,
	uart_0_external_connection_rxd,
	uart_0_external_connection_txd);	

	input	[9:0]	abus_slave_0_abus_address;
	input	[2:0]	abus_slave_0_abus_chipselect;
	input		abus_slave_0_abus_read;
	input	[1:0]	abus_slave_0_abus_write;
	output		abus_slave_0_abus_waitrequest;
	output		abus_slave_0_abus_interrupt;
	inout	[15:0]	abus_slave_0_abus_addressdata;
	output		abus_slave_0_abus_direction;
	output	[1:0]	abus_slave_0_abus_muxing;
	output		abus_slave_0_abus_disableout;
	input		abus_slave_0_conduit_saturn_reset_saturn_reset;
	input		altpll_0_areset_conduit_export;
	output		altpll_0_locked_conduit_export;
	output		altpll_0_phasedone_conduit_export;
	input		clk_clk;
	output	[12:0]	external_sdram_controller_wire_addr;
	output	[1:0]	external_sdram_controller_wire_ba;
	output		external_sdram_controller_wire_cas_n;
	output		external_sdram_controller_wire_cke;
	output		external_sdram_controller_wire_cs_n;
	inout	[15:0]	external_sdram_controller_wire_dq;
	output	[1:0]	external_sdram_controller_wire_dqm;
	output		external_sdram_controller_wire_ras_n;
	output		external_sdram_controller_wire_we_n;
	output	[3:0]	leds_conn_export;
	output		sdram_clkout_clk;
	input	[2:0]	switches_conn_export;
	input		uart_0_external_connection_rxd;
	output		uart_0_external_connection_txd;
endmodule
