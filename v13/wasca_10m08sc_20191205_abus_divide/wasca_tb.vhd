-- Test bench

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use ieee.std_logic_arith.all;
library work;
use work.ALL;


entity wasca_tb is

end entity wasca_tb;


architecture SIMULATE of wasca_tb is

        -- Clock
        signal clk_clk                               : std_logic                     ;

        -- SDRAM
        signal external_sdram_controller_wire_addr   : std_logic_vector(12 downto  0);
        signal external_sdram_controller_wire_ba     : std_logic_vector( 1 downto  0);
        signal external_sdram_controller_wire_cas_n  : std_logic                     ;
        signal external_sdram_controller_wire_cke    : std_logic                     ;
        signal external_sdram_controller_wire_cs_n   : std_logic                     ;
        signal external_sdram_controller_wire_dq     : std_logic_vector(15 downto  0);
        signal external_sdram_controller_wire_dqm    : std_logic_vector( 1 downto  0);
        signal external_sdram_controller_wire_ras_n  : std_logic                     ;
        signal external_sdram_controller_wire_we_n   : std_logic                     ;
        signal external_sdram_clk_pin                : std_logic                     ;

        -- Reset signal from Saturn
        signal reset_reset_n                         : std_logic                     ;

        -- A-Bus
        signal abus_slave_0_abus_address             : std_logic_vector(24 downto 16);
        signal abus_slave_0_abus_addressdata         : std_logic_vector(15 downto  0);
        signal abus_slave_0_abus_chipselect          : std_logic_vector( 2 downto  0);
        signal abus_slave_0_abus_read                : std_logic                     ;
        signal abus_slave_0_abus_write               : std_logic_vector( 1 downto  0);
        signal abus_slave_0_abus_waitrequest         : std_logic                     ;
        signal abus_slave_0_abus_interrupt           : std_logic                     ;
        signal abus_slave_0_abus_disableout          : std_logic                     ;
        signal abus_slave_0_abus_muxing              : std_logic_vector( 1 downto  0);
        signal abus_slave_0_abus_direction           : std_logic                     ;

        -- SPI for SD card
        --signal spi_sd_card_MISO                    : std_logic                     ;
        --signal spi_sd_card_MOSI                    : std_logic                     ;
        --signal spi_sd_card_SCLK                    : std_logic                     ;
        --signal spi_sd_card_SS_n                    : std_logic                     ;

        -- UART (FT232RL)
        signal uart_0_external_connection_txd        : std_logic                     ;
        signal uart_0_external_connection_rxd        : std_logic                     ;

        -- LEDs
        signal leds_conn_export                      : std_logic_vector( 2 downto  0);

        -- Switches
        signal switches_conn_export                  : std_logic_vector( 2 downto  0);

        --- - SPI for STM32
        -- signal spi_stm32_MISO                        : std_logic                     ;
        -- signal spi_stm32_MOSI                        : std_logic                     ;
        -- signal spi_stm32_SCLK                        : std_logic                     ;
        -- signal spi_stm32_SS_n                        : std_logic                     ;

        -- Audio output
        --signal audio_out_BCLK                      : std_logic                     ;
        --signal audio_out_DACDAT                    : std_logic                     ;
        --signal audio_out_DACLRCK                   : std_logic                     ;
        --signal audio_SSEL                          : std_logic                     ;


    -- constant values
    constant clk_in_t : time := 44.289 ns; -- SCSPCLK : 22.579 MHz -> 44.288941051419 ns
