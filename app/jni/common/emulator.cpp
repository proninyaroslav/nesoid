#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>

#define LOG_TAG "libemu"
#include <utils/Log.h>
#include <nativehelper/JNIHelp.h>
#include <nativehelper/jni.h>
#include "emumedia.h"
#include "ticks.h"

static pthread_mutex_t emuStateMutex;
static pthread_cond_t emuStateCond;
static int emuState;

static void *engineLib;
static EmuEngine *engine;
static EmuEngine::Game *currentGame;
static EmuMedia *media;

static jobject jFrameUpdateListener;
static jmethodID midOnFrameUpdate;

static bool resumeRequested;
static unsigned int keyStates;
static bool flipScreen;
static bool autoFrameSkip;
static int maxFrameSkips;
static int refreshRate;
static float gameSpeed;
static bool soundEnabled;

static JNIEnv *emuThreadEnv;
static bool surfaceReady;

static struct {
	int key;
	int duration;
} trackballEvents[2];

enum {
	EMUSTATE_RUNNING,
	EMUSTATE_PAUSED,
	EMUSTATE_REQUEST_PAUSE,
	EMUSTATE_REQUEST_RUN,
};

class EngineCallbacks : public EmuEngine::Callbacks {
public:
	virtual bool lockSurface(EmuEngine::Surface *surface) {
		return media->lockSurface(emuThreadEnv, surface, flipScreen);
	}

	virtual void unlockSurface(const EmuEngine::Surface *surface) {
		media->unlockSurface(emuThreadEnv);
	}

	virtual void playAudio(void *data, int size) {
		media->audioPlay(emuThreadEnv, data, size);
	}
};


static void *loadSharedObject(const char *dir, const char *lib,
		void **handle = NULL)
{
	char path[1024];
	snprintf(path, sizeof(path), "%s/lib%s.so", dir, lib);

	void *h = dlopen(path, RTLD_NOW);
	if (h == NULL) {
		LOGD("Cannot load %s: %s", path, dlerror());
		return NULL;
	}

	void *(*createObject)() = (void *(*)()) dlsym(h, "createObject");
	if (createObject == NULL) {
		dlclose(h);
		return NULL;
	}
	if (handle != NULL)
		*handle = h;
	return createObject();
}

static unsigned int updateTrackballStates(unsigned int states)
{
	for (int i = 0; i < 2; i++) {
		if (trackballEvents[i].duration > 0) {
			trackballEvents[i].duration--;
			states |= trackballEvents[i].key;
		}
	}
	return states;
}

static void showFPS()
{
	static int frames;
	static unsigned int last;
	unsigned int now = ticksGetTicks();

	frames++;
	if (now - last >= 1000) {
		LOGD("fps: %d", frames * 1000 / (now - last));
		last = now;
		frames = 0;
	}
}

static void pauseEmulator(JNIEnv *env, jobject self)
{
	pthread_mutex_lock(&emuStateMutex);
	if (emuState == EMUSTATE_RUNNING) {
		emuState = EMUSTATE_REQUEST_PAUSE;
		while (emuState == EMUSTATE_REQUEST_PAUSE)
			pthread_cond_wait(&emuStateCond, &emuStateMutex);
	}
	pthread_mutex_unlock(&emuStateMutex);
}

static void resumeEmulator()
{
	if (!resumeRequested || !surfaceReady || currentGame == NULL)
		return;

	pthread_mutex_lock(&emuStateMutex);
	if (emuState == EMUSTATE_PAUSED) {
		emuState = EMUSTATE_REQUEST_RUN;
		pthread_cond_signal(&emuStateCond);

		while (emuState == EMUSTATE_REQUEST_RUN)
			pthread_cond_wait(&emuStateCond, &emuStateMutex);
	}
	pthread_mutex_unlock(&emuStateMutex);
}

static void unloadROM(JNIEnv *env, jobject self)
{
	if (currentGame == NULL)
		return;

	pauseEmulator(env, self);

	media->audioStop(env);
	engine->unloadRom();
	currentGame = NULL;
}

