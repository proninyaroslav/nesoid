#define GP2X_PORT_VERSION "0.4"

typedef struct {
	// gp2x specific
	int KeyBinds[32];
	int JoyBinds[4][32];
	int turbo_rate_add;	// 8.24 int
	int sound_rate;		// in Hz
	int showfps;
	int scaling;		// unscaled=0, hw_hor, hw_hor_vert, sw_hor
	int frameskip;		// -1 ~ auto, >=0 ~ count
	int sstate_confirm;
	int region_force;	// 0 ~ off, 1 ~ NTSC, 2 ~ PAL
	int cpuclock;
	int mmuhack;
	int ramtimings;
	int gamma;
	int perfect_vsync;
	int accurate_mode;
} DSETTINGS;

extern DSETTINGS Settings;

void gp2x_opt_setup(void);
void gp2x_opt_update(void);

