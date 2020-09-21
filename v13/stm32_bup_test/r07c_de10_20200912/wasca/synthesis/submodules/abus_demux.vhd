-- abus_demux.vhd

library IEEE;
use IEEE.numeric_std.all;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;

entity abus_demux is
    port (
-- -- External signals used for simulation vvv
--        sim_test1                   : out   std_logic_vector( 7 downto 0) := (others => '0');
--        sim_test2                   : out   std_logic_vector( 7 downto 0) := (others => '0');

--        abus_cspulse_trail_dbg      : out   std_logic_vector(11 downto 0) := (others => '0');
--        abus_read_trail_dbg         : out   std_logic_vector(11 downto 0) := (others => '0');
--        abus_writeneg0_trail_dbg    : out   std_logic_vector(11 downto 0) := (others => '0');
--        abus_writeneg1_trail_dbg    : out   std_logic_vector(11 downto 0) := (others => '0');
--        abus_addresslatched_dbg     : out   std_logic_vector(23 downto 0) := (others => '0');

--        sim_noise                   : in    std_logic                     := '0';
-- -- External signals used for simulation ^^^


        clock                       : in    std_logic                     := '0';             -- clock.clk

        -- A-Bus interface
        abus_address                : in    std_logic_vector( 8 downto 0) := (others => '0'); -- abus.address
        abus_addressdata            : inout std_logic_vector(15 downto 0) := (others => '0'); -- abus.addressdata
        abus_chipselect             : in    std_logic_vector( 2 downto 0) := (others => '0'); --     .chipselect
        abus_read                   : in    std_logic                     := '0';             --     .read
        abus_write                  : in    std_logic_vector( 1 downto 0) := (others => '0'); --     .write

        abus_waitrequest            : out   std_logic                     := '1';             --     .waitrequest
        abus_interrupt              : out   std_logic                     := '0';             --     .interrupt

        abus_direction              : out   std_logic                     := '0';             --     .direction
        abus_muxing                 : out   std_logic_vector( 1 downto 0) := "01";            --     .muxing
        abus_disable_out            : out   std_logic                     := '0';             --     .disableout

        saturn_reset                : in    std_logic                     := '0';             -- .saturn_reset


        -- Demuxed signals
        -- Note : naming is Saturn-centered, ie readdata = read from Saturn = output from A-Bus side = input from demux side
        demux_writeaddress          : out   std_logic_vector(27 downto 0) := (others => '0');
        demux_writedata             : out   std_logic_vector(15 downto 0) := (others => '0');
        demux_writepulse            : out   std_logic                     := '0';
        demux_write_byteenable      : out   std_logic_vector( 1 downto 0) := (others => '0');
        demux_readaddress           : out   std_logic_vector(27 downto 0) := (others => '0');
        demux_readdata              : in    std_logic_vector(15 downto 0) := (others => '0');
        demux_readpulse             : out   std_logic                     := '0';
        demux_readdatavalid         : in    std_logic                     := '0';


        -- Avalon
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
end entity abus_demux;

architecture rtl of abus_demux is

-- Trail size, same for all internal signals.
-- As Quartus won't synthetize unused signals, large enough size is defined
-- so that source readability shall be a bit improved.
constant ABUS_TRAILS_SIZE : integer := 12;


signal abus_address_ms              : std_logic_vector( 8 downto 0) := (others => '0'); -- abus.address
signal abus_address_buf             : std_logic_vector( 8 downto 0) := (others => '0'); -- abus.address
signal abus_addressdata_ms          : std_logic_vector(15 downto 0) := (others => '0'); --     .data
signal abus_addressdata_buf         : std_logic_vector(15 downto 0) := (others => '0'); --     .data

signal abus_chipselect_p1           : std_logic_vector( 2 downto 0) := (others => '0'); --     .chipselect
signal abus_chipselect_p2         : std_logic_vector( 2 downto 0) := (others => '0'); --     .chipselect
signal abus_chipselect_buf2         : std_logic_vector( 2 downto 0) := (others => '0'); --     .chipselect

signal abus_read_ms                 : std_logic                     := '0';             --     .read
signal abus_write_ms                : std_logic_vector( 1 downto 0) := (others => '0'); --     .write
signal abus_writeneg_ms             : std_logic_vector( 1 downto 0) := (others => '0'); --     .write
signal abus_write_buf               : std_logic_vector( 1 downto 0) := (others => '0'); --     .write

signal abus_writedata_buf           : std_logic_vector(15 downto 0) := (others => '0'); --     .data

signal abus_read_trail              : std_logic_vector((ABUS_TRAILS_SIZE-1) downto 0) := (others => '0'); -- RD

signal abus_write_buf2              : std_logic_vector( 1 downto 0) := (others => '0'); --     .write
signal abus_writeneg0_trail         : std_logic_vector((ABUS_TRAILS_SIZE-1) downto 0) := (others => '0'); -- WR0
signal abus_writeneg1_trail         : std_logic_vector((ABUS_TRAILS_SIZE-1) downto 0) := (others => '0'); -- WR1

signal abus_trcntr                  : std_logic_vector( 3 downto 0) := (others => '0'); -- Transaction state counter

signal abus_read_pulse              : std_logic                     := '0';             --     .read
signal abus_reading                 : std_logic                     := '0';             --     .read
signal abus_write_pulse             : std_logic                     := '0';             --     .write
signal abus_writing                 : std_logic                     := '0';             --     .write

-- Trails to properly delay write pipeline
signal abus_write_pulse_trail       : std_logic_vector((ABUS_TRAILS_SIZE-1) downto 0) := (others => '0'); -- write
signal abus_writing_trail           : std_logic_vector((ABUS_TRAILS_SIZE-1) downto 0) := (others => '0'); -- write

-- Buffers to hold avalon parameters during transaction
signal avalon_writedata_buff        : std_logic_vector(15 downto 0) := (others => '0');
signal avalon_byteenable_buff       : std_logic_vector( 1 downto 0) := (others => '0');
signal avalon_write_buff            : std_logic                     := '0';


signal abus_read_pulse_dmy          : std_logic                     := '0';             --     .read
signal abus_write_pulse_dmy         : std_logic_vector( 1 downto 0) := (others => '0'); --     .write
signal abus_chipselect_pulse        : std_logic_vector( 2 downto 0) := (others => '0'); --     .chipselect
signal abus_read_pulse_off          : std_logic                     := '0';             --     .read
signal abus_write_pulse_off         : std_logic_vector( 1 downto 0) := (others => '0'); --     .write
signal abus_chipselect_pulse_off    : std_logic_vector( 2 downto 0) := (others => '0'); --     .chipselect

signal abus_anypulse                : std_logic                     := '0'; 
signal abus_anypulse2               : std_logic                     := '0'; 
signal abus_anypulse3               : std_logic                     := '0'; 
signal abus_anypulse_off            : std_logic                     := '0'; 

signal abus_cspulse_trail           : std_logic_vector((ABUS_TRAILS_SIZE-1) downto 0) := (others => '0');
signal abus_cspulse_off             : std_logic                     := '0'; 

signal abus_chipselect_latched      : std_logic_vector( 2 downto 0) := (others => '1'); -- abus.chipselect
signal abus_read_latched            : std_logic                     := '0';             --     .read
signal abus_write_latched           : std_logic_vector( 1 downto 0) := (others => '0'); --     .write
signal abus_address_latched         : std_logic_vector(23 downto 0) := (others => '0'); --     .address

signal abus_direction_internal      : std_logic                     := '0'; --high-z
signal abus_muxing_internal         : std_logic_vector( 1 downto 0) := "01"; -- sample address

signal abus_data_out                : std_logic_vector(15 downto 0) := (others => '0');
signal abus_data_in                 : std_logic_vector(15 downto 0) := (others => '0');

signal abus_waitrequest_read1       : std_logic                     := '0'; 
signal abus_waitrequest_read2       : std_logic                     := '0'; 
signal abus_waitrequest_write1      : std_logic                     := '0'; 
signal abus_waitrequest_write2      : std_logic                     := '0'; 
signal abus_waitrequest_read_off    : std_logic                     := '0'; 
signal abus_waitrequest_write_off   : std_logic                     := '0'; 


-- Access test stuff, added 2019/11/04 vvv
signal rdwr_access_buff : std_logic_vector(127 downto 0) := x"CAFE0304050607080910111213141516";
-- Access test stuff, added 2019/11/04 ^^^

-- External signals used for simulation vvv
signal sim_test1_internal : std_logic_vector( 7 downto 0) := x"CA";
signal sim_test2_internal : std_logic_vector( 7 downto 0) := x"FE";
-- External signals used for simulation ^^^

-- For Rd/Wr access debug
signal rd_access_cntr : std_logic_vector( 7 downto 0) := x"01";
signal wr_access_cntr : std_logic_vector( 7 downto 0) := x"01";
signal last_wr_data   : std_logic_vector(15 downto 0) := x"5678";


signal REG_PCNTR      : std_logic_vector(15 downto 0) := (others => '0');  
signal REG_STATUS     : std_logic_vector(15 downto 0) := (others => '0'); 
signal REG_MODE       : std_logic_vector(15 downto 0) := (others => '0'); 
signal REG_HWVER      : std_logic_vector(15 downto 0) := X"0002";
signal REG_SWVER      : std_logic_vector(15 downto 0) := (others => '0'); 


TYPE transaction_dir IS (DIR_NONE,DIR_WRITE,DIR_READ);
SIGNAL my_little_transaction_dir : transaction_dir := DIR_NONE;

TYPE wasca_mode_type IS (MODE_INIT,
                          MODE_POWER_MEMORY_05M, MODE_POWER_MEMORY_1M, MODE_POWER_MEMORY_2M, MODE_POWER_MEMORY_4M,
                          MODE_RAM_1M, MODE_RAM_4M,
                          MODE_ROM_KOF95,
                          MODE_ROM_ULTRAMAN,
                          MODE_BOOT);
SIGNAL wasca_mode : wasca_mode_type := MODE_INIT;
 

begin

    --ignoring functioncode, timing and addressstrobe for now

    --abus transactions are async, so first we must latch incoming signals
    --to get rid of metastability
    process (clock)
    begin
        if rising_edge(clock) then
            --1st stage
            abus_address_ms <= abus_address;
            abus_addressdata_ms <= abus_addressdata;
            abus_chipselect_p1 <= abus_chipselect;
            abus_read_ms <= abus_read;
            abus_write_ms <= abus_write;
            abus_writeneg_ms <= not abus_write;

            --2nd stage
            abus_address_buf <= abus_address_ms;
            abus_addressdata_buf <= abus_addressdata_ms;
            abus_chipselect_p2 <= abus_chipselect_p1;
            abus_read_trail(0) <= abus_read_ms;
            abus_write_buf <= abus_write_ms;
            abus_writeneg0_trail(0) <= abus_writeneg_ms(0);
            abus_writeneg1_trail(0) <= abus_writeneg_ms(1);
        end if;
    end process;
    
    --excluding metastability protection is a bad behavior
    --but it looks like we're out of more options to optimize read pipeline
    --abus_read_ms <= abus_read;
    --abus_read_buf <= abus_read_ms;

    --abus read/write latch
    process (clock)
    begin
        if rising_edge(clock) then
            abus_write_buf2      <= abus_write_buf;
            abus_chipselect_buf2 <= abus_chipselect_p2;
            abus_anypulse2       <= abus_anypulse;
            abus_anypulse3       <= abus_anypulse2;

            for i in 0 to (ABUS_TRAILS_SIZE-2) loop
                abus_cspulse_trail  (i+1) <= abus_cspulse_trail  (i);
                abus_read_trail     (i+1) <= abus_read_trail     (i);
                abus_writeneg0_trail(i+1) <= abus_writeneg0_trail(i);
                abus_writeneg1_trail(i+1) <= abus_writeneg1_trail(i);
            end loop;
        end if;
    end process;

    --abus write/read pulse is a falling edge since read and write signals are negative polarity
    abus_write_pulse_dmy   <= abus_write_buf2 and not abus_write_buf;
    abus_read_pulse_dmy    <= abus_read_trail(1) and not abus_read_trail(0);
    --abus_chipselect_pulse  <= abus_chipselect_buf2 and not abus_chipselect_p2;
    abus_chipselect_pulse  <= abus_chipselect_p2 and not abus_chipselect_p1;

    abus_write_pulse_off        <= abus_write_buf and not abus_write_buf2;
    abus_read_pulse_off         <= abus_read_trail(0) and not abus_read_trail(1);
    abus_chipselect_pulse_off   <= abus_chipselect_p2 and not abus_chipselect_buf2;

    abus_anypulse <= abus_write_pulse_dmy(0)
                  or abus_write_pulse_dmy(1)
                  or abus_read_pulse_dmy
                  or abus_chipselect_pulse(0)
                  or abus_chipselect_pulse(1)
                  or abus_chipselect_pulse(2);

    abus_anypulse_off <= abus_write_pulse_off(0)
                      or abus_write_pulse_off(1)
                      or abus_read_pulse_off
                      or abus_chipselect_pulse_off(0)
                      or abus_chipselect_pulse_off(1)
                      or abus_chipselect_pulse_off(2);

    abus_cspulse_trail(0) <= abus_chipselect_pulse(0)
                          or abus_chipselect_pulse(1)
                          or abus_chipselect_pulse(2);

    abus_cspulse_off <= abus_chipselect_pulse_off(0)
                     or abus_chipselect_pulse_off(1)
                     or abus_chipselect_pulse_off(2);

    -- Transaction counter update
    -- 
    -- Concept
    -- | - It is assumed that CS0~1 lines become active last and that RD/WR/ADDR/DATA lines are setup before that moment.
    -- |    -> should need to measure that for real with OLS !
    -- |    -> Timeline is as follow :
    -- |         abus_chipselect_p2 (two clocks before)
    -- |         abus_chipselect_p1 (one clock before)
    -- |         abus_chipselect    (latest)
    -- | - Let's wait for one MAX 10 internal (116 MHz) clock after CS is active and start sampling other signals.
    -- | - During write transaction (write from Saturn to cartridge), data is demuxed during two clocks : this may not be needed but seems safer in a first try.
    -- 
    -- 
    -- COMMON
    -- | - Retrieve read/write type and latch full address
    -- 
    -- CARTRIDGE READ FROM SATURN
    -- | - If register, process it immediately
    -- | - If SDRAM or OCRAM, start transaction and wait until read is valid
    -- 
    -- CARTRIDGE WRITE FROM SATURN
    -- | - Multiplex data/address to retrieve write data
    -- | - If register, process it immediately
    -- | - If SDRAM or OCRAM, start transaction and hold it until write is terminated
    -- 
    -- Rules about mutiplexer control :
    --  1. Control with transaction counter, because in incremented during CS activity.
    --  2. Keep enough clocks at the beginning of CS activity to retrieve full address, and data if needed.
    --  3. After that, select and hold (until transaction counter becomes zero) appropriate bus direction.
    process (clock)
    begin
        if rising_edge(clock) then
            if saturn_reset = '0' then
                abus_trcntr <= x"0";                            -- Return to idle state during Saturn reset
            else
                if abus_trcntr = x"0" then
                    if  ((abus_chipselect_p2(0) = '1') and (abus_chipselect_p2(1) = '1')
                    and ((abus_chipselect_p1(0) = '0') or  (abus_chipselect_p1(1) = '0'))) then
                        abus_trcntr <= x"1";                    -- Transaction startup
                    else
                        abus_trcntr <= x"0";
                    end if;
                else
                    if (abus_chipselect(0) = '1') and (abus_chipselect(1) = '1') then
                        abus_trcntr <= x"0";                    -- Return to idle state when CS returns to idle
                    else
                        if abus_trcntr = x"F" then
                            abus_trcntr <= x"F";                -- Hold counter to max until CS returns to idle
                        else
                            abus_trcntr <= abus_trcntr + x"1";  -- Go to next state
                        end if;
                    end if;
                end if;
            end if;
        end if;
    end process;
    sim_test2_internal(3 downto 0) <= abus_trcntr(3 downto 0);



    --whatever pulse we've got, latch address
    --it might be latched twice per transaction, but it's not a problem
    --multiplexer was switched to address after previous transaction or after boot,
    --so we have address ready to latch
    process (clock)
    begin
        if rising_edge(clock) then
            if abus_trcntr = x"0" then
                demux_readpulse  <= '0';
                demux_writepulse <= '0';

            elsif abus_trcntr = x"1" then

                -- Latch access base signals
                abus_chipselect_latched <= abus_chipselect;
                abus_read_latched       <= abus_read;
                abus_write_latched      <= abus_write;

                -- Generate read pulse (1/2)
                if abus_read = '0' then
                    demux_readpulse <= '1';
                end if;

            elsif abus_trcntr = x"2" then

                -- Generate read pulse (2/2)
                if my_little_transaction_dir = DIR_READ then
                    demux_readpulse <= '0';
                end if;

            elsif abus_trcntr = x"4" then

                -- Retrieve multiplexed data for write transaction, and generate write pulse
                if my_little_transaction_dir = DIR_WRITE then
                    demux_writedata <= abus_data_in;

                    demux_writepulse <= '1';
                end if;

            elsif abus_trcntr = x"5" then

                -- Terminate write pulse
                if my_little_transaction_dir = DIR_WRITE then
                    demux_writepulse <= '0';
                end if;

            end if;
        end if;
    end process;


    -- De-shuffle address
    process(abus_address_latched, abus_address, abus_addressdata)
    begin
        if(abus_muxing_internal = "01") then
        --if(abus_read = '1') then
            -- 2019/07/20 : this is now adapted for "SIM to MAX 10 Board", 
            -- which allows multiplexing simpler than on cartridge
            abus_address_latched <= 
                  abus_address(7) -- abus_address(8) ignored ?!
                & abus_address(6)
                & abus_address(5)
                & abus_address(4)
                & abus_address(3)
                & abus_address(2)
                & abus_address(1)
                & abus_address(0) -- TOP ADDRESS  ^^^
                & abus_addressdata(15) -- MUX vvv
                & abus_addressdata(14)
                & abus_addressdata(13)
                & abus_addressdata(12)
                & abus_addressdata(11)
                & abus_addressdata(10)
                & abus_addressdata( 9)
                & abus_addressdata( 8)
                & abus_addressdata( 7)
                & abus_addressdata( 6)
                & abus_addressdata( 5)
                & abus_addressdata( 4)
                & abus_addressdata( 3)
                & abus_addressdata( 2)
                & abus_addressdata( 1)
                & abus_addressdata( 0);

            --Purpose of A0 line in PCB Rev 1.3 is unknown and consequently        
            --have to be ignored when building address. Instead, address           
            --top bit is stuffed with '0'.                                         
            --Address Mapping for U4 :    And for U1 :    (In PCB Rev 1.3)         abus_address_latched <= abus_address
            -- A13 -> MUX12                A0  -> MUX0                                         & abus_addressdata(11)  -- A14
            -- A6  -> MUX13                A9  -> MUX1                                         & abus_addressdata(12)  -- A13
            -- A5  -> MUX14                A10 -> MUX2                                         & abus_addressdata( 9)  -- A12
            -- A4  -> MUX15                A8  -> MUX3                                         & abus_addressdata(10)  -- A11
            -- A3  -> MUX4                 A7  -> MUX8                                         & abus_addressdata( 2)  -- A10
            -- A2  -> MUX5                 A12 -> MUX9                                         & abus_addressdata( 1)  -- A9
            -- A1  -> MUX6                 A11 -> MUX10                                        & abus_addressdata( 3)  -- A8
            -- DMY -> MUX7                 A14 -> MUX11                                        & abus_addressdata( 8)  -- A7
            --Which gives the following order for de-shuffling address :                       & abus_addressdata(13)  -- A6
            -- A14 -> MUX11                                                                    & abus_addressdata(14)  -- A5
            -- A13 -> MUX12                                                                    & abus_addressdata(15)  -- A4
            -- A12 -> MUX9                                                                     & abus_addressdata( 4)  -- A3
            -- A11 -> MUX10                                                                    & abus_addressdata( 5)  -- A2
            -- A10 -> MUX2                                                                     & abus_addressdata( 6)  -- A1
            -- A9  -> MUX1                                                                     & abus_addressdata( 0); -- A0
            -- A8  -> MUX3                                                         
            -- A7  -> MUX8                                                         
            -- A6  -> MUX13                                                        
            -- A5  -> MUX14                                                        
            -- A4  -> MUX15                                                        
            -- A3  -> MUX4                                                         
            -- A2  -> MUX5                                                         
            -- A1  -> MUX6                                                         
            -- A0  -> MUX0                                                         
        end if;
    end process;



    -- Update the following "static" informations while idle :
    --  - Demuxed address, including CS0~1
    --  - Write byte enable
    process (clock)
    begin
        if rising_edge(clock) then

            -- Address and CS0~1
            if abus_trcntr = x"1" then
            --if((clock = '1') and ((abus_trcntr = x"0") or (abus_trcntr = x"1")))then

                -- Put both address itself and chipselect on the same demuxed address
                -- Upper bits of demuxed address are currently unused and reserved for eventual future purpose.
                -- 
                -- And, separate address for both write and read access, so that write operation
                -- have more chances to terminate even when read operation starts just after.
                if(abus_read = '0') then

                    demux_readaddress(27) <= '0';
                    demux_readaddress(26) <= '0';

                    if abus_chipselect(0) = '0' then
                        demux_readaddress(25 downto 24) <= "00";
                    elsif abus_chipselect(1) = '0' then
                        demux_readaddress(25 downto 24) <= "01";
                    elsif abus_chipselect(2) = '0' then
                        demux_readaddress(25 downto 24) <= "10";
                    else
                        demux_readaddress(25 downto 24) <= "11"; -- Shouldn't happen since transaction is initiated when activity on CS is detected.
                    end if;

                    demux_readaddress(23 downto 0) <= abus_address_latched(23 downto 0);
                else

                    demux_writeaddress(27) <= '0';
                    demux_writeaddress(26) <= '0';

                    if abus_chipselect(0) = '0' then
                        demux_writeaddress(25 downto 24) <= "00";
                    elsif abus_chipselect(1) = '0' then
                        demux_writeaddress(25 downto 24) <= "01";
                    elsif abus_chipselect(2) = '0' then
                        demux_writeaddress(25 downto 24) <= "10";
                    else
                        demux_writeaddress(25 downto 24) <= "11"; -- Shouldn't happen since transaction is initiated when activity on CS is detected.
                    end if;

                    demux_writeaddress(23 downto 0) <= abus_address_latched(23 downto 0);
                end if;
            end if;
        end if;
    end process;


    process (clock)
    begin
        if saturn_reset = '0' then
            demux_write_byteenable(0) <= '0';
            demux_write_byteenable(1) <= '0';
        else
            -- Write byte enable
            -- Keep holding it even while read operation started, 
            -- so that eventual ongoing write operation have more
            -- chances to terminate correctly.
            if((abus_trcntr = x"1") and (abus_read = '1'))then

                demux_write_byteenable(0) <= not abus_write(0);
                demux_write_byteenable(1) <= not abus_write(1);

            end if;
        end if;
    end process;



    -- If valid transaction captured, switch to corresponding multiplex mode
    process (clock)
    begin
        if rising_edge(clock) then
            if((abus_trcntr = x"0") or ((abus_chipselect(0) = '1') and (abus_chipselect(1) = '1')))then
                abus_direction_internal <= '0' ; --high-z
                abus_muxing_internal    <= "01"; --address


            elsif abus_trcntr = x"1" then
                if abus_read = '0' then
                    abus_direction_internal <= '1' ; --active
                    abus_muxing_internal    <= "10"; --data
                else
                    abus_direction_internal <= '0' ; --high-z
                    abus_muxing_internal    <= "10"; --data
                end if;
            elsif abus_trcntr = x"2" then
                if abus_read = '0' then
                    abus_direction_internal <= '1' ; --active
                    abus_muxing_internal    <= "10"; --data
                else
                    abus_direction_internal <= '0' ; --high-z
                    abus_muxing_internal    <= "10"; --data
                end if;
-- Long multiplexer TEST vvv
-- Note 2019/12/13 : multiplexing during 1~2 is necessary for Wasca on real Hardware.
-- And so far it wasn't verified if multiplexing during 3 is necessary for MAX 10 Board r0.7 (b).
            -- elsif abus_trcntr = x"3" then
            --     if abus_read = '0' then
            --         abus_direction_internal <= '1' ; --active
            --         abus_muxing_internal    <= "10"; --data
            --     else
            --         abus_direction_internal <= '0' ; --high-z
            --         abus_muxing_internal    <= "10"; --data
            --     end if;
-- Long multiplexer TEST ^^^

            else
                if my_little_transaction_dir = DIR_READ then
                    abus_direction_internal <= '1' ; --active
                    abus_muxing_internal    <= "10"; --data
                else --if my_little_transaction_dir = DIR_READ then
                    abus_direction_internal <= '0' ; --high-z
                    abus_muxing_internal    <= "01"; --address
                end if;
            end if;
        end if;
    end process;
    abus_direction <= abus_direction_internal;
    abus_muxing <= not abus_muxing_internal;


    -- Update access direction
    process (clock)
    begin
        if rising_edge(clock) then

            if((abus_chipselect(0) = '1') and (abus_chipselect(1) = '1'))then

                my_little_transaction_dir <= DIR_NONE;
                abus_writing              <= '0';
                abus_reading              <= '0';

            elsif abus_trcntr = x"1" then

                -- Decide access direction, and hold it until end of transaction
                if abus_read = '0' then
                    my_little_transaction_dir <= DIR_READ;
                    abus_writing              <= '0';
                    abus_reading              <= '1';

                else --if abus_read_ms = '0' then
                    my_little_transaction_dir <= DIR_WRITE;
                    abus_writing              <= '1';
                    abus_reading              <= '0';

                end if;
            end if;
        end if;
    end process;


    -- Generate a reading/writing pulse during access
    process (clock)
    begin
        if rising_edge(clock) then

            if abus_trcntr = x"0" then
                abus_write_pulse <= '0';                -- No access during idle state
                abus_read_pulse  <= '0';                -- No access during idle state

            elsif abus_trcntr = x"1" then
                if abus_write(0) = '0' or abus_write(1) = '0' then
                    abus_write_pulse <= '0';            -- Wait to receive data during a write transaction
                    abus_read_pulse  <= '0';

                else --if abus_read_ms = '0' then
                    abus_write_pulse <= '0';
                    abus_read_pulse  <= '1';            -- Generate a read pulse when full address is received

                end if;

            elsif abus_trcntr = x"2" then
                if my_little_transaction_dir = DIR_WRITE then
                    abus_write_pulse <= '1';            -- Generate a write pulse when both full address and data are received
                    abus_read_pulse  <= '0';

                else --if my_little_transaction_dir = DIR_READ then
                    abus_write_pulse <= '0';
                    abus_read_pulse  <= '0';            -- Read transaction falling edge

                end if;

            elsif abus_trcntr = x"3" then
                abus_read_pulse  <= '0';                -- Write transaction falling edge
                abus_write_pulse <= '0';

                if my_little_transaction_dir = DIR_WRITE then

                else --if my_little_transaction_dir = DIR_READ then

                end if;

            else
                abus_read_pulse  <= '0';                -- Write transaction falling edge
                abus_write_pulse <= '0';

                if my_little_transaction_dir = DIR_WRITE then

                else --if my_little_transaction_dir = DIR_READ then

                end if;
            end if;


            -- Delay write related signals
            abus_writing_trail    (0) <= abus_writing;
            abus_write_pulse_trail(0) <= abus_write_pulse;
            for i in 0 to (ABUS_TRAILS_SIZE-2) loop
                abus_writing_trail    (i+1) <= abus_writing_trail    (i);
                abus_write_pulse_trail(i+1) <= abus_write_pulse_trail(i);
            end loop;

        end if;
    end process;

sim_test2_internal(7) <= abus_write_pulse_trail(3);
sim_test2_internal(6) <= abus_write_pulse_trail(2);
sim_test2_internal(5) <= abus_write_pulse_trail(1);
sim_test2_internal(4) <= abus_write_pulse_trail(0);



    ---------------------------------------------------------------------------------------
    -- Update buffer data when read data valid pulse if detected
    -- Read data valid pulse may not be super necessary, but this is provided by Avalon, 
    -- and may be helpful when counting clocks elapsed during read access.
    --
    process (abus_reading) begin
        abus_data_out <= demux_readdata;            -- Faster than below
        if rising_edge(clock) then
            if(demux_readdatavalid = '1') then
                --abus_data_out <= demux_readdata;
            end if;
        end if;
    end process;


    ---------------------------------------------------------------------------------------
    -- In/Out process
    --
    -- Access from each CS0-2 is handled from this chip.
    -- In the future, it may be required to manage access to external FTDI chip ?
    --
    process (abus_reading) begin

        --if rising_edge(clock) then
            --if(abus_reading = '1') then
            if(abus_read = '0') then
                -- Output to data bus
                abus_addressdata <= abus_data_out;
--abus_addressdata <= x"ABCD";
--sim_test2_internal <= x"CA";
                abus_data_in <= abus_addressdata;

            else
                -- Disable output to data bus
                abus_addressdata <= "ZZZZZZZZZZZZZZZZ";
                abus_data_in <= abus_addressdata;
--sim_test2_internal <= x"FF";

            end if;
        --end if;
    end process;
sim_test1_internal(7) <= abus_writing;
sim_test1_internal(6) <= abus_reading;
sim_test1_internal(5) <= abus_write_pulse;
sim_test1_internal(4) <= abus_read_pulse;

sim_test1_internal(3) <= abus_writing_trail(0);
sim_test1_internal(2) <= abus_writing_trail(1);
sim_test1_internal(1) <= abus_writing_trail(2);
sim_test1_internal(0) <= '1';

    -- "disable_out" controls refresh timing of wait and IRQ signals
    -- 0:output, 1:hold previous
    --abus_disable_out <= '1' when abus_chipselect_number(1) = '1' else '0';
    abus_disable_out <= '0';


    -- Let's completely neglect usage of wait request signal for now, and ... hope that
    -- SDRAM controller is smarter enough to do things timely.
    -- (Spoiler : SDRAM controller is completely dumb)
    abus_waitrequest <= '1';
    -- process (clock)
    -- begin
    --     if rising_edge(clock) then
    --         abus_waitrequest_read2 <= abus_waitrequest_read1;
    --         abus_waitrequest_write2 <= abus_waitrequest_write1;
    --     end if;
    -- end process;
    -- 
    -- process (clock)
    -- begin
    --     if rising_edge(clock) then
    --         abus_waitrequest_read_off <= '0';
    --         abus_waitrequest_write_off <= '0';
    --         if abus_waitrequest_read1 = '0' and abus_waitrequest_read2 = '1' then
    --             abus_waitrequest_read_off <= '1';
    --         end if;
    --         if abus_waitrequest_write1 = '0' and abus_waitrequest_write2 = '1' then
    --             abus_waitrequest_write_off <= '1';
    --         end if;
    --     end if;
    -- end process;
    -- 
    -- --process (clock)
    -- --begin
    -- --  if rising_edge(clock) then
    -- --      --if abus_read_pulse_dmy='1' or abus_write_pulse_dmy(0)='1' or abus_write_pulse_dmy(1)='1' then
    -- --      --if abus_anypulse = '1' then
    -- --      if abus_chipselect_pulse(0) = '1' or abus_chipselect_pulse(1) = '1' then
    -- --          abus_waitrequest <= '0';
    -- --      elsif abus_waitrequest_read_off='1' or abus_waitrequest_write_off='1' then
    -- --          abus_waitrequest <= '1';
    -- --      end if;
    -- --  end if;
    -- --end process;
    -- 
    -- abus_waitrequest <= not (abus_waitrequest_read1 or abus_waitrequest_write1);



    --Nios II read interface
    process (clock)
    begin
        if rising_edge(clock) then
            if avalon_nios_read = '1' then
                avalon_nios_readdatavalid <= '1';
                case avalon_nios_address is 

                    -- Debug stuff around Rd/Wr access
                    when X"00" => 
                        avalon_nios_readdata <= x"CA0000" & rd_access_cntr;
                    when X"01" => 
                        avalon_nios_readdata <= x"FE0000" & wr_access_cntr;

                    when X"10" => 
                        avalon_nios_readdata <= x"0000" & REG_PCNTR;
                    when X"11" =>
                        avalon_nios_readdata <= x"0000" & REG_STATUS;
                    when X"12" =>
                        avalon_nios_readdata <= x"0000" & REG_MODE;
                    when X"13" =>
                        avalon_nios_readdata <= x"0000" & REG_HWVER;
                    when X"14" =>
                        avalon_nios_readdata <= x"0000" & REG_SWVER;
                    when X"15" =>
                        avalon_nios_readdata <= X"0000ABCD"; --for debug, remove later
                    when others =>
                        avalon_nios_readdata <= x"00000000";
                end case;
            else
                avalon_nios_readdatavalid <= '0';
            end if;
        end if;
    end process;

    --Nios II write interface
    process (clock)
    begin
        if rising_edge(clock) then
            if avalon_nios_write= '1' then
                case avalon_nios_address is 
                    when X"10" => 
                        REG_PCNTR <= avalon_nios_writedata(15 downto 0);
                    when X"11" =>
                        REG_STATUS <= avalon_nios_writedata(15 downto 0);
                    when X"12" =>
                        null;
                    when X"13" =>
                        null;
                    when X"14" =>
                        REG_SWVER <= avalon_nios_writedata(15 downto 0);
                    when others =>
                        null;
                end case;
            end if;
        end if;
    end process;

    --Nios system interface is only regs, so always ready to write.
    avalon_nios_waitrequest <= '0';

-- -- External signals used for simulation vvv
--    sim_test1 <= sim_test1_internal;
--    sim_test2 <= sim_test2_internal;

--    abus_cspulse_trail_dbg   <= abus_cspulse_trail;
--    abus_read_trail_dbg      <= abus_read_trail;
--    abus_writeneg0_trail_dbg <= abus_writeneg0_trail;
--    abus_writeneg1_trail_dbg <= abus_writeneg1_trail;
--    abus_addresslatched_dbg  <= abus_address_latched;
-- -- External signals used for simulation ^^^

end architecture rtl; -- of abus_demux
