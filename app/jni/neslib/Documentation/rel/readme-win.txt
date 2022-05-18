                                   FCE Ultra
                                     0.81

                         http://fceultra.sourceforge.net/


What is new:

	*  Screen snapshots can now be taken while playing an NSF.
	*  Saving screen snapshots will no longer corrupt the frame buffer.
	*  Added many more games to the list of games that usually are found
	   with bad iNES headers.
	*  Fixed more network play bugs.  It should now work correctly(how
	   many times have I said or implied that...).
	*  The NSF player will now disable the FDS sound channel on song
	   initialization(fixes a problem with the Zelda no Densetsu
	   rip).
	*  Reads from $4090 and $4092 while emulating the FDS will now return
	   the current envelope settings.  Affects "Ai Senshi Nicole" 
	   and "Bio Miracle Bokutte Upa", at least.
	*  Merged a lot of pirate MMC3 multicart emulation code with the main
	   MMC3 emulation code.
	*  Added support for the MMC5's split-screen mode.  This fixes the
	   introduction in "Uchuu Keibitai SDF".
	*  Writes to $8000-$FFFF with D7 set during MMC1 emulation will
	   cause the MMC1 mode register to be OR'd with $C.  This fixes
	   "Robocop 3".
	*  Replaced an MMC1 hack that I used to get "Bill and Ted's Excellent
	   Video Game Adventure" to work with something more accurate.
	*  Fixed the MMC5 read handler to return the data last on the data
	   bus instead of $FF when a read occured to an unmapped address.
	   This fixes the lockup problem in "Bandit Kings of Ancient China"
	   and possibly other games.
	*  Added support for the game "Ishin no Arashi" in the iNES format
	   (I added an entry with its CRC32 value and the number of 8KB
           WRAM banks it needs into the MMC5 WRAM size table).
	*  Added support for MMC1 games in the iNES format with 16KB of RAM
           via CRC32 comparisons(currently only Genghis Khan(USA), Romance
	   of the 3 Kingdoms(USA), and Nobunaga's Ambition(USA and Japan) are 
           recognized).
	*  iNES mapper 1 now supports pageable CHR RAM if CHR ROM is not
	   present.  Fixes "Family School".
	*  Added support for iNES mappers 51 and 52.  Thanks to Kevin Horton
	   for the information.
	*  Modified MMC3(iNES mapper 4/118/119) IRQ counter emulation.  Fixes
	   problems in "MegaMan 3", "Gun Nac", and the Japanese version of
	   "Klax", but it *might* cause problems with other games.
	*  Fixed an iNES mapper 32 emulation bug.  "Ai Sensei no Oshiete"
	   works now.
	*  Fixed iNES mapper 33/48 IRQ emulation.
	*  Fixed iNES mapper 16 IRQ emulation.
	*  Added support for "Famicom Jump 2" as iNES mapper 153.
           If a good(as far as I can tell) dump is loaded, FCE Ultra will
           automatically fix the mapper number.
	*  The VS Unisystem bit in iNES headers is no longer recognized.
	   Too many games have this bit set when it shouldn't be set.
	   Now VS Unisystem games are recognized by CRC32 value.
	*  Reads from $4015 no longer reset DMC IRQ.  This fixes the
	   title screen of "Romancia".
	*  PPU NMI now occurs a few cycles later.  Fixes the "BattleToads"
	   lockup problem.
	*  BRK emulation now sets the I flag.
	*  Changed a few zero-page address mode functions to read directly
	   from emulated RAM.


Contents:

  1.  Basic information
        1.0 What FCE Ultra is.
        1.1 System requirements.
  2.  How to use
        2.0 Starting FCE Ultra
        2.1 Using FCE Ultra
  3.  Notes
        3.0 Platform Specific Notes
        3.1 Network Play Notes
        3.2 VS Unisystem Notes
        3.3 Famicom Disk System Notes
        3.4 Light Gun Notes
        3.5 Palette Notes
        3.6 Compressed File Notes
	3.7 Game Genie Notes
  4.  Extra
        4.0 Contacting the author
        4.1 Credits

