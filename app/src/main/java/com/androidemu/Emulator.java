package com.androidemu;

import android.content.Context;
import android.graphics.Canvas;
import android.os.Build;
import android.view.SurfaceHolder;
import java.io.IOException;
import java.nio.Buffer;

public class Emulator {

	public static final int GAMEPAD_A		= 0x01;
	public static final int GAMEPAD_B		= 0x02;
	public static final int GAMEPAD_SELECT	= 0x04;
	public static final int GAMEPAD_START	= 0x08;
	public static final int GAMEPAD_UP		= 0x10;
	public static final int GAMEPAD_DOWN	= 0x20;
	public static final int GAMEPAD_LEFT	= 0x40;
	public static final int GAMEPAD_RIGHT	= 0x80;
	public static final int GAMEPAD_A_TURBO	= (GAMEPAD_A << 8);
	public static final int GAMEPAD_B_TURBO	= (GAMEPAD_B << 8);

	public static final int GAMEPAD_UP_LEFT = (GAMEPAD_UP | GAMEPAD_LEFT);
	public static final int GAMEPAD_UP_RIGHT = (GAMEPAD_UP | GAMEPAD_RIGHT);
	public static final int GAMEPAD_DOWN_LEFT = (GAMEPAD_DOWN | GAMEPAD_LEFT);
	public static final int GAMEPAD_DOWN_RIGHT = (GAMEPAD_DOWN | GAMEPAD_RIGHT);
	public static final int GAMEPAD_AB	= (GAMEPAD_A | GAMEPAD_B);

	public interface OnFrameDrawnListener {
		void onFrameDrawn(Canvas canvas);
	}

	private static String engineLib;
	private static Emulator emulator;
	private final Thread thread;
	private String romFileName;
	private boolean cheatsEnabled;
	private Cheats cheats;

	public static Emulator createInstance(Context context, String engine) {
		if (emulator == null)
			System.loadLibrary("emu");

		final String libDir =
				context.getApplicationInfo().dataDir + "/lib";
		if (!engine.equals(engineLib)) {
			engineLib = engine;
			loadEngine(libDir, engine);
		}

		if (emulator == null)
			emulator = new Emulator(libDir);
		return emulator;
	}

	public static Emulator getInstance() {
		return emulator;
	}

	private Emulator(String libDir) {
		initialize(libDir, Build.VERSION.SDK_INT);

		thread = new Thread() {
			public void run() {
				nativeRun();
			}
		};
		thread.start();
	}

	public final void enableCheats(boolean enable) {
		cheatsEnabled = enable;
		if (romFileName == null)
			return;

		if (enable && cheats == null)
			cheats = new Cheats(romFileName);
		else if (!enable && cheats != null) {
			cheats.destroy();
			cheats = null;
		}
	}

	public final Cheats getCheats() {
		return cheats;
	}

	public final boolean loadROM(String file) {
		if (!nativeLoadROM(file))
			return false;

		romFileName = file;
		if (cheatsEnabled)
			cheats = new Cheats(file);
		return true;
	}

	public final void unloadROM() {
		nativeUnloadROM();

		cheats = null;
		romFileName = null;
	}

	public native void setFrameUpdateListener(FrameUpdateListener l);
	public native void setSurface(SurfaceHolder surface);
	public native void setSurfaceRegion(int x, int y, int w, int h);

	public native void setKeyStates(int states);
	public native void processTrackball(int key1, int duration1,
			int key2, int duration2);
	public native void fireLightGun(int x, int y);
	public native void setOption(String name, String value);
	public native int getOption(String name);

	public native int getVideoWidth();
	public native int getVideoHeight();

	private static native boolean loadEngine(String libDir, String lib);
	private native boolean initialize(String libDir, int sdk);
	private native void nativeRun();
	private native boolean nativeLoadROM(String file);
	private native void nativeUnloadROM();
	public native void reset();
	public native void power();
	public native void pause();
	public native void resume();
	public native void getScreenshot(Buffer buffer);
	public native boolean saveState(String file);
	public native boolean loadState(String file);

	public void setOption(String name, boolean value) {
		setOption(name, value ? "true" : "false");
	}

	public void setOption(String name, int value) {
		setOption(name, Integer.toString(value));
	}

	public interface FrameUpdateListener {
		int onFrameUpdate(int keys)
				throws IOException, InterruptedException;
	}
}
