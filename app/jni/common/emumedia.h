#ifndef EMUMEDIA_H
#define EMUMEDIA_H

#include <nativehelper/jni.h>
#include "emuengine.h"

class EmuMedia {
public:
	virtual ~EmuMedia() {}

	virtual bool init(JNIEnv *env) = 0;
	virtual void destroy(JNIEnv *env) = 0;

	virtual void setSurface(JNIEnv *env, jobject holder) = 0;
	virtual void setSurfaceRegion(JNIEnv *env,
			int x, int y, int w, int h) = 0;
	virtual bool lockSurface(JNIEnv *env,
			EmuEngine::Surface *info, bool flip) = 0;
	virtual void unlockSurface(JNIEnv *env) = 0;

	virtual bool audioCreate(JNIEnv *env,
			unsigned int rate, int bits, int channels) = 0;
	virtual void audioSetVolume(JNIEnv *env, int volume) = 0;
	virtual void audioStart(JNIEnv *env) = 0;
	virtual void audioStop(JNIEnv *env) = 0;
	virtual void audioPause(JNIEnv *env) = 0;
	virtual void audioPlay(JNIEnv *env, void *data, int size) = 0;
};

#endif