/******************************************************************************/
/*  1.0)	What FCE Ultra is:                                            */
/******************************************************************************/

        FCE Ultra is an NTSC and PAL Famicom/NES emulator for various 
        platforms. It is based upon Bero's original FCE source code.  Current
	features include good PPU, CPU, pAPU, expansion chip, and joystick
	emulation.  Also a feature unique to this emulator(at the current
        time) is authentic Game Genie emulation.  Save states and snapshot
	features also have been implemented.  The VS Unisystem is emulated
        as well.  FCE Ultra supports iNES format ROM images, UNIF format ROM
	images, headerless and FWNES style FDS disk images, and NSF files.

        FCE Ultra currently supports the following iNES mappers(many partially):

Number:         Description:                    Game Examples:
--------------------------------------------------------------------------------
  0             No Mapper                       Donkey Kong, Mario Bros
  1             Nintendo MMC1                   MegaMan 2, Final Fantasy
  2             Simple 16KB PRG Switch          MegaMan 1, Archon, 1944
  3             Simple 8KB CHR Switch           Spy Hunter, Gradius
  4             Nintendo MMC3                   Recca, TMNT 2, Final Fantasy 3
  5             Nintendo MMC5                   Castlevania 3, Just Breed, Uchuu Keibitai SDF
  6             FFE F4 Series(hacked)           Saint Seiya, Ganbare Goemon
  7             AOROM                           Battle Toads, Lion King
  8             FFE F3 Series(hacked)           Doraemon Kaitakuhen
  9             Nintendo MMC2                   Punchout!
 10             Nintendo MMC4                   Fire Emblem, Fire Emblem Gaiden
 11             Color Dreams                    Crystal Mines, Bible Adventures
 13             CPROM                           Videomation
 15             Multi-cart(pirate)              100-in-1: Contra Function 16
 16             Bandai                          Dragon Ball Z, Gundam Knight
 17             FFE F8 Series(hacked)           Parodius, Last Armageddon
 18             Jaleco SS806                    Pizza Pop, Plazma Ball
 19             Namco 106                       Splatter House, Mappy Kids                
 21             Konami VRC4 2A                  WaiWai World 2, Ganbare Goemon Gaiden 2
 22             Konami VRC4 1B                  Twinbee 3
 23             Konami VRC2B                    WaiWai World, Getsufuu Maden
 24             Konami VRC6                     Akumajo Densetsu(Dracula 3)
 25             Konami VRC4                     Gradius 2, Bio Miracle: Boku tte Upa
 26             Konami VRC6 A0-A1 Inverse       Esper Dream 2, Madara
 32             Irem G-101                      Image Fight 2, Perman
 33             Taito TC0190/TC0350             Don Doko Don 1&2
 34             NINA-001 and BNROM		Impossible Mission 2, Deadly Towers, Bug Honey
 40             (pirate)                        Super Mario Bros. 2
 41             Caltron 6-in-1                  Caltron 6-in-1
 42		(pirate)			"Mario Baby"
 43		Multi-cart(pirate)		Golden Game 150 in 1
 44		Multi-cart(pirate)		Super HiK 7 in 1	
 45		Multi-cart(pirate)		Super 1000000 in 1
 46		Game Station			Rumble Station
 47		NES-QJ				Nintendo World Cup/Super Spike V.B.
 48		Taito TC190V			Flintstones
 49             Multi-cart(pirate)              Super HiK 4 in 1
 51		Multi-cart(pirate)		11 in 1 Ball Games
 52		Multi-cart(pirate)		Mario Party 7 in 1
 64             Tengen RAMBO-1                  Shinobi, Klax
 65             Irem H-3001                     Daiku no Gensan 2
 66             GNROM                           SMB + Duck Hunt
 67             Sunsoft Mapper "3"              Fantasy Zone 2
 68             Sunsoft Mapper "4"              After Burner 2, Nantetta Baseball 
 69             Sunsoft FME-7                   Batman: ROTJ, Gimmick!
 70		??				Kamen Rider Club
 71             Camerica                        Fire Hawk, Linus Spacehead
 72		Jaleco ??			Pinball Quest
 73             Konami VRC3                     Salamander
 75             Jaleco SS8805/Konami VRC1       Tetsuwan Atom, King Kong 2
 76             Namco 109                       Megami Tenshi 1
 77             Irem ??                         Napoleon Senki
 78             Irem 74HC161/32                 Holy Diver
 79             NINA-06				F15 City War, Krazy Kreatures
 80             Taito X-005                     Minelvation Saga
 82		Taito ??			Kyuukyoku Harikiri Stadium - Heisei Gannen Ban			
 83		Multi-cart(pirate)		Dragon Ball Party
 85             Konami VRC7                     Lagrange Point
 86		Jaleco ??			More Pro Baseball
 87		??				Argus
 89		Sunsoft ??			Mito Koumon
 90		Pirate                          Super Mario World, Mortal Kombat
 92             Jaleco ??                       MOERO Pro Soccer, MOERO Pro Yakyuu '88
 93		??				Fantasy Zone
 94		??				Senjou no Ookami
 95		Namco ??			Dragon Buster
 97		??				Kaiketsu Yanchamaru
 99             VS System 8KB CHR Switch        VS SMB, VS Excite Bike
