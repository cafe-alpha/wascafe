-- abus_slave.vhd

library IEEE;
use IEEE.numeric_std.all;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity abus_slave is
    port (
        clock                       : in    std_logic                     := '0';             -- clock.clk

        -- Demuxed signals
        -- Note : naming is Saturn-centered, ie readdata = read from Saturn = output from A-Bus side = input from demux side
        demux_writeaddress          : in    std_logic_vector(27 downto 0) := (others => '0');
        demux_writedata             : in    std_logic_vector(15 downto 0) := (others => '0');
        demux_writepulse            : in    std_logic                     := '0';
        demux_write_byteenable      : in    std_logic_vector( 1 downto 0) := (others => '0');
        demux_readaddress           : in    std_logic_vector(27 downto 0) := (others => '0');
        demux_readdata              : out   std_logic_vector(15 downto 0) := (others => '0');
        demux_readpulse             : in    std_logic                     := '0';
        demux_readdatavalid         : out   std_logic                     := '0';


        avalon_read                 : out   std_logic;                                        -- avalon_master.read
        avalon_write                : out   std_logic;                                        --              .write
        avalon_waitrequest          : in    std_logic                     := '0';             --              .waitrequest
        avalon_address              : out   std_logic_vector(27 downto 0);                    --              .address
        avalon_readdata             : in    std_logic_vector(15 downto 0) := (others => '0'); --              .readdata
        avalon_writedata            : out   std_logic_vector(15 downto 0);                    --              .writedata
        avalon_byteenable           : out   std_logic_vector( 1 downto 0);                    --              .byteenable
        avalon_burstcount           : out   std_logic;                                        --              .burstcount
        avalon_readdatavalid        : in    std_logic                     := '0';             --              .readdatavalid

        avalon_trace_read           : out   std_logic;                                        -- avalon_trace.read
        avalon_trace_write          : out   std_logic;                                        --             .write
        avalon_trace_waitrequest    : in    std_logic                     := '0';             --             .waitrequest
        avalon_trace_address        : out   std_logic_vector(27 downto 0);                    --             .address
        avalon_trace_readdata       : in    std_logic_vector(63 downto 0) := (others => '0'); --             .readdata
        avalon_trace_writedata      : out   std_logic_vector(63 downto 0);                    --             .writedata
        avalon_trace_byteenable     : out   std_logic_vector( 7 downto 0);                    --             .byteenable
        avalon_trace_burstcount     : out   std_logic;                                        --             .burstcount
        avalon_trace_readdatavalid  : in    std_logic                     := '0';             --             .readdatavalid

        avalon_nios_read            : in    std_logic := '0';                                 -- avalon_master.read
        avalon_nios_write           : in    std_logic := '0';                                 --              .write
        avalon_nios_waitrequest     : out   std_logic                     := '0';             --              .waitrequest
        avalon_nios_address         : in    std_logic_vector( 7 downto 0) := (others => '0'); --              .address
        avalon_nios_writedata       : in    std_logic_vector(31 downto 0) := (others => '0'); --              .writedata
        avalon_nios_burstcount      : in    std_logic;                                        --              .burstcount
        avalon_nios_readdata        : out   std_logic_vector(31 downto 0) := (others => '0'); --              .readdata
        avalon_nios_readdatavalid   : out   std_logic                     := '0';             --              .readdatavalid

        reset                       : in    std_logic                     := '0'              -- reset.reset
    );
end entity abus_slave;

architecture rtl of abus_slave is

-- Avalon/register selection
TYPE transaction_type IS (TYPE_REG, TYPE_AVALON);
SIGNAL my_transaction_type : transaction_type := TYPE_REG;

TYPE wasca_mode_type IS (MODE_INIT,
                          MODE_POWER_MEMORY_05M, MODE_POWER_MEMORY_1M, MODE_POWER_MEMORY_2M, MODE_POWER_MEMORY_4M,
                          MODE_RAM_1M, MODE_RAM_4M,
                          MODE_ROM_KOF95,
                          MODE_ROM_ULTRAMAN,
                          MODE_BOOT);
SIGNAL wasca_mode : wasca_mode_type := MODE_INIT;

signal avalon_readdatavalid_p1  : std_logic                     := '0'; 
signal demux_readdatavalid_p1   : std_logic                     := '0'; 
signal demux_readdata_p1        : std_logic_vector(15 downto 0) := (others => '0');