static void runEmulator()
{
	const bool soundOn = soundEnabled;
	if (soundOn)
		media->audioStart(emuThreadEnv);

	const int fps = (int) (currentGame->fps * gameSpeed);
	const int maxSkips = (int) (maxFrameSkips * gameSpeed);
	const unsigned int frameTime = 1000 / fps;
	const unsigned int refreshTime = (refreshRate ? (1000 / refreshRate) : 0);

	unsigned int initialTicks = ticksGetTicks();
	unsigned int lastTicks = initialTicks;
	unsigned int lastFrameDrawnTime = 0;
	unsigned int virtualFrameCount = 0;
	int skipCounter = 0;

	while (emuState == EMUSTATE_RUNNING) {
		unsigned int now = ticksGetTicks();
		unsigned int realFrameCount = (now - initialTicks) * fps / 1000;

		// frame skips
		virtualFrameCount++;
		if (realFrameCount >= virtualFrameCount) {
			if (realFrameCount > virtualFrameCount &&
					autoFrameSkip && skipCounter < maxSkips) {
				skipCounter++;
			} else {
				virtualFrameCount = realFrameCount;
				if (autoFrameSkip)
					skipCounter = 0;
			}
		} else {
			unsigned int delta = now - lastTicks;
			if (delta < frameTime)
				usleep((frameTime - delta) * 1000);
		}
		if (!autoFrameSkip) {
			if (++skipCounter > maxSkips)
				skipCounter = 0;
		}
		lastTicks = now;

		// trackball events
		unsigned int states = updateTrackballStates(keyStates);

		// FrameUpdateListener.onFrameUpdate(states);
		if (jFrameUpdateListener != NULL) {
			states = emuThreadEnv->CallIntMethod(jFrameUpdateListener,
					midOnFrameUpdate, states);

			if (emuThreadEnv->ExceptionOccurred()) {
				emuThreadEnv->ExceptionClear();
				break;
			}
		}

		// Consider refresh rate
		bool skip = (skipCounter > 0);
		if (!skip && refreshTime) {
			if (now - lastFrameDrawnTime < refreshTime)
				skip = true;
			else
				lastFrameDrawnTime = now;
		}
		engine->runFrame(states, skip);

		//showFPS();
	}

	if (soundOn)
		media->audioPause(emuThreadEnv);
}

static void Emulator_run(JNIEnv *env, jobject self)
{
	emuThreadEnv = env;

	while (true) {
		pthread_mutex_lock(&emuStateMutex);
		while (emuState == EMUSTATE_PAUSED)
			pthread_cond_wait(&emuStateCond, &emuStateMutex);

		if (emuState == EMUSTATE_REQUEST_RUN) {
			emuState = EMUSTATE_RUNNING;
			pthread_cond_signal(&emuStateCond);
		}
		pthread_mutex_unlock(&emuStateMutex);

		runEmulator();

		pthread_mutex_lock(&emuStateMutex);
		if (emuState == EMUSTATE_REQUEST_PAUSE) {
			emuState = EMUSTATE_PAUSED;
			pthread_cond_signal(&emuStateCond);
		}
		pthread_mutex_unlock(&emuStateMutex);
	}

	emuThreadEnv = NULL;
}

