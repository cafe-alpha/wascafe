
module wasca (
	abus_avalon_sdram_bridge_0_abus_address,
	abus_avalon_sdram_bridge_0_abus_read,
	abus_avalon_sdram_bridge_0_abus_waitrequest,
	abus_avalon_sdram_bridge_0_abus_addressdata,
	abus_avalon_sdram_bridge_0_abus_chipselect,
	abus_avalon_sdram_bridge_0_abus_direction,
	abus_avalon_sdram_bridge_0_abus_disable_out,
	abus_avalon_sdram_bridge_0_abus_interrupt,
	abus_avalon_sdram_bridge_0_abus_muxing,
	abus_avalon_sdram_bridge_0_abus_writebyteenable_n,
	abus_avalon_sdram_bridge_0_abus_reset,
	abus_avalon_sdram_bridge_0_sdram_addr,
	abus_avalon_sdram_bridge_0_sdram_ba,
	abus_avalon_sdram_bridge_0_sdram_cas_n,
	abus_avalon_sdram_bridge_0_sdram_cke,
	abus_avalon_sdram_bridge_0_sdram_cs_n,
	abus_avalon_sdram_bridge_0_sdram_dq,
	abus_avalon_sdram_bridge_0_sdram_dqm,
	abus_avalon_sdram_bridge_0_sdram_ras_n,
	abus_avalon_sdram_bridge_0_sdram_we_n,
	abus_avalon_sdram_bridge_0_sdram_clk,
	altpll_1_areset_conduit_export,
	altpll_1_locked_conduit_export,
	altpll_1_phasedone_conduit_export,
	buffered_spi_mosi,
	buffered_spi_clk,
	buffered_spi_miso,
	buffered_spi_cs,
	clk_clk,
	clock_116_mhz_clk,
	extra_leds_conn_export,
	hex0_conn_export,
	hex1_conn_export,
	hex2_conn_export,
	hex3_conn_export,
	hex4_conn_export,
	hex5_conn_export,
	hexdot_conn_export,
	leds_conn_export,
	reset_reset_n,
	reset_controller_0_reset_in1_reset,
	spi_sync_conn_export,
	switches_conn_export,
	uart_0_external_connection_rxd,
	uart_0_external_connection_txd);	

	input	[9:0]	abus_avalon_sdram_bridge_0_abus_address;
	input		abus_avalon_sdram_bridge_0_abus_read;
	output		abus_avalon_sdram_bridge_0_abus_waitrequest;
	inout	[15:0]	abus_avalon_sdram_bridge_0_abus_addressdata;
	input	[2:0]	abus_avalon_sdram_bridge_0_abus_chipselect;
	output		abus_avalon_sdram_bridge_0_abus_direction;
	output		abus_avalon_sdram_bridge_0_abus_disable_out;
	output		abus_avalon_sdram_bridge_0_abus_interrupt;
	output	[1:0]	abus_avalon_sdram_bridge_0_abus_muxing;
	input	[1:0]	abus_avalon_sdram_bridge_0_abus_writebyteenable_n;
	input		abus_avalon_sdram_bridge_0_abus_reset;
	output	[12:0]	abus_avalon_sdram_bridge_0_sdram_addr;
	output	[1:0]	abus_avalon_sdram_bridge_0_sdram_ba;
	output		abus_avalon_sdram_bridge_0_sdram_cas_n;
	output		abus_avalon_sdram_bridge_0_sdram_cke;
	output		abus_avalon_sdram_bridge_0_sdram_cs_n;
	inout	[15:0]	abus_avalon_sdram_bridge_0_sdram_dq;
	output	[1:0]	abus_avalon_sdram_bridge_0_sdram_dqm;
	output		abus_avalon_sdram_bridge_0_sdram_ras_n;
	output		abus_avalon_sdram_bridge_0_sdram_we_n;
	output		abus_avalon_sdram_bridge_0_sdram_clk;
	input		altpll_1_areset_conduit_export;
	output		altpll_1_locked_conduit_export;
	output		altpll_1_phasedone_conduit_export;
	output		buffered_spi_mosi;
	output		buffered_spi_clk;
	input		buffered_spi_miso;
	output		buffered_spi_cs;
	input		clk_clk;
	output		clock_116_mhz_clk;
	output	[4:0]	extra_leds_conn_export;
	output	[6:0]	hex0_conn_export;
	output	[6:0]	hex1_conn_export;
	output	[6:0]	hex2_conn_export;
	output	[6:0]	hex3_conn_export;
	output	[6:0]	hex4_conn_export;
	output	[6:0]	hex5_conn_export;
	output	[5:0]	hexdot_conn_export;
	output	[3:0]	leds_conn_export;
	input		reset_reset_n;
	input		reset_controller_0_reset_in1_reset;
	input		spi_sync_conn_export;
	input	[7:0]	switches_conn_export;
	input		uart_0_external_connection_rxd;
	output		uart_0_external_connection_txd;
endmodule
