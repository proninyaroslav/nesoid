package com.androidemu.nes.input;

import android.view.KeyEvent;
import android.view.View;

import java.util.Arrays;

public class Keyboard implements View.OnKeyListener {

	private static final String LOG_TAG = "Keyboard";

	private final GameKeyListener gameKeyListener;
	private final int[] keysMap = new int[128];
	private int keyStates;

	public Keyboard(View view, GameKeyListener listener) {
		gameKeyListener = listener;
		view.setOnKeyListener(this);
	}

	public final int getKeyStates() {
		return keyStates;
	}

	public void reset() {
		keyStates = 0;
	}

	public void clearKeyMap() {
		Arrays.fill(keysMap, 0);
	}

	public void mapKey(int gameKey, int keyCode) {
		if (keyCode >= 0 && keyCode < keysMap.length)
			keysMap[keyCode] |= gameKey;
	}

	public boolean onKey(View v, int keyCode, KeyEvent event) {
		if (keyCode >= keysMap.length)
			return false;

		int gameKey = keysMap[keyCode];
		if (gameKey != 0) {
			if (event.getRepeatCount() == 0) {
				if (event.getAction() == KeyEvent.ACTION_DOWN)
					keyStates |= gameKey;
				else
					keyStates &= ~gameKey;

				gameKeyListener.onGameKeyChanged();
			}
			return true;
		}
		return false;
	}
}