static jboolean
Emulator_loadEngine(JNIEnv *env, jobject self, jstring jdir, jstring jlib)
{
	const char *dir = env->GetStringUTFChars(jdir, NULL);
	const char *lib = env->GetStringUTFChars(jlib, NULL);

	if (engineLib != NULL) {
		dlclose(engineLib);
		engineLib = NULL;
		engine = NULL;
	}
	engine = (EmuEngine *) loadSharedObject(dir, lib, &engineLib);

	env->ReleaseStringUTFChars(jdir, dir);
	env->ReleaseStringUTFChars(jlib, lib);

	static EngineCallbacks cbs;
	if (engine == NULL || !engine->initialize(&cbs)) {
		LOGE("Cannot load emulator engine");
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

extern EmuMedia *createEmuMedia();

static jboolean
Emulator_initialize(JNIEnv *env, jobject self, jstring jdir, jint sdk)
{
	media = createEmuMedia();
	if (!media->init(env))
		return JNI_FALSE;

	ticksInitialize();

	emuState = EMUSTATE_PAUSED;
	surfaceReady = false;
	currentGame = NULL;
	resumeRequested = false;
	flipScreen = false;
	autoFrameSkip = true;
	maxFrameSkips = 2;
	refreshRate = 0;
	gameSpeed = 1.0f;
	soundEnabled = false;

	jFrameUpdateListener = NULL;
	jclass clazz = env->FindClass("com/androidemu/Emulator$FrameUpdateListener");
	midOnFrameUpdate = env->GetMethodID(clazz, "onFrameUpdate", "(I)I");

	pthread_mutex_init(&emuStateMutex, NULL);
	pthread_cond_init(&emuStateCond, NULL);
	return JNI_TRUE;
}

static void
Emulator_setFrameUpdateListener(JNIEnv *env, jobject self, jobject listener)
{
	pauseEmulator(env, self);

	if (jFrameUpdateListener != NULL)
		env->DeleteGlobalRef(jFrameUpdateListener);
	jFrameUpdateListener = env->NewGlobalRef(listener);

	resumeEmulator();
}

static void
Emulator_setSurface(JNIEnv *env, jobject self, jobject surface)
{
	pauseEmulator(env, self);

	media->setSurface(env, surface);
	if (surface == NULL)
		surfaceReady = false;
}

static void
Emulator_setSurfaceRegion(JNIEnv *env, jobject self,
		int x, int y, int w, int h)
{
	pauseEmulator(env, self);

	media->setSurfaceRegion(env, x, y, w, h);
	surfaceReady = true;

	resumeEmulator();
}

static void
Emulator_setKeyStates(JNIEnv *env, jobject self, jint states)
{
	keyStates = states;
}

static void
Emulator_processTrackball(JNIEnv *env, jobject self,
		jint key1, jint duration1, jint key2, jint duration2)
{
	const int key[] = { key1, key2 };
	const int duration[] = { duration1, duration2 };

	for (int i = 0; i < 2; i++) {
		if (key[i] == 0)
			continue;

		if (trackballEvents[i].key != key[i]) {
			trackballEvents[i].duration = 0;
			trackballEvents[i].key = key[i];
		}
		if ((trackballEvents[i].duration += duration[i]) > 80)
			trackballEvents[i].duration = 80;
	}
}

static void
Emulator_fireLightGun(JNIEnv *env, jobject self, jint x, jint y)
{
	engine->fireLightGun(x, y);
}

static void
Emulator_setOption(JNIEnv *env, jobject self, jstring jname, jstring jvalue)
{
	const char *name = env->GetStringUTFChars(jname, NULL);
	const char *value = NULL;
	if (jvalue != NULL)
		value = env->GetStringUTFChars(jvalue, NULL);

	if (strcmp(name, "frameSkipMode") == 0) {
		autoFrameSkip = (strcmp(value, "auto") == 0);

	} else if (strcmp(name, "maxFrameSkips") == 0) {
		maxFrameSkips = atoi(value);
		if (maxFrameSkips < 0)
			maxFrameSkips = 0;
		else if (maxFrameSkips > 99)
			maxFrameSkips = 99;

	} else if (strcmp(name, "refreshRate") == 0) {
		refreshRate = atoi(value);

	} else if (strcmp(name, "gameSpeed") == 0) {
		gameSpeed = atof(value);
		if (gameSpeed < 0.1f)
			gameSpeed = 1.0f;

		if (currentGame != NULL) {
			media->audioCreate(env,
				(int) (currentGame->soundRate * gameSpeed),
				currentGame->soundBits,
				currentGame->soundChannels);
		}

	} else {
		if (strcmp(name, "soundEnabled") == 0)
			soundEnabled = (strcmp(value, "true") == 0);
		else if (strcmp(name, "soundVolume") == 0)
			media->audioSetVolume(env, atoi(value));	
		else if (strcmp(name, "flipScreen") == 0)
			flipScreen = (strcmp(value, "true") == 0);
		engine->setOption(name, value);
	}

	env->ReleaseStringUTFChars(jname, name);
	if (jvalue != NULL)
		env->ReleaseStringUTFChars(jvalue, value);
}

static jint Emulator_getOption(JNIEnv *env, jobject self, jstring jname)
{
	const char *name = env->GetStringUTFChars(jname, NULL);
	int value = engine->getOption(name);
	env->ReleaseStringUTFChars(jname, name);
	return value;
}

static void Emulator_getScreenshot(JNIEnv *env, jobject self, jobject jbuffer)
{
	pauseEmulator(env, self);

	EmuEngine::Surface surface;
	surface.bits = env->GetDirectBufferAddress(jbuffer);
	surface.w = currentGame->videoWidth;
	surface.h = currentGame->videoHeight;
	surface.bpr = surface.w * 2;
	engine->renderFrame(surface);

	resumeEmulator();
}

static void Emulator_reset(JNIEnv *env, jobject self)
{
	pauseEmulator(env, self);
	engine->reset();
	resumeEmulator();
}

static void Emulator_power(JNIEnv *env, jobject self)
{
	pauseEmulator(env, self);
	engine->power();
	resumeEmulator();
}

static jint Emulator_getVideoWidth(JNIEnv *env, jobject self)
{
	return (currentGame ? currentGame->videoWidth : 0);
}

static jint Emulator_getVideoHeight(JNIEnv *env, jobject self)
{
	return (currentGame ? currentGame->videoHeight : 0);
}

static jboolean Emulator_loadROM(JNIEnv *env, jobject self, jstring jfile)
{
	unloadROM(env, self);

	char *file = const_cast<char *>(env->GetStringUTFChars(jfile, NULL));
	jboolean rv = JNI_FALSE;

	currentGame = engine->loadRom(file);
	if (currentGame == NULL)
		goto error;

	gameSpeed = 1.0f;
	media->audioCreate(env,
			currentGame->soundRate,
			currentGame->soundBits,
			currentGame->soundChannels);

	memset(trackballEvents, 0, sizeof(trackballEvents));

	resumeEmulator();
	rv = JNI_TRUE;
error:
	env->ReleaseStringUTFChars(jfile, file);
	return rv;
}

static void Emulator_unloadROM(JNIEnv *env, jobject self)
{
	unloadROM(env, self);
}

static void Emulator_pause(JNIEnv *env, jobject self)
{
	pauseEmulator(env, self);
	resumeRequested = false;
}

static void Emulator_resume(JNIEnv *env, jobject self)
{
	resumeRequested = true;
	resumeEmulator();
}

static jboolean Emulator_saveState(JNIEnv *env, jobject self, jstring jfile)
{
	const char *file = env->GetStringUTFChars(jfile, NULL);

	pauseEmulator(env, self);
	jboolean rv = engine->saveState(file);
	resumeEmulator();

	env->ReleaseStringUTFChars(jfile, file);
	return rv;
}

static jboolean Emulator_loadState(JNIEnv *env, jobject self, jstring jfile)
{
	const char *file = env->GetStringUTFChars(jfile, NULL);

	pauseEmulator(env, self);
	jboolean rv = engine->loadState(file);
	resumeEmulator();

	env->ReleaseStringUTFChars(jfile, file);
	return rv;
}

static jboolean Cheats_nativeAdd(JNIEnv *env, jobject self, jstring jcode)
{
	const char *code = env->GetStringUTFChars(jcode, NULL);

	pauseEmulator(env, self);
	jboolean rv = engine->addCheat(code);
	resumeEmulator();

	env->ReleaseStringUTFChars(jcode, code);
	return rv;
}

static void Cheats_nativeRemove(JNIEnv *env, jobject self, jstring jcode)
{
	const char *code = env->GetStringUTFChars(jcode, NULL);
	
	pauseEmulator(env, self);
	engine->removeCheat(code);
	resumeEmulator();

	env->ReleaseStringUTFChars(jcode, code);
}

int register_Emulator(JNIEnv *env)
{
	static const JNINativeMethod methods[] = {
		{ "setFrameUpdateListener", "(Lcom/androidemu/Emulator$FrameUpdateListener;)V", (void *) Emulator_setFrameUpdateListener },
		{ "setSurface", "(Landroid/view/SurfaceHolder;)V", (void *) Emulator_setSurface },
		{ "setSurfaceRegion", "(IIII)V", (void *) Emulator_setSurfaceRegion },

		{ "setKeyStates", "(I)V", (void *) Emulator_setKeyStates },
		{ "processTrackball", "(IIII)V", (void *) Emulator_processTrackball },
		{ "fireLightGun", "(II)V", (void *) Emulator_fireLightGun },
		{ "setOption", "(Ljava/lang/String;Ljava/lang/String;)V", (void *) Emulator_setOption },
		{ "getOption", "(Ljava/lang/String;)I", (void *) Emulator_getOption },

		{ "getVideoWidth", "()I", (void *) Emulator_getVideoWidth },
		{ "getVideoHeight", "()I", (void *) Emulator_getVideoHeight },

		{ "loadEngine", "(Ljava/lang/String;Ljava/lang/String;)Z", (void *) Emulator_loadEngine },
		{ "initialize", "(Ljava/lang/String;I)Z", (void *) Emulator_initialize },
		{ "nativeRun", "()V", (void *) Emulator_run },
		{ "nativeLoadROM", "(Ljava/lang/String;)Z", (void *) Emulator_loadROM },
		{ "nativeUnloadROM", "()V", (void *) Emulator_unloadROM },
		{ "reset", "()V", (void *) Emulator_reset },
		{ "power", "()V", (void *) Emulator_power },
		{ "pause", "()V", (void *) Emulator_pause },
		{ "resume", "()V", (void *) Emulator_resume },
		{ "getScreenshot", "(Ljava/nio/Buffer;)V", (void *) Emulator_getScreenshot },
		{ "saveState", "(Ljava/lang/String;)Z", (void *) Emulator_saveState },
		{ "loadState", "(Ljava/lang/String;)Z", (void *) Emulator_loadState },
	};

	return jniRegisterNativeMethods(env, "com/androidemu/Emulator", methods, NELEM(methods));
}

int register_Cheats(JNIEnv *env)
{
	static const JNINativeMethod methods[] = {
		{ "nativeAdd", "(Ljava/lang/String;)Z", (void *) Cheats_nativeAdd },
		{ "nativeRemove", "(Ljava/lang/String;)V",(void *) Cheats_nativeRemove },
	};

	return jniRegisterNativeMethods(env, "com/androidemu/Cheats",methods, NELEM(methods));
}