begin  -- architecture SIMULATE

    -- component instantiation
    uut: entity work.wasca_toplevel
        port map
        (
            clk_clk                               => clk_clk                              , -- in    std_logic                       -- Saturn clock (22.579 MHz)
            external_sdram_controller_wire_addr   => external_sdram_controller_wire_addr  , -- out   std_logic_vector(12 downto  0); -- external_sdram_controller_wire.addr
            external_sdram_controller_wire_ba     => external_sdram_controller_wire_ba    , -- out   std_logic_vector( 1 downto  0); -- .ba
            external_sdram_controller_wire_cas_n  => external_sdram_controller_wire_cas_n , -- out   std_logic                       -- .cas_n
            external_sdram_controller_wire_cke    => external_sdram_controller_wire_cke   , -- out   std_logic                       -- .cke
            external_sdram_controller_wire_cs_n   => external_sdram_controller_wire_cs_n  , -- out   std_logic                       -- .cs_n
            external_sdram_controller_wire_dq     => external_sdram_controller_wire_dq    , -- inout std_logic_vector(15 downto  0)  -- .dq
            external_sdram_controller_wire_dqm    => external_sdram_controller_wire_dqm   , -- out   std_logic_vector( 1 downto  0); -- .dqm
            external_sdram_controller_wire_ras_n  => external_sdram_controller_wire_ras_n , -- out   std_logic                       -- .ras_n
            external_sdram_controller_wire_we_n   => external_sdram_controller_wire_we_n  , -- out   std_logic                       -- .we_n
            external_sdram_clk_pin                => external_sdram_clk_pin               , -- out   std_logic                       -- .clk
            reset_reset_n                         => reset_reset_n                        , -- in    std_logic                       -- Saturn reset, power on.
            abus_slave_0_abus_address             => abus_slave_0_abus_address            , -- in    std_logic_vector(25 downto 16)  --  abus_slave_0_abus.address
            abus_slave_0_abus_addressdata         => abus_slave_0_abus_addressdata        , -- inout std_logic_vector(15 downto  0)  -- .data
            abus_slave_0_abus_chipselect          => abus_slave_0_abus_chipselect         , -- in    std_logic_vector( 2 downto  0)  -- .chipselect
            abus_slave_0_abus_read                => abus_slave_0_abus_read               , -- in    std_logic                       -- .read
            abus_slave_0_abus_write               => abus_slave_0_abus_write              , -- in    std_logic_vector( 1 downto  0)  -- .write
            abus_slave_0_abus_waitrequest         => abus_slave_0_abus_waitrequest        , -- out   std_logic                       -- .waitrequest
            abus_slave_0_abus_interrupt           => abus_slave_0_abus_interrupt          , -- out   std_logic                       -- .interrupt
            abus_slave_0_abus_disableout          => abus_slave_0_abus_disableout         , -- out   std_logic                       -- .muxing
            abus_slave_0_abus_muxing              => abus_slave_0_abus_muxing             , -- out   std_logic_vector( 1 downto  0)  -- .muxing
            abus_slave_0_abus_direction           => abus_slave_0_abus_direction          , -- out   std_logic                       -- .direction
            --spi_sd_card_MISO                    => spi_sd_card_MISO                     , -- in    std_logic                       -- MISO
            --spi_sd_card_MOSI                    => spi_sd_card_MOSI                     , -- out   std_logic                       -- MOSI
            --spi_sd_card_SCLK                    => spi_sd_card_SCLK                     , -- out   std_logic                       -- SCLK
            --spi_sd_card_SS_n                    => spi_sd_card_SS_n                     , -- out   std_logic                       -- SS_n
            uart_0_external_connection_txd        => uart_0_external_connection_txd       , -- out   std_logic                       -- 
            uart_0_external_connection_rxd        => uart_0_external_connection_rxd       , -- in    std_logic                       -- 
            leds_conn_export                      => leds_conn_export                     , -- out   std_logic_vector( 2 downto  0); -- leds_conn_export[0]: ledr1, leds_conn_export[1]: ledg1, leds_conn_export[2]: ledr2
            switches_conn_export                  => switches_conn_export                   -- in    std_logic_vector( 2 downto  0); -- switches_conn_export[0]: sw1, switches_conn_export[1]: sw2, switches_conn_export[2]: STM32 SPI synchronization
            --spi_stm32_MISO                        => spi_stm32_MISO                       , -- in    std_logic                       -- MISO
            --spi_stm32_MOSI                        => spi_stm32_MOSI                       , -- out   std_logic                       -- MOSI
            --spi_stm32_SCLK                        => spi_stm32_SCLK                       , -- out   std_logic                       -- SCLK
            --spi_stm32_SS_n                        => spi_stm32_SS_n                         -- out   std_logic                       -- SS_n
            --audio_out_BCLK                      => audio_out_BCLK                       , -- in    std_logic                       -- BCLK
            --audio_out_DACDAT                    => audio_out_DACDAT                     , -- out   std_logic                       -- DACDAT
            --audio_out_DACLRCK                   => audio_out_DACLRCK                    , -- in    std_logic                       -- DACLRCK
            --audio_SSEL                          => audio_SSEL                           , -- out   std_logic                       -- 
        );



    process is
    begin  -- process
        -- Activate sysres signal on startup
        reset_reset_n <= '0';
        wait for 350 ns;

        reset_reset_n <= '1';
        wait for 999999 ns;
    end process;

    process is
    begin  -- SCSPCLK
        clk_clk <= '0';
        wait for clk_in_t / 2;
        clk_clk <= '1';
        wait for clk_in_t / 2;
    end process;

    -- process is
    -- begin  -- process
    --     -- Dummy values for dout
    --     io_sd_dout <= '0';
    --     wait for 250 ns;
    --     io_sd_dout <= '1';
    --     wait for 150 ns;
    -- end process;

    process is
    begin  -- process
        -- Test switchs always to '1'
        switches_conn_export(0) <= '1';
        switches_conn_export(1) <= '1';
        switches_conn_export(2) <= '1';
        wait for clk_in_t * 2;
    end process;




    -- process is
    -- begin  -- process
    --     -- Beg for hardware version
    --     io_address <= "001";
    --     io_data    <= "ZZZZZZZZ";
    --     io_oe_al   <= '0';
    --     io_rd_al   <= '0';
    --     io_wr0     <= '0';
    --     io_wr1     <= '0';
    --     io_sd_adr  <= '0';
    -- 
    -- 
    -- 
    -- 
    --     wait for 50 ns;
    --     io_wr0     <= '0';
    --     io_wr1     <= '1';
    --     wait for 50 ns;
    --     io_wr0     <= '1';
    --     io_wr1     <= '0';
    -- 
    --     -- Beg for dout pin state
    --     io_address <= "000";
    --     wait for 140 ns;
    --     io_wr0     <= '1';
    --     io_wr1     <= '1';
    -- 
    --     wait for 120 ns;
    -- 
    --     -- Write to CS/DIN/CLK pins
    --     io_address <= "000";
    --     io_data    <= "00000111";
    --     io_oe_al   <= '1';
    --     io_rd_al   <= '1';
    --     wait for 80 ns;
    --     io_data    <= "00000101";
    --     wait for 80 ns;
    --     io_data    <= "00000100";
    --     wait for 80 ns;
    --     io_data    <= "00000001";
    -- 
    -- 
    --     wait for 700 ns;
    -- end process;