signal REG_PCNTR      : std_logic_vector(15 downto 0) := (others => '0');  
signal REG_STATUS     : std_logic_vector(15 downto 0) := (others => '0'); 
signal REG_MODE       : std_logic_vector(15 downto 0) := (others => '0'); 
signal REG_HWVER      : std_logic_vector(15 downto 0) := X"0002";
signal REG_SWVER      : std_logic_vector(15 downto 0) := (others => '0'); 


-- For Rd/Wr access debug
signal rd_access_cntr : std_logic_vector( 7 downto 0) := x"01";
signal wr_access_cntr : std_logic_vector( 7 downto 0) := x"01";

signal last_rd_addr   : std_logic_vector(27 downto 0) := x"ABCDEF1"; -- Last avalon read addr
signal last_rd_addr1  : std_logic_vector(27 downto 0) := x"A1A1A11"; -- Demux read addr full 28 bits, last access
signal last_rd_addr2  : std_logic_vector(27 downto 0) := x"A2A2A22"; -- Demux read addr full 28 bits, last access - 1
signal last_rd_addr3  : std_logic_vector(27 downto 0) := x"A3A3A33"; -- Demux read addr full 28 bits, last access - 2
signal last_rd_addr4  : std_logic_vector(27 downto 0) := x"A4A4A44"; -- Demux read addr full 28 bits, last access - 3

signal last_wr_addr   : std_logic_vector(27 downto 0) := x"0001231";
signal last_wr_data   : std_logic_vector(15 downto 0) := x"5678";

-- Access test stuff, added 2019/11/04 vvv
signal rdwr_access_buff : std_logic_vector(127 downto 0) := x"CAFE0304050607080910111213141516";
-- Access test stuff, added 2019/11/04 ^^^

-- Rd/Wr access trace -->
signal trace_cntr : std_logic_vector(15 downto 0) := x"0000";
-- Rd/Wr access trace <--


begin

    ---------------------------------------------------------------------------------------
    -- Rd/Wr access trace
    process (clock)
    begin
        if rising_edge(clock) then

            if demux_readpulse = '1' then

                avalon_trace_write <= '1';

                avalon_trace_writedata(27 downto 0)  <= demux_writeaddress(27 downto  0);
                avalon_trace_writedata(31 downto 28) <= "0000";
                avalon_trace_writedata(47 downto 32) <= X"1234"; -- Dummy read data
                avalon_trace_writedata(63 downto 48) <= X"5678"; -- Some extra information (example : counter from previous access etc)

                trace_cntr <= trace_cntr + x"0001";
            elsif demux_writepulse = '1' then

                avalon_trace_write <= '1';

                avalon_trace_writedata(27 downto 0)  <= demux_readaddress(27 downto  0);
                avalon_trace_writedata(31 downto 28) <= "0000";
                avalon_trace_writedata(47 downto 32) <= demux_writedata(15 downto  0);
                avalon_trace_writedata(63 downto 48) <= X"5678"; -- Some extra information (example : counter from previous access etc)

                trace_cntr <= trace_cntr + x"0001";
            else

                avalon_trace_write <= '0';
            end if;


            avalon_trace_address        <= "000000000000" & trace_cntr(15 downto 0);

            avalon_trace_byteenable     <= "11111111";
            avalon_trace_burstcount     <= '1';

        end if;
    end process;



    ---------------------------------------------------------------------------------------
    -- T.B.D.
    process (clock)
    begin
        if rising_edge(clock) then
            avalon_readdatavalid_p1 <= avalon_readdatavalid;
            demux_readdata_p1 <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8);
        end if;
    end process;


    process (clock)
    begin
        if rising_edge(clock) then

            if demux_readdatavalid_p1 = '1' then
                -- Indicate that data was valid on previous clock.
                demux_readdatavalid    <= '0';
                demux_readdatavalid_p1 <= '0';

            elsif((my_transaction_type = TYPE_AVALON) and (avalon_waitrequest = '0')) then

                -- Terminate request to Avalon.
                my_transaction_type <= TYPE_REG;
                avalon_read  <= '0';
                avalon_write <= '0';

            elsif((avalon_readdatavalid_p1 = '1') and (avalon_readdatavalid = '0')) then

                -- Pass data read back from Avalon to A-Bus demultiplexer.
                demux_readdata <= demux_readdata_p1;
                demux_readdatavalid    <= '1';
                demux_readdatavalid_p1 <= '1';

            elsif demux_readpulse = '1' then

                -- Debug stuff around Rd/Wr access
                --rd_access_cntr <= rd_access_cntr + x"01";
                last_rd_addr1   <= demux_readaddress;
                last_rd_addr2   <= last_rd_addr1;
                last_rd_addr3   <= last_rd_addr2;
                last_rd_addr4   <= last_rd_addr3;


                if demux_readaddress(25 downto 24) = "00" then
                    --CS0 access
                    if demux_readaddress(23 downto 0) = X"FF0FFE" then
                        --wasca specific SD card control register
                        demux_readdata <= X"CDCD";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;


