	component wasca is
		port (
			abus_slave_0_abus_address                      : in    std_logic_vector(9 downto 0)  := (others => 'X'); -- address
			abus_slave_0_abus_chipselect                   : in    std_logic_vector(2 downto 0)  := (others => 'X'); -- chipselect
			abus_slave_0_abus_read                         : in    std_logic                     := 'X';             -- read
			abus_slave_0_abus_write                        : in    std_logic_vector(1 downto 0)  := (others => 'X'); -- write
			abus_slave_0_abus_waitrequest                  : out   std_logic;                                        -- waitrequest
			abus_slave_0_abus_interrupt                    : out   std_logic;                                        -- interrupt
			abus_slave_0_abus_addressdata                  : inout std_logic_vector(15 downto 0) := (others => 'X'); -- addressdata
			abus_slave_0_abus_direction                    : out   std_logic;                                        -- direction
			abus_slave_0_abus_muxing                       : out   std_logic_vector(1 downto 0);                     -- muxing
			abus_slave_0_abus_disableout                   : out   std_logic;                                        -- disableout
			abus_slave_0_conduit_saturn_reset_saturn_reset : in    std_logic                     := 'X';             -- saturn_reset
			altpll_0_areset_conduit_export                 : in    std_logic                     := 'X';             -- export
			altpll_0_locked_conduit_export                 : out   std_logic;                                        -- export
			altpll_0_phasedone_conduit_export              : out   std_logic;                                        -- export
			clk_clk                                        : in    std_logic                     := 'X';             -- clk
			external_sdram_controller_wire_addr            : out   std_logic_vector(12 downto 0);                    -- addr
			external_sdram_controller_wire_ba              : out   std_logic_vector(1 downto 0);                     -- ba
			external_sdram_controller_wire_cas_n           : out   std_logic;                                        -- cas_n
			external_sdram_controller_wire_cke             : out   std_logic;                                        -- cke
			external_sdram_controller_wire_cs_n            : out   std_logic;                                        -- cs_n
			external_sdram_controller_wire_dq              : inout std_logic_vector(15 downto 0) := (others => 'X'); -- dq
			external_sdram_controller_wire_dqm             : out   std_logic_vector(1 downto 0);                     -- dqm
			external_sdram_controller_wire_ras_n           : out   std_logic;                                        -- ras_n
			external_sdram_controller_wire_we_n            : out   std_logic;                                        -- we_n
			leds_conn_export                               : out   std_logic_vector(3 downto 0);                     -- export
			sdram_clkout_clk                               : out   std_logic;                                        -- clk
			switches_conn_export                           : in    std_logic_vector(2 downto 0)  := (others => 'X'); -- export
			uart_0_external_connection_rxd                 : in    std_logic                     := 'X';             -- rxd
			uart_0_external_connection_txd                 : out   std_logic                                         -- txd
		);
	end component wasca;

	u0 : component wasca
		port map (
			abus_slave_0_abus_address                      => CONNECTED_TO_abus_slave_0_abus_address,                      --                 abus_slave_0_abus.address
			abus_slave_0_abus_chipselect                   => CONNECTED_TO_abus_slave_0_abus_chipselect,                   --                                  .chipselect
			abus_slave_0_abus_read                         => CONNECTED_TO_abus_slave_0_abus_read,                         --                                  .read
			abus_slave_0_abus_write                        => CONNECTED_TO_abus_slave_0_abus_write,                        --                                  .write
			abus_slave_0_abus_waitrequest                  => CONNECTED_TO_abus_slave_0_abus_waitrequest,                  --                                  .waitrequest
			abus_slave_0_abus_interrupt                    => CONNECTED_TO_abus_slave_0_abus_interrupt,                    --                                  .interrupt
			abus_slave_0_abus_addressdata                  => CONNECTED_TO_abus_slave_0_abus_addressdata,                  --                                  .addressdata
			abus_slave_0_abus_direction                    => CONNECTED_TO_abus_slave_0_abus_direction,                    --                                  .direction
			abus_slave_0_abus_muxing                       => CONNECTED_TO_abus_slave_0_abus_muxing,                       --                                  .muxing
			abus_slave_0_abus_disableout                   => CONNECTED_TO_abus_slave_0_abus_disableout,                   --                                  .disableout
			abus_slave_0_conduit_saturn_reset_saturn_reset => CONNECTED_TO_abus_slave_0_conduit_saturn_reset_saturn_reset, -- abus_slave_0_conduit_saturn_reset.saturn_reset
			altpll_0_areset_conduit_export                 => CONNECTED_TO_altpll_0_areset_conduit_export,                 --           altpll_0_areset_conduit.export
			altpll_0_locked_conduit_export                 => CONNECTED_TO_altpll_0_locked_conduit_export,                 --           altpll_0_locked_conduit.export
			altpll_0_phasedone_conduit_export              => CONNECTED_TO_altpll_0_phasedone_conduit_export,              --        altpll_0_phasedone_conduit.export
			clk_clk                                        => CONNECTED_TO_clk_clk,                                        --                               clk.clk
			external_sdram_controller_wire_addr            => CONNECTED_TO_external_sdram_controller_wire_addr,            --    external_sdram_controller_wire.addr
			external_sdram_controller_wire_ba              => CONNECTED_TO_external_sdram_controller_wire_ba,              --                                  .ba
			external_sdram_controller_wire_cas_n           => CONNECTED_TO_external_sdram_controller_wire_cas_n,           --                                  .cas_n
			external_sdram_controller_wire_cke             => CONNECTED_TO_external_sdram_controller_wire_cke,             --                                  .cke
			external_sdram_controller_wire_cs_n            => CONNECTED_TO_external_sdram_controller_wire_cs_n,            --                                  .cs_n
			external_sdram_controller_wire_dq              => CONNECTED_TO_external_sdram_controller_wire_dq,              --                                  .dq
			external_sdram_controller_wire_dqm             => CONNECTED_TO_external_sdram_controller_wire_dqm,             --                                  .dqm
			external_sdram_controller_wire_ras_n           => CONNECTED_TO_external_sdram_controller_wire_ras_n,           --                                  .ras_n
			external_sdram_controller_wire_we_n            => CONNECTED_TO_external_sdram_controller_wire_we_n,            --                                  .we_n
			leds_conn_export                               => CONNECTED_TO_leds_conn_export,                               --                         leds_conn.export
			sdram_clkout_clk                               => CONNECTED_TO_sdram_clkout_clk,                               --                      sdram_clkout.clk
			switches_conn_export                           => CONNECTED_TO_switches_conn_export,                           --                     switches_conn.export
			uart_0_external_connection_rxd                 => CONNECTED_TO_uart_0_external_connection_rxd,                 --        uart_0_external_connection.rxd
			uart_0_external_connection_txd                 => CONNECTED_TO_uart_0_external_connection_txd                  --                                  .txd
		);