--    process is
--    begin  -- process
--        -- Ask for build date #1
--        io_up_addr <= "0110";  -- SD card
--        io_address <= "00000"; -- CPLD version
--        io_data    <= "ZZZZZZZZZZZZZZZZ";
--        io_cs0_al  <= '0';
--        io_rd_al   <= '0';
--        io_wr0     <= '0';
--        io_wr1     <= '0';
--        wait for clk_in_t * 4;
--
--        -- Ask for build date #2
--        io_up_addr <= "0110";  -- SD card
--        io_address <= "00001"; -- CPLD version
--        io_data    <= "ZZZZZZZZZZZZZZZZ";
--        io_cs0_al  <= '0';
--        io_rd_al   <= '0';
--        io_wr0     <= '0';
--        io_wr1     <= '0';
--        wait for clk_in_t * 4;
--
--        -- Ask for DOUT value
--        io_up_addr <= "0110";  -- SD card
--        io_address <= "01010"; -- DOUT read
--        io_data    <= "ZZZZZZZZZZZZZZZZ";
--        io_cs0_al  <= '0';
--        io_rd_al   <= '0';
--        io_wr0     <= '0';
--        io_wr1     <= '0';
--        wait for clk_in_t * 4;
--
--        -- Set DIN/CS/CLK
--        io_up_addr <= "0110";  -- SD card
--        io_address <= "01000"; -- CS/DIN/CLK set
--        io_data    <= "0000000000000100";
--        io_cs0_al  <= '0';
--        io_rd_al   <= '1';
--        io_wr0     <= '1';
--        io_wr1     <= '1';
--        wait for clk_in_t * 4;
--        -- Set DIN/CS/CLK
--        io_up_addr <= "0110";  -- SD card
--        io_address <= "01000"; -- CS/DIN/CLK set
--        io_data    <= "0000000000000010";
--        io_cs0_al  <= '0';
--        io_rd_al   <= '1';
--        io_wr0     <= '1';
--        io_wr1     <= '1';
--        wait for clk_in_t * 4;
--
--    end process;


end architecture SIMULATE;



-------------------------------------------------------------------------------
-- 
-- -- Configuration for simulation
-- library work;
-- configuration wasca_tb_cfg of wasca_tb is
-- for SIMULATE
--     -- for DUTC : wasca_tb
--     --     use entity work.wasca(structure);
--     -- end for;
-- end for;
-- end wasca_tb_cfg;