-- Access test stuff, added 2019/11/04 vvv
                    elsif demux_readaddress(23 downto 0) = X"FFFFD0" then -- 0x23FFFFA0
                        demux_readdata <= x"A" & last_rd_addr(27 downto 16);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFD1" then -- 0x23FFFFA2
                        demux_readdata <= last_rd_addr(15 downto 0);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;



                    elsif demux_readaddress(23 downto 0) = X"FFFFD8" then -- 0x23FFFFB0
                        demux_readdata <= x"0" & last_rd_addr1(27 downto 16);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFD9" then -- 0x23FFFFB2
                        demux_readdata <= last_rd_addr1(15 downto 0);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFDA" then -- 0x23FFFFB4
                        demux_readdata <= x"0" & last_rd_addr2(27 downto 16);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFDB" then -- 0x23FFFFB6
                        demux_readdata <= last_rd_addr2(15 downto 0);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFDC" then -- 0x23FFFFB8
                        demux_readdata <= x"0" & last_rd_addr3(27 downto 16);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFDD" then -- 0x23FFFFBA
                        demux_readdata <= last_rd_addr3(15 downto 0);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFDE" then -- 0x23FFFFBC
                        demux_readdata <= x"0" & last_rd_addr4(27 downto 16);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFDF" then -- 0x23FFFFBE
                        demux_readdata <= last_rd_addr4(15 downto 0);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;




                    elsif demux_readaddress(23 downto 0) = X"FFFFE0" then -- 0x23FFFFC0
                        demux_readdata <= rdwr_access_buff(127 downto 112);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE1" then -- 0x23FFFFC2
                        demux_readdata <= rdwr_access_buff(111 downto  96);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE2" then -- 0x23FFFFC4
                        demux_readdata <= rdwr_access_buff( 95 downto  80);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE3" then -- 0x23FFFFC6
                        demux_readdata <= rdwr_access_buff( 79 downto  64);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE4" then -- 0x23FFFFC8
                        demux_readdata <= rdwr_access_buff( 63 downto  48);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE5" then -- 0x23FFFFCA
                        demux_readdata <= rdwr_access_buff( 47 downto  32);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE6" then -- 0x23FFFFCC
                        demux_readdata <= rdwr_access_buff( 31 downto  16);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE7" then -- 0x23FFFFCE
                        demux_readdata <= rdwr_access_buff( 15 downto   0);
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;

                    elsif demux_readaddress(23 downto 0) = X"FFFFE8" then -- 0x23FFFFD0
                        demux_readdata <= x"5445"; -- "TE"
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFE9" then -- 0x23FFFFD2
                        demux_readdata <= x"5354"; -- "ST"
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFEA" then -- 0x23FFFFD4
                        demux_readdata <= X"FFFF";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFEB" then -- 0x23FFFFD6
                        demux_readdata <= X"5A5A";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFEC" then -- 0x23FFFFD8
                        demux_readdata <= X"FFFF";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFED" then -- 0x23FFFFDA
                        demux_readdata <= X"FFFF";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFEE" then -- 0x23FFFFDC
                        demux_readdata <= X"FFFF";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFEF" then -- 0x23FFFFDE
                        demux_readdata <= X"FFFF";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