105             NES-EVENT                       Nintendo World Championships
112             Asder				Sango Fighter, Hwang Di
113		MB-91				Deathbots
118		MMC3-TLSROM/TKSROM Board	Ys 3, Goal! 2, NES Play Action Football
119             MMC3-TQROM Board                High Speed, Pin*Bot
140		Jaleco ??			Bio Senshi Dan
151             Konami VS System Expansion      VS The Goonies, VS Gradius
152		??				Arkanoid 2, Saint Seiya Ougon Densetsu
153             Bandai ??                       Famicom Jump 2
180             ??                              Crazy Climber
182		??				Super Donkey Kong
184		??				Wing of Madoola, The
189             Micro Genius TXC ??             Thunder Warrior
225             Multi-cart(pirate)              58-in-1/110-in-1/52 Games
226             Multi-cart(pirate)              76-in-1
227		Multi-cart(pirate)		1200-in-1
228             Action 52                       Action 52, Cheetahmen 2
229             Multi-cart(pirate)              31-in-1
232		BIC-48				Quattro Arcade, Quattro Sports
234		Multi-cart ??			Maxi-15
240             ??                              Gen Ke Le Zhuan, Shen Huo Le Zhuan
242		??				Wai Xing Zhan Shi
246		??				Fong Shen Ban
248		??				Bao Qing Tian
250		??				Time Diver Avenger

	FCE Ultra currently supports the following UNIF boards(minus the 
        prefixes HVC-, NES-, BTL-, and BMC-, as they are currently ignored):
        
Group:	Name:			Game Examples:
--------------------------------------------------------------------------------
Bootleg:
        MARIO1-MALEE2		Super Mario Bros. Malee 2
        NovelDiamond9999999in1	Novel Diamond 999999 in 1
	Super24in1SC03		Super 24 in 1
        Supervision16in1	Supervision 16-in-1

Unlicensed:
        Sachen-8259A		Super Cartridge Version 1
        Sachen-8259B		Silver Eagle
        Sachen-74LS374N		Auto Upturn
        SA-016-1M		Master Chu and the Drunkard Hu
        SA-72007		Sidewinder
        SA-72008		Jovial Race
        SA-0036			Mahjong 16
        SA-0037			Mahjong Trap
	TC-U01-1.5M		Challenge of the Dragon

MMC1:
	SAROM			Dragon Warrior
	SBROM			Dance Aerobics
	SCROM			Orb 3D
	SEROM			Boulderdash
	SGROM			Defender of the Crown
	SKROM			Dungeon Magic
	SLROM			Castlevania 2
	SL1ROM			Sky Shark
	SNROM			Shingen the Ruler
	SOROM			Nobunaga's Ambition

MMC3:
	TFROM			Legacy of the Wizard
	TGROM			Megaman 4
	TKROM			Kirby's Adventure
	TKSROM			Ys 3
	TLROM			Super Spike V'Ball
	TLSROM			Goal! 2
	TR1ROM			Gauntlet
	TQROM			Pinbot
	TSROM			Super Mario Bros. 3
	TVROM			Rad Racer 2

MMC5:
	EKROM			Gemfire
	ELROM			Castlevania 3
	ETROM			Nobunaga's Ambition 2
	EWROM			Romance of the Three Kingdoms 2

MMC6:
	HKROM			Star Tropics

