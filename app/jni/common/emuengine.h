#ifndef EMUENGINE_H
#define EMUENGINE_H

class EmuEngine {
public:
	struct Game {
		int videoWidth;
		int videoHeight;
		int soundBits;
		int soundRate;
		int soundChannels;
		int fps;
	};

	struct Surface {
		void *bits;
		int bpr;
		int w;
		int h;
	};

	class Callbacks {
	public:
		virtual ~Callbacks() {}
		virtual bool lockSurface(Surface *surface) = 0;
		virtual void unlockSurface(const Surface *surface) = 0;
		virtual void playAudio(void *data, int size) = 0;
	};

	virtual ~EmuEngine() {}

	virtual bool initialize(Callbacks *cbs) = 0;
	virtual void destroy() = 0;
	virtual void reset() = 0;
	virtual void power() = 0;
	virtual Game *loadRom(char *file) = 0;
	virtual void unloadRom() = 0;
	virtual void runFrame(unsigned int keys, bool skip) = 0;
	virtual void renderFrame(const Surface &surface) = 0;

	virtual bool saveState(const char *file) = 0;
	virtual bool loadState(const char *file) = 0;

	virtual bool addCheat(const char *code) { return false; }
	virtual void removeCheat(const char *code) {}

	virtual void fireLightGun(int x, int y) = 0;
	virtual void setOption(const char *name, const char *value) = 0;
	virtual int getOption(const char *name) { return 0; }
};

#endif