-- Access test stuff, added 2019/11/04 ^^^



                    elsif demux_readaddress(23 downto 0) = X"FFFFF0" then -- 0x23FFFFE0
                        demux_readdata <= X"FFFF"; -- Test for cartridge assembly
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF1" then -- 0x23FFFFE2
                        demux_readdata <= X"0000"; -- Test for cartridge assembly
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF2" then -- 0x23FFFFE4
                        demux_readdata <= X"A5A5"; -- Test for cartridge assembly
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF3" then -- 0x23FFFFE6
                        demux_readdata <= X"5A5A"; -- Test for cartridge assembly
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF4" then -- 0x23FFFFE8
                        demux_readdata <= x"CA" & rd_access_cntr;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF5" then -- 0x23FFFFEA
                        demux_readdata <= x"AC" & rd_access_cntr;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF6" then -- 0x23FFFFEC
                        demux_readdata <= x"FE" & wr_access_cntr;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF7" then -- 0x23FFFFEE
                        demux_readdata <= x"EF" & wr_access_cntr;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;

                    elsif demux_readaddress(23 downto 0) = X"FFFFF8" then -- 0x23FFFFF0
                        --wasca prepare counter
                        demux_readdata <= REG_PCNTR;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFF9" then -- 0x23FFFFF2
                        --wasca status register
                        demux_readdata <= REG_STATUS;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFFA" then -- 0x23FFFFF4
                        --wasca mode register
                        demux_readdata <= REG_MODE;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFFB" then -- 0x23FFFFF6
                        --wasca hwver register
                        demux_readdata <= REG_HWVER;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFFC" then -- 0x23FFFFF8
                        --wasca swver register
                        demux_readdata <= REG_SWVER;
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFFD" then -- 0x23FFFFFA
                        --wasca signature "wa"
                        demux_readdata <= X"7761";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFFE" then
                        --wasca signature "sc"
                        demux_readdata <= X"7363";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    elsif demux_readaddress(23 downto 0) = X"FFFFFF" then
                        --wasca signature "a "
                        demux_readdata <= X"6120";
                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    else
                        --normal CS0 read access
                        avalon_read <= '1';
                        --demux_readdata <= X"FF0A";
                        demux_readdatavalid <= '0';
                        my_transaction_type <= TYPE_AVALON;

                        avalon_address       <= "1" & demux_readaddress(23) & "00" & demux_readaddress(22 downto 0) & "0"; -- SDRAM and OCRAM available from A-Bus
                        -- Set the data masks to read all bytes
                        avalon_byteenable    <= "11";

                        rd_access_cntr <= rd_access_cntr + x"01";
                        last_rd_addr   <= demux_readaddress;


                        -- case wasca_mode is
                        --     when MODE_INIT              => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_POWER_MEMORY_05M  => demux_readdata <= X"FFFF";
                        --     when MODE_POWER_MEMORY_1M   => demux_readdata <= X"FFFF";
                        --     when MODE_POWER_MEMORY_2M   => demux_readdata <= X"FFFF";
                        --     when MODE_POWER_MEMORY_4M   => demux_readdata <= X"FFFF";
                        --     when MODE_RAM_1M            => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_RAM_4M            => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_ROM_KOF95         => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_ROM_ULTRAMAN      => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_BOOT              => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        -- end case;   
                    end if;

                elsif demux_readaddress(25 downto 24) = "01" then
                    --CS1 access
                    if ( demux_readaddress(23 downto 0) = X"FFFFFF" or  demux_readaddress(23 downto 0) = X"FFFFFD" ) then
                        --saturn cart id register
                        case wasca_mode is
                            when MODE_INIT              => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                            when MODE_POWER_MEMORY_05M  => demux_readdata <= X"FF21";
                            when MODE_POWER_MEMORY_1M   => demux_readdata <= X"FF22";
                            when MODE_POWER_MEMORY_2M   => demux_readdata <= X"FF23";
                            when MODE_POWER_MEMORY_4M   => demux_readdata <= X"FF24";
                            when MODE_RAM_1M            => demux_readdata <= X"FF5A"; 
                            when MODE_RAM_4M            => demux_readdata <= X"FF5C"; 
                            when MODE_ROM_KOF95         => demux_readdata <= X"FFFD";
                            when MODE_ROM_ULTRAMAN      => demux_readdata <= X"FFFE";
                            when MODE_BOOT              => demux_readdata <= X"FFAA";
                        end case;

                        demux_readdatavalid    <= '1';
                        demux_readdatavalid_p1 <= '1';
                        my_transaction_type <= TYPE_REG;
                    else
                        --normal CS1 access
                        avalon_read <= '1';
                        --demux_readdata <= X"FF0A";
                        demux_readdatavalid <= '0';
                        my_transaction_type <= TYPE_AVALON;

                        avalon_address       <= "1" & demux_readaddress(23) & "00" & demux_readaddress(22 downto 0) & "0"; -- SDRAM and OCRAM available from A-Bus
                        -- Set the data masks to read all bytes
                        avalon_byteenable    <= "11";

                        rd_access_cntr <= rd_access_cntr + x"01";
                        last_rd_addr   <= demux_readaddress;

                        -- case wasca_mode is
                        -- 
                        --     -- [DEBUG]Show which address is being accessed, 
                        --     -- [DEBUG]in order to verify multiplexer wiring.
                        --     --when MODE_INIT => demux_readdata <= demux_readaddress(15 downto 0);
                        -- 
                        --     when MODE_INIT              => demux_readdata <= avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_POWER_MEMORY_05M  => demux_readdata <=  avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_POWER_MEMORY_1M   => demux_readdata <=  avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_POWER_MEMORY_2M   => demux_readdata <=  avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_POWER_MEMORY_4M   => demux_readdata <=  avalon_readdata(7 downto 0) & avalon_readdata (15 downto 8) ;
                        --     when MODE_RAM_1M            => demux_readdata <= X"FFF1";
                        --     when MODE_RAM_4M            => demux_readdata <= X"FFF2";
                        --     when MODE_ROM_KOF95         => demux_readdata <= X"FFF3";
                        --     when MODE_ROM_ULTRAMAN      => demux_readdata <= X"FFF4";
                        --     when MODE_BOOT              => demux_readdata <= X"FFF5";
                        -- end case;
                    end if;
                else
                    --CS2 access
                    demux_readdata <= X"EEEE";
                    demux_readdatavalid <= '1';
                    my_transaction_type <= TYPE_REG;
                end if;



            elsif demux_writepulse = '1' then

                -- Debug stuff around Rd/Wr access
                wr_access_cntr <= wr_access_cntr + x"01";
                last_wr_addr   <= demux_writeaddress;
                last_wr_data   <= demux_writedata;


                --if demux_writeaddress(25 downto 24) = "00" then       <-- Don't care about CS for now.
                if demux_writeaddress(23 downto 0) = X"FFFFFA" then -- 0x23FFFFF4
                    --wasca mode register
                    REG_MODE <= demux_writedata;
                    case (demux_writedata (3 downto 0)) is 
                        when X"1" => wasca_mode  <= MODE_POWER_MEMORY_05M;
                        when X"2" => wasca_mode  <= MODE_POWER_MEMORY_1M;
                        when X"3" => wasca_mode  <= MODE_POWER_MEMORY_2M;
                        when X"4" => wasca_mode  <= MODE_POWER_MEMORY_4M;
                        when others =>
                            case (demux_writedata (7 downto 4)) is 
                                when X"1" => wasca_mode  <= MODE_RAM_1M;
                                when X"2" => wasca_mode  <= MODE_RAM_4M;
                                when others => 
                                    case (demux_writedata (11 downto 8)) is 
                                        when X"1" => wasca_mode  <= MODE_ROM_KOF95;
                                        when X"2" => wasca_mode  <= MODE_ROM_ULTRAMAN;
                                        when others => null;-- wasca_mode  <= MODE_INIT;
                                    end case;
                            end case;
                    end case;
                    my_transaction_type <= TYPE_REG;