Nintendo
discrete
logic:
        CNROM			Gotcha
        CPROM			Videomation
        MHROM
        NROM-128		Mario Bros.
        NROM-256		Super Mario Bros.
        RROM-128
        UNROM			Megaman


/******************************************************************************/
/*  1.1)	System requirements:                                          */
/******************************************************************************/

        Minimum system specifications:

        Pentium 60 MHz
        8 MB RAM
        400 KB free disk space
	Windows 95
	DirectX 7.0
        Video adapter

        Recommended minimum system specifications:

        Pentium 233 MHz
        16 MB RAM
        5 MB free disk space
	Windows 98SE
        Video adapter with 2D acceleration abilities
	DirectX 8.0
	Joystick
	Mouse
        Sound device capable of handling a sample rate of 44100 hz.


/******************************************************************************/
/*  2.0)        Starting FCE Ultra                                            */
/******************************************************************************/

        FCE Ultra can be started by running the executable "fceu.exe".
        I do not recommend running it from a DOS "box".


/******************************************************************************/
/*  2.1)        Using FCE Ultra:                                              */
/******************************************************************************/

        After starting FCE Ultra, you'll probably want to load a game.  Do
        this by going to File/Open.

        Menu descriptions:

        File
         Open           - Loads a new game.                
         Save State     - Saves the current NES state.
         Load State     - Loads a saved NES state.
         Log Sound As   - Logs sound to a file.  It will not work if sound
                          is disabled.                               
         Exit           - Exit the emulator.

        NES
         Reset          - Resets the virtual NES.
         Power          - Power cycles the virtual NES.
         Cheats         - Activates the cheat interface.  See "cheat.txt" for
                          more details.

        Config
         Hide Menu      - Hides the menu.
         Game Genie     - If checked, enable Game Genie emulation.
                          Game Genie emulation will only begin or end when a new
                          game is loaded.
         PAL Emulation  - If checked, enable PAL emulation.  Changes take effect
                          immediately, though I recommend resetting the virtual
                          NES afterward PAL emulation is enabled or disabled.  
         Directories    - Configure the directories that FCE Ultra will store
                          its files in.
         Input          - Enter input configuration dialog.
                          Note that not all virtual devices are configurable.
         Miscellaneous  - Enter miscellaneous configuration dialog.
         Network Play   - Enter network play configuration dialog.
         Palette        - Enter palette configuration dialog.
         Sound          - Enter sound configuration dialog.
          Sound enabled:
                Sound emulation and output are enabled when this is checked.
          Force 8-bit sound:
                Forces 8-bit sound output.  Use only when absolutely
                necessary(very rare).                        
          Sample rate:
                Specifies how many sound samples will be played back per
                second.  Unless you know what you are doing, you probably
                don't need to change this setting.
          Use secondary sound buffer:
                Uses a secondary sound buffer.  This option may be required
                for sound to work with certain sound cards/devices.
                Selecting "with global focus" will cause sound to be played
                while FCE Ultra has lost window focus, but you will probably
                also want to select "Active While Focus Lost" in the Config
                menu as well, otherwise you will just get repeating sound
                when FCE Ultra loses focus.
          Length of sound buffer:
                Specifies what length of sound(in milliseconds) should be
                buffered by FCE Ultra.  DirectSound and the Windows kernel
                may or may not cause a little more latency than what you
                might expect(usually not any more than a few milliseconds),
                depending on your setup.
                Use larger values if you have sound problems such as popping
                or gaps, though.  Larger values will increase the latency of
                the sound, however.  Finally, larger values are ideal for
                background music listening.
          Volume:
                Specifies the volume of FCE Ultra's sound output.  Setting
                the volume too high MIGHT cause noticeable clipping on some
                sounds(loud drums, for example), but don't let that possibility
                stop you from experimenting.

         Video          - Enter video configuration dialog.

        Default Key Mapping:

         For emulated Family BASIC Keyboard:
          Enable/Disable Keyboard Input         Scroll Lock
                (enabling emulated keyboard input will disable
                 commands keys)
          All emulated keys are mapped to the closest open key on the PC
          keyboard, with a few exceptions.  The emulated "@" key is
          mapped to the "`"(grave) key, and the emulated "kana" key
          is mapped to the "Insert" key(in the 3x2 key block above the
          cursor keys).

         For emulated game pads:
          Left Control             B
          Left Alt                 A
          Enter/Return             Start
          Tab                      Select
          Cursor Down              Down
          Cursor Up                Up
          Cursor Left              Left
          Cursor Right             Right
        
         For emulated power pads(keys correspond to button locations on
         side "B"):
          O P [ ]
          K L ; '
          M , . /

         For FDS games:
          I                        Insert disk.
          E                        Eject disk.
          S                        Select disk/disk side.

         For VS Unisystem games:
          C                        Insert coin.
          V                        Show/Hide dip switches.
           1-8                      Toggle dip switches(when dip switches
                                    are shown).

         0-9                      Select save state.
      
         F5/F7                    Save/Load state.
         F9                       Save screen snapshot.

         F3                       Hide/Show menu.
         F4                       Toggle between windowed/full screen modes.
         F10                      Reset.
         F11                      Power off/on.
         F12                      Exit.