-- Access test stuff, added 2019/11/04 vvv
                elsif demux_writeaddress(23 downto 0) = X"FFFFE0" then -- 0x23FFFFC0
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff(127 downto 120) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff(119 downto 112) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE1" then -- 0x23FFFFC2
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff(111 downto 104) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff(103 downto  96) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE2" then -- 0x23FFFFC4
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff( 95 downto  88) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff( 87 downto  80) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE3" then -- 0x23FFFFC6
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff( 79 downto  72) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff( 71 downto  64) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE4" then -- 0x23FFFFC8
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff( 63 downto  56) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff( 55 downto  48) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE5" then -- 0x23FFFFCA
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff( 47 downto  40) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff( 39 downto  32) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE6" then -- 0x23FFFFCC
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff( 31 downto  24) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff( 23 downto  16) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;
                elsif demux_writeaddress(23 downto 0) = X"FFFFE7" then -- 0x23FFFFCE
                    if(demux_write_byteenable(1) = '1') then rdwr_access_buff( 15 downto   8) <= demux_writedata(15 downto 8); end if;
                    if(demux_write_byteenable(0) = '1') then rdwr_access_buff(  7 downto   0) <= demux_writedata( 7 downto 0); end if;
                    my_transaction_type <= TYPE_REG;

-- Access test stuff, added 2019/11/04 ^^^


                else

                    -- avalon-to-abus mapping
                    -- SDRAM is mapped to both CS0 and CS1
                    --
                    -- Note about address : from NIOS side, SDRAM is mapped to 0x0800_0000, 
                    -- | so that the prefix at upper bits of the address passed to avalon.
                    -- | And A-Bus data width is 16 bits so that lower address bit is zeroed.
                    -- 
                    -- Additionally, extra OCRAM is mapped to 0x0C00_0000 from NIOS side, 
                    -- and is (temporarily) available from A-Bus' second half of CS0.
                    --avalon_address <= "010" & demux_writeaddress(23 downto 0) & "0"; -- SDRAM available from A-Bus (old mapping, just here for reference)
                    avalon_writedata     <= demux_writedata(7 downto 0) & demux_writedata(15 downto 8);
                    avalon_byteenable(0) <= demux_write_byteenable(0);
                    avalon_byteenable(1) <= demux_write_byteenable(1);
                    avalon_burstcount    <= '1';
                    avalon_write         <= '1';
                    my_transaction_type  <= TYPE_AVALON;

                    avalon_address       <= "1" & demux_writeaddress(23) & "00" & demux_writeaddress(22 downto 0) & "0"; -- SDRAM and OCRAM available from A-Bus
                end if;

            end if;
        end if;
    end process;





    ---------------------------------------------------------------------------------------
    --Nios II read interface
    process (clock)
    begin
        if rising_edge(clock) then
            if avalon_nios_read = '1' then
                avalon_nios_readdatavalid <= '1';
                case avalon_nios_address is 

                    -- Debug stuff around Rd/Wr access
                    when X"00" => 
                        avalon_nios_readdata <= x"0" & last_rd_addr;
                    when X"02" => 
                        avalon_nios_readdata <= x"0" & last_rd_addr1;
                    when X"03" => 
                        avalon_nios_readdata <= x"0" & last_rd_addr2;
                    when X"04" => 
                        avalon_nios_readdata <= x"0" & last_rd_addr3;
                    when X"05" => 
                        avalon_nios_readdata <= x"0" & last_rd_addr4;


                    when X"08" => 
                        avalon_nios_readdata <= x"0000CA" & rd_access_cntr;
                    when X"09" => 
                        avalon_nios_readdata <= x"0000FE" & wr_access_cntr;

                    when X"10" => 
                        avalon_nios_readdata <= x"0" & last_wr_addr;
                    when X"11" => 
                        avalon_nios_readdata <= x"0000" & last_wr_data;

                    --when X"F0" => 
                    --    avalon_nios_readdata <= REG_PCNTR;
                    --when X"F2" =>
                    --    avalon_nios_readdata <= REG_STATUS;
                    --when X"F4" =>
                    --    avalon_nios_readdata <= REG_MODE;
                    --when X"F6" =>
                    --    avalon_nios_readdata <= REG_HWVER;
                    --when X"F8" =>
                    --    avalon_nios_readdata <= REG_SWVER;
                    when others =>
                        avalon_nios_readdata <= X"00000000";
                end case;
            else
                avalon_nios_readdatavalid <= '0';
            end if;
        end if;
    end process;

    ---------------------------------------------------------------------------------------
    --Nios II write interface
    process (clock)
    begin
        if rising_edge(clock) then
            if avalon_nios_write= '1' then
                case avalon_nios_address is 
                    --when X"F0" => 
                    --    REG_PCNTR <= avalon_nios_writedata(15 downto 0);
                    --when X"F2" =>
                    --    REG_STATUS <= avalon_nios_writedata(15 downto 0);
                    --when X"F4" =>
                    --    null;
                    --when X"F6" =>
                    --    null;
                    --when X"F8" =>
                    --    REG_SWVER <= avalon_nios_writedata(15 downto 0);
                    when others =>
                        null;
                end case;
            end if;
        end if;
    end process;

    --Nios system interface is only regs, so always ready to write.
    avalon_nios_waitrequest <= '0';

end architecture rtl; -- of abus_slave