/******************************************************************************/
/*  3.0)	Platform Specific Notes                                       */
/******************************************************************************/

        Your desktop color depth must be 16bpp, 24bpp, or 32bpp for FCE Ultra
        to run properly in windowed mode.

	FCE Ultra's base directory is the directory in which the executable
	is located.


/******************************************************************************/
/*  3.1)        Network Play Notes                                            */
/******************************************************************************/

        In TCP/IP network play, the server will be player one, and the
        client will be player 2.

        Zapper emulation and power pad emulation currently do not work with
        network play.

        Having Game Genie or PAL emulation enabled on only one side
        will cause problems.

        Both players MUST use the same ROM/disk image and SRAM
        file(if applicable).

	When using FDS or VS Unisystem games with network play, only player
	1 will be able to insert disk, eject disk, insert coins, toggle dip
	switches, etc.

/******************************************************************************/
/*  3.2)	VS Unisystem Notes                                            */
/******************************************************************************/

        FCE Ultra currently only supports VS Unisystem ROM images in the
        iNES format. 

        ROM Images:

         * VS Unisystem games that are about 49,000 bytes in size most likely
           use mapper 99.
         * Other VS Unisystem games will use other mappers.  Here is a short
           list of games and the mappers they use:

               CastleVania - 2
               Dr. Mario   - 1
               Goonies     - 151
               Gradius     - 151
	       Ice Climber - 99
               Platoon     - 68

        Palette(s):

         * The colors in many VS Unisystem games may be incorrect.  This
           is due to each game having its own PPU, and thus using a
           different palette than games that use a different PPU.


/******************************************************************************/
/*  3.3)        Famicom Disk System Notes                                     */
/******************************************************************************/

	You will need the FDS BIOS ROM image in the base FCE Ultra directory.
	It must be named "disksys.rom".  I will not give this file to you, so
	don't ask.

	Two types of FDS disk images are supported:  disk images with the 
	FWNES-style header, and disk images with no header.

	You should make backups of all of your FDS games you use with FCE 
	Ultra.  This is because FCE Ultra will write the disk image back to 
	the storage medium, and the disk image in RAM might have been corrupted
        because of inaccurate emulation(this case is not likely to occur, but 
	it could occur).


/******************************************************************************/
/*  3.4)        Light Gun Notes                                               */
/******************************************************************************/

	Currently, the NES Zapper and the light gun used with the VS
	Unisystem(I will call both the same name, Zapper) are supported.
	Most(all?) NES games expect the Zapper to be plugged into port 2.
	and most(all?) VS Unisystem games expect the Zapper to be plugged 
	into port(?) 1.

	The LEFT mouse button is the emulated trigger button for the
	Zapper.  The RIGHT mouse button is also emulated as the trigger,
	but as long as you have the RIGHT mouse button held down, no color
	detection will take place, which is effectively like pulling the
	trigger while the Zapper is pointed away from the television screen.
	Note that you must hold the RIGHT button down for a short
	time(greater than just a fast click, shorter than a second).
	
	Zapper emulation currently does NOT work with network play, so
	don't even try it. I may add support in the future if enough
	people want it or if I want it.


/******************************************************************************/
/*  3.5)        Palette Notes                                                 */
/******************************************************************************/

	Palettes files are expected to contain 64 8-bit RGB triplets(each in
	that order; red comes first in the triplet in the file, then green, 
	then blue).  Each 8-bit value represents brightness for that particular
	color.  0 is minimum, 255 is maximum.

	Palettes can be set on a per-game basis.  To do this, put a palette
	file in the "gameinfo" directory with the same base filename
	as the game you wish to associate with and the extension "pal".  
	Examples:

		File name:		Palette file name:
		 BigBad.nes		 BigBad.pal
		 BigBad.zip		 BigBad.pal
		 BigBad.Better.nes	 BigBad.Better.pal


        With so many ways to choose a palette, figuring out which one will
        be active may be difficult.  Here's a list of what palettes will
        be used, in order from highest priority to least priority(if a condition
        doesn't exist for a higher priority palette, the emulator will
        continue down its list of palettes).

        NSF Palette(for NSFs only)
         Palette loaded from the "gameinfo" directory.
          NTSC Color Emulation(only for NTSC NES games).
           VS Unisystem palette(if the game is a VS Unisystem game and a palette
           is available).
            Custom global palette.
             Default NES palette.


/******************************************************************************/
/*  3.6)        Compressed File Notes                                         */
/******************************************************************************/

        FCE Ultra can load data from both PKZIP-format files and
        gzip-format files.  Only one type of (de)compression algorithm is
        supported:  "deflate"; this seems to be the most popular compression
        algorithm, though.

        A compressed FDS disk image will only be saved back to disk if it
        uses the gzip format.

        All files in a PKZIP format file will be scanned for the
        followings extensions:  .nes, .fds, .nsf, .unf, .nez, .unif
        The first compressed file to have one of these extensions will be
        loaded. If no compressed file has one of these extensions, the
        first compressed file will be loaded.


/******************************************************************************/
/*  3.7)        Game Genie Notes	                                      */
/******************************************************************************/

	The Game Genie ROM image is loaded from the file "gg.rom" in the
	base directory the first time Game Genie emulation is enabled and
	a ROM image is loaded since the time FCE Ultra has run.

	The ROM image may either be the 24592 byte iNES-format image, or
	the 4352 raw ROM image.

	Remember that enabling/disabling Game Genie emulation will not take
	effect until a new game is loaded(this statement shouldn't concern
	any of the "run once" command-line driven ports).

/******************************************************************************/
/*  4.0)        Contacting the author                                         */
/******************************************************************************/

        I can be reached via email at xodnizel@users.sourceforge.net.
        Bero can be reached via email at 9bero9@geocities.co.jp
         (Note that Bero can not and will not answer any questions
          regarding the operation of FCE Ultra, so please don't ask him.
          If you understand this, remove the 9's from the email address
	  provided to get his real email address.)


/******************************************************************************/
/*  4.1)        Credits                                                       */
/******************************************************************************/

\Firebug\       -       High-level mapper information.
Bero            -       Original FCE source code.
Brad Taylor	-	NES sound channel guide.
Chris Hickman	-	Archaic Ruins.
Donald Moore    -       DC PasoFami NES packs.
Fredrik Olson	-	NES four-player adapter information.
Gilles Vollant	-	PKZIP file loading functions.
goroh           -       Various documents.
Jeff Tamer      -       Insaniacal fun.
Jeremy Chadwick -       General NES information.
Justin Smith    -       Giving me obscure ROM images in the "dark ages of
                        NES emulation".
Kevin Horton    -       Low level NES information and sound information.
Ki		-	Various technical information.
Mark Knibbs     -       Various NES information.
Marat Fayzullin -       General NES information.
Matthew Conte   -       Sound information.
N. Andou	-	Awesome NES/SNES emulators, at the time...
nori		-	FDS sound information.
Quietust        -       VRC7 sound translation code by The Quietust
                        (quietust@ircN.org).
			Ideas and corrections.
R. Hoelscher	-	Famicom four-player adapter information.
Rob Mocca       -       DC PasoFami NES packs, testing.
Sean Whalen	-	Node99.
Tatsuyuki Satoh -       OPL2 emulator
TheRedEye	-	ROM images, testing.


Info-ZIP	-	ZLIB

...and everyone else who has helped me.
